/*
 * patch_game.h
 *
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_PATCH_GAME_H
#define SOLOADER_PATCH_GAME_H

#include "config.h"
#include "utils/utils.h"

extern void (* gl_flush_cache)(void); // _Z14gl_flush_cachev

void patch_game();

#endif // SOLOADER_PATCH_GAME_H
