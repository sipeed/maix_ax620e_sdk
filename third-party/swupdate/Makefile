CUR_PATH		   := $(shell pwd)
HOME_PATH		   := $(abspath $(CUR_PATH)/../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

OPT_TARGET_PATH := $(MSP_OUT_PATH)/third-party/swupdate

all:
	@echo -e $(GREEN)"\nBuild swupdate success!!\n"  $(DONE)


install:
	@rm $(OPT_TARGET_PATH) -rf
	@mkdir -p $(OPT_TARGET_PATH)/bin
	@cp -rf $(CUR_PATH)/$(ARCH)_$(LIBC)/bin/* $(OPT_TARGET_PATH)/bin
	@chmod 777 $(OPT_TARGET_PATH)/bin/*
	@mkdir -p $(OPT_TARGET_PATH)/lib
	@cp -rf $(CUR_PATH)/$(ARCH)_$(LIBC)/lib/* $(OPT_TARGET_PATH)/lib
	@chmod 777 $(OPT_TARGET_PATH)/lib/*
	@echo -e $(GREEN)"\nInstall swupdate success!!\n"  $(DONE)

clean:
	@rm -rf $(OPT_TARGET_PATH)

	@echo -e $(GREEN)"\nClean swupdate success!!\n"  $(DONE)







