HOME_PATH    := $(abspath $(shell pwd)/../../../)
BUILD_PATH   := $(HOME_PATH)/build
BUSYBOX_PATH := $(shell pwd)/busybox

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

OUT_DIR                  := $(BUILD_PATH)/out/$(PROJECT)/objs
IMAGE_OUTDIR             := $(BUILD_PATH)/out/$(PROJECT)/images
TOOLS_PATH               := $(HOME_PATH)/tools
MAKE_EXT4FS              := $(TOOLS_PATH)/mkext4fs/make_ext4fs
COPY_FILE                := $(BUILD_PATH)/tools/copy_files.py
PROJECT_MAK              := $(BUILD_PATH)/projects/$(PROJECT)/project.mak
COMM_SWUPDATE_PATH       := $(BUILD_PATH)/tools/swupdate

#boot
DST_BOOTFS_DIR           := $(OUT_DIR)/bootfs
BOOTFS_IMG               := $(IMAGE_OUTDIR)/bootfs.fat32

#rootfs
SRC_ROOTFS_DIR           := $(HOME_PATH)
DST_ROOTFS_DIR           := $(OUT_DIR)/rootfs
ROOTFS_IMG               := $(IMAGE_OUTDIR)/rootfs.ext4
ROOTFS_SPARSE_IMG        := $(IMAGE_OUTDIR)/rootfs_sparse.ext4
rootfs_img_size          := $(call KM2Bytes,$(ROOTFS_PARTITION_SIZE))
MODIFY_FSTAB_FILE        := $(BUILD_PATH)/tools/modify_fstab.sh
FSTAB_FILE               := $(DST_ROOTFS_DIR)/etc/fstab

#ubuntu_rootfs
SRC_UBUNTU_ROOTFS_DIR    := $(HOME_PATH)/rootfs/$(ARCH)/$(LIBC)/ubuntu_rootfs
DST_UBUNTU_ROOTFS_DIR    := $(OUT_DIR)/ubuntu_rootfs
UBUNTU_ROOTFS_IMG        := $(IMAGE_OUTDIR)/ubuntu_rootfs.ext4
UBUNTU_ROOTFS_SPARSE_IMG := $(IMAGE_OUTDIR)/ubuntu_rootfs_sparse.ext4
UBUNTU_ROOTFS_FSTAB_FILE := $(DST_UBUNTU_ROOTFS_DIR)/etc/fstab

ifeq ($(use_ubuntu_rootfs),)
	use_ubuntu_rootfs := yes
	use_rootfs := no
    $(info default use_ubuntu_rootfs for nanokvm)
endif
ifeq ($(use_ubuntu_rootfs),no)
	use_ubuntu_rootfs := no
	use_rootfs := yes
    $(info use_ubuntu_rootfs is not empty)
else
	use_ubuntu_rootfs := yes
	use_rootfs := no
    $(info use_ubuntu_rootfs for nanokvm)
endif

#soc
# SRC_SOC_DIR       := $(HOME_PATH)
# SCRIPTS_DIR       := $(HOME_PATH)/rootfs/rootfs/opt/scripts
# DST_SOC_DIR       := $(OUT_DIR)/soc
# SOC_IMG           := $(IMAGE_OUTDIR)/soc.ext4
# SOC_SPARSE_IMG    := $(IMAGE_OUTDIR)/soc_sparse.ext4
# soc_img_size      := $(call KM2Bytes,$(SOC_PARTITION_SIZE))

#opt
# SRC_OPT_DIR       := $(HOME_PATH)
# DST_OPT_DIR       := $(OUT_DIR)/opt
# OPT_IMG           := $(IMAGE_OUTDIR)/opt.ext4
# OPT_SPARSE_IMG    := $(IMAGE_OUTDIR)/opt_sparse.ext4
# opt_img_size      := $(call KM2Bytes,$(OPT_PARTITION_SIZE))

#optee
LIB_OPTEE_PATH    := lib/optee_armtz

