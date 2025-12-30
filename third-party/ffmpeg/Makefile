CUR_PATH		   := $(shell pwd)
HOME_PATH		   := $(abspath $(CUR_PATH)/../..)


include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

ROOTFS_TARGET_PATH := $(MSP_OUT_PATH)/third-party

all:
	@echo -e $(GREEN)"\nBuild ffmpeg success!!\n"  $(DONE)


install:
	@mkdir -p $(ROOTFS_TARGET_PATH)/lib
	@cp -f $(CUR_PATH)/lib/$(ARCH)/$(LIBC)/libavcodec.so* $(ROOTFS_TARGET_PATH)/lib
	@cp -f $(CUR_PATH)/lib/$(ARCH)/$(LIBC)/libavfilter.so* $(ROOTFS_TARGET_PATH)/lib
	@cp -f $(CUR_PATH)/lib/$(ARCH)/$(LIBC)/libavformat.so* $(ROOTFS_TARGET_PATH)/lib
	@cp -f $(CUR_PATH)/lib/$(ARCH)/$(LIBC)/libavutil.so* $(ROOTFS_TARGET_PATH)/lib
	@cp -f $(CUR_PATH)/lib/$(ARCH)/$(LIBC)/libswresample.so* $(ROOTFS_TARGET_PATH)/lib
	@cp -f $(CUR_PATH)/lib/$(ARCH)/$(LIBC)/libswscale.so* $(ROOTFS_TARGET_PATH)/lib
	@echo -e $(GREEN)"\nInstall ffmpeg success!!\n"  $(DONE)

clean:
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libavcodec.so*
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libavfilter.so*
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libavformat.so*
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libavutil.so*
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libswresample.so*
	@rm -f $(ROOTFS_TARGET_PATH)/lib/libswscale.so*

	@echo -e $(GREEN)"\nClean ffmpeg success!!\n"  $(DONE)
