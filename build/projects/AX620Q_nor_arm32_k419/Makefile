HOME_PATH    := $(abspath $(shell pwd)/../../../)
BUILD_PATH   := $(HOME_PATH)/build
BUSYBOX_PATH := $(shell pwd)/busybox

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

OUT_DIR           := $(BUILD_PATH)/out/$(PROJECT)/objs
IMAGE_OUTDIR      := $(BUILD_PATH)/out/$(PROJECT)/images
TOOLS_PATH        := $(HOME_PATH)/tools
MAKE_EXT4FS       := $(TOOLS_PATH)/mkext4fs/make_ext4fs
MKFS_JFFS2        := $(TOOLS_PATH)/mkjffs2fs/mkfs.jffs2
COPY_FILE         := $(BUILD_PATH)/tools/copy_files.py
MAKE_SQUASHFS     := $(TOOLS_PATH)/mksquashfs/mksquashfs
PROJECT_MAK       := $(BUILD_PATH)/projects/$(PROJECT)/project.mak

#rootfs
SRC_ROOTFS_DIR    := $(HOME_PATH)
DST_ROOTFS_DIR    := $(OUT_DIR)/rootfs
ROOTFS_IMG        := $(IMAGE_OUTDIR)/rootfs.img
rootfs_img_size   := $(call KM2Bytes,$(ROOTFS_PARTITION_SIZE))
MODIFY_FSTAB_FILE := $(BUILD_PATH)/tools/modify_fstab.sh
OUT_FSTAB_FILE    := $(DST_ROOTFS_DIR)/etc/fstab

#soc
SRC_SOC_DIR       := $(HOME_PATH)
SCRIPTS_DIR       := $(SRC_ROOTFS_DIR)/rootfs/opt/scripts
DST_SOC_DIR       := $(DST_ROOTFS_DIR)/soc

#opt
SRC_OPT_DIR       := $(HOME_PATH)
DST_OPT_DIR       := $(OUT_DIR)/opt
OPT_IMG           := $(IMAGE_OUTDIR)/opt.img
opt_img_size      := $(call KM2Bytes,$(OPT_PARTITION_SIZE))

#customer
SRC_CUSTOMER_DIR    := $(HOME_PATH)
DST_CUSTOMER_DIR    := $(OUT_DIR)/customer
CUSTOMER_IMG        := $(IMAGE_OUTDIR)/customer.img
customer_img_size   := $(call KM2Bytes,$(CUSTOMER_PARTITION_SIZE))

#optee
LIB_OPTEE_PATH    := $(DST_ROOTFS_DIR)/lib/optee_armtz

#filelist
ROOTFS_FILELIST  := rootfs_$(LIBC).filelist
SOC_FILELIST     := soc.filelist
OPT_FILELIST     := opt.filelist

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

.PHONY: prepare all install rootfs image rootfs_image check

default: all
all: prepare rootfs
	@$(ECHO) -e $(GREEN)"\nBuild Rootfs success!!\n" $(DONE)

prepare:
	@$(ECHO) -e $(GREEN) "Making $@..." $(DONE)
	@$(MKDIR) -p $(OUT_DIR)

