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

#include "utils/utils.h"

#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "OCDFAInspection"

so_hook fun0026d4e4_a;
so_hook fun0026d4e4_b;
so_hook fun0026d4e4_c;

uint StrlcpyUTF16ToUTF8_a(char *param_1,uint param_2,wchar_t *param_3,uint param_4) {
    fprintf(stderr, "Hook via symbol worked!\n");
    return 0;
}

uint StrlcpyUTF16ToUTF8_b(char *param_1,uint param_2,wchar_t *param_3,uint param_4) {
    fprintf(stderr, "Hook via addr worked, going to SO_CONTINUE!\n");
    return SO_CONTINUE(uint, fun0026d4e4_b, param_1, param_2, param_3, param_4);
}

typedef char byte;

uint StrlcpyUTF16ToUTF8_c(char *param_1,uint param_2,wchar_t *param_3,uint param_4) {
    fprintf(stderr, "Hook via addr worked, proceeding with reimpl!\n");
    ushort uVar1;
    uint uVar2;
    uint uVar3;

    uVar3 = 0;
    if (param_4 != 0) {
        do {
            uVar1 = *(ushort *)param_3;
            uVar2 = (uint)uVar1;
            if (uVar2 < 0x80) {
                if (uVar2 == 0) break;
                if ((byte *)param_1 == (byte *)0x0) {
                    uVar3 = uVar3 + 1;
                }
                else {
                    uVar3 = uVar3 + 1;
                    if (uVar3 < param_2) {
                        *param_1 = (byte)uVar1;
                        param_1 = (char *)((byte *)param_1 + 1);
                    }
                }
            }
            else if (uVar2 < 0x800) {
                if ((byte *)param_1 == (byte *)0x0) {
                    uVar3 = uVar3 + 2;
                }
                else {
                    uVar3 = uVar3 + 2;
                    if (uVar3 < param_2) {
                        *param_1 = ~((byte)~(byte)(((uint)(uVar1 >> 6) << 0x1a) >> 0x18) >> 2);
                        ((byte *)param_1)[1] = ~((byte)~(byte)(((uVar2 & 0x3f) << 0x19) >> 0x18) >> 1);
                        param_1 = (char *)((byte *)param_1 + 2);
                    }
                }
            }
            else if ((byte *)param_1 == (byte *)0x0) {
                uVar3 = uVar3 + 3;
            }
            else {
                uVar3 = uVar3 + 3;
                if (uVar3 < param_2) {
                    *param_1 = ~((byte)~(byte)(((uint)(uVar1 >> 0xc) << 0x1b) >> 0x18) >> 3);
                    ((byte *)param_1)[1] = ~((byte)~(byte)(((uVar1 >> 6 & 0x3f) << 0x19) >> 0x18) >> 1);
                    ((byte *)param_1)[2] = ~((byte)~(byte)(((uVar2 & 0x3f) << 0x19) >> 0x18) >> 1);
                    param_1 = (char *)((byte *)param_1 + 3);
                }
            }
            param_4 = param_4 - 1;
            param_3 = (wchar_t *)((int)param_3 + 2);
        } while (param_4 != 0);
    }
    if ((byte *)param_1 != (byte *)0x0 && param_2 != 0) {
        *param_1 = 0;
    }
    return uVar3;
}

void wchar_to_char(const char* src, char* dst) {
    int i = 0;
    int u = 0;
    while (1) {
        dst[i] = *(src+u);
        i += 1;
        u += 2;
        if(*(src+u) == '\0' && *(src+u+1) == '\0') {
            dst[i] = '\0';
            break;
        }
    }
}

