###################################################################################
# Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
#
# This source file is the property of Axera Semiconductor Co., Ltd. and
# may not be copied or distributed in any isomorphic form without the prior
# written consent of Axera Semiconductor Co., Ltd.
###################################################################################

CUR_PATH        := $(shell pwd)
HOME_PATH       := $(abspath $(shell pwd)/../../..)
BUILD_PATH      := $(HOME_PATH)/build
IMGSIGN_SCRIPT  := $(BUILD_PATH)/tools/imgsign

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

CC := $(CROSS)gcc
LD := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump

IMAGE := spl_$(PROJECT).axf
SPL_BIN := spl_$(PROJECT).bin
SPL_SEC_BIN := spl_$(PROJECT)_enc.bin
SPL_SING_BIN := spl_$(PROJECT)_signed.bin
SPL_SEC_SING_BIN := spl_$(PROJECT)_enc_signed.bin
DDRINIT_SING_BIN := ddrinit_$(PROJECT)_signed.bin

# SIGN_WITH_RISCV := true
SIGN_USE_RSA3072 ?= FALSE

OBJS := ../board/arch/$(ARCH)/start.o ../board/board.o ../board/arch/$(ARCH)/stack.o ../common/cmn.o ../board/meminit.o spl_main.o
OBJS += ../driver/uart/uart.o ../driver/timer/timer.o \
		../driver/axdma/axdma_checksum.o \
		../driver/secure/efuse_drv.o ../driver/secure/eip130_drv.o ../driver/adc/ax_adc.o\
		../driver/ddr/ddr_init.o ../driver/ddr/ddrmc_train_flow_lp4.o ../driver/ddr/ddrmc_train_flow_no_lp.o ../driver/ddr/ddr_scan.o \
		../driver/usb/printf.o ../core/trace/trace.o\
		../driver/secure/secure.o ../driver/atf/atf.o ../driver/pwm/pwm.o \
		../driver/gzipd/ax_gzipd_drv.o

OBJS += ../core/boot/boot.o

ifeq ($(strip $(AX_SPL_SUPPORT_MODIFY_BOOTARGS)),TRUE)
OBJS += ../core/libfdt/fdt.o
OBJS += ../core/libfdt/fdt_rw.o
OBJS += ../core/libfdt/fdt_ro.o
CPPFLAGS += -DKERNEL_BOOTARGS=\"$(KERNEL_BOOTARGS)\"
endif

ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
OBJS += ../driver/riscv/riscv.o ../driver/riscv/riscv_sw_int.o
OBJS += ../core/ax_env/fw_env.o
endif

#ifeq ($(PROJECT), AX630C_emmc_arm64_k515)
ifneq (,$(findstring $(FLASH_TYPE), "emmc" "none"))
OBJS += ../driver/mmc/sdhci_cdns.o ../driver/mmc/mmc.o ../driver/mmc/axera_mmc.o
endif
ifeq ($(FLASH_TYPE), nand)
OBJS += ../driver/spi/spinand.o ../driver/spi/dw_spi_mst.o
endif
ifeq ($(FLASH_TYPE), nor)
OBJS += ../driver/spi/spinor.o ../driver/spi/dw_spi_mst.o
endif

USB_INC = ../driver/usb

DEPS := $(patsubst %.o,%.d,$(OBJS))

# CPPFLAGS := -g -Wall -MMD -MP -fno-builtin
ifeq (arm, $(ARCH))
CPPFLAGS := -O1 -Wall -Werror -MMD -MP -fno-builtin -s -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer
else
CPPFLAGS := -Os -Wall -Werror -MMD -MP -fno-builtin -s -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer -mstrict-align
endif

CPPFLAGS +=  -I ../driver/include \
			 -I ../board/include \
			 -I ../common/include \
			 -I ../core/include \
			 -I$(USB_INC) \
			 -I ../driver/ddr/ddr \
			 -I ../driver/adc\
			 -I ../core/ax_env \

CPPFLAGS += -D$(CONFIG_PROJECT)=1 -D$(PROJECT)=1
LDFLAGS  := --entry=_start
LDFLAGS  += -T ../board/arch/$(ARCH)/bl1.lds

ifeq ($(DEBUG_BOOT_TIME), TRUE)
CPPFLAGS += -DDEBUG_BOOT_TIME
endif

ifeq ($(SUPPORT_RECOVERY),TRUE)
CPPFLAGS += -DSUPPORT_RECOVERY
endif

