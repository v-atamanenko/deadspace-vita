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

void resolve_imports(so_module* mod);

#endif // SOLOADER_DYNLIB_H
