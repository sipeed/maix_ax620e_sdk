HOME_PATH    := $(abspath $(shell pwd)/../../../)
BUILD_PATH   := $(HOME_PATH)/build
BUSYBOX_PATH := $(shell pwd)/busybox

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

OUT_DIR           := $(BUILD_PATH)/out/$(PROJECT)/objs
IMAGE_OUTDIR      := $(BUILD_PATH)/out/$(PROJECT)/images
TOOLS_PATH        := $(HOME_PATH)/tools
MAKE_EXT4FS       := $(TOOLS_PATH)/mkext4fs/make_ext4fs
COPY_FILE         := $(BUILD_PATH)/tools/copy_files.py
PROJECT_MAK       := $(BUILD_PATH)/projects/$(PROJECT)/project.mak

#rootfs
SRC_ROOTFS_DIR    := $(HOME_PATH)
DST_ROOTFS_DIR    := $(OUT_DIR)/rootfs
MODIFY_FSTAB_FILE := $(BUILD_PATH)/tools/modify_fstab.sh
WAIT_FLASH_FILE   := $(BUILD_PATH)/tools/wait_flash.sh
OUT_FSTAB_FILE    := $(DST_ROOTFS_DIR)/etc/fstab
OUT_RCS_FILE      := $(DST_ROOTFS_DIR)/etc/init.d/rcS

#soc
SRC_SOC_DIR       := $(HOME_PATH)
DST_SOC_DIR       := $(OUT_DIR)/soc
SOC_IMG           := $(IMAGE_OUTDIR)/soc.ext4
SOC_SPARSE_IMG    := $(IMAGE_OUTDIR)/soc_sparse.ext4
soc_img_size      := $(call KM2Bytes,$(SOC_PARTITION_SIZE))

#customer
SRC_CUSTOMER_DIR     := $(HOME_PATH)
DST_CUSTOMER_DIR     := $(OUT_DIR)/customer
CUSTOMER_IMG         := $(IMAGE_OUTDIR)/customer.ext4
CUSTOMER_SPARSE_IMG  := $(IMAGE_OUTDIR)/customer_sparse.ext4
customer_img_size    := $(call KM2Bytes,$(CUSTOMER_PARTITION_SIZE))

#param
SRC_PARAM_DIR        := $(HOME_PATH)
DST_PARAM_DIR        := $(OUT_DIR)/param
PARAM_IMG            := $(IMAGE_OUTDIR)/param.ext4
PARAM_SPARSE_IMG     := $(IMAGE_OUTDIR)/param_sparse.ext4
param_img_size       := $(call KM2Bytes,$(PARAM_PARTITION_SIZE))

#filelist
ROOTFS_FILELIST      := rootfs.filelist
SOC_FILELIST         := soc.filelist
CUSTOMER_FILELIST    := customer.filelist
MODEL_FILELIST       := model.filelist
PARAM_FILELIST       := param.filelist

CROSS_PATH         := $(abspath $(dir $(shell whereis $(CC)|awk '{print $$2}'))/..)
CROSS_NAME         := $(shell echo $(CROSS) | sed -e 's/-$$//g')
CROSS_LIBC_PATH    := $(CROSS_PATH)/$(CROSS_NAME)/libc/lib64
CROSS_ASANLIB_PATH := $(CROSS_PATH)/$(CROSS_NAME)/lib64

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

.PHONY: prepare all install rootfs images rootfs_image soc_image customer_image param_image

default: all

SUPPORT_VALGRIND := no
ifeq ($(valgrind), yes)
VALGRIND_PATH := $(HOME_PATH)/tools/valgrind/bin/$(CHIP_NAME)/valgrind
endif
ifneq ($(wildcard $(VALGRIND_PATH)),)
SUPPORT_VALGRIND := yes
endif

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
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/soc
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/customer
	@$(MKDIR) $(DST_ROOTFS_DIR)/var/www
	@$(MKDIR) $(IMAGE_OUTDIR)
ifeq ($(SUPPORT_CPIO),TRUE)
	@cd $(DST_ROOTFS_DIR) && find . | cpio -o -H newc > $(IMAGE_OUTDIR)/rootfs.cpio
endif

install:
	@$(ECHO) -e $(GREEN) "rootfs $@..." $(DONE)
#param
	@$(MKDIR) -p $(DST_PARAM_DIR)


image: rootfs_image soc_image customer_image param_image
	@$(ECHO) -e $(GREEN) "images $@..." $(DONE)

rootfs_image:
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_ROOTFS_DIR) $(ROOTFS_FILELIST) $(DST_ROOTFS_DIR)
	@for element in $(SENSOR_TYPE); do \
		if [ ! -d $(CUR_DIR)/sensors/$$element ]; then \
			echo "Error: Unsupported sensor: $$element"; \
			exit 1; \
		fi; \
		python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_ROOTFS_DIR) $(CUR_DIR)/sensors/$$element/$(MODEL_FILELIST) $(DST_ROOTFS_DIR)/opt; \
	done
	@chmod 755 $(MODIFY_FSTAB_FILE)
	@bash $(MODIFY_FSTAB_FILE) $(OUT_FSTAB_FILE) "$(FLASH_PARTITIONS)"
	@bash $(WAIT_FLASH_FILE) $(OUT_FSTAB_FILE) $(OUT_RCS_FILE)
