#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/excpmgr.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/proc_event.h>
#include <psp2kern/kernel/processmgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/sysmem/memtype.h>
#include <psp2kern/kernel/sysroot.h>
#include <psp2kern/kernel/threadmgr.h>

#include <psp2/kernel/error.h>

#include <taihen.h>

#include "internal.h"
#include "exceptions_user.h"

#if !defined(EXCEPTION_SAFETY)
#define EXCEPTION_SAFETY 1
#endif

typedef struct SceKernelExceptionHandler
{
    struct SceKernelExceptionHandler *nextHandler;
    SceUInt32 padding;

    SceUInt32 impl[];
} SceKernelExceptionHandler;

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);

static int (*_sceGUIDGetName)(SceUID guid, char **name);
static int (*_sceKernelRegisterExceptionHandler)(SceExcpKind excpKind, SceUInt32 prio, SceKernelExceptionHandler *handler);
static void *(*_sceKernelAllocRemoteProcessHeap)(SceUID pid, SceSize size, void *pOpt);
static void (*_sceKernelFreeRemoteProcessHeap)(SceUID pid, void *ptr);
static int (*_sceKernelProcCopyToUserRx)(SceUID pid, void *dst, const void *src, SceSize size);

extern SceKernelExceptionHandler DabtExceptionHandler_lvl0;
extern SceKernelExceptionHandler PabtExceptionHandler_lvl0;
extern SceKernelExceptionHandler UndefExceptionHandler_lvl0;

void *userAbortBase;
static SceUID userAbortMemBlockPid; // PID of process that owns userAbortMemBlock
KuKernelAbortHandler defaultUserAbortHandler;

static ProcessAbortHandler *handlers;
int32_t handlersMutex = 0;
static SceUID userAbortMemBlock = -1;

int CheckStackPointer(void *stackPointer)
{
    void *stackBounds[2];
    uint32_t *tlsAddr;
    asm volatile("mrc p15, 0, %0, c13, c0, #3" : "=r" (tlsAddr)); // Load TLS address from TPIDRURO register
    if (tlsAddr == NULL)
    {
        LOG("Error: Invalid TLS Address");
        return 0;
    }

    tlsAddr -= (0x800 / sizeof(uint32_t));

#if (EXCEPTION_SAFETY >= 2)
    if (ksceKernelMemcpyFromUser(&stackBounds[0], &tlsAddr[2], sizeof(stackBounds)) < 0)
    {
        LOG("Error: Failed to load stack bounds from TLS");
        return 0;
    }
#else
    stackBounds[0] = (void *)tlsAddr[2];
    stackBounds[1] = (void *)tlsAddr[3];
#endif

    if (stackPointer > stackBounds[0] || stackPointer <= stackBounds[1])
    {
        LOG("Error: Stack pointer for thread 0x%X is out-of-bounds (sp: %p, stack bottom: %p, stack top: %p)", ksceKernelGetThreadId(), stackPointer, stackBounds[0], stackBounds[1]);
        return 0;
    }

    stackPointer -= (0x160 + 0x400); // 0x160 for the abort context, plus an extra 0x400 for use in the abort handler
    if (stackPointer > stackBounds[0] || stackPointer <= stackBounds[1])
    {
        LOG("Insufficient stack space on thread 0x%X to call the abort handler (sp: %p, stack bottom: %p, stack top: %p)", ksceKernelGetThreadId(), stackPointer + (0x160 + 0x400), stackBounds[0], stackBounds[1]);
        return 0;
    }

    return 1;
}

uintptr_t GetProcessExitAddr()
{
    uintptr_t addr;

    LOG("Fatal error encountered while reloading the abort context. Process will be terminated");

    module_get_export_func(ksceKernelGetProcessId(), "SceLibKernel", 0xCAE9ACE6, 0x7595D9AA, &addr); // sceKernelExitProcess

    return addr;
}

void RemoveProcessAbortHandler(SceUID pid);
ProcessAbortHandler *GetProcessAbortHandler(SceUID pid)
{
    ProcessAbortHandler *cur = handlers;
    SceBool dontCreate = pid == -1;

    if (pid == -1)
        pid = ksceKernelGetProcessId();

    while (cur != NULL)
    {
        if (cur->pid == pid)
            break;

        cur = cur->pNext;
    }

    if (cur == NULL && !dontCreate)
    {
        cur = _sceKernelAllocRemoteProcessHeap(pid, sizeof(ProcessAbortHandler), NULL);
        if (cur == NULL)
        {
            LOG("Failed to allocate ProcessAbortHandler for process 0x%X", pid);
            return NULL;
        }

        cur->pid = pid;
        cur->pHandler = defaultUserAbortHandler;
        cur->pNext = handlers;
        cur->userAbortMemBlock = -1;
        handlers = cur;

        if (pid != userAbortMemBlockPid)
        {
            SceKernelAllocMemBlockKernelOpt opt;
            memset(&opt, 0, sizeof(opt));
            opt.size = sizeof(opt);
            opt.attr = SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_PID | SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_MIRROR_BLOCKID | 0x1000000 | 0x800000; // (HAS_PID | HAS_BASE | SHARE_PHYPAGE | SHARE_VBASE)
            opt.mirror_blockid = userAbortMemBlock;
            opt.pid = pid;
            SceUID memBlock = ksceKernelAllocMemBlock("KuBridgeAbortDispatchBlock", SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_SHARED_RX, 0x1000, &opt);
            if (memBlock < 0)
            {
                LOG("Failed to allocate memBlock for user abort handler dispatch (0x%08X)", memBlock);
                RemoveProcessAbortHandler(pid);
                return NULL;
            }
            cur->userAbortMemBlock = memBlock;
        }
        else
        {
            cur->userAbortMemBlock = userAbortMemBlock;
        }
    }
    else if (cur == NULL)
        LOG("No user abort handler found for process 0x%X", pid);

    return cur;
}

