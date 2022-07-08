/*
 * env.h
 *
 * Implemetation for getenv() function with predefined environment variables.
 *
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_GETENV_H
#define SOLOADER_GETENV_H

char * getenv_soloader(const char *var);

#endif // SOLOADER_GETENV_H