#param
# DST_PARAM_DIR     := $(OUT_DIR)/param
# PARAM_IMG         := $(IMAGE_OUTDIR)/param.ext4
# PARAM_SPARSE_IMG  := $(IMAGE_OUTDIR)/param_sparse.ext4
# param_img_size    := $(call KM2Bytes,$(PARAM_PARTITION_SIZE))

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

.PHONY: prepare all install rootfs images rootfs_image bootfs bootfs_image

default: all

SUPPORT_VALGRIND := no
ifeq ($(valgrind), yes)
VALGRIND_PATH := $(HOME_PATH)/tools/valgrind/bin/$(CHIP_NAME)/valgrind
endif
ifneq ($(wildcard $(VALGRIND_PATH)),)
SUPPORT_VALGRIND := yes
endif

all: prepare rootfs ubuntu_rootfs bootfs
	@$(ECHO) -e $(GREEN)"\nBuild Rootfs success!!\n" $(DONE)

prepare:
	@$(ECHO) -e $(GREEN) "Making $@..." $(DONE)
	@$(MKDIR) -p $(OUT_DIR)
	@$(ECHO) -e $(GREEN) "gen_initramfs.sh" $(DONE)
	./gen_initramfs.sh

bootfs:
	@$(ECHO) -e $(GREEN) "Making $@..." $(DONE)
	@$(MKDIR) -p $(DST_BOOTFS_DIR)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(HOME_PATH) bootfs.filelist $(DST_BOOTFS_DIR)

rootfs:
ifeq ($(strip $(use_rootfs)),yes)
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
	@$(MKDIR) $(DST_ROOTFS_DIR)/var/www
	@$(MKDIR) $(IMAGE_OUTDIR)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(HOME_PATH) rootfs.filelist $(DST_ROOTFS_DIR)
ifeq ($(debug), yes)
	@cp -rf $(CROSS_LIBC_PATH)/libc-2.*.so $(DST_ROOTFS_DIR)/lib
endif

