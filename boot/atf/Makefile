HOME_PATH  := $(abspath $(shell pwd)/../..)
BUILD_PATH := $(HOME_PATH)/build
TOOLS_PATH := $(HOME_PATH)/tools
IMGSIGN_SCRIPT := $(BUILD_PATH)/tools/imgsign

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

ATF           := atf
ATF_DIR       := arm-trusted-firmware-2.7

ATF_BL31_ELF  := bl31.elf
ATF_BL31_BIN  := bl31.bin
ATF_BL31_SIGNED_BIN  := atf_bl31_signed.bin
ATF_BL31_SIGNED_BIN_BK  := atf_b_bl31_signed.bin
ATF_BL31_IMG  := atf_bl31.img
ATF_ARCH := aarch64
ATF_PLAT := ax620e
ATF_TARGET := bl31
AES_KEY := aes-256.key
SUPPORT_FIREWALL ?= FALSE
SIGN_USE_RSA3072 ?= FALSE

ifeq ($(SUPPORT_OPTEE),TRUE)
SUPPORT_FIREWALL := TRUE
ifeq ($(SUPPORT_FIREWALL),TRUE)
USERCFLAGS	+= -DFIREWALL_IS_ENABLED
endif
USERCFLAGS	+= -DOPTEE_BOOT
USERCFLAGS	+= -DOPTEE_IMAGE_ADDR="$(OPTEE_IMAGE_ADDR)"
USERCFLAGS	+= -DOPTEE_RESERVED_SIZE="$(OPTEE_RESERVED_SIZE)"
USERCFLAGS	+= -DOPTEE_SHMEM_SIZE="$(OPTEE_SHMEM_SIZE)"
USERCFLAGS	+= -DDDR_START_ADDR="$(SYS_DRAM_BASE)"
endif

USERCFLAGS	+= -DATF_IMG_ADDR="$(ATF_IMG_ADDR)"
USERCFLAGS	+= -DATF_IMG_PKG_SIZE="$(ATF_IMG_PKG_SIZE)"

EXT_FLAGS = EXT_CFLAGS="$(USERCFLAGS)"

.PHONY: all prepare atf_bl31
.NOTPARALLEL: clean install

#ATF build param
BUILD_BASE   = $(HOME_PATH)/build/out/$(PROJECT)/objs/boot/$(ATF)/$(ATF_DIR)
export BUILD_BASE

IMAGE_OUTDIR := $(BUILD_PATH)/out/$(PROJECT)/images
DEBUG_EN := 0
ifeq ($(DEBUG_EN),1)
ATF_RELEASE_PATH := debug
else
ATF_RELEASE_PATH := release
endif

atf_img_size := $(call KM2Bytes,$(ATF_PARTITION_SIZE))

default: atf_bl31

all: prepare atf_bl31
	@$(ECHO) -e $(GREEN)"\nBuild atf_bl31 success!!\n" $(DONE)

prepare:
	@$(ECHO)
	@$(ECHO) -e $(GREEN) "first build prepare" $(DONE)

atf_bl31:
	@$(ECHO)
	@$(ECHO) -e $(GREEN) "Making $@..." $(DONE)
ifeq ($(strip $(SUPPORT_OPTEE)),TRUE)
	@$(MAKE) -C $(ATF_DIR)/ ARCH=$(ATF_ARCH) CROSS_COMPILE=$(CROSS) PLAT=$(ATF_PLAT) SPD=opteed $(ATF_TARGET) DEBUG=$(DEBUG_EN) $(EXT_FLAGS) -j4
else
	@$(MAKE) -C $(ATF_DIR)/ ARCH=$(ATF_ARCH) CROSS_COMPILE=$(CROSS) PLAT=$(ATF_PLAT) $(ATF_TARGET) DEBUG=$(DEBUG_EN) $(EXT_FLAGS) -j4
endif

