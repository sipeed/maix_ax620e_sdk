HOME_PATH    := $(abspath $(shell pwd)/../..)
BUILD_PATH   := $(HOME_PATH)/build

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

.PHONY: all install clean
.NOTPARALLEL: clean install

all:
ifeq ($(FDL2_UBOOT_COMPILE_INDEPENDENT), TRUE)
	@$(MAKE) p=$(PROJECT) clean -f Makefile.uboot
	@$(MAKE) p=$(PROJECT) all -f Makefile.uboot
	@$(MAKE) p=$(PROJECT) clean -f Makefile.fdl2
endif
	@$(MAKE) p=$(PROJECT) all -f Makefile.fdl2

### SIPEED EDIT ###
menuconfig:
ifeq ($(FDL2_UBOOT_COMPILE_INDEPENDENT), TRUE)
	@$(MAKE) p=$(PROJECT) menuconfig -f Makefile.uboot
endif
	@$(MAKE) p=$(PROJECT) menuconfig -f Makefile.fdl2
### SIPEED EDIT END ###

install:
ifeq ($(FDL2_UBOOT_COMPILE_INDEPENDENT), TRUE)
	@$(MAKE) p=$(PROJECT) install -f Makefile.uboot
endif
	@$(MAKE) p=$(PROJECT) install -f Makefile.fdl2

clean:
ifeq ($(FDL2_UBOOT_COMPILE_INDEPENDENT), TRUE)
	@$(MAKE) p=$(PROJECT) clean -f Makefile.uboot
endif
	@$(MAKE) p=$(PROJECT) clean -f Makefile.fdl2

