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

#if GRAPHICS_API == GRAPHICS_API_PVR
#include <GLES2/gl2.h>
#else
#include <vitaGL.h>
#include <stdio.h>
#include <so_util.h>
#include <main.h>
#include <malloc.h>
#include <stdlib.h>

#endif

#include "utils/utils.h"

#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "OCDFAInspection"

extern void *__cxa_guard_abort; // NOLINT(bugprone-reserved-identifier)
extern void *__cxa_guard_acquire; // NOLINT(bugprone-reserved-identifier)
extern void *__cxa_guard_release; // NOLINT(bugprone-reserved-identifier)

void _ZN2EA5Blast6System4InitEv(void* t) {
    fprintf(stderr, "!!!!_ZN2EA5Blast6System4InitEv\n");
}
void* _ZN2EA5Blast13SystemAndroid9InitEAMIOEv(void* a, void* b) {
    fprintf(stderr, "!!!AAAAAAAAADAJSJASJDJAJ 0x%x 0x%x %s\n", a, b, (char*)b);
}
void _ZN2EA5Blast13SystemAndroid4InitEv(void* t) {
    fprintf(stderr, "!!!!_ZN2EA5Blast13SystemAndroid4InitEv\n");
}
void _ZNK2EA5Blast6System7IsAliveEv(void* t) {
    fprintf(stderr, "!!!!_ZNK2EA5Blast6System7IsAliveEv\n");
}

char * eastl_search_charptr_char_constptr(char *param_1,char *param_2,char *param_3,char **param_4)
{
    fprintf(stderr, "eastl_search_charptr_char_constptr(\"%s\", \"0x%x/%s\", \"%i\", \"N\")\n", param_1, (int)param_2, param_2, (int)param_4);
    char cVar1;
    char *pcVar2;
    char *pcVar3;
    char *pcVar4;
    char *pcVar5;
    char *pcVar6;

    pcVar4 = param_1;
    if (param_3 != "appbundle:/") {
        if (param_3 == "ppbundle:/") {
            pcVar4 = param_2;
            if ((param_2 != param_1) && (pcVar4 = param_1, *param_1 != 'a')) {
                pcVar3 = param_1 + 1;
                do {
                    pcVar4 = pcVar3;
                    if (pcVar4 == param_2) {
                        fprintf(stderr, "eastl_search_charptr_char_constptr(\"%s\", \"%s\", \"%s\", \"%s\"): 1ret %s\n", param_1, param_2, param_3, param_4, pcVar4);
                        return pcVar4;
                    }
                    pcVar3 = pcVar4 + 1;
                } while (*pcVar4 != 'a');
            }
        }
        else if (param_2 != param_1) {
            do {
                cVar1 = *param_1;
                pcVar4 = param_1;
                while (cVar1 != 'a') {
                    pcVar4 = pcVar4 + 1;
                    if (pcVar4 == param_2) {
                        fprintf(stderr, "eastl_search_charptr_char_constptr(\"%s\", \"%s\", \"%s\", \"%s\"): 2ret %s\n", param_1, param_2, param_3, param_4, pcVar4);
                        return pcVar4;
                    }
                    cVar1 = *pcVar4;
                }
                param_1 = pcVar4 + 1;
                pcVar3 = "ppbundle:/";
                pcVar2 = param_1;
                pcVar6 = param_2;
                while( 1 ) {
                    if (param_2 == pcVar2) {
                        fprintf(stderr, "eastl_search_charptr_char_constptr(\"%s\", \"%s\", \"%s\", \"%s\"): 3ret %s\n", param_1, param_2, param_3, param_4, pcVar6);
                        return pcVar6;
                    }
                    pcVar5 = pcVar3 + 1;
                    pcVar6 = pcVar2 + 1;
                    if (*pcVar3 != *pcVar2) break;
                    pcVar3 = pcVar5;
                    pcVar2 = pcVar6;
                    if (param_3 == pcVar5) {
                        fprintf(stderr, "eastl_search_charptr_char_constptr(\"%s\", \"%s\", \"%s\", \"%s\"): 4ret %s\n", param_1, param_2, param_3, param_4, pcVar4);
                        return pcVar4;
                    }
                }
            } while( 1 );
        }
    }
    fprintf(stderr, "eastl_search_charptr_char_constptr(\"%s\", \"%s\", \"%s\", \"%s\"): 5ret %s\n", param_1, param_2, param_3, param_4, pcVar4);
    return pcVar4;
}

