############################################################################################
CUR_DIR          := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
PRJECT_DIR       := $(shell dirname $(CUR_DIR))

ARCH             := arm
KERNEL_DIR       := linux-4.19.125
CHIP_TYPE        := AX620Q
LIBC             := glibc

# The libc type can be modified by compiling parameters
ifneq ($(strip $(libc)),)
LIBC             := $(libc)
endif

ifneq ($(strip $(LIBC)),glibc)
ifneq ($(strip $(LIBC)),uclibc)
$(error "only support glibc & uclibc")
endif
endif

BUILD_DIR        := $(shell dirname $(PRJECT_DIR))
HOME_PATH        := $(shell dirname $(BUILD_DIR))
MSP_OUT_DIR      := $(ARCH)_$(LIBC)
OSDRV_OUT_DIR    := $(ARCH)_$(LIBC)_$(KERNEL_DIR)
MSP_OUT_PATH     := $(HOME_PATH)/msp/out/$(MSP_OUT_DIR)
OSDRV_OUT_PATH   := $(HOME_PATH)/kernel/osdrv/out/$(OSDRV_OUT_DIR)
ROOTFS_TARGET_PATH := $(MSP_OUT_PATH)

include $(BUILD_DIR)/cross_$(ARCH)_$(LIBC)_nanoagent.mak
include $(PRJECT_DIR)/common.mak

# You are not advised to modify the following options
BUILD_BUSYBOX                := FALSE
SUPPPORT_GZIPD               := TRUE
AX_BOOT_OPTIMIZATION_SUPPORT := FALSE
COMPRESS_KO                  := TRUE
FLASH_TYPE                   := emmc
SUPPORT_ATF                  := FALSE
# optee=true depends on atf=true
SUPPORT_OPTEE                := FALSE
SUPPORT_KERNEL_BOOTARGS      := TRUE
ifeq ($(strip $(AX_BOOT_OPTIMIZATION_SUPPORT)),TRUE)
AX_SPL_UBOOT_KERNEL_SUPPORT  := FALSE
AX_SPL_SUPPORT_MODIFY_BOOTARGS := TRUE
else
AX_SPL_UBOOT_KERNEL_SUPPORT  := TRUE
endif
ifeq ($(strip $(SUPPORT_ATF)),FALSE)
SUPPORT_OPTEE                := FALSE
endif
use_buildroot_rootfs        := no
use_ubuntu_rootfs           := no
use_debian_rootfs           := yes

############################################################################################
# Note: The above configuration is generally not modified.

# Default use rsa2048
#SIGN_USE_RSA3072            := FALSE

SUPPORT_DDRINIT_PART         := TRUE
AX_SYSDUMP_EMMC              := FALSE
ifeq ($(strip $(buildin_prvdrv)),yes)
KERNEL_BUILDIN_PRVDRV := TRUE
endif

AX620Q_EMMC                  := TRUE
UBOOT_SPI2_SIPEED_LOGO       := TRUE

include $(PRJECT_DIR)/AX620Q_emmc_arm32_k419_sipeed_nanoagent/partition.mak