int EA__IO__Directory__Exists(char* path) {
    char p[1024];
    wchar_to_char(path, p);
    return file_exists(p);
}
/*
uint EA__IO__StrlcpyUTF16ToUTF8(char *dst,uint param_2,char *src,uint param_4) {
    //if (dst) free(dst);
    dst = malloc(1024);
    wchar_to_char(src, dst);
    return 0x100;
}*/
uint EA__IO__StrlcpyUTF16ToUTF8(char *param_1,uint param_2,char *param_3,uint param_4) {
    ushort uVar1;
    uint uVar2;
    uint uVar3;

    uVar3 = 0;
    if (param_4 != 0) {
        do {
            uVar1 = *(ushort *)param_3;
            uVar2 = (uint)uVar1;
            if (uVar2 < 0x80) {
                if (uVar2 == 0) break;
                if ((byte *)param_1 == (byte *)0x0) {
                    uVar3 = uVar3 + 1;
                }
                else {
                    uVar3 = uVar3 + 1;
                    if (uVar3 < param_2) {
                        *param_1 = (byte)uVar1;
                        param_1 = (char *)((byte *)param_1 + 1);
                    }
                }
            }
            else if (uVar2 < 0x800) {
                if ((byte *)param_1 == (byte *)0x0) {
                    uVar3 = uVar3 + 2;
                }
                else {
                    uVar3 = uVar3 + 2;
                    if (uVar3 < param_2) {
                        *param_1 = ~((byte)~(byte)(((uint)(uVar1 >> 6) << 0x1a) >> 0x18) >> 2);
                        ((byte *)param_1)[1] = ~((byte)~(byte)(((uVar2 & 0x3f) << 0x19) >> 0x18) >> 1);
                        param_1 = (char *)((byte *)param_1 + 2);
                    }
                }
            }
            else if ((byte *)param_1 == (byte *)0x0) {
                uVar3 = uVar3 + 3;
            }
            else {
                uVar3 = uVar3 + 3;
                if (uVar3 < param_2) {
                    *param_1 = ~((byte)~(byte)(((uint)(uVar1 >> 0xc) << 0x1b) >> 0x18) >> 3);
                    ((byte *)param_1)[1] = ~((byte)~(byte)(((uVar1 >> 6 & 0x3f) << 0x19) >> 0x18) >> 1);
                    ((byte *)param_1)[2] = ~((byte)~(byte)(((uVar2 & 0x3f) << 0x19) >> 0x18) >> 1);
                    param_1 = (char *)((byte *)param_1 + 3);
                }
            }
            param_4 = param_4 - 1;
            param_3 = (wchar_t *)((int)param_3 + 2);
        } while (param_4 != 0);
    }
    if ((byte *)param_1 != (byte *)0x0 && param_2 != 0) {
        *param_1 = 0;
    }
    return uVar3;
}

void patch_game(void) {
    // Always fail check for "appbundle:" in filename ==> don't use JNI IO funcs.
    uint32_t fix = 0xe35a0001;
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00272e50), &fix, sizeof(fix));

    // Same for xperia
    // original
    // 0023bf6c 07 00 00 1a     bne        LAB_0023bf90
    // patch
    // 0023bf6c 07 00 00 ea     b          LAB_0023bf90
    //uint32_t fix = 0xea000007;
    //kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0023bf6c), &fix, sizeof(fix));

    uint32_t nop = 0xe1a00000;
    //kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0026d4ec), &nop, sizeof(nop));

    //fun0026d4e4_a = hook_addr(so_symbol(&so_mod, "_ZN2EA2IO18StrlcpyUTF16ToUTF8EPcjPKwj"), (uintptr_t)&StrlcpyUTF16ToUTF8_a);
    //fun0026d4e4_b = hook_addr((uintptr_t)so_mod.text_base + 0x0026d4e4, (uintptr_t)&StrlcpyUTF16ToUTF8_b);
    //fun0026d4e4_c = hook_addr((uintptr_t)so_mod.text_base + 0x0026d4e4, (uintptr_t)&StrlcpyUTF16ToUTF8_c);
    //fun0026d4e4_c = hook_addr((uintptr_t)so_mod.text_base + 0x0027cb90, (uintptr_t)&EA__IO__Directory__Exists);

    //fun0026d4e4_c = hook_addr((uintptr_t)so_mod.text_base + 0x0026d4e4, (uintptr_t)&EA__IO__StrlcpyUTF16ToUTF8);


}