install:
	@$(ECHO) -e $(GREEN) "atf_bl31 $@..." $(DONE)
	@$(MKDIR) $(IMAGE_OUTDIR)
	@$(MKDIR) $(IMAGE_OUTDIR)/debug
	$(CP) $(BUILD_BASE)/$(ATF_PLAT)/$(ATF_RELEASE_PATH)/$(ATF_BL31_BIN) $(IMAGE_OUTDIR)/atf_bl31.bin
	$(CP) $(BUILD_BASE)/$(ATF_PLAT)/$(ATF_RELEASE_PATH)/$(ATF_TARGET)/$(ATF_BL31_ELF) $(IMAGE_OUTDIR)/debug/$(ATF)_$(ATF_BL31_ELF)

ifeq ($(SUPPPORT_GZIPD), TRUE)
	$(TOOLS_PATH)/ax_gzip_tool/ax_gzip -9 $(BUILD_BASE)/$(ATF_PLAT)/$(ATF_RELEASE_PATH)/$(ATF_BL31_BIN)
ifeq ($(strip $(SIGN_USE_RSA3072)),TRUE)
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(BUILD_BASE)/$(ATF_PLAT)/$(ATF_RELEASE_PATH)/bl31_axgzip.bin -pub $(TOOLS_PATH)/imgsign/key_3072/pubkey.pem -prv $(TOOLS_PATH)/imgsign/key_3072/private.pem  -o $(IMAGE_OUTDIR)/$(ATF_BL31_SIGNED_BIN) -cap 0x54FEFE -key_bit 3072
else
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(BUILD_BASE)/$(ATF_PLAT)/$(ATF_RELEASE_PATH)/bl31_axgzip.bin -pub $(TOOLS_PATH)/imgsign/public.pem -prv $(TOOLS_PATH)/imgsign/private.pem  -o $(IMAGE_OUTDIR)/$(ATF_BL31_SIGNED_BIN) -cap 0x54FAFE -key_bit 2048
endif
	$(CP) $(IMAGE_OUTDIR)/$(ATF_BL31_SIGNED_BIN) $(IMAGE_OUTDIR)/$(ATF_BL31_SIGNED_BIN_BK)

else

ifeq ($(strip $(SIGN_USE_RSA3072)),TRUE)
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(BUILD_BASE)/$(ATF_PLAT)/$(ATF_RELEASE_PATH)/$(ATF_BL31_BIN) -pub $(TOOLS_PATH)/imgsign/key_3072/pubkey.pem -prv $(TOOLS_PATH)/imgsign/key_3072/private.pem  -o $(IMAGE_OUTDIR)/$(ATF_BL31_SIGNED_BIN) -cap 0x54FEFE -key_bit 3072
else
	@python3 $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py -i $(BUILD_BASE)/$(ATF_PLAT)/$(ATF_RELEASE_PATH)/$(ATF_BL31_BIN) -pub $(TOOLS_PATH)/imgsign/public.pem -prv $(TOOLS_PATH)/imgsign/private.pem  -o $(IMAGE_OUTDIR)/$(ATF_BL31_SIGNED_BIN) -cap 0x54FAFE -key_bit 2048
endif
	$(CP) $(IMAGE_OUTDIR)/$(ATF_BL31_SIGNED_BIN) $(IMAGE_OUTDIR)/$(ATF_BL31_SIGNED_BIN_BK)
endif
	@imgsize=$$(stat -c %s $(IMAGE_OUTDIR)/$(ATF_BL31_SIGNED_BIN));\
	if [ $$imgsize -gt $(atf_img_size) ]; then \
		echo "Error: $(ATF_BL31_SIGNED_BIN) file size($$imgsize bytes) exceeds $(strip $(atf_img_size)) bytes"; \
		exit 1; \
	fi

clean:
	@$(ECHO) -e $(GREEN) "atf_bl31 $@..." $(DONE)
	@$(MAKE) -C $(ATF_DIR)/ realclean
	@rm -rf $(IMAGE_OUTDIR)/debug/$(ATF)_$(ATF_BL31_ELF)
	@rm -rf $(IMAGE_OUTDIR)/$(ATF_BL31_IMG)
	@rm -rf $(IMAGE_OUTDIR)/$(ATF_BL31_SIGNED_BIN)