ifeq ($(AX630C_DDR4_RETRAIN), TRUE)
CPPFLAGS += -DAX630C_DDR4_RETRAIN
endif

ifeq ($(AX_SUPPORT_AB_PART), TRUE)
CPPFLAGS += -DUBOOT_IMG_HEADER_BASE="$(UBOOT_IMG_HEADER_BASE)"
CPPFLAGS += -DUBOOT_HEADER_FLASH_BASE="$(UBOOT_A_HEADER_FLASH_BASE)"
CPPFLAGS += -DUBOOT_HEADER_BAK_FLASH_BASE="$(UBOOT_B_HEADER_FLASH_BASE)"

CPPFLAGS += -DDDRINIT_PARAM_HEADER_BASE="$(DDRINIT_PARAM_HEADER_BASE)"
ifneq ($(strip $(SUPPORT_DDRINIT_PART)),FALSE)
CPPFLAGS += -DSUPPORT_DDRINIT_PART
CPPFLAGS += -DDDRINIT_HEADER_FLASH_BASE="$(DDRINIT_HEADER_FLASH_BASE)"
endif

CPPFLAGS += -DATF_IMG_HEADER_BASE="$(ATF_IMG_HEADER_BASE)"
CPPFLAGS += -DATF_HEADER_FLASH_BASE="$(ATF_A_HEADER_FLASH_BASE)"
CPPFLAGS += -DATF_HEADER_BAK_FLASH_BASE="$(ATF_B_HEADER_FLASH_BASE)"

CPPFLAGS += -DDTB_IMG_HEADER_ADDR="$(DTB_IMG_HEADER_ADDR)"
CPPFLAGS += -DDTB_HEADER_FLASH_BASE="$(DTB_A_HEADER_FLASH_BASE)"
CPPFLAGS += -DDTB_HEADER_BAK_FLASH_BASE="$(DTB_B_HEADER_FLASH_BASE)"

CPPFLAGS += -DKERNEL_IMG_HEADER_ADDR="$(KERNEL_IMG_HEADER_ADDR)"
CPPFLAGS += -DKERNEL_HEADER_FLASH_BASE="$(KERNEL_A_HEADER_FLASH_BASE)"
CPPFLAGS += -DKERNEL_HEADER_BAK_FLASH_BASE="$(KERNEL_B_HEADER_FLASH_BASE)"
ifeq ($(strip $(SUPPORT_OPTEE)),TRUE)
CPPFLAGS += -DOPTEE_BOOT
CPPFLAGS += -DOPTEE_IMAGE_ADDR="$(OPTEE_IMAGE_ADDR)"
CPPFLAGS += -DOPTEE_HEADER_FLASH_BASE="$(OPTEE_A_HEADER_FLASH_BASE)"
CPPFLAGS += -DOPTEE_HEADER_BAK_FLASH_BASE="$(OPTEE_B_HEADER_FLASH_BASE)"
endif
else
CPPFLAGS += -DUBOOT_IMG_HEADER_BASE="$(UBOOT_IMG_HEADER_BASE)"
CPPFLAGS += -DUBOOT_HEADER_FLASH_BASE="$(UBOOT_A_HEADER_FLASH_BASE)"

CPPFLAGS += -DDDRINIT_PARAM_HEADER_BASE="$(DDRINIT_PARAM_HEADER_BASE)"
ifneq ($(strip $(SUPPORT_DDRINIT_PART)),FALSE)
CPPFLAGS += -DSUPPORT_DDRINIT_PART
CPPFLAGS += -DDDRINIT_HEADER_FLASH_BASE="$(DDRINIT_HEADER_FLASH_BASE)"
endif

CPPFLAGS += -DATF_IMG_HEADER_BASE="$(ATF_IMG_HEADER_BASE)"
CPPFLAGS += -DATF_HEADER_FLASH_BASE="$(ATF_A_HEADER_FLASH_BASE)"


CPPFLAGS += -DDTB_IMG_HEADER_ADDR="$(DTB_IMG_HEADER_ADDR)"
CPPFLAGS += -DDTB_HEADER_FLASH_BASE="$(DTB_A_HEADER_FLASH_BASE)"
CPPFLAGS += -DDTB_HEADER_BAK_FLASH_BASE="0x0"