ifeq ($(AX_SUPPORT_AB_PART),TRUE)
	@cp -rf rootfs/etc/init.d/* $(DST_ROOTFS_DIR)/etc/init.d/
endif
ifeq ($(ENABLE_SWUPDATE),TRUE)
	@cp -rf swupdate/hwrevision $(DST_ROOTFS_DIR)/etc/hwrevision
	@cp -rf $(COMM_SWUPDATE_PATH)/mkswu/swupdate.cfg $(DST_ROOTFS_DIR)/etc/swupdate.cfg
endif
	@chmod 755 $(MODIFY_FSTAB_FILE)
	@bash $(MODIFY_FSTAB_FILE) $(FSTAB_FILE) "$(FLASH_PARTITIONS)"

ifeq ($(SUPPORT_CPIO),TRUE)
	@cd $(DST_ROOTFS_DIR) && find . | cpio -o -H newc > $(IMAGE_OUTDIR)/rootfs.cpio
endif
endif

ubuntu_rootfs:
ifeq ($(strip $(use_ubuntu_rootfs)),yes)
	@$(ECHO) -e $(GREEN) "Making $@..." $(DONE)
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)
ifeq ($(wildcard $(SRC_UBUNTU_ROOTFS_DIR)/ubuntu_rootfs_base.tar.gz),)
	@echo "file ubuntu_rootfs_base.tar.gz does not exist, don't do ubuntu rootfs operation"
else
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/proc
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/sys
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/mnt
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/tmp
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/run
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/root
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/media
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/param
	@$(MKDIR) $(DST_UBUNTU_ROOTFS_DIR)/var/www
	@$(MKDIR) $(DST_UBUNTU_ROOTFS_DIR)/etc
	@$(MKDIR) $(IMAGE_OUTDIR)
ifeq ($(AX_SUPPORT_AB_PART),TRUE)
	mkdir -p $(DST_UBUNTU_ROOTFS_DIR)/etc/init.d/
	@cp -rf rootfs/etc/init.d/* $(DST_UBUNTU_ROOTFS_DIR)/etc/init.d/
endif
	@bash mk_ubuntu_rootfs_sipeed.sh $(DST_UBUNTU_ROOTFS_DIR) $(SRC_UBUNTU_ROOTFS_DIR)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_ROOTFS_DIR) ubuntu_rootfs.filelist $(DST_UBUNTU_ROOTFS_DIR)

ifeq ($(ENABLE_SWUPDATE),TRUE)
	@cp -rf swupdate/hwrevision $(DST_UBUNTU_ROOTFS_DIR)/etc/hwrevision
	@cp -rf $(COMM_SWUPDATE_PATH)/mkswu/swupdate.cfg $(DST_UBUNTU_ROOTFS_DIR)/etc/swupdate.cfg
endif
	@chmod 755 $(MODIFY_FSTAB_FILE)
	@bash $(MODIFY_FSTAB_FILE) $(UBUNTU_ROOTFS_FSTAB_FILE) "$(FLASH_PARTITIONS)"
endif
endif

install:
	@$(ECHO) -e $(GREEN) "rootfs $@..." $(DONE)

image: rootfs_image ubuntu_rootfs_image bootfs_image
	@$(ECHO) -e $(GREEN) "images maked" $(DONE)

bootfs_image:
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
	@partsize_m=$$(echo $(BOOT_PARTITION_SIZE) | sed 's/M//g');\
	dd if=/dev/zero of=$(BOOTFS_IMG) bs=1M count=$$partsize_m
	mkfs.vfat -F 32 -n BOOT $(BOOTFS_IMG)
	mkdir -p /tmp/boot_mount
	fusefat -o rw+ $(BOOTFS_IMG) /tmp/boot_mount
	cp -rf $(DST_BOOTFS_DIR)/* /tmp/boot_mount/
	fusermount -u /tmp/boot_mount


rootfs_image:
ifeq ($(strip $(use_rootfs)),yes)
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
ifeq ($(strip $(SUPPORT_OPTEE)),TRUE)
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/$(LIB_OPTEE_PATH)
	@cp -rf $(MSP_OUT_PATH)/ta/* $(DST_ROOTFS_DIR)/$(LIB_OPTEE_PATH)
endif
ifeq ($(SUPPORT_VALGRIND), yes)
	@sed -i '/VALGRIND_LIB/{d}' $(DST_ROOTFS_DIR)/etc/profile
	@sed -i '$$a\export VALGRIND_LIB=/opt/libexec/valgrind' $(DST_ROOTFS_DIR)/etc/profile
endif
ifneq ($(findstring yes, $(debugkconfig) $(debug)),)
	echo "chmod 755 /soc/scripts/kmemleak_scan.sh" >> $(DST_ROOTFS_DIR)/etc/init.d/rcS
	echo "/soc/scripts/kmemleak_scan.sh &" >> $(DST_ROOTFS_DIR)/etc/init.d/rcS
endif

# copy param soc opt files start
	@$(ECHO) -e $(GREEN) "========= Copy /param dir files ... =========" $(DONE)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(HOME_PATH) param.filelist $(DST_ROOTFS_DIR)/param
	@$(ECHO) -e $(GREEN) "========= Copy /soc dir files ... =========" $(DONE)
ifeq ($(debug), yes)
	@python3 $(COPY_DEBUG) $(DEBUG_KO_PATH) $(DEBUG_KO_TYPE) $(DST_KO_PATH) $(DST_KO_TYPE)
endif
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(HOME_PATH) soc.filelist $(DST_ROOTFS_DIR)/soc
ifneq ($(strip $(CMM_POOL_PARAM)),)
	@sed -i "s/cmmpool*=.*/cmmpool=$(CMM_POOL_PARAM)/g" $(DST_ROOTFS_DIR)/soc/scripts/auto_load_all_drv.sh
