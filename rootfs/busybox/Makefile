CUR_PATH           := $(shell pwd)
HOME_PATH          := $(abspath $(CUR_PATH)/../..)
BUILD_PATH         := $(HOME_PATH)/build

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

ROOTFS_PATH        := $(HOME_PATH)/rootfs/$(ARCH)/$(LIBC)/rootfs
SRC_PATH           := $(CUR_PATH)/busybox-1.35.0
OUT_DIR            := $(BUILD_PATH)/out/$(PROJECT)/objs
KBUILD_OUTDIR      := $(OUT_DIR)/rootfs/busybox
INSTALL_DIR        := $(KBUILD_OUTDIR)/_install
COMPILE_ARGS       := ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) O=$(KBUILD_OUTDIR)
DEFCONFIG_NAME     := AX620E_defconfig

.PHONY: all prepare install clean menuconfig
.NOTPARALLEL: clean install prepare

prepare:
	@$(MKDIR) -p $(KBUILD_OUTDIR)
	@$(MAKE) -C $(SRC_PATH) $(COMPILE_ARGS) $(DEFCONFIG_NAME)

menuconfig: prepare
	@$(MAKE) -C $(SRC_PATH) $(COMPILE_ARGS) menuconfig

all: prepare
	@$(MAKE) -C $(SRC_PATH) $(COMPILE_ARGS)
	@$(ECHO) -e $(GREEN)"\nBuild busybox $@ success!!\n"  $(DONE)

install:
	@$(MAKE) -C $(SRC_PATH) install $(COMPILE_ARGS)
	@cp -avf  $(INSTALL_DIR)/* $(ROOTFS_PATH)/
	@cp $(KBUILD_OUTDIR)/busybox $(SRC_PATH)/busybox_$(ARCH)
	@cp $(KBUILD_OUTDIR)/busybox_unstripped $(SRC_PATH)/busybox_$(ARCH)_unstripped
	@$(ECHO) -e $(GREEN)"\nInstall busybox success!!\n"  $(DONE)

clean:
	@$(MAKE) -C $(SRC_PATH) clean
	@$(ECHO) -e $(GREEN)"\nClean busybox success!!\n"  $(DONE)
