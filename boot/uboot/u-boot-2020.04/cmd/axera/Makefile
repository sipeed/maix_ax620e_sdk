#
# AXERA
#
# SPDX-License-Identifier:	GPL-2.0+
# https://spdx.org/licenses

ifeq ($(debugkconfig), yes)
EXTRA_CFLAGS   +=  -DAXERA_DEBUG_KCONFIG
endif

# download
obj-$(CONFIG_CMD_AXERA_DOWNLOAD) += download/download.o
obj-$(CONFIG_CMD_AXERA_DOWNLOAD) += download/fdl_channel.o
obj-$(CONFIG_CMD_AXERA_DOWNLOAD) += download/fdl_channel_serial.o
obj-$(CONFIG_CMD_AXERA_DOWNLOAD) += download/fdl_engine.o
obj-$(CONFIG_CMD_AXERA_DOWNLOAD) += download/fdl_frame.o
obj-$(CONFIG_CMD_AXERA_DOWNLOAD) += download/fdl_usb_driver.o
obj-$(CONFIG_CMD_AXERA_DOWNLOAD) += download/fdl_channel_usb.o
obj-y += update/sparse_img.o update/update_verify.o
obj-$(CONFIG_AXERA_SECURE_BOOT) += secureboot/secureboot.o

# memtest
obj-$(CONFIG_CMD_AXERA_MEMTEST)  += memtest/tests.o
obj-$(CONFIG_CMD_AXERA_MEMTEST)  += memtest/memtester.o

# boot os
obj-$(CONFIG_CMD_AXERA_BOOT) += boot/axera_boot.o

# sd update
obj-$(CONFIG_CMD_AXERA_SDUPDATE) += sd_update/sd_update.o
obj-$(CONFIG_CMD_AXERA_SDBOOT) += sd_boot/sd_boot.o
obj-$(CONFIG_CMD_AXERA_TFTP_OTA) += tftp_update/axera_ota.o
obj-$(CONFIG_CMD_AXERA_UPDATE) += update/axera_update.o
obj-y += setup_boot/setup_boot.o
obj-$(CONFIG_CMD_AXERA_CIPHER) += cipher/eip130_drv.o cipher/ax_cipher_api.o cipher/sample_cipher.o cipher/sample_hash.o cipher/sample_trng.o cipher/sample_rsa.o
obj-$(CONFIG_CMD_AXERA_EMMC_SCAN) += emmc_scan/emmc_scan.o

obj-$(CONFIG_CMD_AXERA_BOOT_RISCV) += riscv/boot_riscv.o
obj-$(CONFIG_CMD_AXERA_USB_STOR_UPDATE) += usb_stor_update/usb_storage_update.o
obj-$(CONFIG_CMD_AXERA_GZIPD) += gzipd/ax_gzipd_drv.o gzipd/sample_gzipd.o
obj-$(CONFIG_AXERA_SUPPORT_MEMORY_DUMP) += memory_dump/memory_dump_common.o
obj-$(CONFIG_AXERA_MEMORY_DUMP_SD) += memory_dump/memory_dump_sd.o
obj-$(CONFIG_AXERA_MEMORY_DUMP_EMMC) += memory_dump/memory_dump_emmc.o
obj-$(CONFIG_AXERA_MEMORY_DUMP_USB_STORAGE) += memory_dump/memory_dump_usb_storage.o
obj-$(CONFIG_CMD_AXERA_EXT4) += ax_ext4_tools/ax_ext4.o
