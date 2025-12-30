CUR_PATH		:= $(shell pwd)
HOME_PATH		:= $(abspath $(CUR_PATH)/../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

OUT_PATH		:= $(MSP_OUT_PATH)
SRC_PATH		:= $(CUR_PATH)/src
BASE_PATH		:= $(HOME_PATH)/msp/component
PRJ_OUT_HOME	:= $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH	:= $(PRJ_OUT_HOME)/objs
SRC_RELATIVE_PATH	:= $(subst $(HOME_PATH)/,,$(CUR_PATH))
TARGET_OUT_PATH	:= $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)

################################################################################
# build
################################################################################

ifeq ($(debug),yes)
STRIP_APP           := no
else
STRIP_APP           := yes
endif
QSDEMO_AUDIO_SUPPORT  := yes
QSDEMO_AUDIO_AAC_SUPPORT  := yes
QSDEMO_MP4ENC_SUPPORT := no

.PHONY : all clean install
.NOTPARALLEL: clean install
all: app
	@echo -e $(GREEN)"\nBuild $@ success!!\n"  $(DONE)

clean:
	# $(VERB) $(MAKE) -f Makefile.dynamic clean
	$(VERB) $(MAKE) -f Makefile.static clean
	@echo -e $(GREEN)"\nClean $@ success!!\n"  $(DONE)

install:
	@echo -e $(GREEN)"Install $@ ..."  $(DONE)
	# $(VERB) $(MAKE) -f Makefile.dynamic install
	$(VERB) $(MAKE) -f Makefile.static install

	$(VERB) $(MKDIR) $(ROOTFS_TARGET_PATH)/data/qsdemo
	$(VERB) $(CP) $(SRC_PATH)/res $(ROOTFS_TARGET_PATH)/data/qsdemo -R

ifeq ($(STRIP_APP),no)
	@$(CP) $(OUT_PATH)/bin/qsdemo_s $(TARGET_OUT_PATH)/qsdemo_s_debug
	@$(CROSS)strip --strip-debug $(OUT_PATH)/bin/qsdemo_s
endif

	@$(MKDIR) $(PRJ_OUT_HOME)/images/debug
	@$(CP) $(TARGET_OUT_PATH)/qsdemo_s_debug $(PRJ_OUT_HOME)/images/debug/qsdemo_s

app:
	# @echo -e $(GREEN)"Build dynamic $@ ..."  $(DONE)
	# $(VERB) $(MAKE) -B -f Makefile.dynamic
	@echo -e $(GREEN)"Build static $@ ..."  $(DONE)
	$(VERB) $(MAKE) -B -f Makefile.static

export STRIP_APP
export QSDEMO_AUDIO_SUPPORT
export QSDEMO_AUDIO_AAC_SUPPORT
export QSDEMO_MP4ENC_SUPPORT
