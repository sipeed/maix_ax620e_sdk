PLATFORM_FLAVOR ?= ax620e

include core/arch/arm/cpu/cortex-armv8-0.mk

$(call force,CFG_WARN_INSECURE,n)
$(call force,CFG_TEE_CORE_NB_CORE,2)
$(call force,CFG_CORE_CLUSTER_SHIFT,0)
$(call force,CFG_WITH_ARM_TRUSTED_FW,y)
$(call force,CFG_8250_UART,y)
$(call force,CFG_CORE_ASLR,n)
ifeq ($(CFG_ARM64_core),y)
$(call force,CFG_CORE_LARGE_PHYS_ADDR,y)
supported-ta-targets = ta_arm64
endif
#ifeq ($(CFG_ARM64_core),y)
#$(call force,CFG_WITH_LPAE,y)
#else
#$(call force,CFG_ARM32_core,y)
#endif
$(call force,CFG_CORE_ARM64_PA_BITS,36)
$(call force,CFG_LPAE_ADDR_SPACE_BITS,36)
$(call force,CFG_ARM64_core,y)
$(call force,CFG_WITH_LPAE,y)

$(call force,CFG_GIC,y)
$(call force,CFG_SECURE_TIME_SOURCE_CNTPCT,y)

CFG_WITH_STACK_CANARIES ?= y
CFG_AX_EFUSE ?= y
# Overrides default in mk/config.mk with 128 kB
CFG_CORE_HEAP_SIZE ?= 131072
CFG_AXERA_CIPHER ?= y
CFG_HW_UNQ_KEY_REQUEST ?= y
ifeq ($(CFG_AXERA_CIPHER),y)
$(call force,CFG_WITH_SOFTWARE_PRNG,n)
endif
ifneq ($(PLAT_OPTEE_ADDR_START),)
core-platform-cppflags += -DTZDRAM_BASE="$(PLAT_OPTEE_ADDR_START)"
endif

ifneq ($(PLAT_OPTEE_SHMEM_SIZE),)
core-platform-cppflags += -DTEE_SHMEM_SIZE="$(PLAT_OPTEE_SHMEM_SIZE)"
endif

ifneq ($(PLAT_OPTEE_RESERVED_SIZE),)
core-platform-cppflags += -DTEE_RESERVED_SIZE="$(PLAT_OPTEE_RESERVED_SIZE)"
endif