void RemoveProcessAbortHandler(SceUID pid)
{
    ProcessAbortHandler *cur = handlers, *prev = NULL;

    if (pid == -1)
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

        if (cur->userAbortMemBlock != -1 && cur->userAbortMemBlock != userAbortMemBlock)
            ksceKernelFreeMemBlock(cur->userAbortMemBlock);

        _sceKernelFreeRemoteProcessHeap(pid, cur);
    }
    else
        LOG("No user abort hanlder found for process 0x%X", pid);
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
        opt.attr = 0x8000000 | SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_PID; // Attr 0x8000000 allocates the memBlock at the highest address available
        opt.pid = pid;
        userAbortMemBlock = ksceKernelAllocMemBlock("KuBridgeAbortHandlerUserBlock", SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_SHARED_RX, 0x1000, &opt);
        if (userAbortMemBlock < 0)
        {
            LOG("Failed to allocate base memBlock for user abort handler dispatch (0x%08X)", userAbortMemBlock);
            return 0;
        }
        ksceKernelGetMemBlockBase(userAbortMemBlock, &userAbortBase);

        _sceKernelProcCopyToUserRx(pid, userAbortBase, &exceptions_user_bin[0], exceptions_user_bin_len);

        defaultUserAbortHandler = (KuKernelAbortHandler)(userAbortBase + (sizeof(SceUInt32) * 5));
        userAbortMemBlockPid = pid;
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

void InitExceptionHandlers()
{
    if (module_get_export_func(KERNEL_PID, "SceExcpmgr", TAI_ANY_LIBRARY, 0x03499636, (uintptr_t *)&_sceKernelRegisterExceptionHandler) < 0) // 3.60
        module_get_export_func(KERNEL_PID, "SceExcpmgr", TAI_ANY_LIBRARY, 0x00063675, (uintptr_t *)&_sceKernelRegisterExceptionHandler); // >= 3.63

    if (module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0x30931572, (uintptr_t *)&_sceKernelProcCopyToUserRx) < 0) // 3.60
        module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0x2995558D, (uintptr_t *)&_sceKernelProcCopyToUserRx);     // >= 3.63

    module_get_export_func(KERNEL_PID, "SceProcessmgr", TAI_ANY_LIBRARY, 0x00B1CA0F, (uintptr_t *)&_sceKernelAllocRemoteProcessHeap);
    module_get_export_func(KERNEL_PID, "SceProcessmgr", TAI_ANY_LIBRARY, 0x9C28EA9A, (uintptr_t *)&_sceKernelFreeRemoteProcessHeap);
    module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0xA78755EB, (uintptr_t *)&_sceGUIDGetName);

    _sceKernelRegisterExceptionHandler(SCE_EXCP_DABT, 0, &DabtExceptionHandler_lvl0);
    _sceKernelRegisterExceptionHandler(SCE_EXCP_PABT, 0, &PabtExceptionHandler_lvl0);
    _sceKernelRegisterExceptionHandler(SCE_EXCP_UNDEF_INSTRUCTION, 0, &UndefExceptionHandler_lvl0);

    SceProcEventHandler handler;
    memset(&handler, 0, sizeof(handler));
    handler.size = sizeof(handler);
    handler.create = CreateProcess;
    handler.exit = DestroyProcess;
    handler.kill = DestroyProcess;

    ksceKernelRegisterProcEventHandler("KuBridgeProcessHandler", &handler, 0);
}

int kuKernelRegisterAbortHandler(KuKernelAbortHandler pHandler, KuKernelAbortHandler *pOldHandler, KuKernelAbortHandlerOpt *pOpt)
{
    int ret;
    int irqState = ksceKernelCpuSpinLockIrqSave(&handlersMutex);

    if (pHandler == NULL)
    {
        LOG("Invalid pHandler");
        ret = SCE_KERNEL_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    if (pOpt != NULL)
    {
        KuKernelAbortHandlerOpt opt;
        ret = ksceKernelMemcpyFromUser(&opt, pOpt, sizeof(KuKernelAbortHandlerOpt));
        if (ret < 0)
        {
            LOG("Invalid pOpt");
            goto exit;
        }

        if (opt.size != sizeof(KuKernelAbortHandlerOpt))
        {
            LOG("pOpt->size != sizeof(KuKernelAbortHandlerOpt)");
            ret = SCE_KERNEL_ERROR_INVALID_ARGUMENT_SIZE;
            goto exit;
        }
    }

    ProcessAbortHandler *procHandler = GetProcessAbortHandler(ksceKernelGetProcessId());
    if (procHandler == NULL)
    {
        ret = SCE_KERNEL_ERROR_NO_MEMORY;
        goto exit;
    }

    if (pOldHandler != NULL)
    {
        ret = ksceKernelMemcpyToUser(pOldHandler, &procHandler->pHandler, sizeof(KuKernelAbortHandler));
        if (ret < 0)
        {
            LOG("Invalid pOldHandler");
            goto exit;
        }
    }
        
    procHandler->pHandler = pHandler;
    if (pHandler == defaultUserAbortHandler)
        RemoveProcessAbortHandler(ksceKernelGetProcessId());

    ret = 0;
exit:
    ksceKernelCpuSpinLockIrqRestore(&handlersMutex, irqState);

    return ret;
}

void kuKernelReleaseAbortHandler()
{
    int irqState = ksceKernelCpuSpinLockIrqSave(&handlersMutex);

    RemoveProcessAbortHandler(ksceKernelGetProcessId());

    ksceKernelCpuSpinLockIrqRestore(&handlersMutex, irqState);
}
