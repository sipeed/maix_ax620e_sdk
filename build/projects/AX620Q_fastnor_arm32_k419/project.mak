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
AX_BOOT_OPTIMIZATION_SUPPORT   := TRUE
FLASH_TYPE                     := nor
SUPPORT_ATF                    := FALSE
# optee=true depends on atf=true
SUPPORT_OPTEE                  := FALSE
SUPPORT_KERNEL_BOOTARGS        := TRUE
SUPPORT_DDRINIT_PART           := TRUE
ifeq ($(strip $(AX_BOOT_OPTIMIZATION_SUPPORT)),TRUE)
AX_SPL_UBOOT_KERNEL_SUPPORT    := FALSE
SUPPORT_RAMDISK                := TRUE
AX_SPL_SUPPORT_MODIFY_BOOTARGS := TRUE
else
AX_SPL_UBOOT_KERNEL_SUPPORT    := TRUE
endif
COMPRESS_KO                    := TRUE
COPY_NFS_KO                    := TRUE
COPY_USB_KO                    := TRUE
COPY_DWMAC_KO                  := TRUE
FDL2_UBOOT_COMPILE_INDEPENDENT := TRUE
COPY_SD_KO                     := TRUE
SUPPORT_RECOVERY               := FALSE
AX_RISCV_SUPPORT               := TRUE
ifeq ($(strip $(SUPPORT_ATF)),FALSE)
SUPPORT_OPTEE                  := FALSE
endif
ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
AX_RISCV_LOAD_ROOTFS_SUPPORT   := TRUE
AX_RISCV_LOAD_MODEL_SUPPORT    := TRUE
AX_RISCV_ISP_SUPPORT           := TRUE
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

DEBUG_BOOT_TIME              := TRUE
KERNEL_BUILDIN_PRVDRV        := TRUE

# The detect type can be modified by compiling parameters

ifeq ($(strip $(detect)), raw)
DETECT_TYPE			:= raw
AX_FAST_RISCV_RAW_DET_SUPPORT	:= TRUE
else ifeq ($(strip $(detect)), md)
DETECT_TYPE			:= md
AX_FAST_RISCV_MD_DET_SUPPORT	:= TRUE
else ifeq ($(strip $(detect)), yuv)
DETECT_TYPE			:= yuv
AX_FAST_RISCV_YUV_DET_SUPPORT	:= TRUE
else
AX_FAST_RISCV_YUV_DET_SUPPORT	:= TRUE	 #default config
endif

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
CONFIG_FS_TYPE               := SQUASHFS # SQUASHFS or JFFS2

# Partition size config
NORFLASH_SIZE           := 16M
SPL_PARTITION_SIZE      := 128K
DDRINIT_PARTITION_SIZE  := 64K
UBOOT_PARTITION_SIZE    := 384K
ENV_PARTITION_SIZE      := 64K
RAWDATA_PARTITION_SIZE  := 128K
DTB_PARTITION_SIZE      := 64K
KERNEL_PARTITION_SIZE   := 3456K
ifeq ($(strip $(SUPPORT_RECOVERY)),TRUE)
RECOVERY_PARTITION_SIZE := 3M
endif
ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
MODEL_PARTITION_SIZE    := 3M
endif
ifeq ($(strip $(AX_FAST_RISCV_RAW_DET_SUPPORT)),TRUE)
RISCV_PARTITION_SIZE    := 2M
else
RISCV_PARTITION_SIZE    := 256K
endif
CUSTOMER_PARTITION_SIZE := 1M
ROOTFS_PARTITION_SIZE   := $(call AutoCalculateRest, \
                                  $(NORFLASH_SIZE) \
                                  $(SPL_PARTITION_SIZE) \
                                  $(DDRINIT_PARTITION_SIZE) \
                                  $(UBOOT_PARTITION_SIZE) \
                                  $(ENV_PARTITION_SIZE) \
                                  $(RAWDATA_PARTITION_SIZE) \
                                  $(DTB_PARTITION_SIZE) \
                                  $(KERNEL_PARTITION_SIZE) \
                                  $(RECOVERY_PARTITION_SIZE) \
                                  $(MODEL_PARTITION_SIZE) \
                                  $(RISCV_PARTITION_SIZE) \
                                  $(CUSTOMER_PARTITION_SIZE))

