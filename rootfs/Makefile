HOME_PATH    := $(abspath $(shell pwd)/..)
BUILD_PATH   := $(HOME_PATH)/build
BUSYBOX_PATH := $(shell pwd)/busybox

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

PROJECT_PATH := $(BUILD_PATH)/projects/$(PROJECT)

.PHONY: all rootfs image
.NOTPARALLEL: clean install

default: all

ifeq ($(busybox), yes)
BUILD_BUSYBOX := TRUE
endif

all:
ifeq ($(BUILD_BUSYBOX),TRUE)
	@$(MAKE) -C $(BUSYBOX_PATH) p=$(PROJECT) all
endif
	@$(MAKE) -C $(PROJECT_PATH) p=$(PROJECT) all
	@$(ECHO) -e $(GREEN)"\nBuild Rootfs success!!\n" $(DONE)

install:
	@$(ECHO) -e $(GREEN) "rootfs $@..." $(DONE)
ifeq ($(BUILD_BUSYBOX),TRUE)
	@$(MAKE) -C $(BUSYBOX_PATH) p=$(PROJECT) install
endif
	@$(MAKE) -C $(PROJECT_PATH) p=$(PROJECT) install

image:
	@$(ECHO) -e $(GREEN) "rootfs making $@..." $(DONE)
	@$(MAKE) -C $(PROJECT_PATH) p=$(PROJECT) image

clean:
	@$(ECHO) -e $(GREEN) "rootfs clean$@..." $(DONE)
ifeq ($(BUILD_BUSYBOX),TRUE)
	@$(MAKE) -C $(BUSYBOX_PATH) clean
endif
	@$(MAKE) -C $(PROJECT_PATH) p=$(PROJECT) clean
