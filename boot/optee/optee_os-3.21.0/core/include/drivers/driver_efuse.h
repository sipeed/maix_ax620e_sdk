#ifndef DRIVER_EFUSE_H
#define DRIVER_EFUSE_H
#include <types_ext.h>

struct efuse_driver_ops_s {
	int (*efuse_init)(void);
	int (*efuse_write)(uint32_t blk, uint32_t data);
	int (*efuse_read)(uint32_t blk, uint32_t *data);
	int (*efuse_deinit)(void);
	int (*efuse_blk_protect)(uint32_t blk);
};

extern int _efuse_read(unsigned long efuseBase, uint32_t blk, uint32_t *data);
extern int ax_efuse_init(void);
extern int ax_efuse_deinit(void);
extern int ax_efuse_read(uint32_t blk, uint32_t *data);
extern int ax_efuse_write(uint32_t blk, uint32_t data);
extern int ax_efuse_blk_protect(uint32_t blk);

#endif