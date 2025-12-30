CUR_PATH		   := $(shell pwd)
HOME_PATH		   := $(abspath $(CUR_PATH)/../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

ROOTFS_TARGET_PATH := $(MSP_OUT_PATH)/third-party

all:
	@echo -e $(GREEN)"\nBuild fdk-aac success!!\n"  $(DONE)


install:
	@mkdir -p $(ROOTFS_TARGET_PATH)/lib
	@cp -rf $(CUR_PATH)/lib/$(ARCH)/$(LIBC)/* $(ROOTFS_TARGET_PATH)/lib
	@echo -e $(GREEN)"\nInstall fdk-aac success!!\n"  $(DONE)

clean:
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libfdk-aac.so
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libfdk-aac.so.2
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libfdk-aac.so.2.0.1

	@echo -e $(GREEN)"\nClean fdk-aac success!!\n"  $(DONE)







