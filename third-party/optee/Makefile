CUR_PATH		   := $(shell pwd)
HOME_PATH		   := $(abspath $(CUR_PATH)/../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

ROOTFS_TARGET_PATH := $(HOME_PATH)/build/out/$(PROJECT)/objs/rootfs

all:
	@echo -e $(GREEN)"\nBuild optee success!!\n"  $(DONE)

install:
ifeq ($(strip $(SUPPORT_OPTEE)),TRUE)
	@mkdir -p $(ROOTFS_TARGET_PATH)/usr/lib
	@cp -d $(CUR_PATH)/libteec/lib/$(ARCH)/$(LIBC)/libteec.so* $(ROOTFS_TARGET_PATH)/usr/lib
	@mkdir -p $(ROOTFS_TARGET_PATH)/usr/sbin
	@cp -d $(CUR_PATH)/usr/sbin/tee-supplicant $(ROOTFS_TARGET_PATH)/usr/sbin
	@mkdir -p $(ROOTFS_TARGET_PATH)/etc/init.d
	@cp -d $(CUR_PATH)/etc/init.d/S30optee $(ROOTFS_TARGET_PATH)/etc/init.d
endif
	@echo -e $(GREEN)"\nInstall optee success!!\n"  $(DONE)

clean:
	@rm -f $(ROOTFS_TARGET_PATH)/usr/lib/libteec.so*
	@rm -f $(ROOTFS_TARGET_PATH)/usr/sbin/tee-supplicant
	@rm -f $(ROOTFS_TARGET_PATH)/etc/init.d/S30optee
	@echo -e $(GREEN)"\nClean optee success!!\n"  $(DONE)