AUTO_FIT_PARTITION      := OPT

# env part size, size is equal to ENV_PARTITION_SIZE
ENV_IMG_PKG_SIZE        := 0x10000

# Dram base address and size
SYS_DRAM_BASE           := 0x40000000
SYS_DRAM_SIZE           := 256 #MB

# linux OS memory config
ifeq ($(SENSOR_TYPE), sc850sl)
OS_MEM_SIZE          := 135 #MB
ISP_IMAGE_LEN_MB     := 50 #MB
# ramdisk size
ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
RAMDISK_SIZE_MB      := 10 #MB
else
RAMDISK_SIZE_MB      := 16 #MB
endif
else ifeq ($(SENSOR_TYPE), sc500ai)
OS_MEM_SIZE          := 160 #MB
ISP_IMAGE_LEN_MB     := 34 #MB
# ramdisk size
ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
RAMDISK_SIZE_MB      := 10 #MB
else
RAMDISK_SIZE_MB      := 16 #MB
endif
else
OS_MEM_SIZE          := 160 #MB
ISP_IMAGE_LEN_MB     := 32 #MB
# ramdisk size
ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
RAMDISK_SIZE_MB      := 10 #MB
else
RAMDISK_SIZE_MB      := 16 #MB
endif
endif
OS_MEM               := mem=$(strip $(OS_MEM_SIZE))M

# Memory space for fast boot riscv to save images.
ISP_IMAGE_DDR_END       := $(call AddAddressMB, $(SYS_DRAM_BASE), $(OS_MEM_SIZE))
ISP_IMAGE_DDR_START     := $(call SubAddressMB, $(ISP_IMAGE_DDR_END), $(ISP_IMAGE_LEN_MB))
ISP_IMAGE_LEN_B         := $(shell printf "0x%X" $$(($(ISP_IMAGE_LEN_MB)*1048576)))

# Allocate running memory space for riscv.
ifeq ($(strip $(AX_FAST_RISCV_RAW_DET_SUPPORT)),TRUE)
RISCV_MEM_SIZE_MB              := 10 #MB
else ifeq ($(strip $(AX_FAST_RISCV_MD_DET_SUPPORT)),TRUE)
RISCV_MEM_SIZE_MB              := 10 #MB
else
RISCV_MEM_SIZE_MB              := 3 #MB
endif
RISCV_MEM_SIZE_B               := $(shell printf "0x%X" $$(($(RISCV_MEM_SIZE_MB)*1048576)))
RISCV_DDR_END_ADDR             := $(ISP_IMAGE_DDR_START)
RISCV_BIN_DDR_START            := $(call SubAddressMB, $(RISCV_DDR_END_ADDR), $(RISCV_MEM_SIZE_MB))
RISCV_HEADER_DDR_START         := $(call SubAddressB, $(RISCV_BIN_DDR_START), $(SEC_SIGN_HEADER_SIZE))

#ramdisk config
RAMDISK_SIZE         := $(shell printf "0x%X" $$(($(RAMDISK_SIZE_MB)*1048576)))
RAMDISK_END_ADDR     := $(RISCV_BIN_DDR_START)
KERN_RAMDISK_SADDR   := $(call SubAddressMB, $(RAMDISK_END_ADDR), $(RAMDISK_SIZE_MB))
RAMDISK_START_ADDR   := $(call SubAddressB, $(KERN_RAMDISK_SADDR), $(SEC_SIGN_HEADER_SIZE))
RAMDISK_HEADER_RESERVE_START  := $(call SubAddressB, $(KERN_RAMDISK_SADDR), 4096)
RAMDISK_HEADER_RESERVE_SIZE   := 4096

# Riscv log mem reserve address.
RISCV_LOG_MEM_RESERVE_START  := $(call SubAddressB, $(RAMDISK_HEADER_RESERVE_START), 4096)
RISCV_LOG_MEM_RESERVE_SIZE   := 4096

