#ifndef _EFUSE_DRV_H_
#define _EFUSE_DRV_H_
#define EFSC_MAX_BLK 32
typedef enum {
	EFUSE_SUCCESS = 0,
	EFUSE_PARAM_FAIL = -1,
	EFUSE_INIT_FAIL = -2,
	EFUSE_READ_FAIL = -3,
	EFUSE_READ_WRITE = -4,
} EFUSE_STAUS_T;

EFUSE_STAUS_T efuse_init(void);
EFUSE_STAUS_T efuse_read(int blk, int *data);
EFUSE_STAUS_T efuse_write(int blk, int data);
EFUSE_STAUS_T efuse1_read(int blk, int *data);
EFUSE_STAUS_T efuse_deinit();
#endif
