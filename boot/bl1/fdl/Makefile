###################################################################################
# Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
#
# This source file is the property of Axera Semiconductor Co., Ltd. and
# may not be copied or distributed in any isomorphic form without the prior
# written consent of Axera Semiconductor Co., Ltd.
###################################################################################

CUR_PATH        := $(shell pwd)
HOME_PATH       := $(shell pwd)/../../..
BUILD_PATH      := $(HOME_PATH)/build
IMGSIGN_SCRIPT  := $(BUILD_PATH)/tools/imgsign

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

CC := $(CROSS)gcc
LD := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump

IMAGE := fdl_$(PROJECT).axf
FDL_BIN := fdl_$(PROJECT).bin
FDL_SING_BIN := fdl_$(PROJECT)_signed.bin

OBJS := ../board/arch/$(ARCH)/start.o ../board/board.o ../board/arch/$(ARCH)/stack.o ../board/meminit.o fdl_main.o
OBJS += ../common/cmn.o
OBJS += ../driver/uart/uart.o ../driver/timer/timer.o ../driver/dmaper/dmaper.o \
		../driver/axdma/axdma_checksum.o \
		../driver/secure/efuse_drv.o ../driver/secure/eip130_drv.o \
		../driver/secure/secure.o ../driver/spi/dw_spi_slv.o\
		../driver/adc/ax_adc.o \
		../driver/ddr/ddr_init.o ../driver/ddr/ddrmc_train_flow_lp4.o ../driver/ddr/ddrmc_train_flow_no_lp.o ../driver/ddr/ddr_scan.o \
		../driver/pwm/pwm.o ../driver/gpio/gpio.o

OBJS += ../core/fdl/fdl_uart.o ../core/fdl/fdl_channel.o ../core/fdl/fdl_spi_slv.o \
		../core/fdl/fdl_engine.o ../core/fdl/fdl_frame.o \
		../core/trace/trace.o\
		../core/fdl/fdl_usb.o

OBJS += ../driver/usb/printf.o ../driver/usb/fdl_usb_driver.o ../driver/usb/fdl_usb.o

USB_INC = ../driver/usb

DEPS := $(patsubst %.o,%.d,$(OBJS))

# CPPFLAGS := -g -Wall -MMD -MP -fno-builtin
ifeq (arm, $(ARCH))
CPPFLAGS := -O1 -g -Wall -Werror -MMD -MP -fno-builtin
else
CPPFLAGS := -Os -g -Wall -Werror -MMD -MP -fno-builtin -mstrict-align
endif
CPPFLAGS +=  -I ../driver/include \
			 -I ../board/include \
			 -I ../common/include \
			 -I ../core/include \
			 -I$(USB_INC) \
			 -I ../driver/ddr/ddr \
			 -I ../driver/adc/ \

CPPFLAGS += -D$(CONFIG_PROJECT)=1 -D$(PROJECT)=1
LDFLAGS  := -g --entry=_start
LDFLAGS  += -T ../board/arch/$(ARCH)/bl1.lds

CPPFLAGS += -DIMG_HEADER_SIZE="$(IMG_HEADER_SIZE)"
SIGN_USE_RSA3072 ?= FALSE

ifeq ($(AX630C_DDR4_RETRAIN), TRUE)
CPPFLAGS += -DAX630C_DDR4_RETRAIN
endif

ifneq ($(lpddr4_clk),)
CPPFLAGS += -DCFG_LPDDR4_CLK_CMDLINE
CPPFLAGS += -DLPDDR4_CONFIG_$(lpddr4_clk)=1
endif

ifneq ($(ddr4_clk),)
CPPFLAGS += -DCFG_DDR4_CLK_CMDLINE
CPPFLAGS += -DDDR4_CONFIG_$(ddr4_clk)=1
endif

ifeq ($(SENSOR_TYPE), sc200ai)
CPPFLAGS += -DSENSOR_2M_CONFIG_1600
endif

ifeq ($(debug), yes)
CPPFLAGS += -DDEBUG
endif

CPPFLAGS += -DFDL_SUPPORT

ifeq (arm, $(ARCH))
CPPFLAGS += -DCONFIG_ARM
endif
ifeq (arm64, $(ARCH))
CPPFLAGS += -DCONFIG_ARM64
endif

all: $(IMAGE)

$(IMAGE): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ -L $(shell dirname `$(CC) $(c_flags) -print-libgcc-file-name`) -lgcc
	$(OBJCOPY) -O binary $(IMAGE) $(FDL_BIN)
	$(OBJDUMP) -S $(IMAGE) > fdl.dis

install:
	@$(ECHO) -e $(GREEN)"bl1 fdl $@..." $(DONE)
	@$(MKDIR) $(BUILD_PATH)/out/$(PROJECT)/
	@$(MKDIR) $(BUILD_PATH)/out/$(PROJECT)/images/debug
	$(CP) $(CUR_PATH)/$(FDL_BIN) $(BUILD_PATH)/out/$(PROJECT)/images
ifeq ($(strip $(SIGN_USE_RSA3072)),TRUE)
	$(warning use RSA3072 sign)
	@python3 $(IMGSIGN_SCRIPT)/fdl_AX620E_sign_3072.py -i \
		$(BUILD_PATH)/out/$(PROJECT)/images/$(FDL_BIN) -o \
		$(BUILD_PATH)/out/$(PROJECT)/images/$(FDL_SING_BIN) -pub \
		$(HOME_PATH)/tools/imgsign/key_3072/pubkey.pem -prv \
		$(HOME_PATH)/tools/imgsign/key_3072/private.pem -fw \
		$(IMGSIGN_SCRIPT)/eip_ax620e.bin -packsize 0x16800 -cap 0x47efe
else
	@python3 $(IMGSIGN_SCRIPT)/fdl_AX620E_sign.py -i \
		$(BUILD_PATH)/out/$(PROJECT)/images/$(FDL_BIN) -o \
		$(BUILD_PATH)/out/$(PROJECT)/images/$(FDL_SING_BIN) -pub \
		$(HOME_PATH)/tools/imgsign/public.pem -prv \
		$(HOME_PATH)/tools/imgsign/private.pem -fw \
		$(IMGSIGN_SCRIPT)/eip_ax620e.bin -packsize 0x16800 -cap 0x47afe
endif

clean:
	rm -rf $(IMAGE) $(OBJS) $(DEPS) $(FDL_BIN)

.PHONY: all clean

-include $(DEPS)
