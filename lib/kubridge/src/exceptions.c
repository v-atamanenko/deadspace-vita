#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/excpmgr.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/proc_event.h>
#include <psp2kern/kernel/processmgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/sysmem/memtype.h>
#include <psp2kern/kernel/sysroot.h>
#include <psp2kern/kernel/threadmgr.h>
#include <exceptions_user.h>

#include <psp2/kernel/error.h>

#include <taihen.h>
#include "internal.h"

typedef struct SceKernelExceptionHandler
{
    struct SceKernelExceptionHandler *nextHandler;
    SceUInt32 padding;

    SceUInt32 impl[];
} SceKernelExceptionHandler;

int ksceKernelSwitchPidContext(SceKernelProcessContext *new_context, SceKernelProcessContext *prev_context);
int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
void kuKernelFlushCaches(const void *ptr, SceSize len);

static int (*_sceGUIDGetName)(SceUID guid, char **name);
static int (*_sceKernelGetFaultingProcess)(SceKernelFaultingProcessInfo *);
static int (*_sceKernelRegisterExceptionHandler)(SceExcpKind excpKind, SceUInt32 prio, void *handler);
static void (*_sceKernelReturnFromException)(SceExcpmgrExceptionContext *context) __attribute__((__noreturn__)); // SceExcpmgrForKernel_D17EEE40 / SceExcpmgrForKernel_3E55B5C3
static void *(*_sceKernelAllocRemoteProcessHeap)(SceUID pid, SceSize size, void *pOpt);
static void (*_sceKernelFreeRemoteProcessHeap)(SceUID pid, void *ptr);

extern uint32_t DabtExceptionHandler_lvl0[];
extern uint32_t DabtExceptionHandler_lvl0_retAddr;
extern SceKernelExceptionHandler DabtExceptionHandler_lvl1;
extern SceKernelExceptionHandler PabtExceptionHandler_lvl1;
SceKernelExceptionHandler *DabtExceptionHandler_default;

void *userAbortBase;
KuKernelAbortHandler defaultUserAbortHandler;

static ProcessAbortHandler *handlers;
int32_t handlersMutex = 0;
static SceUID userAbortMemBlock = -1;

__attribute__((target("arm"), __noreturn__)) void ReturnFromException(SceExcpmgrExceptionContext *excpContext)
{
    LOG("Returning from exception to 0x%08X\n", excpContext->address_of_faulting_instruction);

    // The TPID* registers aren't set by _sceKernelReturnFromException, so we have to do it ourselves.
    asm volatile("mcr p15, #0x0, %0, c13, c0, #0x2" :: "r" (excpContext->TPIDRURO));
    asm volatile("mcr p15, #0x0, %0, c13, c0, #0x3" :: "r" (excpContext->TPIDRURW));
    asm volatile("mcr p15, #0x0, %0, c13, c0, #0x4" :: "r" (excpContext->TPIDRPRW));
    _sceKernelReturnFromException(excpContext);
}

__attribute__((target("arm"))) int CheckStackPointer(void *stackPointer)
{
    void *stackBounds[2];
    SceKernelFaultingProcessInfo procInfo;
    _sceKernelGetFaultingProcess(&procInfo);
    ksceKernelProcMemcpyFromUser(procInfo.pid, &stackBounds[0], ksceKernelGetThreadTLSAddr(procInfo.faultingThreadId, 2), sizeof(stackBounds));
    if (stackPointer > stackBounds[0] || stackPointer < stackBounds[1])
    {
        LOG("Invalid stack pointer 0x%08X. (0x%08X/0x%08X)", stackPointer, stackBounds[1], stackBounds[0]);
        return 0; // Stack pointer is not valid
    }

    stackPointer -= (0x160 + 0x400); // 0x160 for the abort context, plus an extra 0x400 for use in the abort handler
    if (stackPointer > stackBounds[0] || stackPointer < stackBounds[1])
    {
        LOG("Insufficient stack space (0x%08X/0x%08X)", stackBounds[1], stackPointer);
        return 0; // Stack pointer is not valid
    }

    return 1;
}

__attribute__((target("arm"))) void *CopyFromExcp(void *dst, const void *src, size_t len)
{
    SceKernelFaultingProcessInfo procInfo;
    _sceKernelGetFaultingProcess(&procInfo);
    SceUID pid = procInfo.pid;

    ksceKernelProcMemcpyToUser(pid, dst, src, len);

    return dst + len;
}

