/*
 * env.c
 *
 * Implemetation for getenv() function with predefined environment variables.
 *
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <string.h>
#include "env.h"
#include "utils/utils.h"

typedef enum EnvVarsIds {
    ENV_VAR_UNKNOWN = 0,

} EnvVarsIds;

typedef struct {
    char *name;
    enum EnvVarsIds id;
} NameToEnvVarID;

static NameToEnvVarID name_to_env_var_ids[] = {
    { "UNK", ENV_VAR_UNKNOWN }
};

int get_env_var_id(const char* name) {
    for(int i=0; i < sizeof(name_to_env_var_ids)/sizeof(NameToEnvVarID); i++) {
        if (strcmp(name, name_to_env_var_ids[i].name) == 0) {
            return name_to_env_var_ids[i].id;
        }
    }
    return ENV_VAR_UNKNOWN;
}

char * getenv_soloader(const char *var) {
    debugPrintf("Requested getenv(\"%s\")\n", var);
    switch (get_env_var_id(var)) {
        case ENV_VAR_UNKNOWN:
        default:
            debugPrintf("Requested unknown env var: %s\n", var);
            return NULL;
    }
}