endif
	@$(ECHO) -e $(GREEN) "========= Copy /opt dir files ... =========" $(DONE)
ifeq ($(debug), yes)
	@python3 $(COPY_DEBUG) $(DEBUG_SO_PATH) $(DEBUG_SO_TYPE) $(DST_SO_PATH) $(DST_SO_TYPE)
endif
ifeq ($(asan), yes)
	@mkdir -p $(DST_ROOTFS_DIR)/opt/lib
	@cp -rf $(CROSS_ASANLIB_PATH)/libasan*.so* $(DST_ROOTFS_DIR)/opt/lib
endif
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(HOME_PATH) opt.filelist $(DST_ROOTFS_DIR)/opt
	./change_libs_attrs.sh $(DST_ROOTFS_DIR)/opt/lib

ifeq ($(ENABLE_SWUPDATE), TRUE)
	@mkdir -p $(DST_ROOTFS_DIR)/opt/swupdate/
	@cp -rf $(MSP_OUT_PATH)/third-party/swupdate/* $(DST_ROOTFS_DIR)/opt/swupdate/
	@cp -rf swupdate/swupdate.sh $(DST_ROOTFS_DIR)/opt/swupdate/bin
	# @cp -rf $(COMM_SWUPDATE_PATH)/mkswu/public.pem $(DST_ROOTFS_DIR)/opt/swupdate/
endif

ifeq ($(SUPPORT_VALGRIND), yes)
	@cp -rf $(VALGRIND_PATH)/bin/* $(DST_ROOTFS_DIR)/opt/bin
	@cp -rf $(VALGRIND_PATH)/libexec $(DST_ROOTFS_DIR)/opt
endif
	@$(ECHO) -e $(GREEN) "========= Copy /opt dir files end ... =========" $(DONE)
# copy param soc opt files end

	# $(MAKE_EXT4FS) -l $(ROOTFS_PARTITION_SIZE) $(ROOTFS_IMG) $(DST_ROOTFS_DIR)
	$(MAKE_EXT4FS) -l $(ROOTFS_PARTITION_SIZE) -s $(ROOTFS_SPARSE_IMG) $(DST_ROOTFS_DIR)

	@imgsize=$$(stat -c %s $(ROOTFS_SPARSE_IMG));\
	if [ $$imgsize -gt $(rootfs_img_size) ]; then \
		echo "Error: $(ROOTFS_SPARSE_IMG) file size($$imgsize bytes) exceeds $(strip $(rootfs_img_size)) bytes"; \
		exit 1; \
	fi

ifeq ($(SUPPORT_CPIO),TRUE)
	@cd $(DST_ROOTFS_DIR) && rm -rf soc opt && find . | cpio -o -H newc > $(IMAGE_OUTDIR)/rootfs.cpio
endif
endif

ubuntu_rootfs_image:
ifeq ($(strip $(use_ubuntu_rootfs)),yes)
	@$(ECHO) -e $(GREEN) "making $@ ..." $(DONE)
ifeq ($(wildcard $(SRC_UBUNTU_ROOTFS_DIR)/ubuntu_rootfs_base.tar.gz),)
	@echo "file ubuntu_rootfs_base.tar.gz does not exist, don't generate image file"
else
ifeq ($(strip $(SUPPORT_OPTEE)),TRUE)
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/$(LIB_OPTEE_PATH)
	@cp -rf $(MSP_OUT_PATH)/ta/* $(DST_UBUNTU_ROOTFS_DIR)/$(LIB_OPTEE_PATH)
endif
ifeq ($(SUPPORT_VALGRIND), yes)
	@sed -i '/VALGRIND_LIB/{d}' $(DST_UBUNTU_ROOTFS_DIR)/etc/profile
	@sed -i '$$a\export VALGRIND_LIB=/opt/libexec/valgrind' $(DST_UBUNTU_ROOTFS_DIR)/etc/profile
endif
ifneq ($(findstring yes, $(debugkconfig) $(debug)),)
	echo "chmod 755 /soc/scripts/kmemleak_scan.sh" >> $(DST_UBUNTU_ROOTFS_DIR)/etc/init.d/rc.local
	echo "/soc/scripts/kmemleak_scan.sh &" >> $(DST_UBUNTU_ROOTFS_DIR)/etc/init.d/rc.local
endif

# copy param soc opt files start
	@$(ECHO) -e $(GREEN) "========= Copy /param dir files ... =========" $(DONE)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(HOME_PATH) param.filelist $(DST_UBUNTU_ROOTFS_DIR)/param
	@$(ECHO) -e $(GREEN) "========= Copy /soc dir files ... =========" $(DONE)
ifeq ($(debug), yes)
	@python3 $(COPY_DEBUG) $(DEBUG_KO_PATH) $(DEBUG_KO_TYPE) $(DST_KO_PATH) $(DST_KO_TYPE)
endif
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(HOME_PATH) soc.filelist $(DST_UBUNTU_ROOTFS_DIR)/soc
ifneq ($(strip $(CMM_POOL_PARAM)),)
	@sed -i "s/cmmpool*=.*/cmmpool=$(CMM_POOL_PARAM)/g" $(DST_UBUNTU_ROOTFS_DIR)/soc/scripts/auto_load_all_drv.sh
