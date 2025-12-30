#ifndef __FW_ENV_H_
#define __FW_ENV_H_


/**
 * fw_env_load() - read enviroment from flash into RAM cache
 * Return:
 *  0 on success, -1 on failure (modifies errno)
 */
int fw_env_load(u32 flash_type);

char *fw_getenv(char *name);

int fw_printenv(int argc, char *argv[], int value_only, u32 flash_type);

#endif