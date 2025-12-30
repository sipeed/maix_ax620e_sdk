HOME_PATH    := $(abspath $(shell pwd)/../../../)
BUILD_PATH   := $(HOME_PATH)/build
BUSYBOX_PATH := $(shell pwd)/busybox

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

OUT_DIR           := $(BUILD_PATH)/out/$(PROJECT)/objs
IMAGE_OUTDIR      := $(BUILD_PATH)/out/$(PROJECT)/images
TOOLS_PATH        := $(HOME_PATH)/tools
GEN_EXT2FS        := $(TOOLS_PATH)/genext2fs/genext2fs
MKFS_JFFS2        := $(TOOLS_PATH)/mkjffs2fs/mkfs.jffs2
MKFS_UBIFS        := $(TOOLS_PATH)/mkubifs/mkfs.ubifs
MK_UBINIZE        := $(TOOLS_PATH)/mkubifs/ubinize
COPY_FILE         := $(BUILD_PATH)/tools/copy_files.py
MODEL_PAC         := $(TOOLS_PATH)/mkaxp/model_pac.py
MAKE_SQUASHFS     := $(TOOLS_PATH)/mksquashfs/mksquashfs
AX_GZIP           := $(TOOLS_PATH)/ax_gzip_tool/ax_gzip
PROJECT_MAK       := $(BUILD_PATH)/projects/$(PROJECT)/project.mak

ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
CHIP_NAME=$(PROJECT)
CHIP_GROUP := $(shell echo $(CHIP_NAME) | cut -c1-6)
VERSION_EXT=$(SDK_VERSION)_$(shell date "+%Y%m%d%H%M%S")
endif

#rootfs
SRC_ROOTFS_DIR    := $(HOME_PATH)
DST_ROOTFS_DIR    := $(OUT_DIR)/rootfs
ROOTFS_EXT2_IMG   := $(IMAGE_OUTDIR)/rootfs.ext2
ROOTFS_IMG        := $(IMAGE_OUTDIR)/rootfs.img
rootfs_img_size   := $(call KM2Bytes,$(ROOTFS_PARTITION_SIZE))
MODIFY_FSTAB_FILE := $(BUILD_PATH)/tools/modify_fstab.sh
OUT_FSTAB_FILE    := $(DST_ROOTFS_DIR)/etc/fstab

#opt
SRC_OPT_DIR       := $(HOME_PATH)
DST_OPT_DIR       := $(DST_ROOTFS_DIR)/opt

#soc
SRC_SOC_DIR       := $(HOME_PATH)
SCRIPTS_DIR       := $(SRC_ROOTFS_DIR)/opt/scripts
DST_SOC_DIR       := $(DST_ROOTFS_DIR)/soc

#cust
SRC_CUST_DIR        := $(HOME_PATH)
DST_CUST_DIR        := $(OUT_DIR)/customer
CUSTOMER_UBI_IMG    := customer.ubifs
CUSTOMER_IMG        := $(IMAGE_OUTDIR)/customer.ubi
customer_img_size   := $(call KM2Bytes,$(CUSTOMER_PARTITION_SIZE))

#models
ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
SRC_MODEL_DIR      := $(HOME_PATH)
DST_MODEL_DIR      := $(OUT_DIR)/model
MODEL_IMG          := $(IMAGE_OUTDIR)/model.img
model_img_size     := $(call KM2Bytes,$(MODEL_PARTITION_SIZE))
MODEL_IMG_NAME     := model.img
MODEL_IMG_PATH     := $(OUT_DIR)/$(MODEL_IMG_NAME)
endif

#optee
LIB_OPTEE_PATH    := $(DST_ROOTFS_DIR)/lib/optee_armtz

#filelist
ROOTFS_FILELIST  := rootfs.filelist
SOC_FILELIST     := soc.filelist
OPT_FILELIST     := opt.filelist
MODEL_FILELIST   := model.filelist
CUSTOMER_FILELIST:= customer.filelist

ifeq ($(debug), yes)
COPY_DEBUG        := $(BUILD_PATH)/tools/copy_debug.py
DEBUG_SO_PATH     := $(MSP_OUT_PATH)/lib
DEBUG_SO_TYPE     := .debug
DST_SO_PATH       := $(MSP_OUT_PATH)/lib
DST_SO_TYPE       := .so
DEBUG_KO_PATH     := $(OSDRV_OUT_PATH)/debug_ko
DEBUG_KO_TYPE     := .ko
DST_KO_PATH       := $(OSDRV_OUT_PATH)/ko
DST_KO_TYPE       := .ko
endif