ifeq ($(debug), yes)
	@cp -rf $(CROSS_LIBC_PATH)/libc-2.*.so $(DST_ROOTFS_DIR)/lib
	@cp -rf $(SRC_ROOTFS_DIR)/rootfs/$(ARCH)/$(LIBC)/rootfs/lib/libstdc++.so.6* $(DST_ROOTFS_DIR)/lib
endif
ifeq ($(SUPPORT_CPIO),TRUE)
	@cd $(DST_ROOTFS_DIR) && find . | cpio -o -H newc > $(IMAGE_OUTDIR)/rootfs.cpio
endif

soc_image:
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
ifeq ($(debug), yes)
	#@python3 $(COPY_DEBUG) $(DEBUG_KO_PATH) $(DEBUG_KO_TYPE) $(DST_KO_PATH) $(DST_KO_TYPE)
endif
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_SOC_DIR) $(SOC_FILELIST) $(DST_SOC_DIR)
ifneq ($(strip $(CMM_POOL_PARAM)),)
	@sed -i "s/cmmpool*=.*/cmmpool=$(CMM_POOL_PARAM)/g" $(DST_SOC_DIR)/scripts/auto_load_all_drv.sh
endif
	@find $(OUT_DIR)/kernel/linux/$(KERNEL_DIR) -name "*\.ko"|xargs cp -t $(DST_SOC_DIR)/ko
	@$(MAKE_EXT4FS) -l $(SOC_PARTITION_SIZE) $(SOC_IMG) $(DST_SOC_DIR)
	@$(MAKE_EXT4FS) -l $(SOC_PARTITION_SIZE) -s $(SOC_SPARSE_IMG) $(DST_SOC_DIR)
	@imgsize=$$(stat -c %s $(SOC_SPARSE_IMG));\
	if [ $$imgsize -gt $(soc_img_size) ]; then \
		echo "Error: $(SOC_SPARSE_IMG) file size($$imgsize bytes) exceeds $(strip $(soc_img_size)) bytes"; \
		exit 1; \
	fi

customer_image:
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
ifeq ($(debug), yes)
	#@python3 $(COPY_DEBUG) $(DEBUG_SO_PATH) $(DEBUG_SO_TYPE) $(DST_SO_PATH) $(DST_SO_TYPE)
endif
ifeq ($(asan), yes)
	@mkdir -p $(DST_CUSTOMER_DIR)/lib
	@cp -rf $(CROSS_ASANLIB_PATH)/libasan*.so* $(DST_CUSTOMER_DIR)/lib
endif
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_CUSTOMER_DIR) $(CUSTOMER_FILELIST) $(DST_CUSTOMER_DIR)

ifeq ($(SUPPORT_VALGRIND), yes)
	@cp -rf $(VALGRIND_PATH)/bin/* $(DST_CUSTOMER_DIR)/bin
	@cp -rf $(VALGRIND_PATH)/libexec $(DST_CUSTOMER_DIR)/
endif
	@$(MAKE_EXT4FS) -l $(CUSTOMER_PARTITION_SIZE) $(CUSTOMER_IMG) $(DST_CUSTOMER_DIR)
	@$(MAKE_EXT4FS) -l $(CUSTOMER_PARTITION_SIZE) -s $(CUSTOMER_SPARSE_IMG) $(DST_CUSTOMER_DIR)
	@imgsize=$$(stat -c %s $(CUSTOMER_SPARSE_IMG));\
	if [ $$imgsize -gt $(customer_img_size) ]; then \
		echo "Error: $(CUSTOMER_SPARSE_IMG) file size($$imgsize bytes) exceeds $(strip $(customer_img_size)) bytes"; \
		exit 1; \
	fi
ifeq ($(AX_SUPPORT_AB_PART),TRUE)
	@cp rootfs/etc/init.d/S99checkboot $(DST_ROOTFS_DIR)/etc/init.d/
endif
param_image:
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_PARAM_DIR) $(PARAM_FILELIST) $(DST_PARAM_DIR)

	@$(MAKE_EXT4FS) -l $(PARAM_PARTITION_SIZE) $(PARAM_IMG) $(DST_PARAM_DIR)
	@$(MAKE_EXT4FS) -l $(PARAM_PARTITION_SIZE) -s $(PARAM_SPARSE_IMG) $(DST_PARAM_DIR)
	@imgsize=$$(stat -c %s $(PARAM_SPARSE_IMG));\
	if [ $$imgsize -gt $(param_img_size) ]; then \
		echo "Error: $(PARAM_SPARSE_IMG) file size($$imgsize bytes) exceeds $(strip $(param_img_size)) bytes"; \
		exit 1; \
	fi

clean:
	@$(ECHO) -e $(GREEN) "rootfs $@..." $(DONE)
	@rm -rf $(DST_ROOTFS_DIR)
	@rm -rf $(DST_SOC_DIR)
	@rm -rf $(DST_CUSTOMER_DIR)
	@rm -rf $(DST_CUST_DIR)
