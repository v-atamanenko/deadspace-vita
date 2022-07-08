/*
 * default_dynlib.h
 *
 * Resolving dynamic imports of the .so.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_DYNLIB_H
#define SOLOADER_DYNLIB_H

#include "config.h"
#include "utils/utils.h"

#include "so_util.h"

#define SO_DYNLIB_LENGTH 426
#define SO_DYNLIB_SIZE (sizeof(so_default_dynlib) * SO_DYNLIB_LENGTH)
extern so_default_dynlib default_dynlib[SO_DYNLIB_LENGTH];

#endif // SOLOADER_DYNLIB_H
