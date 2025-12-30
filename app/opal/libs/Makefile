CUR_PATH            := $(shell pwd)
HOME_PATH           := $(abspath $(CUR_PATH)/../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

TUNING_CTRL_SUPPORT := yes
OS04A10_SUPPORT     := yes
SC450AI_SUPPORT     := yes
IMX678_SUPPORT      := yes
SC200AI_SUPPORT     := yes
SC500AI_SUPPORT     := yes
SC850SL_SUPPORT     := yes
C4395_SUPPORT       := yes
GC4653_SUPPORT      := yes
MIS2032_SUPPORT     := yes
SNS_MODE_DYNAMIC_SWITCH_SUPPORT      := yes

FONT_USE_FREETYPE   := no

ifeq ($(CHIP_TYPE), AX620Q)
APP_AAC_SUPPORT     := faac
APP_OPUS_SUPPORT    := no
else
APP_AAC_SUPPORT     := fdk
APP_OPUS_SUPPORT    := no
endif

PRJ_OUT_HOME        := $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH        := $(PRJ_OUT_HOME)/objs

################################################################################
# build
################################################################################

.PHONY : all clean install lib modversion
.NOTPARALLEL: clean install modversion

all: modversion lib
	@echo -e $(GREEN)"\nBuild $@ success!!\n"  $(DONE)

clean:
	$(VERB) $(MAKE) -f Makefile.dynamic clean
	$(VERB) $(MAKE) -f Makefile.static clean

	@$(RM) $(MSP_OUT_PATH)/include/ax_opal_api.h
	@$(RM) $(MSP_OUT_PATH)/include/ax_opal_type.h
	@echo -e $(GREEN)"\nClean $@ success!!\n"  $(DONE)

install:
	@echo -e $(GREEN)"Install $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.dynamic install
	$(VERB) $(MAKE) -f Makefile.static install

lib:
	@echo -e $(GREEN)"Build dynamic $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.dynamic

	@echo -e $(GREEN)"Build static $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.static

modversion:
	@echo -e $(GREEN)"Build $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.dynamic modversion

export FONT_USE_FREETYPE
export TUNING_CTRL_SUPPORT
export OS04A10_SUPPORT
export SC450AI_SUPPORT
export IMX678_SUPPORT
export SC200AI_SUPPORT
export SC500AI_SUPPORT
export SC850SL_SUPPORT
export C4395_SUPPORT
export GC4653_SUPPORT
export MIS2032_SUPPORT
export SNS_MODE_DYNAMIC_SWITCH_SUPPORT
export APP_AAC_SUPPORT
export APP_OPUS_SUPPORT
