/*
 * loading_screen.c
 *
 * Simple text-based loading screen using vita2d.
 *
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "loading_screen.h"

#include <stdint.h>
#include <stdio.h>
#include "vita2d.h"

uint32_t white = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
uint32_t pink = RGBA8(0xDE, 0x39, 0x6B, 0xFF);
uint32_t green = RGBA8(0x00, 0xFF, 0x00, 0xFF);
uint32_t red = RGBA8(0XEE, 0xEE, 0x00, 0x00);
uint32_t gray = RGBA8(0x44, 0x44, 0x44, 0x44);

typedef struct credits_voice{
    int x;
    int y;
    uint32_t *color;
    char text[256];
} credits_voice;

credits_voice intro[] = {
        {0, 40, &pink,   "BABA IS YOU"},
        {0, 60, &white,  "A game by Arvi Teikari"},
        {0, 80, &white,  "Ported by gl33ntwine"},
        {0, 270, &green, "Loading, please wait about 40 seconds..."},
        {0, 450, &pink,  "Special thanks to:"},
        {0, 470, &white, "psykana - Rinnegatamante - Northfear"},
        {0, 490, &white, "TheFloW - GrapheneCt"}
};

credits_voice intro_preloader[] = {
        {0, 40, &pink,   "BABA IS YOU"},
        {0, 60, &white,  "A game by Arvi Teikari"},
        {0, 80, &white,  "Ported by gl33ntwine"},
        {0, 235, &green, "Preloading assets."},
        {0, 255, &green, "Grab a coffee, the process will take 6 minutes."},
        {0, 275, &red,   "This only needs to be done once!"},
        {0, 450, &pink,  "Special thanks to:"},
        {0, 470, &white, "psykana - Rinnegatamante - Northfear"},
        {0, 490, &white, "TheFloW - GrapheneCt"}
};

vita2d_pgf *font;

void loading_screen_run() {
    vita2d_init();
    font = vita2d_load_default_pgf();
    size_t n = sizeof(intro)/sizeof(intro[0]);

    for (int z = 0; z < n; z++) {
        int w = vita2d_pgf_text_width(font, 1.0f, intro[z].text);
        intro[z].x = (960 - w) / 2;
    }

    for (int i = 0; i < 3; i++) {
        vita2d_start_drawing();
        for (int z = 0; z < n; z++) {
            vita2d_pgf_draw_text(font, intro[z].x, intro[z].y, *intro[z].color,
                                 1.0f, intro[z].text);
        }
        vita2d_end_drawing();
        vita2d_swap_buffers();
    }
}

void loading_screen_run_assets_preloader() {
    vita2d_init();
    font = vita2d_load_default_pgf();
    size_t n = sizeof(intro_preloader)/sizeof(intro_preloader[0]);
    fprintf(stderr, "Size: %i\n", n);

    for (int z = 0; z < n; z++) {
        int w = vita2d_pgf_text_width(font, 1.0f, intro_preloader[z].text);
        intro_preloader[z].x = (960 - w) / 2;
    }

    loading_screen_update_assets_preloader(0.f);
}

void loading_screen_update_assets_preloader(float progress) {
    size_t n = sizeof(intro_preloader)/sizeof(intro_preloader[0]);

    for (int i = 0; i < 3; i++) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        for (int z = 0; z < n; z++) {
            vita2d_pgf_draw_text(font, intro_preloader[z].x,
                                 intro_preloader[z].y,
                                 *intro_preloader[z].color, 1.0f,
                                 intro_preloader[z].text);
            vita2d_draw_rectangle(330, 300, 300, 25, gray);
            vita2d_draw_rectangle(330, 300, (float)300*progress, 25, white);
        }
        vita2d_end_drawing();
        vita2d_swap_buffers();
    }
}

void loading_screen_quit() {
    vita2d_free_pgf(font);
    vita2d_fini();
}