endif
	@$(ECHO) -e $(GREEN) "========= Copy /opt dir files ... =========" $(DONE)
ifeq ($(debug), yes)
	@python3 $(COPY_DEBUG) $(DEBUG_SO_PATH) $(DEBUG_SO_TYPE) $(DST_SO_PATH) $(DST_SO_TYPE)
endif
ifeq ($(asan), yes)
	@mkdir -p $(DST_UBUNTU_ROOTFS_DIR)/opt/lib
	@cp -rf $(CROSS_ASANLIB_PATH)/libasan*.so* $(DST_UBUNTU_ROOTFS_DIR)/opt/lib
endif
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(HOME_PATH) opt.filelist $(DST_UBUNTU_ROOTFS_DIR)/opt
	./change_libs_attrs.sh $(DST_UBUNTU_ROOTFS_DIR)/opt/lib

ifeq ($(ENABLE_SWUPDATE), TRUE)
	@mkdir -p $(DST_UBUNTU_ROOTFS_DIR)/opt/swupdate/
	@cp -rf $(MSP_OUT_PATH)/third-party/swupdate/* $(DST_UBUNTU_ROOTFS_DIR)/opt/swupdate/
	@cp -rf swupdate/swupdate.sh $(DST_UBUNTU_ROOTFS_DIR)/opt/swupdate/bin
	# @cp -rf $(COMM_SWUPDATE_PATH)/mkswu/public.pem $(DST_UBUNTU_ROOTFS_DIR)/opt/swupdate/
endif

ifeq ($(SUPPORT_VALGRIND), yes)
	@cp -rf $(VALGRIND_PATH)/bin/* $(DST_UBUNTU_ROOTFS_DIR)/opt/bin
	@cp -rf $(VALGRIND_PATH)/libexec $(DST_UBUNTU_ROOTFS_DIR)/opt
endif
	@$(ECHO) -e $(GREEN) "========= Copy /opt dir files end ... =========" $(DONE)
# copy param soc opt files end

	# $(MAKE_EXT4FS) -l $(ROOTFS_PARTITION_SIZE) $(UBUNTU_ROOTFS_IMG) $(DST_UBUNTU_ROOTFS_DIR)
	$(MAKE_EXT4FS) -l $(ROOTFS_PARTITION_SIZE) -s $(UBUNTU_ROOTFS_SPARSE_IMG) $(DST_UBUNTU_ROOTFS_DIR)
endif
endif

clean:
	@$(ECHO) -e $(GREEN) "rootfs $@..." $(DONE)
	@rm -rf $(DST_ROOTFS_DIR)
	@rm -rf $(DST_UBUNTU_ROOTFS_DIR)