#model config
ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
MODEL_MEM_SIZE_MB  := 6 #MB
MODEL_MEM_SIZE_B   := $(shell printf "0x%X" $$(($(MODEL_MEM_SIZE_MB)*1048576)))
MODEL_END_ADDR     := $(RISCV_LOG_MEM_RESERVE_START)
KERN_MODEL_SADDR   := $(call SubAddressMB, $(MODEL_END_ADDR), $(MODEL_MEM_SIZE_MB))
MODEL_START_ADDR   := $(call SubAddressB, $(KERN_MODEL_SADDR), $(SEC_SIGN_HEADER_SIZE))
MODEL_DATA_RESERVE_START  := $(MODEL_START_ADDR)
MODEL_DATA_RESERVE_SIZE   := $(MODEL_MEM_SIZE_B)
endif

# cmm memory config
CMM_START_ADDR       := $(call AddAddressMB, $(SYS_DRAM_BASE), $(OS_MEM_SIZE))
CMM_SIZE             := $(shell echo $$(($(SYS_DRAM_SIZE) - $(OS_MEM_SIZE))))
CMM_POOL_PARAM       := anonymous,0,$(strip $(CMM_START_ADDR)),$(CMM_SIZE)M

# riscv load rootfs image addr
CMM_END_ADDR                   := $(call AddAddressMB, $(SYS_DRAM_BASE), $(SYS_DRAM_SIZE))
RISCV_ROOTFS_COMPRESSED_ADDR   := $(call SubAddressMB, $(CMM_END_ADDR), 16)

# uboot boot args
include $(PRJECT_DIR)/partition_tbl.mak
FLASH_PARTITIONS  := $(SPL_PART),
ifeq ($(strip $(SUPPORT_DDRINIT_PART)),TRUE)
FLASH_PARTITIONS  += $(DDRINIT_PART),
endif
FLASH_PARTITIONS  += $(UBOOT_PART),$(ENV_PART),$(RAWDATA_PART),$(DTB_PART),
ifeq ($(strip $(SUPPORT_RECOVERY)),TRUE)
FLASH_PARTITIONS  += $(KERNEL_PART),$(ROOTFS_PART),$(RECOVERY_PART),$(CUSTOMER_PART),
else
FLASH_PARTITIONS  += $(KERNEL_PART),$(ROOTFS_PART),$(CUSTOMER_PART),
endif
ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
FLASH_PARTITIONS  += $(RISCV_PART),
endif
ifeq ($(strip $(AX_RISCV_LOAD_MODEL_SUPPORT)),TRUE)
FLASH_PARTITIONS  += $(MODEL_PART)
endif
FLASH_PARTITIONS  := $(shell echo '$(FLASH_PARTITIONS)' | sed 's/,\s*$$//' | tr -d ' ')
ROOTFS_TYPE       := ext2
ROOTFS_DEV        := /dev/axramdisk
RAMDISK_BOOTARGS  := "$(OS_MEM) console=ttyS0,115200n8 loglevel=4 earlycon=uart8250,mmio32,0x4880000 board_id=0x0,boot_reason=0x00,noinitrd \
root=$(ROOTFS_DEV) rw rootfstype=$(ROOTFS_TYPE) init=/linuxrc mtdparts=spi4.0:$(FLASH_PARTITIONS)"
KERNEL_BOOTARGS   := $(RAMDISK_BOOTARGS)
ifeq ($(strip $(SUPPORT_RECOVERY)),TRUE)
RECOVERY_BOOTARGS := "$(OS_MEM) console=ttyS0,115200n8 loglevel=8 earlycon=uart8250,mmio32,0x4880000 rdinit=/linuxrc \
mtdparts=spi4.0:$(FLASH_PARTITIONS)"
endif

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
RAWDATA_FLASH_POS             := $(call calculate_flash_base,$(FLASH_PARTITIONS),rawdata)
RISCV_HEADER_FLASH_BASE       := $(call calculate_flash_base,$(FLASH_PARTITIONS),riscv)
RAMDISK_FLASH_POS             := $(call calculate_flash_base,$(FLASH_PARTITIONS),rootfs)
MODEL_FLASH_POS               := $(call calculate_flash_base,$(FLASH_PARTITIONS),model)
