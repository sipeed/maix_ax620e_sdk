HOME_PATH    := $(abspath $(shell pwd)/../../../)
BUILD_PATH   := $(HOME_PATH)/build
BUSYBOX_PATH := $(shell pwd)/busybox

include $(BUILD_PATH)/color.mk
include $(BUILD_PATH)/config.mak

ifeq ($(use_buildroot_rootfs),)
	use_buildroot_rootfs := yes
	$(info default use_buildroot_rootfs for ax620q nanoagent)
endif

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
#buildroot_rootfs
BUILDROOT_VERSION        := buildroot-2025.11
SRC_BUILDROOT_ROOTFS_DIR := $(HOME_PATH)/rootfs/$(ARCH)/$(LIBC)/buildroot/$(BUILDROOT_VERSION)/output/images
DST_BUILDROOT_ROOTFS_DIR := $(OUT_DIR)/buildroot_rootfs
BUILDROOT_ROOTFS_FSTAB_FILE := $(DST_BUILDROOT_ROOTFS_DIR)/etc/fstab

ifeq ($(use_buildroot_rootfs),yes)
DST_ROOTFS_DIR           := $(DST_BUILDROOT_ROOTFS_DIR)
else
DST_ROOTFS_DIR           := $(OUT_DIR)/rootfs
endif
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

#soc
SRC_SOC_DIR       := $(HOME_PATH)
SCRIPTS_DIR       := $(HOME_PATH)/rootfs/rootfs/opt/scripts
DST_SOC_DIR       := $(OUT_DIR)/soc

#opt
SRC_OPT_DIR       := $(HOME_PATH)
DST_OPT_DIR       := $(OUT_DIR)/opt

#optee
LIB_OPTEE_PATH    := lib/optee_armtz

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
ifeq ($(strip $(use_buildroot_rootfs)),yes)
	@$(ECHO) -e $(GREEN) "Making buildroot $@..." $(DONE)
ifeq ($(wildcard $(SRC_BUILDROOT_ROOTFS_DIR)/rootfs.tar),)
	@echo "file $(SRC_BUILDROOT_ROOTFS_DIR)/rootfs.tar does not exist, please build the buildroot!"
else
	@bash mk_buildroot_rootfs_sipeed.sh $(DST_BUILDROOT_ROOTFS_DIR) $(SRC_BUILDROOT_ROOTFS_DIR)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_ROOTFS_DIR) buildroot_rootfs.filelist $(DST_BUILDROOT_ROOTFS_DIR)
	@$(MKDIR) -p $(DST_BUILDROOT_ROOTFS_DIR)/proc
	@$(MKDIR) -p $(DST_BUILDROOT_ROOTFS_DIR)/sys
	@$(MKDIR) -p $(DST_BUILDROOT_ROOTFS_DIR)/mnt
	@$(MKDIR) -p $(DST_BUILDROOT_ROOTFS_DIR)/tmp
	@$(MKDIR) -p $(DST_BUILDROOT_ROOTFS_DIR)/run
	@$(MKDIR) -p $(DST_BUILDROOT_ROOTFS_DIR)/root
	@$(MKDIR) -p $(DST_BUILDROOT_ROOTFS_DIR)/boot
	@$(MKDIR) -p $(DST_BUILDROOT_ROOTFS_DIR)/media
	@$(MKDIR) -p $(DST_BUILDROOT_ROOTFS_DIR)/param
	@$(MKDIR) $(DST_BUILDROOT_ROOTFS_DIR)/var/www
	@$(MKDIR) $(IMAGE_OUTDIR)
	@chmod 755 $(MODIFY_FSTAB_FILE)
	@bash $(MODIFY_FSTAB_FILE) $(BUILDROOT_ROOTFS_FSTAB_FILE) "$(FLASH_PARTITIONS)"
endif
else
	@$(ECHO) -e $(GREEN) "Making $@..." $(DONE)
	@$(MKDIR) -p $(DST_ROOTFS_DIR)
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/proc
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/sys
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/mnt
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/tmp
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/run
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/root
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/boot
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/media
	@$(MKDIR) -p $(DST_ROOTFS_DIR)/param
	@$(MKDIR) $(DST_ROOTFS_DIR)/var/www
	@$(MKDIR) $(IMAGE_OUTDIR)
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_ROOTFS_DIR) rootfs.filelist $(DST_ROOTFS_DIR)
	@chmod 755 $(MODIFY_FSTAB_FILE)
	@bash $(MODIFY_FSTAB_FILE) $(FSTAB_FILE) "$(FLASH_PARTITIONS)"
endif

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

ifeq ($(SUPPORT_CPIO),TRUE)
	@cd $(DST_ROOTFS_DIR) && find . | cpio -o -H newc > $(IMAGE_OUTDIR)/rootfs.cpio
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
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/boot
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/media
	@$(MKDIR) -p $(DST_UBUNTU_ROOTFS_DIR)/param
	@$(MKDIR) $(DST_UBUNTU_ROOTFS_DIR)/var/www
	@$(MKDIR) $(DST_UBUNTU_ROOTFS_DIR)/etc
	@$(MKDIR) $(IMAGE_OUTDIR)
