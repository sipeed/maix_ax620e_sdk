#include <tee/tee_efuse.h>
#include <tee/tee_svc.h>
#include <trace.h>

struct efuse_service_ops  efuse_ops = {
	.name = "Efuse-Driver",
	.efuse_driver = {
	.efuse_init = ax_efuse_init,
	.efuse_write = ax_efuse_write,
	.efuse_read = ax_efuse_read,
	.efuse_deinit = ax_efuse_deinit,
	.efuse_blk_protect = ax_efuse_blk_protect,
	},
};


int syscall_efuse_driver_write(uint32_t blk, uint32_t data)
{
	return efuse_ops.efuse_driver.efuse_write(blk, data);
}


int syscall_efuse_driver_read(uint32_t blk, uint32_t *data)
{
	return  efuse_ops.efuse_driver.efuse_read(blk, data);
}

int syscall_efuse_driver_init(void)
{
	return efuse_ops.efuse_driver.efuse_init();
}

int syscall_efuse_driver_deinit(void)
{
	return efuse_ops.efuse_driver.efuse_deinit();
}

int syscall_efuse_driver_blk_protect(uint32_t blk)
{
	return  efuse_ops.efuse_driver.efuse_blk_protect(blk);
}
