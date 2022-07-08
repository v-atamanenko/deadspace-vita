/*
 * main.c
 *
 * ARMv7 Shared Libraries loader. Baba Is You edition.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_MAIN_H
#define SOLOADER_MAIN_H

#include "config.h"
#include "utils/utils.h"

#include "so_util.h"

extern so_module so_mod;

void *baba_main();

#endif // SOLOADER_MAIN_H