CPPFLAGS += -DKERNEL_IMG_HEADER_ADDR="$(KERNEL_IMG_HEADER_ADDR)"
CPPFLAGS += -DKERNEL_HEADER_FLASH_BASE="$(KERNEL_A_HEADER_FLASH_BASE)"
CPPFLAGS += -DKERNEL_HEADER_BAK_FLASH_BASE="0x0"

### SIPEED EDIT ###
ifneq (,$(findstring $(PROJECT), "AX630C_emmc_arm64_k515" "AX630C_emmc_arm64_k419" "AX630C_emmc_arm64_k419_sipeed_maixcam2"))
### SIPEED EDIT END ###
CPPFLAGS += -DUBOOT_HEADER_BAK_FLASH_BASE="$(UBOOT_B_HEADER_FLASH_BASE)"
CPPFLAGS += -DATF_HEADER_BAK_FLASH_BASE="$(ATF_B_HEADER_FLASH_BASE)"
else ifneq (,$(findstring $(PROJECT), "AX630C_emmc_arm32_k419"))
CPPFLAGS += -DUBOOT_HEADER_BAK_FLASH_BASE="$(UBOOT_B_HEADER_FLASH_BASE)"
CPPFLAGS += -DATF_HEADER_BAK_FLASH_BASE="0x0"
else
CPPFLAGS += -DUBOOT_HEADER_BAK_FLASH_BASE="0x0"
CPPFLAGS += -DATF_HEADER_BAK_FLASH_BASE="0x0"
endif

ifeq ($(strip $(SUPPORT_OPTEE)),TRUE)
CPPFLAGS += -DOPTEE_BOOT
CPPFLAGS += -DOPTEE_IMAGE_ADDR="$(OPTEE_IMAGE_ADDR)"
CPPFLAGS += -DOPTEE_HEADER_FLASH_BASE="$(OPTEE_A_HEADER_FLASH_BASE)"
CPPFLAGS += -DOPTEE_HEADER_BAK_FLASH_BASE="0x0"
endif
endif

CPPFLAGS += -DRAMDISK_START_ADDR="$(RAMDISK_START_ADDR)"
CPPFLAGS += -DRAMDISK_FLASH_POS="$(RAMDISK_FLASH_POS)"

CPPFLAGS += -DRAWDATA_FLASH_POS="$(RAWDATA_FLASH_POS)"

ifeq ($(strip $(AX_SPL_UBOOT_KERNEL_SUPPORT)),TRUE)
CPPFLAGS += -DAX_SPL_UBOOT_KERNEL_SUPPORT
endif
ifeq ($(strip $(AX_BOOT_OPTIMIZATION_SUPPORT)),TRUE)
CPPFLAGS += -DAX_BOOT_OPTIMIZATION_SUPPORT
endif
ifeq ($(strip $(SUPPPORT_GZIPD)),TRUE)
CPPFLAGS += -DSUPPPORT_GZIPD
endif
ifeq ($(strip $(SECURE_BOOT_TEST)),TRUE)
CPPFLAGS += -DSECURE_BOOT_TEST
endif
ifeq ($(strip $(SUPPORT_RAMDISK)),TRUE)
CPPFLAGS += -DSUPPORT_RAMDISK
endif
ifeq ($(strip $(SUPPORT_ATF)),TRUE)
CPPFLAGS += -DSUPPORT_ATF
endif

ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
CPPFLAGS += -DAX_RISCV_SUPPORT
ifeq ($(strip $(AX_RISCV_LOAD_ROOTFS_SUPPORT)),TRUE)
CPPFLAGS += -DAX_RISCV_LOAD_ROOTFS_SUPPORT
endif
endif

ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
CPPFLAGS += -DRISCV_BIN_DDR_START="$(RISCV_BIN_DDR_START)"
CPPFLAGS += -DRISCV_HEADER_FLASH_BASE="$(RISCV_HEADER_FLASH_BASE)"
ifeq ($(strip $(AX_SUPPORT_AB_PART)),TRUE)
CPPFLAGS += -DRISCV_B_HEADER_FLASH_BASE="$(RISCV_B_HEADER_FLASH_BASE)"
endif
CPPFLAGS += -DRISCV_HEADER_DDR_START="$(RISCV_HEADER_DDR_START)"
endif

ifeq ($(strip $(AX_SUPPORT_AB_PART)),TRUE)
CPPFLAGS += -DAX_SUPPORT_AB_PART
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

