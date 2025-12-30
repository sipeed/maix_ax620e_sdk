############################################################################################
CUR_DIR          := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
PRJECT_DIR       := $(shell dirname $(CUR_DIR))

ARCH             := arm
KERNEL_DIR       := linux-4.19.125
CHIP_TYPE        := AX620Q
LIBC             := uclibc

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

include $(BUILD_DIR)/cross_$(ARCH)_$(LIBC).mak
include $(PRJECT_DIR)/common.mak

# You are not advised to modify the following options
BUILD_BUSYBOX                  := FALSE
SUPPORT_CPIO                   := FALSE
SUPPPORT_GZIPD                 := TRUE
AX_BOOT_OPTIMIZATION_SUPPORT   := FALSE
FLASH_TYPE                     := nor
SUPPORT_ATF                    := FALSE
# optee=true depends on atf=true
SUPPORT_OPTEE                  := FALSE
SUPPORT_KERNEL_BOOTARGS        := TRUE
COPY_NFS_KO                    := TRUE
SUPPORT_DDRINIT_PART           := FALSE
ifeq ($(strip $(AX_BOOT_OPTIMIZATION_SUPPORT)),TRUE)
AX_SPL_UBOOT_KERNEL_SUPPORT    := FALSE
AX_SPL_SUPPORT_MODIFY_BOOTARGS := TRUE
else
AX_SPL_UBOOT_KERNEL_SUPPORT    := TRUE
endif
SUPPPORT_KERNEL_LZMA           := TRUE
COMPRESS_KO                    := TRUE
FDL2_UBOOT_COMPILE_INDEPENDENT := TRUE
ifeq ($(strip $(SUPPORT_ATF)),FALSE)
SUPPORT_OPTEE                  := FALSE
endif

# The address of the text segment in the startup phase, do not recommend modification.
IMG_HEADER_SIZE                 := (0x400)
ATF_IMG_HEADER_BASE             := (0x4003FC00)
UBOOT_IMG_HEADER_BASE           := (0x5C000000)
DDRINIT_PARAM_HEADER_BASE       := (0x03200000)
AXERA_DTB_IMG_ADDR              := 0x42008000
DTB_IMG_HEADER_ADDR             := ($(AXERA_DTB_IMG_ADDR) - $(IMG_HEADER_SIZE))
AXERA_KERNEL_IMG_ADDR           := 0x40008000
KERNEL_IMG_HEADER_ADDR          := ($(AXERA_KERNEL_IMG_ADDR) - $(IMG_HEADER_SIZE))
############################################################################################
# Note: The above configuration is generally not modified.

AX_SYSDUMP_SD                := TRUE
ifeq ($(strip $(buildin_prvdrv)),yes)
KERNEL_BUILDIN_PRVDRV   := TRUE
endif

# recycle CMM memory to OS memory
SUPPORT_RECYCLE_CMM          := TRUE

# The sensor type can be modified by compiling parameters
SENSOR_TYPE                  := os04a10
ifneq ($(strip $(sensor)),)
SENSOR_TYPE                  := $(sensor)
endif
SENSOR_PATH                  := $(CUR_DIR)/sensors/$(SENSOR_TYPE)
ifeq ($(wildcard $(SENSOR_PATH)),)
$(error "Unsupported sensor: $(SENSOR_TYPE)")
endif

# Used to automatically generate fastab
CONFIG_FS_TYPE               := squashfs # squashfs or jffs2
CONFIG_OPT_FS_TYPE           := squashfs
CONFIG_CUSTOMER_FS_TYPE      := jffs2

#partition size config
NORFLASH_SIZE                   := 16M
SPL_PARTITION_SIZE              := 128K
UBOOT_PARTITION_SIZE            := 384K
ENV_PARTITION_SIZE              := 64K
DTB_PARTITION_SIZE              := 64K
ifeq ($(LIBC), uclibc)
KERNEL_PARTITION_SIZE           := 2432K
ROOTFS_PARTITION_SIZE           := 2368K
CUSTOMER_PARTITION_SIZE         := 768K
else
KERNEL_PARTITION_SIZE           := 2432K
ROOTFS_PARTITION_SIZE           := 3008K
CUSTOMER_PARTITION_SIZE         := 1M
endif
OPT_PARTITION_SIZE              := $(call AutoCalculateRest, \
                                          $(NORFLASH_SIZE) \
                                          $(SPL_PARTITION_SIZE) \
                                          $(UBOOT_PARTITION_SIZE) \
                                          $(ENV_PARTITION_SIZE) \
                                          $(DTB_PARTITION_SIZE) \
                                          $(KERNEL_PARTITION_SIZE) \
                                          $(ROOTFS_PARTITION_SIZE) \
                                          $(CUSTOMER_PARTITION_SIZE))
AUTO_FIT_PARTITION              := OPT

