
################################################################################
#	prepare param
################################################################################
HOME_PATH  := $(abspath $(shell pwd)/..)
BUILD_PATH := $(HOME_PATH)/build
TOOL_PATH  := $(HOME_PATH)/tools/imgsign
IMGSIGN_SCRIPT := $(BUILD_PATH)/tools/imgsign

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

CURDIR	:= $(abspath $(shell pwd))
CONFIG	:= $(CURDIR)/config/

KBUILD_OUTDIR    := $(CURDIR)/config
IMAGE_OUTDIR     := $(BUILD_PATH)/out/$(PROJECT)/images

RISCV_BIN := rtthread.bin
RISCV_SIGN_BIN := rtthread_signed.bin
RISCV_SIGN_BIN_BK := rtthread_b_signed.bin
SIGN_USE_RSA3072 ?= FALSE

RISCV_MACRO_FROM_BUILD := "macro_from_build"

ifeq ($(strip $(CHIP_TYPE)), AX630C)
RISCV_MACRO_FROM_BUILD += "CHIP_AX630C"
endif
ifeq ($(strip $(CHIP_TYPE)), AX620Q)
RISCV_MACRO_FROM_BUILD += "CHIP_AX620Q"
endif

ifeq ($(strip $(FLASH_TYPE)), nor)
RISCV_MACRO_FROM_BUILD += "AX620E_NOR"
endif
ifeq ($(strip $(FLASH_TYPE)), emmc)
RISCV_MACRO_FROM_BUILD += "AX620E_EMMC"
endif
ifeq ($(strip $(FLASH_TYPE)), nand)
RISCV_MACRO_FROM_BUILD += "AX620E_NAND"
endif

ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
RISCV_MACRO_FROM_BUILD += "RISCV_DDR_BASE=$(strip $(RISCV_BIN_DDR_START))"
RISCV_MACRO_FROM_BUILD += "RISCV_MEM_LEN_MB=$(strip $(RISCV_MEM_SIZE_MB))"
RISCV_MACRO_FROM_BUILD += "LOG_MEM_DDR_START=$(strip $(RISCV_LOG_MEM_RESERVE_START))"
RISCV_MACRO_FROM_BUILD += "LOG_MEM_DDR_LEN=$(strip $(RISCV_LOG_MEM_RESERVE_SIZE))"
RISCV_MACRO_FROM_BUILD += "SYS_DRAM_SIZE=$(strip $$(($(SYS_DRAM_SIZE)*1048576)))"
endif

ifeq ($(strip $(AX_RISCV_ISP_SUPPORT)),TRUE)
RISCV_MACRO_FROM_BUILD += "AX_RISCV_ISP_SUPPORT"
RISCV_MACRO_FROM_BUILD += "ISP_RESERVED_DDR_BASE=$(strip $(ISP_IMAGE_DDR_START))"
RISCV_MACRO_FROM_BUILD += "ISP_RESERVED_DDR_END=$(strip $(ISP_IMAGE_DDR_END))"
RISCV_MACRO_FROM_BUILD += "RAMDISK_END_ADDR=$(strip $(RAMDISK_END_ADDR))"
endif

ifeq ($(strip $(AX_RISCV_LOAD_ROOTFS_SUPPORT)),TRUE)
RISCV_MACRO_FROM_BUILD += "AX_RISCV_LOAD_ROOTFS_SUPPORT"
RISCV_MACRO_FROM_BUILD += "RAMDISK_FLASH_BASE=$(strip $(RAMDISK_FLASH_POS))"
RISCV_MACRO_FROM_BUILD += "RAMDISK_DDR_BASE=$(strip $(RAMDISK_START_ADDR))"
endif

ifeq ($(strip $(SUPPPORT_GZIPD)),TRUE)
RISCV_MACRO_FROM_BUILD += "SUPPPORT_GZIPD"
RISCV_MACRO_FROM_BUILD += "ROOTFS_COMPRESSED_ADDR=$(strip $(RISCV_ROOTFS_COMPRESSED_ADDR))"
endif

ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
RISCV_MACRO_FROM_BUILD += "AX_RISCV_LOAD_MODEL_SUPPORT"
RISCV_MACRO_FROM_BUILD += "MODEL_FLASH_BASE=$(strip $(MODEL_FLASH_POS))"
RISCV_MACRO_FROM_BUILD += "MODEL_DDR_BASE=$(strip $(MODEL_START_ADDR))"
endif

ifeq ($(strip $(AX_FAST_RISCV_RAW_DET_SUPPORT)), TRUE)
RISCV_MACRO_FROM_BUILD += "AX_FAST_RISCV_RAW_DET_SUPPORT"
endif

ifeq ($(strip $(AX_FAST_RISCV_MD_DET_SUPPORT)), TRUE)
RISCV_MACRO_FROM_BUILD += "AX_FAST_RISCV_MD_DET_SUPPORT"
endif
ifeq ($(strip $(AX_BOARD_MINION_SUPPORT)),TRUE)
RISCV_MACRO_FROM_BUILD += "AX_BOARD_MINION_SUPPORT"
endif

MAKE    := scons

.PHONY: clean all install
.NOTPARALLEL: clean install

default: all

all:
	@echo $(RISCV_MACRO_FROM_BUILD)
	@cp $(CONFIG)/link.lds $(CONFIG)/link.lds-org
	@cp $(CONFIG)/rtconfig.h $(CONFIG)/rtconfig.h-org
ifeq ($(AX_RISCV_SUPPORT), TRUE)
	@sed -i '/RISCV_BASE_ADDR =/c\RISCV_BASE_ADDR = $(strip $(RISCV_BIN_DDR_START));' $(CONFIG)/link.lds
	@sed -i '/RISCV_LEN =/c\RISCV_LEN = $(strip $(RISCV_MEM_SIZE_MB)) * 1024 * 1024;' $(CONFIG)/link.lds
