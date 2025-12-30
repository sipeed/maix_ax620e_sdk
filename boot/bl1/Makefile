###############################################################################
#                                                                             #
# Copyright (c) 2019-2023 AXERA Technology Co., Ltd. All Rights Reserved.    #
#                                                                             #
# This source file is the property of AXERA Technology Co., Ltd. and         #
# may not be copied or distributed in any isomorphic form without the prior   #
# written consent of AXERA Technology Co., Ltd.                              #
#                                                                             #
###############################################################################
SUBDIRS = fdl spl sd

################################################################################
#	prepare param
################################################################################
LOCAL_PATH        := $(shell pwd)
HOME_PATH         := $(LOCAL_PATH)/../..

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

ifneq ($(findstring AX620E, $(PROJECT)), )
PRODUCT_CONFIG := AX620E_CFG
else
PRODUCT_CONFIG := AX620E_CFG
endif

################################################################################
#	set task
################################################################################
SUBDIRS_ALL   = $(addsuffix .all, $(SUBDIRS))
SUBDIRS_CLEAN   = $(addsuffix .clean, $(SUBDIRS))
SUBDIRS_INSTALL = $(addsuffix .install, $(SUBDIRS))
SUBDIRS_SCAN = $(addsuffix .scan, $(SUBDIRS))

.PHONY: $(SUBDIRS) $(SUBDIRS_INSTALL) $(SUBDIRS_CLEAN)
.NOTPARALLEL: clean install

default: all

all: $(SUBDIRS_ALL)
	@echo -e $(GREEN)"\nBuild bl1 success!!\n"  $(DONE)

install: $(SUBDIRS_INSTALL)
	@echo -e $(GREEN)"\nInstall bl1 success!!\n"  $(DONE)

clean:	$(SUBDIRS_CLEAN)
	@echo -e $(GREEN)"\nClean bl1 directory!!\n"  $(DONE)
	@echo -e $(GREEN)"\nClean bl1 success!!\n"  $(DONE)

$(SUBDIRS):
	$(ECHO)
	$(ECHO) -e $(CYAN)"In subdir $@ ..." $(DONE)
	$(MAKE) -C $(basename $@ ) CONFIG_PROJECT=$(PRODUCT_CONFIG)

$(SUBDIRS_ALL):
	$(ECHO) -e $(CYAN)"Making $(notdir $@)" $(DONE)
	$(ECHO) -e $(CYAN)"In subdir $(basename $(notdir $@) )..." $(DONE)
	$(MAKE) -C $(basename $@ ) clean
	$(MAKE) -C $(basename $@ ) all CONFIG_PROJECT=$(PRODUCT_CONFIG)

$(SUBDIRS_INSTALL):
	@$(ECHO)
	@$(ECHO) -e $(CYAN)"In subdir $(basename $@ )..." $(DONE)
	@$(MAKE) -C $(basename $@ ) install CONFIG_PROJECT=$(PRODUCT_CONFIG)

$(SUBDIRS_CLEAN):
	@$(ECHO) -e $(CYAN)"In subdir $(basename $@ )..." $(DONE)
	@$(MAKE) -C $(basename $@ ) clean

$(SUBDIRS_SCAN):
	@$(ECHO) -e $(CYAN)"In subdir $(basename $(notdir $@) )..." $(DONE)
	@$(MAKE) -C $(basename $@ ) scan

# Export val
export ROOTFS_TARGET_PATH