IMGSIGN_SCRIPT  := $(BUILD_PATH)/tools/imgsign
SIGN_SCRIPT     := $(IMGSIGN_SCRIPT)/sec_boot_AX620E_sign.py
ifeq ($(strip $(SIGN_USE_RSA3072)),TRUE)
PUB_KEY         := $(TOOLS_PATH)/imgsign/key_3072/pubkey.pem
PRIV_KEY        := $(TOOLS_PATH)/imgsign/key_3072/private.pem
SIGN_PARAMS     := -cap 0x54FEFE -key_bit 3072
else
PUB_KEY         := $(TOOLS_PATH)/imgsign/public.pem
PRIV_KEY        := $(TOOLS_PATH)/imgsign/private.pem
SIGN_PARAMS     := -cap 0x54FAFE -key_bit 2048
endif

.PHONY: prepare all install rootfs image rootfs_image customer_image model_image

default: all
all: prepare rootfs
	@$(ECHO) -e $(GREEN)"\nBuild Rootfs success!!\n" $(DONE)

prepare:
	@$(ECHO) -e $(GREEN) "Making $@..." $(DONE)
	@$(MKDIR) -p $(OUT_DIR)

rootfs:
	@$(ECHO) -e $(GREEN) "Making $@..." $(DONE)
	@$(MKDIR) -p $(DST_ROOTFS_DIR)
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/proc
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/sys
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/mnt
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/tmp
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/run
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/root
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/media
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/param
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/var/www
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/customer
	@$(MKDIR) -p $(DST_CUST_DIR)
ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
	@$(MKDIR) -p $(DST_MODEL_DIR)
endif
	@$(MKDIR) -p $(IMAGE_OUTDIR)

install:
	@$(ECHO) -e $(GREEN) "rootfs $@..." $(DONE)

image: rootfs_image customer_image model_image
	@$(ECHO) -e $(GREEN) "make $@..." $(DONE)

rootfs_image:
	@$(ECHO) -e $(GREEN) "making rootfs image ..." $(DONE)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_ROOTFS_DIR) $(ROOTFS_FILELIST) $(DST_ROOTFS_DIR)
ifeq ($(strip $(DEBUG_BOOT_TIME)),TRUE)
	@sed -i "s#DEBUG_BOOT_TIME=\"false\"#DEBUG_BOOT_TIME=\"true\"#g" $(DST_ROOTFS_DIR)/etc/init.d/rcS
endif
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/tmp/cron.d
	@rm -f $(DST_ROOTFS_DIR)/etc/cron.d
	@ln -s ../tmp/cron.d $(DST_ROOTFS_DIR)/etc/cron.d
	@rm -rf $(DST_ROOTFS_DIR)/etc/ax_syslog.conf
	@ln -s /var/log/AXSyslog/ax_syslog.conf $(DST_ROOTFS_DIR)/etc/ax_syslog.conf
	@rm -rf $(DST_ROOTFS_DIR)/etc/syslog.conf
	@ln -s /var/log/AXSyslog/syslog.conf $(DST_ROOTFS_DIR)/etc/syslog.conf
	@sed -i '/\/proc\/sys\/kernel\/core_pattern/d' $(DST_ROOTFS_DIR)/etc/profile
	@cp -rf rootfs/etc/fstab $(DST_ROOTFS_DIR)/etc/fstab
	@chmod 755 $(MODIFY_FSTAB_FILE)
	@bash $(MODIFY_FSTAB_FILE) $(OUT_FSTAB_FILE) "$(FLASH_PARTITIONS)" "/dev/root" $(CONFIG_FS_TYPE)

