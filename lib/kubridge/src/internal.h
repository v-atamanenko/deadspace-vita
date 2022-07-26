#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include <psp2common/types.h>
#include <psp2kern/kernel/debug.h>

#ifndef NDEBUG
#define LOG(msg, ...) ksceDebugPrintf("[kubridge ] - %s: "msg, __FUNCTION__, ##__VA_ARGS__)
#else
#define LOG(msg, ...)
#endif

#define KU_KERNEL_ABORT_TYPE_DATA_ABORT 0
#define KU_KERNEL_ABORT_TYPE_PREFETCH_ABORT 1

typedef struct KuKernelAbortContext
{
    SceUInt32 r0;
    SceUInt32 r1;
    SceUInt32 r2;
    SceUInt32 r3;
    SceUInt32 r4;
    SceUInt32 r5;
    SceUInt32 r6;
    SceUInt32 r7;
    SceUInt32 r8;
    SceUInt32 r9;
    SceUInt32 r10;
    SceUInt32 r11;
    SceUInt32 r12;
    SceUInt32 sp;
    SceUInt32 lr;
    SceUInt32 pc;
    SceUInt64 vfpRegisters[32];
    SceUInt32 SPSR;
    SceUInt32 FPSCR;
    SceUInt32 FPEXC;
    SceUInt32 FSR;
    SceUInt32 FAR;
    SceUInt32 abortType;
} KuKernelAbortContext;

typedef void (*KuKernelAbortHandler)(KuKernelAbortContext *);

typedef struct ProcessAbortHandler
{
    SceUID pid;
    SceUID userAbortMemBlock;
    KuKernelAbortHandler pHandler;
    struct ProcessAbortHandler *pNext;
} ProcessAbortHandler;

void SetupExceptionHandlers();

#endif