### SIPEED EDIT ###
ifeq ($(strip $(FLASH_TYPE)), emmc)
    CPPFLAGS += -DAX620E_EMMC=1
else
### SIPEED EDIT END ###

ifneq (,$(findstring $(PROJECT), "AX630C_emmc_arm64_k515" "AX620E_haps" "AX620E_zebu" "AX630C_fastemmc_arm64_k515" "AX630C_emmc_arm64_k419" "AX630C_fastemmc_arm64_k419" "AX630C_fastemmc_arm32_k419" "AX630C_emmc_arm32_k419"))
CPPFLAGS += -DAX620E_EMMC=1
else
OBJS += ../driver/dmaper/dmaper.o
endif

### SIPEED EDIT ###
endif
### SIPEED EDIT END ###

ifeq ($(strip $(FLASH_TYPE)), nand)
CPPFLAGS += -DAX620E_NAND=1
endif

ifeq ($(strip $(FLASH_TYPE)), nor)
CPPFLAGS += -DAX620E_NOR=1
endif

ifeq ($(strip $(AX_SPL_SUPPORT_MODIFY_BOOTARGS)),TRUE)
CPPFLAGS += -DAX_SPL_SUPPORT_MODIFY_BOOTARGS
CPPFLAGS += -DKERNEL_BOOTARGS=\"$(KERNEL_BOOTARGS)\"
endif

spl_img_size     := $(call KM2Bytes,$(SPL_PARTITION_SIZE))
ifneq ($(strip $(DDRINIT_PARTITION_SIZE)),)
ddrinit_img_size := $(call KM2Bytes,$(DDRINIT_PARTITION_SIZE))
endif

ifeq (arm, $(ARCH))
CPPFLAGS += -DCONFIG_ARM
endif
ifeq (arm64, $(ARCH))
CPPFLAGS += -DCONFIG_ARM64
endif

all: $(IMAGE)
$(IMAGE): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ -L $(shell dirname `$(CC) $(c_flags) -print-libgcc-file-name`) -lgcc
# $(IMAGE): $(OBJS) Makefile
# 	$(LD) $(LDFLAGS) $(OBJS) -o $@ -Map spl_mc20e.map

	$(OBJCOPY) -O binary $(IMAGE) $(SPL_BIN)
	$(OBJDUMP) -S $(IMAGE) > spl.dis

install:
	@$(ECHO) -e $(GREEN)"bl1 fdl $@..." $(DONE)
	@$(ECHO) -e $(GREEN)"bl1 spl $@..." $(DONE)
	@$(MKDIR) $(BUILD_PATH)/out/$(PROJECT)/
	@$(MKDIR) $(BUILD_PATH)/out/$(PROJECT)/images/debug
	$(CP) $(CUR_PATH)/$(SPL_BIN) $(BUILD_PATH)/out/$(PROJECT)/images
	$(CP) $(IMGSIGN_SCRIPT)/eip_ax620e.bin $(BUILD_PATH)/out/$(PROJECT)/images/eip_ax620e.bin

ifeq ($(strip $(SIGN_USE_RSA3072)),TRUE)
	$(warning use RSA3072 sign)
	@openssl aes-256-ecb -e -in $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN) -out $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_BIN) -K $(shell hexdump -e '16/1 "%04x"' $(HOME_PATH)/boot/bl1/aes-256-c-key1) -nosalt -p
	@openssl aes-256-ecb -e -in $(HOME_PATH)/boot/bl1/aes-256-c-key1 -out $(HOME_PATH)/boot/bl1/aes-256-p-key0 -K $(shell hexdump -e '16/1 "%04x"' $(HOME_PATH)/boot/bl1/aes-256-c-key1) -nosalt -p
ifeq ($(strip $(SIGN_WITH_RISCV)),true)
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign_3072.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/key_3072/pubkey.pem -prv $(HOME_PATH)/tools/imgsign/key_3072/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0xD4FFFE -riscv $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN) -key $(HOME_PATH)/boot/bl1/aes-256-p-key0
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign_3072.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/key_3072/pubkey.pem -prv $(HOME_PATH)/tools/imgsign/key_3072/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0xD4FEFE -riscv $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN)
else
ifeq ($(FLASH_TYPE), nor)
	$(info "AX620E nor sign bin")
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign_3072.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/key_3072/pubkey.pem -prv $(HOME_PATH)/tools/imgsign/key_3072/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0x54FFFE -key $(HOME_PATH)/boot/bl1/aes-256-p-key0 -small_size_nor
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign_3072.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/key_3072/pubkey.pem -prv $(HOME_PATH)/tools/imgsign/key_3072/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0x54FEFE -small_size_nor

