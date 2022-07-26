#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <taihen.h>

#include "internal.h"

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);

void (* _ksceKernelCpuDcacheWritebackInvalidateRange)(const void *ptr, SceSize len);
void (* _ksceKernelCpuIcacheInvalidateRange)(const void *ptr, SceSize len);
void (* _ksceKernelCpuIcacheAndL2WritebackInvalidateRange)(const void *ptr, SceSize len);

void kuKernelFlushCaches(const void *ptr, SceSize len) {
  uintptr_t ptr_aligned;
  ptr_aligned = (uintptr_t)ptr & ~0x1F;
  len = (((uintptr_t)ptr + len + 0x1F) & ~0x1F) - ptr_aligned;
  _ksceKernelCpuDcacheWritebackInvalidateRange((void *)ptr_aligned, len);
  // _ksceKernelCpuIcacheAndL2WritebackInvalidateRange((void *)ptr_aligned, len);
  _ksceKernelCpuIcacheInvalidateRange((void *)ptr_aligned, len);
}

SceUID kuKernelAllocMemBlock(const char *name, SceKernelMemBlockType type, SceSize size, SceKernelAllocMemBlockKernelOpt *opt) {
  char k_name[32];
  SceKernelAllocMemBlockKernelOpt k_opt;
  uint32_t state;
  int res;

  ENTER_SYSCALL(state);

  res = ksceKernelStrncpyUserToKernel(k_name, name, sizeof(k_name));
  if (res < 0)
    goto error;

  res = ksceKernelMemcpyUserToKernel(&k_opt, opt, sizeof(k_opt));
  if (res < 0)
    goto error;

  res = ksceKernelAllocMemBlock(k_name, type, size, &k_opt);
  if (res < 0)
    goto error;

  res = ksceKernelCreateUserUid(ksceKernelGetProcessId(), res);

error:
  EXIT_SYSCALL(state);
  return res;
}

int kuKernelCpuUnrestrictedMemcpy(void *dst, const void *src, SceSize len) {
  int prev_dacr;

  asm volatile("mrc p15, 0, %0, c3, c0, 0" : "=r" (prev_dacr));
  asm volatile("mcr p15, 0, %0, c3, c0, 0" :: "r" (0x15450FC3));

  for (int i = 0; i < len; i++) {
    uint32_t val;
    __asm__ volatile ("ldrbt %0, [%1]" : "=r" (val) : "r" (src + i));
    __asm__ volatile ("strbt %0, [%1]" :: "r" (val), "r" (dst + i));
  }

  asm volatile("mcr p15, 0, %0, c3, c0, 0" :: "r" (prev_dacr));
  return 0;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  int res;

  res = module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0x6BA2E51C, (uintptr_t *)&_ksceKernelCpuDcacheWritebackInvalidateRange);
  if (res < 0)
    module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0x4F442396, (uintptr_t *)&_ksceKernelCpuDcacheWritebackInvalidateRange);
  res = module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0xF4C7F578, (uintptr_t *)&_ksceKernelCpuIcacheInvalidateRange);
  if (res < 0)
    module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0x2E637B1D, (uintptr_t *)&_ksceKernelCpuIcacheInvalidateRange);
  res = module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0x19F17BD0, (uintptr_t *)&_ksceKernelCpuIcacheAndL2WritebackInvalidateRange);
  if (res < 0)
    module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0x73E895EA, (uintptr_t *)&_ksceKernelCpuIcacheAndL2WritebackInvalidateRange);

  SetupExceptionHandlers();

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  return SCE_KERNEL_STOP_SUCCESS;
}
