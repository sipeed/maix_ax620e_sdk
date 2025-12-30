# The address of the text segment in the startup phase, do not recommend modification.
IMG_HEADER_SIZE                 := 1024
DDRINIT_PARAM_HEADER_BASE       := 0x03200000
ATF_IMG_HEADER_BASE             := 0x4003FC00
ATF_IMG_ADDR                    := 0x40040000
ATF_IMG_PKG_SIZE                := 0x40000
UBOOT_IMG_HEADER_BASE           := 0x5C000000
ifeq ($(strip $(SUPPORT_OPTEE)),TRUE)
OPTEE_IMAGE_ADDR                := 0x44200000
OPTEE_RESERVED_SIZE             := 0x2000000
OPTEE_SHMEM_SIZE                := 0x200000
endif
AXERA_DTB_IMG_ADDR              := 0x40001000
DTB_IMG_HEADER_ADDR             := ($(AXERA_DTB_IMG_ADDR) - $(IMG_HEADER_SIZE))
AXERA_KERNEL_IMG_ADDR           := 0x40280000
KERNEL_IMG_HEADER_ADDR          := ($(AXERA_KERNEL_IMG_ADDR) - $(IMG_HEADER_SIZE))
############################################################################################
# Note: The above configuration is generally not modified.

#partition size config
SPL_PARTITION_SIZE        := 768K
DDRINIT_PARTITION_SIZE    := 512K
ATF_PARTITION_SIZE        := 256K
ATF_B_PARTITION_SIZE      := 256K
UBOOT_PARTITION_SIZE      := 1536K
UBOOT_B_PARTITION_SIZE    := 1536K
ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
RAWDATA_PARTITION_SIZE    := 128K
endif
ENV_PARTITION_SIZE        := 1M
PARAM_PARTITION_SIZE      := 5M
LOGO_PARTITION_SIZE       := 6M
DTB_PARTITION_SIZE        := 1M
KERNEL_PARTITION_SIZE     := 64M
ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
RISCV_PARTITION_SIZE      := 256K
endif
SOC_PARTITION_SIZE        := 1024M
CUSTOMER_PARTITION_SIZE   := 6144M
AUTO_FIT_PARTITION        := CUSTOMER

# env part size, size is equal to ENV_PARTITION_SIZE
ENV_IMG_PKG_SIZE          := 0x100000

# Dram base address and size
SYS_DRAM_BASE             := 0x40000000
SYS_DRAM_SIZE             := 2048 #MB

# linux OS memory config
OS_MEM_SIZE               := 1024 #MB
OS_MEM                    := mem=$(strip $(OS_MEM_SIZE))M

ISP_IMAGE_LEN_MB          := 59 #MB
ISP_IMAGE_LEN_B           := $(shell printf "0x%X" $$(($(ISP_IMAGE_LEN_MB)*1048576)))
ISP_IMAGE_DDR_END         := $(call AddAddressMB, $(SYS_DRAM_BASE), $(OS_MEM_SIZE))
ISP_IMAGE_DDR_START       := $(call SubAddressMB, $(ISP_IMAGE_DDR_END), $(ISP_IMAGE_LEN_MB))

RISCV_MEM_SIZE_MB         := 3 #MB
RISCV_MEM_SIZE_B          := $(shell printf "0x%X" $$(($(RISCV_MEM_SIZE_MB)*1048576)))
RISCV_DDR_END_ADDR        := $(ISP_IMAGE_DDR_START)
RISCV_BIN_DDR_START       := $(call SubAddressMB, $(RISCV_DDR_END_ADDR), $(RISCV_MEM_SIZE_MB))
RISCV_HEADER_DDR_START    := $(call SubAddressB, $(RISCV_BIN_DDR_START), $(SEC_SIGN_HEADER_SIZE))

RISCV_LOG_MEM_RESERVE_START  := $(call SubAddressB, $(RISCV_BIN_DDR_START), 4096)
RISCV_LOG_MEM_RESERVE_SIZE   := 4096

