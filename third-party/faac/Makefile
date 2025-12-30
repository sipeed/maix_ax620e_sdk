CUR_PATH		   := $(shell pwd)
HOME_PATH		   := $(abspath $(CUR_PATH)/../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

ROOTFS_TARGET_PATH := $(MSP_OUT_PATH)/third-party

all:
	@echo -e $(GREEN)"\nBuild faac success!!\n"  $(DONE)


install:
	@mkdir -p $(ROOTFS_TARGET_PATH)/lib
	@cp -rf $(CUR_PATH)/lib/$(ARCH)/$(LIBC)/* $(ROOTFS_TARGET_PATH)/lib
	@echo -e $(GREEN)"\nInstall faac success!!\n"  $(DONE)

clean:
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libfaac.so
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libfaac.so.0
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libfaac.so.0.0.0

	@echo -e $(GREEN)"\nClean faac success!!\n"  $(DONE)