rootfs:
	@$(ECHO) -e $(GREEN) "Making $@..." $(DONE)
	@$(MKDIR) -p $(DST_ROOTFS_DIR)
	@$(MKDIR) -p $(DST_OPT_DIR)
	@$(MKDIR) -p $(DST_CUSTOMER_DIR)
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/proc
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/sys
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/mnt/tfcard
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/tmp
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/run
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/root
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/media
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/param
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/customer
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/opt
	@$(MKDIR) $(DST_ROOTFS_DIR)/var/www
	@$(MKDIR) $(IMAGE_OUTDIR)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK)  $(SRC_ROOTFS_DIR) $(ROOTFS_FILELIST) $(DST_ROOTFS_DIR)
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/tmp/cron.d
	@rm -f $(DST_ROOTFS_DIR)/etc/cron.d
	@ln -s ../tmp/cron.d $(DST_ROOTFS_DIR)/etc/cron.d
	@rm -rf $(DST_ROOTFS_DIR)/etc/ax_syslog.conf
	@ln -s /var/log/AXSyslog/ax_syslog.conf $(DST_ROOTFS_DIR)/etc/ax_syslog.conf
	@rm -rf $(DST_ROOTFS_DIR)/etc/syslog.conf
	@ln -s /var/log/AXSyslog/syslog.conf $(DST_ROOTFS_DIR)/etc/syslog.conf
	@sed -i '/\/proc\/sys\/kernel\/core_pattern/d' $(DST_ROOTFS_DIR)/etc/profile
ifneq ($(findstring yes, $(debugkconfig) $(debug)),)
	echo "/soc/scripts/kmemleak_scan.sh &" >> $(DST_ROOTFS_DIR)/etc/init.d/rcS
endif
	@chmod 755 $(MODIFY_FSTAB_FILE)
	@bash $(MODIFY_FSTAB_FILE) $(OUT_FSTAB_FILE) "$(FLASH_PARTITIONS)" "/dev/root" $(CONFIG_FS_TYPE) "/customer" $(CONFIG_CUSTOMER_FS_TYPE) "/opt" $(CONFIG_OPT_FS_TYPE)

install:
	@$(ECHO) -e $(GREEN) "rootfs $@..." $(DONE)
#optee
ifeq ($(strip $(SUPPORT_OPTEE)),TRUE)
	@$(MKDIR) -p $(LIB_OPTEE_PATH)
	@cp -rf $(MSP_OUT_PATH)/ta/* $(LIB_OPTEE_PATH)
endif

image: rootfs_image opt_image customer_image
	@$(ECHO) -e $(GREEN) "make $@..." $(DONE)

rootfs_image:
	@$(ECHO) -e $(GREEN) "making rootfs image ..." $(DONE)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK)  $(SRC_SOC_DIR) $(SOC_FILELIST) $(DST_SOC_DIR)
	@chmod 755 $(DST_SOC_DIR)/scripts/*.sh
ifneq ($(strip $(CMM_POOL_PARAM)),)
	@sed -i "s/cmmpool*=.*/cmmpool=$(CMM_POOL_PARAM)/g" $(DST_SOC_DIR)/scripts/auto_load_all_drv.sh
endif
ifeq ($(debug), yes)
	@python3 $(COPY_DEBUG) $(DEBUG_SO_PATH) $(DEBUG_SO_TYPE) $(DST_SO_PATH) $(DST_SO_TYPE)
	@python3 $(COPY_DEBUG) $(DEBUG_KO_PATH) $(DEBUG_KO_TYPE) $(DST_KO_PATH) $(DST_KO_TYPE)
endif
ifeq ($(strip $(CONFIG_FS_TYPE)),squashfs)
	@rm -rf $(ROOTFS_IMG)
	@$(MAKE_SQUASHFS) $(DST_ROOTFS_DIR) $(ROOTFS_IMG) -b 512K -comp xz
else
	$(eval pad_size = $(call KM2Bytes, $(ROOTFS_PARTITION_SIZE)))
	@$(MKFS_JFFS2) -r $(DST_ROOTFS_DIR) -o $(ROOTFS_IMG) --pad=$(pad_size) -n –l
endif
ifeq ($(SUPPORT_CPIO),TRUE)
	@cd $(DST_ROOTFS_DIR) && find . | cpio -o -H newc > $(IMAGE_OUTDIR)/rootfs.cpio
endif
	@imgsize=$$(stat -c %s $(ROOTFS_IMG));\
	if [ $$imgsize -gt $(rootfs_img_size) ]; then \
		echo "Error: $(ROOTFS_IMG) file size($$imgsize bytes) exceeds $(strip $(rootfs_img_size)) bytes"; \
		exit 1; \
	fi

