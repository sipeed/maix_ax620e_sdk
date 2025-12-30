#ifndef TEE_EFUSE_H
#define TEE_EFUSE_H
#include <drivers/driver_efuse.h>
#include <stdio.h>

struct efuse_service_ops {
	const char* name;
	struct efuse_driver_ops_s efuse_driver;
};

extern int syscall_efuse_driver_init(void);
extern int syscall_efuse_driver_write(uint32_t blk, uint32_t data);
extern int syscall_efuse_driver_read(uint32_t blk, uint32_t *data);
extern int syscall_efuse_driver_blk_protect(uint32_t blk);
extern int syscall_efuse_driver_deinit(void);

#endif