void RemoveProcessAbortHandler(SceUID pid);
ProcessAbortHandler *GetProcessAbortHandler(SceUID pid)
{
    ProcessAbortHandler *cur = handlers;
    SceBool fromExcp = pid == -1;

    if (pid == -1)
    {
        SceKernelFaultingProcessInfo procInfo;
        _sceKernelGetFaultingProcess(&procInfo);
        pid = procInfo.pid;
    }
    else if (pid == 0)
        pid = ksceKernelGetProcessId();

    while (cur != NULL)
    {
        if (cur->pid == pid)
            break;

        cur = cur->pNext;
    }

    if (cur == NULL && !fromExcp)
    {
        cur = _sceKernelAllocRemoteProcessHeap(pid, sizeof(ProcessAbortHandler), NULL);
        if (cur == NULL)
            return NULL;

        cur->pid = pid;
        cur->pHandler = defaultUserAbortHandler;
        cur->pNext = handlers;
        cur->userAbortMemBlock = -1;
        handlers = cur;

        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(opt));
        opt.size = sizeof(opt);
        opt.attr = SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_MIRROR_BLOCKID | 0x1000000 | 0x800000; // (HAS_BASE | SHARE_PHYPAGE | SHARE_VBASE)
        opt.mirror_blockid = userAbortMemBlock;
        SceUID memBlock = ksceKernelAllocMemBlock("KuBridgeAbortDispatchBlock", SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_SHARED_RX, 0x1000, &opt);
        if (memBlock < 0)
        {
            LOG("Failed to allocate memBlock for user abort handler dispatch\n");
            RemoveProcessAbortHandler(pid);
            return NULL;
        }

        cur->userAbortMemBlock = memBlock;
    }
    else if (cur == NULL)
        LOG("No abort handler found for process 0x%08X\n", pid);

    return cur;
}

void RemoveProcessAbortHandler(SceUID pid)
{
    ProcessAbortHandler *cur = handlers, *prev = NULL;

    if (pid == -1)
    {
        SceKernelFaultingProcessInfo procInfo;
        _sceKernelGetFaultingProcess(&procInfo);
        pid = procInfo.pid;
    }
    else if (pid == 0)
        pid = ksceKernelGetProcessId();

    while (cur != NULL)
    {
        if (cur->pid == pid)
            break;

        prev = cur;
        cur = cur->pNext;
    }

    if (cur != NULL)
    {
        if (prev == NULL)
            handlers = NULL;
        else
            prev->pNext = cur->pNext;

        if (cur->userAbortMemBlock != -1)
            ksceKernelFreeMemBlock(cur->userAbortMemBlock);

        _sceKernelFreeRemoteProcessHeap(pid, cur);
    }
}

static int CreateProcess(SceUID pid, SceProcEventInvokeParam2 *a2, int a3)
{
    char *name;
    _sceGUIDGetName(pid, &name);

    if ((strncmp(name, "main", 4) == 0) && (userAbortMemBlock == -1))
    {
        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(opt));
        opt.size = sizeof(opt);
        opt.attr = 0x8000000 | SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_PID; // Attr 0x8000000 allocates the memBlock at the highest address possible
        opt.pid = pid;
        userAbortMemBlock = ksceKernelAllocMemBlock("KuBridgeAbortHandlerUserBlock", SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_SHARED_RW, 0x1000, &opt);
        if (userAbortMemBlock < 0)
        {
            LOG("Failed to allocate memBlock for user abort handler dispatch\n");
            return 0;
        }
        ksceKernelGetMemBlockBase(userAbortMemBlock, &userAbortBase);

        ksceKernelProcMemcpyToUser(pid, userAbortBase, &exceptions_user_bin[0], exceptions_user_bin_len);

        defaultUserAbortHandler = (KuKernelAbortHandler)(userAbortBase + (sizeof(SceUInt32) * 4));
    }
    return 0;
}

static int DestroyProcess(SceUID pid, SceProcEventInvokeParam1 *a2, int a3)
{
    int irqState = ksceKernelCpuSpinLockIrqSave(&handlersMutex);

    RemoveProcessAbortHandler(pid);

    ksceKernelCpuSpinLockIrqRestore(&handlersMutex, irqState);

    return 0;
}

