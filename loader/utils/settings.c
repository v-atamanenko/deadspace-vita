/*
 * utils/settings.c
 *
 * Loader settings that can be set via the companion app.
 *
 * Copyright (C) 2021 TheFloW
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include "settings.h"

#define CONFIG_FILE_PATH DATA_PATH"config.txt"

float leftStickDeadZone;
float rightStickDeadZone;
int fpsLock;
bool fakeAccel_enabled;

void resetSettings() {
    leftStickDeadZone = 0.11f;
    rightStickDeadZone = 0.11f;
    fpsLock = 0;
    fakeAccel_enabled = false;
}

void loadSettings(void) {
    resetSettings();

    char buffer[30];
    int value;

    FILE *config = fopen(CONFIG_FILE_PATH, "r");

    if (config) {
        while (EOF != fscanf(config, "%[^ ] %d\n", buffer, &value)) {
            if (strcmp("leftStickDeadZone", buffer) == 0) leftStickDeadZone = ((float)value / 100.f);
            else if (strcmp("rightStickDeadZone", buffer) == 0) rightStickDeadZone = ((float)value / 100.f);
            else if (strcmp("fpsLock", buffer) == 0) fpsLock = value;
            else if (strcmp("fakeAccel_enabled", buffer) == 0) fakeAccel_enabled = (bool)value;
        }
        fclose(config);
    }
}