# cmm memory config
CMM_START_ADDR            := $(call AddAddressMB, $(SYS_DRAM_BASE), $(OS_MEM_SIZE))
CMM_SIZE                  := $(shell echo $$(($(SYS_DRAM_SIZE) - $(OS_MEM_SIZE))))
CMM_POOL_PARAM            := anonymous,0,$(strip $(CMM_START_ADDR)),$(CMM_SIZE)M

# uboot boot args
include $(PRJECT_DIR)/partition_tbl.mak
FLASH_PARTITIONS  := $(SPL_PART),
ifeq ($(strip $(SUPPORT_DDRINIT_PART)),TRUE)
FLASH_PARTITIONS  += $(DDRINIT_PART),
endif
FLASH_PARTITIONS  += $(ATF_PART),$(ATF_B_PART),$(UBOOT_PART),$(UBOOT_B_PART),$(ENV_PART),
ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
FLASH_PARTITIONS  += $(RAWDATA_PART),$(RISCV_PART),
endif
FLASH_PARTITIONS  += $(LOGO_PART),$(DTB_PART),$(KERNEL_PART),
FLASH_PARTITIONS  += $(PARAM_PART),$(SOC_PART),$(CUSTOMER_PART)
FLASH_PARTITIONS  := $(shell echo '$(FLASH_PARTITIONS)' | tr -d ' ')
#ROOTFS_TYPE       := ext4
#ROOTFS_DEV        := /dev/mmcblk0p13
KERNEL_BOOTARGS   := "$(OS_MEM) console=ttyS0,115200n8 loglevel=1 earlycon=uart8250,mmio32,0x4880000 board_id=0x0,boot_reason=0x00,initcall_debug=0 \
usbcore.autosuspend=-1 rdinit=/linuxrc rw blkdevparts=mmcblk0:$(FLASH_PARTITIONS)"

# Calculates the position of each partition in flash
DDRINIT_HEADER_FLASH_BASE     := $(call calculate_flash_base,$(FLASH_PARTITIONS),ddrinit)
ATF_A_HEADER_FLASH_BASE       := $(call calculate_flash_base,$(FLASH_PARTITIONS),atf)
ATF_B_HEADER_FLASH_BASE       := $(call calculate_flash_base,$(FLASH_PARTITIONS),atf_b)
UBOOT_A_HEADER_FLASH_BASE     := $(call calculate_flash_base,$(FLASH_PARTITIONS),uboot)
UBOOT_B_HEADER_FLASH_BASE     := $(call calculate_flash_base,$(FLASH_PARTITIONS),uboot_b)
ENV_DATA_FLASH_BASE           := $(call calculate_flash_base,$(FLASH_PARTITIONS),env)
ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
RAWDATA_FLASH_POS             := $(call calculate_flash_base,$(FLASH_PARTITIONS),rawdata)
endif
ifeq ($(strip $(SUPPORT_OPTEE)),TRUE)
OPTEE_A_HEADER_FLASH_BASE     := $(call calculate_flash_base,$(FLASH_PARTITIONS),optee)
OPTEE_B_HEADER_FLASH_BASE     := 0
endif
DTB_A_HEADER_FLASH_BASE       := $(call calculate_flash_base,$(FLASH_PARTITIONS),dtb)
DTB_B_HEADER_FLASH_BASE       := 0
KERNEL_A_HEADER_FLASH_BASE    := $(call calculate_flash_base,$(FLASH_PARTITIONS),kernel)
KERNEL_B_HEADER_FLASH_BASE    := 0
ifeq ($(strip $(AX_RISCV_SUPPORT)),TRUE)
RISCV_HEADER_FLASH_BASE       := $(call calculate_flash_base,$(FLASH_PARTITIONS),riscv)
endif

XML_IMG_OPTION := "param:select=1", "param:flag=1"

# DDR retrain config
DDR_RETRAIN_SIZE             := 0x1000
DDR_RETRAIN_START            := $(SYS_DRAM_BASE)