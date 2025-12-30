
CUR_PATH            := $(shell pwd)
HOME_PATH           := $(abspath $(CUR_PATH)/../../..)
OPAL_NAME 			:= opal

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

OUT_PATH            := $(MSP_OUT_PATH)
PRJ_OUT_HOME        := $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH        := $(PRJ_OUT_HOME)/objs
LIB_PATH            := $(MSP_OUT_PATH)/lib
INC_PATH            := $(MSP_OUT_PATH)/include

SRC_RELATIVE_PATH   := $(subst $(HOME_PATH)/,,$(CUR_PATH))
TARGET_OUT_PATH     := $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)

BIN_PATH            := $(HOME_PATH)/app/opal/demo/bin
RESOURCE_PATH       := $(HOME_PATH)/app/component/resource

FONT_USE_FREETYPE   := no
MP4ENC_SUPPORT      := yes
STRIP_APP           := no

ifeq ($(CHIP_TYPE), AX620Q)
APP_AAC_SUPPORT     := faac
APP_OPUS_SUPPORT    := no
else
APP_AAC_SUPPORT     := fdk
APP_OPUS_SUPPORT    := no
endif

################################################################################
# build
################################################################################


.PHONY : all clean install
.NOTPARALLEL: clean install
all: app
	@echo -e $(GREEN)"\nBuild $@ success!!\n"  $(DONE)

clean:
	$(VERB) $(RM) $(ROOTFS_TARGET_PATH)/bin/OPALDemo -R
	$(VERB) $(MAKE) -f Makefile.dynamic clean
	$(VERB) $(MAKE) -f Makefile.static clean
	@echo -e $(GREEN)"\nClean $@ success!!\n"  $(DONE)

install:
	@echo -e $(GREEN)"Install $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.dynamic install
	$(VERB) $(MAKE) -f Makefile.static install
	$(VERB) $(CP) $(BIN_PATH)/* $(ROOTFS_TARGET_PATH)/bin/OPALDemo -R

	$(VERB) $(RM) $(ROOTFS_TARGET_PATH)/bin/OPALDemo/lib -R
	$(VERB) $(MKDIR) $(ROOTFS_TARGET_PATH)/bin/OPALDemo/lib
	$(VERB) $(CP) $(BIN_PATH)/lib/$(ARCH)/$(LIBC)/appweb/* $(ROOTFS_TARGET_PATH)/bin/OPALDemo/lib/ -R

ifeq ($(FONT_USE_FREETYPE),yes)
	$(VERB) $(CP) $(RESOURCE_PATH)/font/GB2312.ttf $(ROOTFS_TARGET_PATH)/bin/OPALDemo/res
endif

ifeq ($(STRIP_APP),no)
	@$(CP) $(OUT_PATH)/bin/OPALDemo/opaldemo $(TARGET_OUT_PATH)/opaldemo_debug
	@$(CP) $(OUT_PATH)/bin/OPALDemo/opaldemo_s $(TARGET_OUT_PATH)/opaldemo_s_debug
endif

	@$(MKDIR) $(PRJ_OUT_HOME)/images/debug
	@$(CP) $(TARGET_OUT_PATH)/opaldemo_debug $(PRJ_OUT_HOME)/images/debug/opaldemo
	@$(CP) $(TARGET_OUT_PATH)/opaldemo_s_debug $(PRJ_OUT_HOME)/images/debug/opaldemo_s

app:
	@echo -e $(GREEN)"Build dynamic $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.dynamic

	@echo -e $(GREEN)"Build static $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.static

export FONT_USE_FREETYPE
export MP4ENC_SUPPORT
export STRIP_APP
export APP_AAC_SUPPORT
export APP_OPUS_SUPPORT