void SetupExceptionHandlers()
{
    int res = module_get_export_func(KERNEL_PID, "SceExcpmgr", TAI_ANY_LIBRARY, 0x03499636, (uintptr_t *)&_sceKernelRegisterExceptionHandler); // 3.60
    if (res < 0)
        module_get_export_func(KERNEL_PID, "SceExcpmgr", TAI_ANY_LIBRARY, 0x00063675, (uintptr_t *)&_sceKernelRegisterExceptionHandler); // >= 3.63
    res = module_get_export_func(KERNEL_PID, "SceExcpmgr", TAI_ANY_LIBRARY, 0xD17EEE40, (uintptr_t *)&_sceKernelReturnFromException);    // 3.60
    if (res < 0)
        module_get_export_func(KERNEL_PID, "SceExcpmgr", TAI_ANY_LIBRARY, 0x3E55B5C3, (uintptr_t *)&_sceKernelReturnFromException);          // >= 3.63
    res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", TAI_ANY_LIBRARY, 0xD8B9AC8D, (uintptr_t *)&_sceKernelGetFaultingProcess); // 3.60
    if (res < 0)
        module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", TAI_ANY_LIBRARY, 0x6C1F092F, (uintptr_t *)&_sceKernelGetFaultingProcess); // >= 3.63

    module_get_export_func(KERNEL_PID, "SceProcessmgr", TAI_ANY_LIBRARY, 0x00B1CA0F, (uintptr_t *)&_sceKernelAllocRemoteProcessHeap);
    module_get_export_func(KERNEL_PID, "SceProcessmgr", TAI_ANY_LIBRARY, 0x9C28EA9A, (uintptr_t *)&_sceKernelFreeRemoteProcessHeap);
    module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0xA78755EB, (uintptr_t *)&_sceGUIDGetName);

    DabtExceptionHandler_default = (SceKernelExceptionHandler *)((((uint32_t *)ksceSysrootGetSysroot()->VBAR)[0x40 + SCE_EXCP_DABT]) - 0x8);

    _sceKernelRegisterExceptionHandler(SCE_EXCP_DABT, 1, &DabtExceptionHandler_lvl1);
    _sceKernelRegisterExceptionHandler(SCE_EXCP_PABT, 1, &PabtExceptionHandler_lvl1);

    const uint32_t defaultHandlerPatch[2] = {0xE51FF004, (uintptr_t)(&DabtExceptionHandler_lvl0[0])};
    const uint32_t *lvl0HandlerReturnAddr = (&DabtExceptionHandler_default->impl[2]);

    ksceKernelCpuUnrestrictedMemcpy(&DabtExceptionHandler_default->impl[0], &defaultHandlerPatch[0], sizeof(defaultHandlerPatch));
    ksceKernelCpuUnrestrictedMemcpy(&DabtExceptionHandler_lvl0_retAddr, &lvl0HandlerReturnAddr, sizeof(lvl0HandlerReturnAddr));

    SceProcEventHandler handler;
    memset(&handler, 0, sizeof(handler));
    handler.size = sizeof(handler);
    handler.create = CreateProcess;
    handler.exit = DestroyProcess;
    handler.kill = DestroyProcess;

    ksceKernelRegisterProcEventHandler("KuBridgeProcessHandler", &handler, 0);
}

int kuKernelRegisterAbortHandler(KuKernelAbortHandler pHandler, KuKernelAbortHandler *pOldHandler)
{
    int irqState = ksceKernelCpuSpinLockIrqSave(&handlersMutex);

    if (pHandler == NULL)
        return SCE_KERNEL_ERROR_INVALID_ARGUMENT;

    ProcessAbortHandler *procHandler = GetProcessAbortHandler(0);
    if (procHandler == NULL)
        return SCE_KERNEL_ERROR_NO_MEMORY;

    if (pOldHandler != NULL)
        ksceKernelMemcpyToUser(pOldHandler, &procHandler->pHandler, sizeof(KuKernelAbortHandler));

    procHandler->pHandler = pHandler;

    ksceKernelCpuSpinLockIrqRestore(&handlersMutex, irqState);

    return 0;
}

void kuKernelReleaseAbortHandler()
{
    int irqState = ksceKernelCpuSpinLockIrqSave(&handlersMutex);

    ProcessAbortHandler *procHandler = GetProcessAbortHandler(0);
    if (procHandler == NULL)
        return;

    procHandler->pHandler = defaultUserAbortHandler;

    ksceKernelCpuSpinLockIrqRestore(&handlersMutex, irqState);
}