# env part size, size is equal to ENV_PARTITION_SIZE
ENV_IMG_PKG_SIZE                := 0x10000
# Dram base address and size
SYS_DRAM_BASE                   := 0x40000000
SYS_DRAM_SIZE                   := 256 #MB

# linux OS memory config
ifeq ($(strip $(SUPPORT_RECYCLE_CMM)),TRUE)
OS_MEM_SIZE          := $(SYS_DRAM_SIZE)
CMM_OFFSET_SIZE      := 96 #MB
else
OS_MEM_SIZE          := 96 #MB
endif
OS_MEM               := mem=$(strip $(OS_MEM_SIZE))M

# cmm memory config
ifeq ($(strip $(SUPPORT_RECYCLE_CMM)),TRUE)
CMM_START_ADDR       := $(call AddAddressMB, $(SYS_DRAM_BASE), $(CMM_OFFSET_SIZE))
CMM_SIZE             := $(shell echo $$(($(SYS_DRAM_SIZE) - $(CMM_OFFSET_SIZE))))
CMM_RECYCLE_START    := $(CMM_START_ADDR)
CMM_RECYCLE_SIZE     := $(call MB2Bytes, $(CMM_SIZE))
else
CMM_START_ADDR       := $(call AddAddressMB, $(SYS_DRAM_BASE), $(OS_MEM_SIZE))
CMM_SIZE             := $(shell echo $$(($(SYS_DRAM_SIZE) - $(OS_MEM_SIZE))))
endif
CMM_POOL_PARAM       := anonymous,0,$(strip $(CMM_START_ADDR)),$(CMM_SIZE)M

# uboot boot args
include $(PRJECT_DIR)/partition_tbl.mak
FLASH_PARTITIONS  := $(SPL_PART),
ifeq ($(strip $(SUPPORT_DDRINIT_PART)),TRUE)
FLASH_PARTITIONS  += $(DDRINIT_PART),
endif
FLASH_PARTITIONS  += $(UBOOT_PART),$(ENV_PART),$(DTB_PART),
FLASH_PARTITIONS  += $(KERNEL_PART),$(ROOTFS_PART),$(CUSTOMER_PART),$(OPT_PART)
FLASH_PARTITIONS  := $(shell echo '$(FLASH_PARTITIONS)' | tr -d ' ')
ROOTFS_TYPE       := $(CONFIG_FS_TYPE)
ROOTFS_POSITION   := $(shell echo "$(FLASH_PARTITIONS)" | tr ',' '\n' | grep -n 'rootfs' | cut -d ':' -f 1)
ROOTFS_DEV        := /dev/mtdblock$(strip $(ROOTFS_POSITION))
KERNEL_BOOTARGS   := "$(OS_MEM) console=ttyS0,115200n8 loglevel=8 earlycon=uart8250,mmio32,0x4880000 board_id=0x0,boot_reason=0x00,noinitrd \
root=$(ROOTFS_DEV) rw rootfstype=$(ROOTFS_TYPE) init=/linuxrc mtdparts=spi4.0:$(FLASH_PARTITIONS)"

# Calculates the position of each partition in flash
DDRINIT_HEADER_FLASH_BASE     := $(call calculate_flash_base,$(FLASH_PARTITIONS),ddrinit)
ATF_A_HEADER_FLASH_BASE       := $(call calculate_flash_base,$(FLASH_PARTITIONS),atf)
ATF_B_HEADER_FLASH_BASE       := $(call calculate_flash_base,$(FLASH_PARTITIONS),atf_b)
UBOOT_A_HEADER_FLASH_BASE     := $(call calculate_flash_base,$(FLASH_PARTITIONS),uboot)
UBOOT_B_HEADER_FLASH_BASE     := $(call calculate_flash_base,$(FLASH_PARTITIONS),uboot_b)
ENV_DATA_FLASH_BASE           := $(call calculate_flash_base,$(FLASH_PARTITIONS),env)
ifeq ($(strip $(SUPPORT_OPTEE)),TRUE)
OPTEE_A_HEADER_FLASH_BASE     := $(call calculate_flash_base,$(FLASH_PARTITIONS),optee)
OPTEE_B_HEADER_FLASH_BASE     := $(call calculate_flash_base,$(FLASH_PARTITIONS),optee_b)
endif
DTB_A_HEADER_FLASH_BASE       := $(call calculate_flash_base,$(FLASH_PARTITIONS),dtb)
DTB_B_HEADER_FLASH_BASE       := $(call calculate_flash_base,$(FLASH_PARTITIONS),dtb_b)
KERNEL_A_HEADER_FLASH_BASE    := $(call calculate_flash_base,$(FLASH_PARTITIONS),kernel)
KERNEL_B_HEADER_FLASH_BASE    := $(call calculate_flash_base,$(FLASH_PARTITIONS),kernel_b)