/*


so_hook display2d_hook_2;
void EA__IO__FileStream__FileStream_2(void *this, wchar_t *param_1) {
    //strremove(param_1, "appbundle:/");
    char*test = malloc(1024);
    wcstombs(test,param_1,1024);
    fprintf(stderr, "ABABABA: %s\n", test);
    free(test);

    SO_CONTINUE(int, display2d_hook_2, this, param_1);
}
 */

extern void* (*IO__GetAllocator)();
extern void (*eastl_strappend)(void*, char*, char*);
//Blast * __thiscall EA::Blast::GetAppBundledResourceDirectory(Blast *this)
uint8_t * EA__Blast__GetAppBundledResourceDirectory(uint8_t *this)

{
    uint8_t *pBVar2;
    char *pcVar3;

    void* uVar1 = IO__GetAllocator();
    pBVar2 = this + 0x18;
    *(void **)(this + 0xc) = uVar1;
    *(int *)(this + 0x10) = 0;
    *(uint8_t **)(this + 0x14) = pBVar2;
    *(uint8_t **)(this + 4) = pBVar2;
    *(uint8_t **)this = pBVar2;
    *(uint8_t **)(this + 8) = this + 0x78;
    this[0x18] = 0;
    pcVar3 = "appbundle:/";
    do {
        pcVar3 = pcVar3 + 1;
    } while (*pcVar3 != '\0');


    eastl_strappend(this,"appbundle:/",pcVar3);
    return this;
}

so_hook display2d_hook;
int EA__IO__FileStream__FileStream(void *this, int param_1, int param_2, int param_3, int param_4) {
    //strremove(param_1, "appbundle:/");
    fprintf(stderr, "EA__IO__FileStream__FileStream: %s\n", *(char **)(this + 0x14));

    SO_CONTINUE(int, display2d_hook, this, param_1, param_2, param_3, param_4);
}

void patch_game(void) {

    //uint32_t fix = 0x1F0000EA;
    //kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00272e30), &fix, sizeof(fix));

    //uint32_t fix = 0x130000EA;
    //kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00272e30), &fix, sizeof(fix));

    // Always fail check for "appbundle:" in filename ==> don't use JNI IO funcs.
    uint32_t fix = 0xe35a0001;
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x00272e50), &fix, sizeof(fix));

    uint32_t nop = 0xe1a00000;
    //kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0026d4ec), &nop, sizeof(nop));
    //kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0029eb08), &nop, sizeof(nop));
    //kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0029eb24), &nop, sizeof(nop));

    //kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0029ea14), &nop, sizeof(nop));


    //uint32_t fix2 = 0x1A0001B3;
    //kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0029e0a8), &nop, sizeof(nop));


            //uint32_t fix2 = 0xe3540001;
    //kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x0029ebc0), &fix2, sizeof(fix2));
    // game-specific
    {
        //fprintf(stderr, "\n\n\n\n\n\nLOLOLO\n\n\n\n\n\n\n");
        //display2d_hook = hook_addr((uintptr_t)so_mod.text_base + 0x00272ddc, (uintptr_t)&EA__IO__FileStream__FileStream);
        //display2d_hook = hook_addr((uintptr_t)so_mod.text_base + 0x0028b3a4, (uintptr_t)&EA__IO__FileStream__FileStream);
        //display2d_hook_2 = hook_addr((uintptr_t)so_mod.text_base + 0x0028a718, (uintptr_t)&EA__IO__FileStream__FileStream_2);
        //hook_addr((uintptr_t)so_mod.text_base + 0x00284d38, (uintptr_t)&EA__Blast__GetAppBundledResourceDirectory);
        //hook_addr((uintptr_t)so_mod.text_base + 0x00271ac0, (uintptr_t)&eastl_search_charptr_char_constptr);
        //hook_addr(so_symbol(&so_mod, "_ZNK2EA5Blast6System7IsAliveEv"), (uintptr_t)&_ZNK2EA5Blast6System7IsAliveEv);
        //hook_addr(so_symbol(&so_mod, "_ZN2EA5Blast13SystemAndroid4InitEv"), (uintptr_t)&_ZN2EA5Blast13SystemAndroid4InitEv);
        //hook_addr(so_symbol(&so_mod, "_ZN2EA5Blast13SystemAndroid9InitEAMIOEv"), );
        //hook_addr(so_symbol(&so_mod, "_ZN2EA5Blast6System4InitEv"), (uintptr_t)&_ZN2EA5Blast6System4InitEv);
    }
}