else
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign_3072.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/key_3072/pubkey.pem -prv $(HOME_PATH)/tools/imgsign/key_3072/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0x54FFFE -key $(HOME_PATH)/boot/bl1/aes-256-p-key0
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign_3072.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/key_3072/pubkey.pem -prv $(HOME_PATH)/tools/imgsign/key_3072/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0x54FEFE
endif
endif

	touch $(BUILD_PATH)/out/$(PROJECT)/images/ddrinit.bin
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(BUILD_PATH)/out/$(PROJECT)/images/ddrinit.bin -o $(BUILD_PATH)/out/$(PROJECT)/images/$(DDRINIT_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/key_3072/pubkey.pem -prv $(HOME_PATH)/tools/imgsign/key_3072/private.pem -cap 0x54FEFE -key_bit 3072

else

	@openssl aes-256-ecb -e -in $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN) -out $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_BIN) -K $(shell hexdump -e '16/1 "%04x"' $(HOME_PATH)/boot/bl1/aes-256-c-key1) -nosalt -p
	@openssl aes-256-ecb -e -in $(HOME_PATH)/boot/bl1/aes-256-c-key1 -out $(HOME_PATH)/boot/bl1/aes-256-p-key0 -K $(shell hexdump -e '16/1 "%04x"' $(HOME_PATH)/boot/bl1/aes-256-c-key1) -nosalt -p
ifeq ($(strip $(SIGN_WITH_RISCV)),true)
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/public.pem -prv $(HOME_PATH)/tools/imgsign/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0xD4FBFE -riscv $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN) -key $(HOME_PATH)/boot/bl1/aes-256-p-key0
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/public.pem -prv $(HOME_PATH)/tools/imgsign/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0xD4FAFE -riscv $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN)
else

ifeq ($(FLASH_TYPE), nor)
	$(info "AX620E nor sign bin")
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/public.pem -prv $(HOME_PATH)/tools/imgsign/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0x54FBFE -key $(HOME_PATH)/boot/bl1/aes-256-p-key0 -small_size_nor
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/public.pem -prv $(HOME_PATH)/tools/imgsign/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0x54FAFE -small_size_nor
else
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SEC_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/public.pem -prv $(HOME_PATH)/tools/imgsign/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0x54FBFE -key $(HOME_PATH)/boot/bl1/aes-256-p-key0
	@python3 $(IMGSIGN_SCRIPT)/spl_AX620E_sign.py -i $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/public.pem -prv $(HOME_PATH)/tools/imgsign/private.pem -fw $(IMGSIGN_SCRIPT)/eip_ax620e.bin -cap 0x54FAFE
endif
endif

	touch $(BUILD_PATH)/out/$(PROJECT)/images/ddrinit.bin
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(BUILD_PATH)/out/$(PROJECT)/images/ddrinit.bin -o $(BUILD_PATH)/out/$(PROJECT)/images/$(DDRINIT_SING_BIN) -pub $(HOME_PATH)/tools/imgsign/public.pem -prv $(HOME_PATH)/tools/imgsign/private.pem -cap 0x54FAFE -key_bit 2048
endif
	@splsize=$$(stat -c %s $(BUILD_PATH)/out/$(PROJECT)/images/$(SPL_SING_BIN));\
	if [ $$splsize -gt $(spl_img_size) ]; then \
		echo "Error: $(SPL_SING_BIN) file size($$splsize bytes) exceeds $(strip $(spl_img_size)) bytes"; \
		exit 1; \
	fi
ifneq ($(strip $(DDRINIT_PARTITION_SIZE)),)
	@ddrsize=$$(stat -c %s $(BUILD_PATH)/out/$(PROJECT)/images/$(DDRINIT_SING_BIN));\
	if [ $$ddrsize -gt $(ddrinit_img_size) ]; then \
		echo "Error: $(DDRINIT_SING_BIN) file size($$ddrsize bytes) exceeds $(strip $(ddrinit_img_size)) bytes"; \
		exit 1; \
	fi
endif

clean:
	rm -rf $(IMAGE) $(OBJS) $(DEPS) $(SPL_BIN)

.PHONY: all clean

-include $(DEPS)