endif

	@sed -i '/AX_RISCV_SNS_/c\#define AX_RISCV_SNS_OS04A10' $(CONFIG)/rtconfig.h
ifeq ($(SENSOR_TYPE), os04d10)
	@sed -i '/AX_RISCV_SNS_/c\#define AX_RISCV_SNS_OS04D10' $(CONFIG)/rtconfig.h
endif
ifeq ($(SENSOR_TYPE), sc200ai)
	@sed -i '/AX_RISCV_SNS_/c\#define AX_RISCV_SNS_SC200AI' $(CONFIG)/rtconfig.h
endif
ifeq ($(SENSOR_TYPE), sc850sl)
	@sed -i '/AX_RISCV_SNS_/c\#define AX_RISCV_SNS_SC850SL' $(CONFIG)/rtconfig.h
endif
ifeq ($(SENSOR_TYPE), sc450ai)
	@sed -i '/AX_RISCV_SNS_/c\#define AX_RISCV_SNS_SC450AI' $(CONFIG)/rtconfig.h
endif
ifeq ($(SENSOR_TYPE), sc500ai)
	@sed -i '/AX_RISCV_SNS_/c\#define AX_RISCV_SNS_SC500AI' $(CONFIG)/rtconfig.h
endif
ifeq ($(SENSOR_TYPE), sc200ai sc200ai)
	@sed -i '/AX_RISCV_SNS_/c\#define AX_RISCV_SNS_SC200AI_SC200AI' $(CONFIG)/rtconfig.h
endif
ifeq ($(SENSOR_TYPE), sc200ai sc200ai sc200ai)
	@sed -i '/AX_RISCV_SNS_/c\#define AX_RISCV_SNS_SC200AI_SC200AI_SC200AI' $(CONFIG)/rtconfig.h
endif
ifeq ($(SENSOR_TYPE), os04a10 os04a10)
	@sed -i '/AX_RISCV_SNS_/c\#define AX_RISCV_SNS_OS04A10_OS04A10' $(CONFIG)/rtconfig.h
endif
ifeq ($(SENSOR_TYPE), sc850sl os04a10)
	@sed -i '/AX_RISCV_SNS_/c\#define AX_RISCV_SNS_SC850SL_OS04A10' $(CONFIG)/rtconfig.h
endif
ifeq ($(strip $(AX_RISCV_LOAD_ROOTFS_SUPPORT)),TRUE)
	@sed -i '/AX_LOAD_ROOTFS_NOT_SUPPORT/c\#define AX_LOAD_ROOTFS_SUPPORT' $(CONFIG)/rtconfig.h
endif
	@$(MAKE) -C $(CONFIG) CPPDEFINES="$(RISCV_MACRO_FROM_BUILD)" -j16
	@mv $(CONFIG)/link.lds-org $(CONFIG)/link.lds
	@mv $(CONFIG)/rtconfig.h-org $(CONFIG)/rtconfig.h
	@echo -e $(GREEN)"\nBuild All $(CURDIR) Modules success!!\n"  $(DONE)

install:
	@mkdir -p $(BUILD_PATH)/out/$(PROJECT)/images
	$(CP) $(KBUILD_OUTDIR)/$(RISCV_BIN) $(BUILD_PATH)/out/$(PROJECT)/images

ifeq ($(SUPPPORT_GZIPD), TRUE)
	$(HOME_PATH)/tools/ax_gzip_tool/ax_gzip -9 $(KBUILD_OUTDIR)/$(RISCV_BIN)
ifeq ($(strip $(SIGN_USE_RSA3072)),TRUE)
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(KBUILD_OUTDIR)/rtthread_axgzip.bin -o $(BUILD_PATH)/out/$(PROJECT)/images/$(RISCV_SIGN_BIN) -pub $(HOME_PATH)/tools/imgsign/key_3072/pubkey.pem -prv $(HOME_PATH)/tools/imgsign/key_3072/private.pem -cap 0x54FEFE -key_bit 3072
else
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(KBUILD_OUTDIR)/rtthread_axgzip.bin -o $(BUILD_PATH)/out/$(PROJECT)/images/$(RISCV_SIGN_BIN) -pub $(TOOL_PATH)/public.pem -prv $(TOOL_PATH)/private.pem -cap 0x54FAFE -key_bit 2048
endif
else
ifeq ($(strip $(SIGN_USE_RSA3072)),TRUE)
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(KBUILD_OUTDIR)/$(RISCV_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(RISCV_SIGN_BIN) -pub $(HOME_PATH)/tools/imgsign/key_3072/pubkey.pem -prv $(HOME_PATH)/tools/imgsign/key_3072/private.pem -cap 0x54FEFE -key_bit 3072
else
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(KBUILD_OUTDIR)/$(RISCV_BIN) -o $(BUILD_PATH)/out/$(PROJECT)/images/$(RISCV_SIGN_BIN) -pub $(TOOL_PATH)/public.pem -prv $(TOOL_PATH)/private.pem -cap 0x54FAFE -key_bit 2048
endif
endif
	$(CP) $(IMAGE_OUTDIR)/$(RISCV_SIGN_BIN) $(IMAGE_OUTDIR)/$(RISCV_SIGN_BIN_BK)
	@echo -e $(GREEN)"\nInstall riscv bin success!!\n"  $(DONE)

clean:
	@rm -rf $(CONFIG)/build
	@echo -e $(GREEN)"\nClean $(CURDIR) success!!\n"  $(DONE)
