/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <rtdevice.h>
#include "ax_env.h"
#include "ax_log.h"


#define OCM_BASE_ADDR 0x3000000
#define OCM_TOTAL_SIZE 0x280000
#define AX_ENV_SIZE 0x20000
#define AX_ENV_ADDR (OCM_BASE_ADDR + OCM_TOTAL_SIZE - AX_ENV_SIZE)
#define ENV_SIZE (AX_ENV_SIZE - 4) //real env data size, crc use 4 bytes

typedef uint32_t u32;


struct env_image_single {
    u32 crc;		/* CRC32 over data bytes    */
    char data[];
};

struct environment {
    void *image;
    u32 *crc;
    unsigned char *flags;
    char *data;
    int dirty;
};

static struct environment environment;

/*
 * s1 is either a simple 'name', or a 'name=value' pair.
 * s2 is a 'name=value' pair.
 * If the names match, return the value of s2, else NULL.
 */
static char *envmatch(char *s1, char *s2)
{
    if (s1 == NULL || s2 == NULL)
        return NULL;

    while (*s1 == *s2++)
        if (*s1++ == '=')
            return s2;
    if (*s1 == '\0' && *(s2 - 1) == '=')
        return s2;

    return NULL;
}


static int load_env(void)
{
    struct env_image_single *single;

    single = (void *)AX_ENV_ADDR;
    environment.data = single->data;

    return 0;
}


/**
 * Search the environment for a variable.
 * Return the value, if found, or NULL, if not found.
 */
char *fw_getenv(char *name)
{
    char *env, *nxt;

    load_env();

    for (env = environment.data; *env; env = nxt + 1) {
        char *val;

        for (nxt = env; *nxt; ++nxt) {
            if (nxt >= &environment.data[ENV_SIZE]) {
                rt_kprintf("env not terminated\r\n");
                return NULL;
            }
        }

        val = envmatch(name, env);
        if (!val)
            continue;
        return val;
    }

    return NULL;
}


/*
 * Print all environment variables
 */
int fw_printenv(void)
{
    char *env, *nxt;

    load_env();

    for (env = environment.data; *env; env = nxt + 1) {
        for (nxt = env; *nxt; ++nxt) {
            if (nxt >= &environment.data[ENV_SIZE]) {
                rt_kprintf("env not terminated\n");
                return -1;
            }
        }
        rt_kprintf("%s\n", env);
    }

    return 0;
}


#if 1
static void printenv(rt_int32_t argc, char* argv[])
{
    int i;

    if (argc == 1) {
        rt_kprintf("print all ax env:\n");
        fw_printenv();
    } else {
        for (i = 1; i < argc; ++i) {	/* print a subset of env variables */
            char *name = argv[i];
            char *val = NULL;

            val = fw_getenv(name);
            if (!val) {
                rt_kprintf("Error: \"%s\" not defined\n", name);
                continue;
            }

            rt_kprintf("%s=%s\n", name, val);
        }
    }
}

MSH_CMD_EXPORT(printenv, print ax env);
#endif