#soc
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_SOC_DIR) $(SOC_FILELIST) $(DST_SOC_DIR)
	@chmod 755 $(DST_SOC_DIR)/scripts/*.sh
#opt
	@$(ECHO) -e $(GREEN) "making $@ opt..." $(DONE)
	@mkdir -p $(DST_OPT_DIR)/etc/models/aiisp
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_OPT_DIR) $(OPT_FILELIST) $(DST_OPT_DIR)
	@for element in $(SENSOR_TYPE); do \
		if [ ! -d $(CUR_DIR)/sensors/$$element ]; then \
			echo "Error: Unsupported sensor: $$element"; \
			exit 1; \
		fi; \
		python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_OPT_DIR) $(CUR_DIR)/sensors/$$element/$(MODEL_FILELIST) $(DST_OPT_DIR); \
	done
ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
	@rm -rf $(DST_OPT_DIR)/etc/models
endif
	@mkdir -p $(DST_ROOTFS_DIR)/opt/data
	@rm -rf $(DST_ROOTFS_DIR)/opt/data/AXSyslog
	@ln -s /var/log/AXSyslog $(DST_ROOTFS_DIR)/opt/data/AXSyslog
#image
	$(GEN_EXT2FS) -b $(shell printf "0x%X" $$(($(RAMDISK_SIZE_MB)*1024))) -d $(DST_ROOTFS_DIR) $(ROOTFS_EXT2_IMG)
	@$(AX_GZIP) -9 $(ROOTFS_EXT2_IMG)
	@python3 $(SIGN_SCRIPT) -i $(ROOTFS_EXT2_IMG).axgzip -pub $(PUB_KEY) -prv $(PRIV_KEY)  -o $(ROOTFS_IMG) $(SIGN_PARAMS)
	@imgsize=$$(stat -c %s $(ROOTFS_IMG));\
	if [ $$imgsize -gt $(rootfs_img_size) ]; then \
		echo "Error: $(ROOTFS_IMG) file size($$imgsize bytes) exceeds $(strip $(rootfs_img_size)) bytes"; \
		exit 1; \
	fi

customer_image:
	@rm -rf $(CUSTOMER_IMG)
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_CUST_DIR) $(CUSTOMER_FILELIST) $(DST_CUST_DIR)
	$(eval pad_size = $(call KM2Bytes, $(CUSTOMER_PARTITION_SIZE)))
	@$(MKFS_UBIFS) -F -q -r $(DST_CUST_DIR) -m 2048 -e 126976 -c 2047 -j 1048576 -o $(CUSTOMER_UBI_IMG)
	@$(MK_UBINIZE) -o $(CUSTOMER_IMG) -m 2048 -p 128KiB ubi/ubinize_customer.cfg
	@imgsize=$$(stat -c %s $(CUSTOMER_IMG));\
	if [ $$imgsize -gt $(customer_img_size) ]; then \
		echo "Error: $(CUSTOMER_IMG) file size($$imgsize bytes) exceeds $(strip $(customer_img_size)) bytes"; \
		exit 1; \
	fi

model_image:
ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
	@rm -rf $(MODEL_IMG) $(MODEL_IMG_PATH)
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
	$(eval pad_size = $(call KM2Bytes, $(MODEL_PARTITION_SIZE)))
	@for element in $(SENSOR_TYPE); do \
		if [ ! -d $(CUR_DIR)/sensors/$$element ]; then \
			echo "Error: Unsupported sensor: $$element"; \
			exit 1; \
		fi; \
		python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_MODEL_DIR) $(CUR_DIR)/sensors/$$element/$(MODEL_FILELIST) $(DST_MODEL_DIR); \
	done
	@cp -rf $(DST_MODEL_DIR)/etc/models/aiisp/* $(DST_MODEL_DIR)
	@rm -rf $(DST_MODEL_DIR)/etc
	@python3 $(MODEL_PAC) -p$(CHIP_GROUP) -v$(VERSION_EXT) -o$(MODEL_IMG_PATH) $(DST_MODEL_DIR)
	@$(AX_GZIP) -9 $(MODEL_IMG_PATH)
	@python3 $(SIGN_SCRIPT) -i $(MODEL_IMG_PATH).axgzip -pub $(PUB_KEY) -prv $(PRIV_KEY)  -o $(MODEL_IMG) $(SIGN_PARAMS)
	@imgsize=$$(stat -c %s $(MODEL_IMG));\
	if [ $$imgsize -gt $(model_img_size) ]; then \
		echo "Error: $(MODEL_IMG) file size($$imgsize bytes) exceeds $(strip $(model_img_size)) bytes"; \
		exit 1; \
	fi
else
	@$(ECHO) -e $(GREEN) "making $@ not support!!" $(DONE)
endif

ifeq ($(SUPPORT_RECOVERY), TRUE)
	@bash ./recovery/make_recovery_pkg.sh $(PROJECT)
endif

clean:
	@$(ECHO) -e $(GREEN) "rootfs $@..." $(DONE)
	@rm -rf $(DST_SOC_DIR)
	@rm -rf $(DST_OPT_DIR)
	@rm -rf $(DST_ROOTFS_DIR)
	@rm -rf $(DST_MODEL_DIR)
	@rm -rf $(DST_CUST_DIR)
