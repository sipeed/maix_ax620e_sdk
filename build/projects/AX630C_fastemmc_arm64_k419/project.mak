############################################################################################
CUR_DIR          := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
PRJECT_DIR       := $(shell dirname $(CUR_DIR))

ARCH             := arm64
KERNEL_DIR       := linux-4.19.125
CHIP_TYPE        := AX630C
LIBC             := glibc

BUILD_DIR        := $(shell dirname $(PRJECT_DIR))
HOME_PATH        := $(shell dirname $(BUILD_DIR))
MSP_OUT_DIR      := $(ARCH)_$(LIBC)
OSDRV_OUT_DIR    := $(ARCH)_$(LIBC)_$(KERNEL_DIR)
MSP_OUT_PATH     := $(HOME_PATH)/msp/out/$(MSP_OUT_DIR)
OSDRV_OUT_PATH   := $(HOME_PATH)/kernel/osdrv/out/$(OSDRV_OUT_DIR)
ROOTFS_TARGET_PATH := $(MSP_OUT_PATH)

include $(BUILD_DIR)/cross_$(ARCH)_$(LIBC).mak
include $(PRJECT_DIR)/common.mak

# You are not advised to modify the following options
BUILD_BUSYBOX                := FALSE
SUPPPORT_GZIPD               := TRUE
SECURE_BOOT_TEST             := FALSE
AX_BOOT_OPTIMIZATION_SUPPORT := TRUE
COMPRESS_KO                  := TRUE
FLASH_TYPE                   := emmc
SUPPORT_ATF                  := TRUE
# optee=true depends on atf=true
SUPPORT_OPTEE                := FALSE
SUPPORT_CPIO                 := TRUE
SUPPORT_KERNEL_BOOTARGS      := TRUE
AX_SUPPORT_AB_PART           := TRUE
ENABLE_SWUPDATE              := FALSE
SUPPORT_DDRINIT_PART         := TRUE
AX_RISCV_SUPPORT             := TRUE
AX630C_DDR4_RETRAIN          := TRUE
ifeq ($(strip $(AX_BOOT_OPTIMIZATION_SUPPORT)),TRUE)
AX_SPL_UBOOT_KERNEL_SUPPORT    := FALSE
AX_SPL_SUPPORT_MODIFY_BOOTARGS := FALSE
else
AX_SPL_UBOOT_KERNEL_SUPPORT    := TRUE
endif
ifeq ($(strip $(SUPPORT_ATF)),FALSE)
SUPPORT_OPTEE                  := FALSE
endif
ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
AX_RISCV_ISP_SUPPORT           := TRUE
endif
COPY_DWMAC_KO                  := TRUE

############################################################################################
# Note: The above configuration is generally not modified.

# The sensor type can be modified by compiling parameters
SENSOR_TYPE                  := os04a10
ifneq ($(strip $(sensor)),)
SENSOR_TYPE      := $(sensor)
endif
SENSOR_PATH      := $(CUR_DIR)/sensors/$(SENSOR_TYPE)
ifeq ($(wildcard $(SENSOR_PATH)),)
$(error "Unsupported sensor: $(SENSOR_TYPE)")
endif

AX_SYSDUMP_EMMC              := TRUE
#default use rsa2048
#SIGN_USE_RSA3072            := FALSE
KERNEL_BUILDIN_PRVDRV        := TRUE
DEBUG_BOOT_TIME              := TRUE

ifeq ($(strip $(AX_SUPPORT_AB_PART)),TRUE)
include $(PRJECT_DIR)/AX630C_fastemmc_arm64_k419/partition_ab.mak
else
include $(PRJECT_DIR)/AX630C_fastemmc_arm64_k419/partition.mak
endif
