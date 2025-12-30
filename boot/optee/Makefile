HOME_PATH  := $(abspath $(shell pwd)/../..)
BUILD_PATH := $(HOME_PATH)/build
TOOLS_PATH := $(HOME_PATH)/tools
IMGSIGN_SCRIPT := $(BUILD_PATH)/tools/imgsign

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

OPTEE           := optee
OPTEE_DIR       := optee_os-3.21.0

OPTEE_ELF  := tee.elf
OPTEE_BIN_NAME  := tee-pager_v2
OPTEE_IMG  := optee.img
OPTEE_SIGNED_BIN  := optee_signed.bin
OPTEE_PLAT := ax620e
OPTEE_TARGET := optee
AES_KEY := aes-256.key
SIGN_USE_RSA3072 ?= FALSE

optee_img_size := $(call KM2Bytes,$(OPTEE_PARTITION_SIZE))

.PHONY: all prepare optee
.NOTPARALLEL: clean install

#OPTEE build param
BUILD_BASE   = $(HOME_PATH)/build/out/$(PROJECT)/objs/boot/$(OPTEE)/$(OPTEE_DIR)
export BUILD_BASE

IMAGE_OUTDIR := $(BUILD_PATH)/out/$(PROJECT)/images
DEBUG_LOG_LEVEL := 1
USERCFLAGS   += PLAT_OPTEE_ADDR_START="$(OPTEE_IMAGE_ADDR)"
USERCFLAGS   += PLAT_OPTEE_SHMEM_SIZE="$(OPTEE_SHMEM_SIZE)"
USERCFLAGS   += PLAT_OPTEE_RESERVED_SIZE="$(OPTEE_RESERVED_SIZE)"

EXT_FLAGS    += $(USERCFLAGS)

default: optee

all: prepare optee
	@$(ECHO) -e $(GREEN)"\nBuild optee success!!\n" $(DONE)

prepare:
	@$(ECHO)
	@$(ECHO) -e $(GREEN) "first build prepare" $(DONE)

optee:
	@$(ECHO)
	@$(ECHO) -e $(GREEN) "Making $@..." $(DONE)
	@$(MAKE) -C $(OPTEE_DIR)/ CROSS_COMPILE=$(CROSS) CROSS_COMPILE64=$(CROSS) CFG_TEE_CORE_LOG_LEVEL=$(DEBUG_LOG_LEVEL) PLATFORM=axera-$(OPTEE_PLAT) O=$(BUILD_BASE) CFG_ARM64_core=y $(EXT_FLAGS) -j4

install:
	@$(ECHO) -e $(GREEN) "optee $@..." $(DONE)
	@$(MKDIR) $(IMAGE_OUTDIR)
	@$(MKDIR) $(IMAGE_OUTDIR)/debug
	$(CP) $(BUILD_BASE)/core/$(OPTEE_ELF) $(IMAGE_OUTDIR)/debug/$(OPTEE_ELF)

ifeq ($(SUPPPORT_GZIPD), TRUE)
	$(TOOLS_PATH)/ax_gzip_tool/ax_gzip -9 $(BUILD_BASE)/core/$(OPTEE_BIN_NAME).bin
ifeq ($(strip $(SIGN_USE_RSA3072)),TRUE)
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(BUILD_BASE)/core/$(OPTEE_BIN_NAME)_axgzip.bin -pub $(TOOLS_PATH)/imgsign/key_3072/pubkey.pem -prv $(TOOLS_PATH)/imgsign/key_3072/private.pem  -o $(IMAGE_OUTDIR)/$(OPTEE_SIGNED_BIN) -cap 0x54fefe -key_bit 3072
else
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(BUILD_BASE)/core/$(OPTEE_BIN_NAME)_axgzip.bin -pub $(TOOLS_PATH)/imgsign/public.pem -prv $(TOOLS_PATH)/imgsign/private.pem  -o $(IMAGE_OUTDIR)/$(OPTEE_SIGNED_BIN) -cap 0x54fafe -key_bit 2048
endif
else
ifeq ($(strip $(SIGN_USE_RSA3072)),TRUE)
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(BUILD_BASE)/core/$(OPTEE_BIN_NAME).bin -pub $(TOOLS_PATH)/imgsign/key_3072/pubkey.pem -prv $(TOOLS_PATH)/imgsign/key_3072/private.pem  -o $(IMAGE_OUTDIR)/$(OPTEE_SIGNED_BIN) -cap 0x54fefe -key_bit 3072
else
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(BUILD_BASE)/core/$(OPTEE_BIN_NAME).bin -pub $(TOOLS_PATH)/imgsign/public.pem -prv $(TOOLS_PATH)/imgsign/private.pem  -o $(IMAGE_OUTDIR)/$(OPTEE_SIGNED_BIN) -cap 0x54fafe -key_bit 2048
endif
endif
	@imgsize=$$(stat -c %s $(IMAGE_OUTDIR)/$(OPTEE_SIGNED_BIN));\
	if [ $$imgsize -gt $(optee_img_size) ]; then \
		echo "Error: $(OPTEE_SIGNED_BIN) file size($$imgsize bytes) exceeds $(strip $(optee_img_size)) bytes"; \
		exit 1; \
	fi

clean:
	@$(ECHO) -e $(GREEN) "optee $@..." $(DONE)
	@$(MAKE) -C $(OPTEE_DIR)/ PLATFORM=axera-$(OPTEE_PLAT) O=$(BUILD_BASE) CFG_ARM64_core=y clean
	@rm -rf $(BUILD_BASE)
	@rm -f $(IMAGE_OUTDIR)/$(OPTEE_IMG)
	@rm -f $(IMAGE_OUTDIR)/$(OPTEE_SIGNED_BIN)
	@rm -f $(IMAGE_OUTDIR)/debug/$(OPTEE_ELF)
