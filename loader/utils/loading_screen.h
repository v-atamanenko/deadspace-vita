/*
 * loading_screen.h
 *
 * Simple text-based loading screen using vita2d.
 *
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_LOADING_SCREEN_H
#define SOLOADER_LOADING_SCREEN_H

void loading_screen_run();

void loading_screen_quit();

void loading_screen_run_assets_preloader();

void loading_screen_update_assets_preloader(float progress);

#endif // SOLOADER_LOADING_SCREEN_H
