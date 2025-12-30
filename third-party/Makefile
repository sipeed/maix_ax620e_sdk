
################################################################################
#	prepare param
################################################################################
LOCAL_PATH        := $(shell pwd)
HOME_PATH         := $(abspath $(LOCAL_PATH)/..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

SUBDIRS = tinyalsa libsamplerate opus fdk-aac faac
ifeq ($(SUPPORT_OPTEE),TRUE)
SUBDIRS += optee
endif

ifeq ($(ENABLE_SWUPDATE), TRUE)
SUBDIRS += swupdate
endif

################################################################################
#	set task
################################################################################
SUBDIRS_ALL     = $(addsuffix .all, $(SUBDIRS))
SUBDIRS_CLEAN   = $(addsuffix .clean, $(SUBDIRS))
SUBDIRS_INSTALL = $(addsuffix .install, $(SUBDIRS))

.PHONY: $(SUBDIRS) $(SUBDIRS_INSTALL) $(SUBDIRS_CLEAN)
.NOTPARALLEL: clean install

all: $(SUBDIRS_ALL)
	@echo -e $(GREEN)"\nBuild All Third-party Modules success!!\n"  $(DONE)

install: $(SUBDIRS_INSTALL)
	@echo -e $(GREEN)"\nInstall Third-party success!!\n"  $(DONE)

clean:	$(SUBDIRS_CLEAN)
	@echo -e $(GREEN)"\nClean Third-party directory!!\n"  $(DONE)

$(SUBDIRS_ALL):
	@$(ECHO)
	@$(ECHO) -e $(CYAN)"In subdir $@ ..." $(DONE)
	@$(MAKE) -C $(basename $@ )

$(SUBDIRS_INSTALL):
	@$(ECHO)
	@$(ECHO) -e $(CYAN)"In subdir $(basename $@ )..." $(DONE)
	@$(MAKE) -C $(basename $@ ) install

$(SUBDIRS_CLEAN):
	@$(ECHO) -e $(CYAN)"In subdir $(basename $@ )..." $(DONE)
	@$(MAKE) -C $(basename $@ ) clean
