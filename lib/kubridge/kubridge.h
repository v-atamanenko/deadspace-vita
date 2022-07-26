#ifndef __KUBRDIGE_H__
#define __KUBRDIGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <psp2/types.h>
#include <psp2/kernel/sysmem.h>

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

typedef struct SceKernelAddrPair {
  uint32_t addr;                  //!< Address
  uint32_t length;                //!< Length
} SceKernelAddrPair;

typedef struct SceKernelPaddrList {
  uint32_t size;                  //!< sizeof(SceKernelPaddrList)
  uint32_t list_size;             //!< Size in elements of the list array
  uint32_t ret_length;            //!< Total physical size of the memory pairs
  uint32_t ret_count;             //!< Number of elements of list filled by ksceKernelGetPaddrList
  SceKernelAddrPair *list;        //!< Array of physical addresses and their lengths pairs
} SceKernelPaddrList;

typedef struct SceKernelAllocMemBlockKernelOpt {
  SceSize size;                   //!< sizeof(SceKernelAllocMemBlockKernelOpt)
  SceUInt32 field_4;
  SceUInt32 attr;                 //!< OR of SceKernelAllocMemBlockAttr
  SceUInt32 field_C;
  SceUInt32 paddr;
  SceSize alignment;
  SceUInt32 extraLow;
  SceUInt32 extraHigh;
  SceUInt32 mirror_blockid;
  SceUID pid;
  SceKernelPaddrList *paddr_list;
  SceUInt32 field_2C;
  SceUInt32 field_30;
  SceUInt32 field_34;
  SceUInt32 field_38;
  SceUInt32 field_3C;
  SceUInt32 field_40;
  SceUInt32 field_44;
  SceUInt32 field_48;
  SceUInt32 field_4C;
  SceUInt32 field_50;
  SceUInt32 field_54;
} SceKernelAllocMemBlockKernelOpt;

SceUID kuKernelAllocMemBlock(const char *name, SceKernelMemBlockType type, SceSize size, SceKernelAllocMemBlockKernelOpt *opt);

void kuKernelFlushCaches(const void *ptr, SceSize len);

int kuKernelCpuUnrestrictedMemcpy(void *dst, const void *src, SceSize len);

int kuKernelRegisterAbortHandler(KuKernelAbortHandler pHandler, KuKernelAbortHandler *pOldHandler);
void kuKernelReleaseAbortHandler();

#ifdef __cplusplus
}
#endif

#endif
