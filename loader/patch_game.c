/*
 * patch_game.c
 *
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 * Copyright (C) 2022 psykana
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "patch_game.h"

#include <vitaGL.h>
#include <stdio.h>
#include <so_util.h>
#include <main.h>
#include <malloc.h>
#include <stdlib.h>
#include <kubridge.h>
#include <time.h>
#include <string.h>

#include "utils/utils.h"

#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "OCDFAInspection"

// Ghidra typedefs
#define byte char
#define longlong long long
#define ulonglong unsigned long long
#define undefined4 int
#define undefined8 int64_t
#define dword int
#define bool int
#define false 0
#define true 1
#define code void
#define SBORROW4(x, y) ((y) > (x))

uint8_t* DAT_003dca80;
uint8_t* DAT_003dca84;
uint8_t* DAT_003dcaa0;

uint FUN_00264a04(uint param_1,int param_2,float *param_3,int *param_4,int *param_5,int param_6) {
    float *pfVar1;
    uint uVar2;
    float *pfVar3;
    float *pfVar4;
    float *pfVar5;
    uint in_fpscr;
    float fVar6;
    float fVar7;
    float fVar8;
    float fVar9;
    float fVar10;
    float fVar11;
    float fVar12;
    float fVar13;
    int local_68;
    uint local_64;

    local_64 = (uint)*(ushort *)((int)param_5 + 2);
    pfVar1 = param_3 + param_1;
    local_68 = *param_4;
    pfVar5 = param_3 + (param_1 & 0xfff8);
    pfVar4 = param_3;
    if (param_3 < pfVar5) {
        do {
            pfVar3 = (float *)(local_68 * 4 + param_2);
            fVar6 = *pfVar3;
            fVar7 = *(float *)(((local_64 + param_6 >> 0x10) + local_68) * 4 + param_2);
            uVar2 = local_64 + param_6 + param_6;
            fVar8 = *(float *)(((uVar2 >> 0x10) + local_68) * 4 + param_2);
            uVar2 = uVar2 + param_6;
            fVar9 = *(float *)(((uVar2 >> 0x10) + local_68) * 4 + param_2);
            uVar2 = uVar2 + param_6;
            fVar10 = *(float *)(((uVar2 >> 0x10) + local_68) * 4 + param_2);
            uVar2 = uVar2 + param_6;
            fVar11 = *(float *)(((uVar2 >> 0x10) + local_68) * 4 + param_2);
            uVar2 = uVar2 + param_6;
            fVar12 = *(float *)(((uVar2 >> 0x10) + local_68) * 4 + param_2);
            uVar2 = uVar2 + param_6;
            fVar13 = *(float *)(((uVar2 >> 0x10) + local_68) * 4 + param_2);
            *pfVar4 = fVar6 + (float)(long long)(int)local_64 * 1.5258e-05 * (pfVar3[1] - fVar6);
            pfVar4[1] = fVar7;
            pfVar4[2] = fVar8;
            pfVar4[3] = fVar9;
            pfVar4[4] = fVar10;
            pfVar4[5] = fVar11;
            pfVar4[6] = fVar12;
            pfVar4[7] = fVar13;
            uVar2 = uVar2 + param_6;
            local_64 = uVar2 & 0xffff;
            local_68 = (uVar2 >> 0x10) + local_68;
            pfVar4 = pfVar4 + 8;
        } while (pfVar4 < pfVar5);
        param_3 = (float *)((int)param_3 + (~(uint)param_3 + (int)pfVar5 & 0xffffffe0) + 0x20);
    }
    uVar2 = in_fpscr & 0xffc8ffff;
    for (; param_3 < pfVar1; param_3 = param_3 + 1) {
        fVar6 = *(float *)(param_2 + local_68 * 4);
        *param_3 = fVar6 + (*(float *)(param_2 + (local_68 + 1) * 4) - fVar6) *
                           (float)(unsigned long long)local_64 * 1.5258e-05;
        local_68 = local_68 + (param_6 + local_64 >> 0x10);
        uVar2 = param_6 + local_64 & 0xffff;
        local_64 = uVar2;
    }
    *param_5 = local_64 << 0x10;
    *param_4 = local_68;
    return uVar2;
}

uint FUN_0025e044(undefined4 param_1,byte *param_2,float *param_3) {
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  int iVar5;
  byte *pbVar6;
  uint in_fpscr;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  
  bVar1 = *param_2;
  *param_3 = (float)(longlong)(int)((bVar1 & 0xf0) + (char)param_2[1] * 0x100) * 3.051758e-05;
  bVar2 = param_2[2];
  param_3[1] = (float)(longlong)(int)((bVar2 & 0xf0) + (char)param_2[3] * 0x100) * 3.051758e-05;
  param_3[0x20] =
       (float)(longlong)(int)((param_2[4] & 0xf0) + (char)param_2[5] * 0x100) * 3.051758e-05;
  param_3[0x21] =
       (float)(longlong)(int)((param_2[6] & 0xf0) + (char)param_2[7] * 0x100) * 3.051758e-05;
  iVar5 = (bVar1 & 0xf) * 8;
  fVar7 = *(float *)(DAT_003dca80 + iVar5);
  fVar8 = *(float *)(DAT_003dca84 + iVar5);
  fVar9 = *(float *)(DAT_003dcaa0 + (bVar2 & 0xf) * 4);
  param_3[0x40] =
       (float)(longlong)(int)((param_2[8] & 0xf0) + (char)param_2[9] * 0x100) * 3.051758e-05;
  param_3[0x41] =
       (float)(longlong)(int)((param_2[10] & 0xf0) + (char)param_2[0xb] * 0x100) * 3.051758e-05;
  param_3[0x60] =
       (float)(longlong)(int)((param_2[0xc] & 0xf0) + (char)param_2[0xd] * 0x100) * 3.051758e-05;
  pbVar6 = param_2 + 0x10;
  param_3[0x61] =
       (float)(longlong)(int)((param_2[0xe] & 0xf0) + (char)param_2[0xf] * 0x100) * 3.051758e-05;
  param_3 = param_3 + 2;
  iVar5 = 0xf;
  do {
    bVar1 = *pbVar6;
    bVar2 = pbVar6[1];
    bVar3 = pbVar6[2];
    bVar4 = pbVar6[3];
    fVar10 = (float)(longlong)(int)((uint)(bVar1 >> 4) << 0x1c) * fVar9 + fVar7 * param_3[-1] +
             fVar8 * param_3[-2];
    *param_3 = fVar10;
    param_3[0x20] = (float)(longlong)(int)((uint)(bVar2 >> 4) << 0x1c);
    param_3[0x40] = (float)(longlong)(int)((uint)(bVar3 >> 4) << 0x1c);
    param_3[0x60] = (float)(longlong)(int)((uint)(bVar4 >> 4) << 0x1c);
    param_3[1] = (float)(longlong)(int)((uint)bVar1 << 0x1c) * fVar9 + fVar7 * fVar10 +
                 fVar8 * param_3[-1];
    param_3[0x21] = (float)(longlong)(int)((uint)bVar2 << 0x1c);
    param_3[0x41] = (float)(longlong)(int)((uint)bVar3 << 0x1c);
    param_3[0x61] = (float)(longlong)(int)((uint)bVar4 << 0x1c);
    param_3 = param_3 + 2;
    pbVar6 = pbVar6 + 4;
    iVar5 = iVar5 + -1;
  } while (iVar5 != 0);
  return in_fpscr & 0xffc8ffff;
}

float * FUN_0026f0d4(float *param_1,float *param_2,float param_3,uint param_4) {
  uint uVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  int iVar5;
  bool bVar6;
  uint in_fpscr;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  ulonglong uVar15;
  
  if (((((uint)param_2 | (uint)param_1) & 0xf) == 0) && ((param_4 & 0xf) == 0)) {
    uVar1 = (uint)(param_3 == 1.0) << 0x1e;
    bVar6 = false;
    if (!(uVar1 >> 0x1e)) {
      param_1 = (float *)memcpy(param_1,param_2,param_4 << 2);
    }
    else {
      do {
        fVar7 = *param_2;
        fVar8 = param_2[1];
        fVar9 = param_2[2];
        fVar10 = param_2[3];
        fVar11 = param_2[4];
        fVar12 = param_2[5];
        fVar13 = param_2[6];
        fVar14 = param_2[7];
        uVar15 = *(ulonglong *)(param_2 + 8);
        uVar2 = *(undefined8 *)(param_2 + 10);
        uVar3 = *(undefined8 *)(param_2 + 0xc);
        uVar4 = *(undefined8 *)(param_2 + 0xe);
        param_2 = param_2 + 0x10;
        if (!bVar6) {
          fVar7 = fVar7 * param_3;
          uVar15 = uVar15 & 0xffffffff00000000 | (ulonglong)(uint)((float)uVar15 * param_3);
        }
        *param_1 = fVar7;
        param_1[1] = fVar8;
        param_1[2] = fVar9;
        param_1[3] = fVar10;
        param_1[4] = fVar11;
        param_1[5] = fVar12;
        param_1[6] = fVar13;
        param_1[7] = fVar14;
        param_1[8] = (float)uVar15;
        param_1[9] = (float)(uVar15 >> 0x20);
        param_1[10] = (float)uVar2;
        param_1[0xb] = (float)((ulonglong)uVar2 >> 0x20);
        param_1[0xc] = (float)uVar3;
        param_1[0xd] = (float)((ulonglong)uVar3 >> 0x20);
        param_1[0xe] = (float)uVar4;
        param_1[0xf] = (float)((ulonglong)uVar4 >> 0x20);
        param_1 = param_1 + 0x10;
        bVar6 = param_4 > 0x10;
        param_4 = param_4 - 0x10;
      } while (param_4 != 0);
      param_1 = (float *)(in_fpscr & 0xfc8ffff | (uint)(param_3 < 1.0) << 0x1f | uVar1 |
                         (uint)(1.0 <= param_3) << 0x1d);
    }
  }
  else if (param_1 < param_1 + param_4) {
    iVar5 = 0;
    do {
      *(float *)((int)param_1 + iVar5) = param_3 * *(float *)((int)param_2 + iVar5);
      iVar5 = iVar5 + 4;
    } while ((float *)((int)param_1 + iVar5) < param_1 + param_4);
  }
  return param_1;
}

void FUN_00278424(int param_1,int param_2,int param_3) {
  ushort uVar1;
  ulonglong uVar2;
  ulonglong uVar3;
  ulonglong uVar4;
  uint uVar5;
  float *pfVar6;
  uint uVar7;
  uint uVar8;
  int iVar9;
  undefined4 uVar10;
  ulonglong *puVar11;
  float *pfVar12;
  bool bVar13;
  float fVar14;
  float fVar15;
  ulonglong uVar16;
  
  uVar5 = *(uint *)(param_1 + 0x300);
  uVar10 = *(undefined4 *)(param_3 + 4);
  if (uVar5 != 0) {
    uVar1 = *(ushort *)(param_2 + 0xe);
    uVar7 = 0;
    iVar9 = param_1;
    while( true ) {
      FUN_0026f0d4(uVar7 * 4 * (uint)uVar1 + *(int *)(param_2 + 4),uVar10,
                   *(undefined4 *)(iVar9 + 0x1cc),0x100);
      uVar5 = *(uint *)(param_1 + 0x300);
      uVar7 = uVar7 + 1;
      iVar9 = iVar9 + 4;
      if (uVar5 <= uVar7) break;
      uVar1 = *(ushort *)(param_2 + 0xe);
    }
  }
  uVar7 = *(uint *)(param_1 + 0x2fc);
  if (1 < uVar7) {
    uVar8 = 1;
    do {
      if (uVar5 != 0) {
        pfVar6 = (float *)(uVar8 * (uint)*(ushort *)(param_3 + 0xe) * 4 + *(int *)(param_3 + 4));
        uVar1 = *(ushort *)(param_2 + 0xe);
        pfVar12 = (float *)(param_1 + uVar8 * 0x20 + 0x1cc);
        uVar7 = 0;
        while( true ) {
          fVar15 = *pfVar12;
          puVar11 = (ulonglong *)(uVar7 * 4 * (uint)uVar1 + *(int *)(param_2 + 4));
          if ((((uint)puVar11 | (uint)pfVar6) & 0xf) == 0) {
            bVar13 = false;
            if (fVar15 == 1.0) {
              iVar9 = 0x100;
              do {
                fVar15 = *pfVar6;
                pfVar6 = pfVar6 + 8;
                uVar16 = *puVar11;
                uVar2 = puVar11[1];
                uVar3 = puVar11[2];
                uVar4 = puVar11[3];
                if (!bVar13) {
                  uVar16 = uVar16 & 0xffffffff00000000 | (ulonglong)(uint)((float)uVar16 + fVar15);
                }
                *(int *)puVar11 = (int)uVar16;
                *(int *)((int)puVar11 + 4) = (int)(uVar16 >> 0x20);
                *(int *)(puVar11 + 1) = (int)uVar2;
                *(int *)((int)puVar11 + 0xc) = (int)(uVar2 >> 0x20);
                *(int *)(puVar11 + 2) = (int)uVar3;
                *(int *)((int)puVar11 + 0x14) = (int)(uVar3 >> 0x20);
                *(int *)(puVar11 + 3) = (int)uVar4;
                *(int *)((int)puVar11 + 0x1c) = (int)(uVar4 >> 0x20);
                puVar11 = puVar11 + 4;
                bVar13 = SBORROW4(iVar9,8);
                iVar9 = iVar9 + -8;
              } while (iVar9 != 0);
            }
            else {
              iVar9 = 0x100;
              do {
                fVar14 = *pfVar6;
                pfVar6 = pfVar6 + 8;
                uVar16 = *puVar11;
                uVar2 = puVar11[1];
                uVar3 = puVar11[2];
                uVar4 = puVar11[3];
                if (!bVar13) {
                  uVar16 = uVar16 & 0xffffffff00000000 |
                           (ulonglong)(uint)((float)uVar16 + fVar14 * fVar15);
                }
                *(int *)puVar11 = (int)uVar16;
                *(int *)((int)puVar11 + 4) = (int)(uVar16 >> 0x20);
                *(int *)(puVar11 + 1) = (int)uVar2;
                *(int *)((int)puVar11 + 0xc) = (int)(uVar2 >> 0x20);
                *(int *)(puVar11 + 2) = (int)uVar3;
                *(int *)((int)puVar11 + 0x14) = (int)(uVar3 >> 0x20);
                *(int *)(puVar11 + 3) = (int)uVar4;
                *(int *)((int)puVar11 + 0x1c) = (int)(uVar4 >> 0x20);
                puVar11 = puVar11 + 4;
                bVar13 = SBORROW4(iVar9,8);
                iVar9 = iVar9 + -8;
              } while (iVar9 != 0);
            }
          }
          else {
            iVar9 = 0;
            do {
              *(float *)((int)puVar11 + iVar9) =
                   *(float *)((int)puVar11 + iVar9) + fVar15 * *(float *)((int)pfVar6 + iVar9);
              iVar9 = iVar9 + 4;
            } while (iVar9 != 0x400);
          }
          uVar5 = *(uint *)(param_1 + 0x300);
          uVar7 = uVar7 + 1;
          pfVar12 = pfVar12 + 1;
          if (uVar5 <= uVar7) break;
          uVar1 = *(ushort *)(param_2 + 0xe);
        }
        uVar7 = *(uint *)(param_1 + 0x2fc);
      }
      uVar8 = uVar8 + 1;
    } while (uVar8 < uVar7);
  }
  return;
}

volatile int sus_thread_count = 0;
so_hook susthread_hook;

int SusThread(void* arg) {
    printf("SusThread wants to run with arg 0x%x, delaying\n", arg);
    sus_thread_count++;
    sceKernelDelayThread(sus_thread_count * 4 * 1000 * 1000); // delay 4xThreadNum seconds
    SO_CONTINUE(int, susthread_hook, arg);
}


void patch_game(void) {
    // Always fail check for "appbundle:" in filename ==> don't use JNI IO funcs.
    uint32_t fix = 0xea000007;
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0022bf6c), &fix, sizeof(fix));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0022bbe8), &fix, sizeof(fix));
    uint32_t nop = 0xe1a00000;
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0022b214), &nop, sizeof(nop));
    
    // Functions using broken vfp that were easy enough to reimplement.
    DAT_003dca80 = (uintptr_t)so_mod.text_base + 0x003dca80;
    DAT_003dca84 = (uintptr_t)so_mod.text_base + 0x003dca84;
    DAT_003dcaa0 = (uintptr_t)so_mod.text_base + 0x003dcaa0;

    hook_addr((uintptr_t)so_mod.text_base + 0x00264a04, (uintptr_t)&FUN_00264a04);
    hook_addr((uintptr_t)so_mod.text_base + 0x0025e044, (uintptr_t)&FUN_0025e044);
    hook_addr((uintptr_t)so_mod.text_base + 0x0026f0d4, (uintptr_t)&FUN_0026f0d4);
    hook_addr((uintptr_t)so_mod.text_base + 0x00278424, (uintptr_t)&FUN_00278424);
    
    // Couple of broken vmlavc.f32/vaddvc.f32 instructions related to UI drawing.
    // Without the following patches, it crashes; with them, there is a barely noticeable visual glitch.
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00291a28), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00291ae0), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00291848), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00291970), &nop, sizeof(nop));

    // The rest of vmlavc.f32, just in case.
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00277ea0), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x002785cc), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00278dcc), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x002922fc), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00292344), &nop, sizeof(nop));
    
    // The rest of vaddvc.f32, just in case.
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00277d64), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00278508), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00278cd4), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00292118), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00292258), &nop, sizeof(nop));

    // Sus thread
    // A thread that causes some kind of undefined behaviour and crashes in different places if not delayed
    susthread_hook = hook_addr((uintptr_t)so_mod.text_base + 0x00320624, (uintptr_t)&SusThread);
}