opt_image:
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
ifeq ($(asan), yes)
	@mkdir -p $(DST_OPT_DIR)/lib
	@cp -rf $(CROSS_LIB_PATH)/libasan*.so* $(DST_OPT_DIR)/lib
endif
	@mkdir -p $(DST_OPT_DIR)/etc/models/aiisp
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK)  $(SRC_OPT_DIR) $(OPT_FILELIST) $(DST_OPT_DIR)
	@for element in $(SENSOR_TYPE); do \
		if [ ! -d $(CUR_DIR)/sensors/$$element ]; then \
			echo "Error: Unsupported sensor: $$element"; \
			exit 1; \
		fi; \
		python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK)  $(SRC_OPT_DIR) $(CUR_DIR)/sensors/$$element/so.filelist $(DST_OPT_DIR); \
		python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK)  $(SRC_OPT_DIR) $(CUR_DIR)/sensors/$$element/model.filelist $(DST_OPT_DIR); \
	done
	@mkdir -p $(DST_OPT_DIR)/data
	@mkdir -p $(DST_OPT_DIR)/usr/bin
	@rm -rf $(DST_OPT_DIR)/data/AXSyslog
	@ln -s /var/log/AXSyslog $(DST_OPT_DIR)/data/AXSyslog
ifeq ($(SUPPORT_VALGRIND), yes)
	@cp -rf $(VALGRIND_PATH)/bin/* $(DST_OPT_DIR)/bin
	@cp -rf $(VALGRIND_PATH)/libexec $(DST_OPT_DIR)/
endif

ifeq ($(strip $(CONFIG_OPT_FS_TYPE)),squashfs)
	@rm -rf $(OPT_IMG)
	@$(MAKE_SQUASHFS) $(DST_OPT_DIR) $(OPT_IMG) -b 512K -comp xz
else
	$(eval pad_size = $(call KM2Bytes, $(OPT_PARTITION_SIZE)))
	@$(MKFS_JFFS2) -r $(DST_OPT_DIR) -o $(OPT_IMG) --pad=$(pad_size) -n –l
endif
	@imgsize=$$(stat -c %s $(OPT_IMG)); \
	echo "opt_image size: $$imgsize/$(opt_img_size) bytes"; \
	if [ $$imgsize -gt $(opt_img_size) ]; then \
		echo "Error: $(OPT_IMG) file size($$imgsize bytes) exceeds $(strip $(opt_img_size)) bytes"; \
		exit 1; \
	fi

customer_image:
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
ifeq ($(strip $(CONFIG_CUSTOMER_FS_TYPE)),squashfs)
	@rm -rf $(CUSTOMER_IMG)
	@$(MAKE_SQUASHFS) $(DST_CUSTOMER_DIR) $(CUSTOMER_IMG) -b 512K -comp xz
else
	@rm -rf $(CUSTOMER_IMG)
	$(eval pad_size = $(call KM2Bytes, $(CUSTOMER_PARTITION_SIZE)))
	@$(MKFS_JFFS2) -r $(DST_CUSTOMER_DIR) -o $(CUSTOMER_IMG) --pad=$(pad_size) -n –l
endif
	@imgsize=$$(stat -c %s $(CUSTOMER_IMG));\
	if [ $$imgsize -gt $(customer_img_size) ]; then \
		echo "Error: $(CUSTOMER_IMG) file size($$imgsize bytes) exceeds $(strip $(customer_img_size)) bytes"; \
		exit 1; \
	fi

clean:
	@$(ECHO) -e $(GREEN) "rootfs $@..." $(DONE)
	@rm -rf $(DST_SOC_DIR)
	@rm -rf $(DST_OPT_DIR)
	@rm -rf $(DST_CUSTOMER_DIR)
	@rm -rf $(DST_ROOTFS_DIR)