ifeq ($(AX_SUPPORT_AB_PART),TRUE)
	mkdir -p $(DST_UBUNTU_ROOTFS_DIR)/etc/init.d/
	@cp -rf rootfs/etc/init.d/* $(DST_UBUNTU_ROOTFS_DIR)/etc/init.d/
endif
	@bash $(SRC_UBUNTU_ROOTFS_DIR)/mk_ubuntu_rootfs.sh $(DST_UBUNTU_ROOTFS_DIR) $(SRC_UBUNTU_ROOTFS_DIR)
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
	@$(ECHO) -e $(GREEN) "images $@..." $(DONE)

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
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_SOC_DIR) soc.filelist $(DST_ROOTFS_DIR)/soc
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_OPT_DIR) opt.filelist $(DST_ROOTFS_DIR)/opt
	@mkdir -p $(DST_ROOTFS_DIR)/opt/etc/models/aiisp
	@for element in $(SENSOR_TYPE); do \
		if [ ! -d $(CUR_DIR)/sensors/$$element ]; then \
			echo "Error: Unsupported sensor: $$element"; \
			exit 1; \
		fi; \
		python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK)  $(SRC_OPT_DIR) $(CUR_DIR)/sensors/$$element/so.filelist $(DST_ROOTFS_DIR)/opt; \
		python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK)  $(SRC_OPT_DIR) $(CUR_DIR)/sensors/$$element/model.filelist $(DST_ROOTFS_DIR)/opt; \
	done

ifeq ($(ENABLE_SWUPDATE), TRUE)
	@mkdir -p $(DST_ROOTFS_DIR)/opt/swupdate/
	@cp -rf $(MSP_OUT_PATH)/third-party/swupdate/* $(DST_ROOTFS_DIR)/opt/swupdate/
	@cp -rf swupdate/swupdate.sh $(DST_ROOTFS_DIR)/opt/swupdate/bin
	# @cp -rf $(COMM_SWUPDATE_PATH)/mkswu/public.pem $(DST_ROOTFS_DIR)/opt/swupdate/
endif

ifeq ($(SUPPORT_VALGRIND), yes)
	@cp -rf $(VALGRIND_PATH)/bin/* $(DST_ROOTFS_DIR)/opt/bin
	@cp -rf $(VALGRIND_PATH)/libexec $(DST_ROOTFS_DIR)/opt/
endif
	$(MAKE_EXT4FS) -l $(ROOTFS_PARTITION_SIZE) $(ROOTFS_IMG) $(DST_ROOTFS_DIR)
	$(MAKE_EXT4FS) -l $(ROOTFS_PARTITION_SIZE) -s $(ROOTFS_SPARSE_IMG) $(DST_ROOTFS_DIR)

	@imgsize=$$(stat -c %s $(ROOTFS_SPARSE_IMG));\
	if [ $$imgsize -gt $(rootfs_img_size) ]; then \
		echo "Error: $(ROOTFS_SPARSE_IMG) file size($$imgsize bytes) exceeds $(strip $(rootfs_img_size)) bytes"; \
		exit 1; \
	fi

ifeq ($(SUPPORT_CPIO),TRUE)
	@cd $(DST_ROOTFS_DIR) && rm -rf soc opt && find . | cpio -o -H newc > $(IMAGE_OUTDIR)/rootfs.cpio
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
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_SOC_DIR) soc.filelist $(DST_UBUNTU_ROOTFS_DIR)/soc
	@python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK) $(SRC_OPT_DIR) opt.filelist $(DST_UBUNTU_ROOTFS_DIR)/opt
	@mkdir -p $(DST_UBUNTU_ROOTFS_DIR)/opt/etc/models/aiisp
	@for element in $(SENSOR_TYPE); do \
		if [ ! -d $(CUR_DIR)/sensors/$$element ]; then \
			echo "Error: Unsupported sensor: $$element"; \
			exit 1; \
		fi; \
		python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK)  $(SRC_OPT_DIR) $(CUR_DIR)/sensors/$$element/so.filelist $(DST_UBUNTU_ROOTFS_DIR)/opt; \
		python3 $(COPY_FILE) $(PROJECT) $(PROJECT_MAK)  $(SRC_OPT_DIR) $(CUR_DIR)/sensors/$$element/model.filelist $(DST_UBUNTU_ROOTFS_DIR)/opt; \
	done

ifeq ($(ENABLE_SWUPDATE), TRUE)
	@mkdir -p $(DST_UBUNTU_ROOTFS_DIR)/opt/swupdate/
	@cp -rf $(MSP_OUT_PATH)/third-party/swupdate/* $(DST_UBUNTU_ROOTFS_DIR)/opt/swupdate/
	@cp -rf swupdate/swupdate.sh $(DST_UBUNTU_ROOTFS_DIR)/opt/swupdate/bin
	# @cp -rf $(COMM_SWUPDATE_PATH)/mkswu/public.pem $(DST_UBUNTU_ROOTFS_DIR)/opt/swupdate/
endif

ifeq ($(SUPPORT_VALGRIND), yes)
	@cp -rf $(VALGRIND_PATH)/bin/* $(DST_UBUNTU_ROOTFS_DIR)/opt/bin
	@cp -rf $(VALGRIND_PATH)/libexec $(DST_UBUNTU_ROOTFS_DIR)/opt/
endif
	$(MAKE_EXT4FS) -l $(ROOTFS_PARTITION_SIZE) $(UBUNTU_ROOTFS_IMG) $(DST_UBUNTU_ROOTFS_DIR)
	$(MAKE_EXT4FS) -l $(ROOTFS_PARTITION_SIZE) -s $(UBUNTU_ROOTFS_SPARSE_IMG) $(DST_UBUNTU_ROOTFS_DIR)
endif
endif

clean:
	@$(ECHO) -e $(GREEN) "rootfs $@..." $(DONE)
	@rm -rf $(DST_BOOTFS_DIR)
	@rm -rf $(DST_ROOTFS_DIR)
	@rm -rf $(DST_UBUNTU_ROOTFS_DIR)
	@rm -rf $(DST_SOC_DIR)
	@rm -rf $(DST_OPT_DIR)
