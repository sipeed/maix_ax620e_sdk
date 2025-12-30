#
# Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include drivers/arm/gic/v2/gicv2.mk
include lib/xlat_tables_v2/xlat_tables.mk

AX_PLAT			:=	plat/axera
AX_PLAT_SOC		:=	${AX_PLAT}/${PLAT}

#DISABLE_BIN_GENERATION	:=	1

PLAT_INCLUDES		:=	-Idrivers/arm/gic/common/			\
				-Idrivers/arm/gic/v2/			\
				-I${AX_PLAT_SOC}/				\
				-I${AX_PLAT_SOC}/include/

AX_GIC_SOURCES		:=	${GICV2_SOURCES}				\
				${XLAT_TABLES_LIB_SRCS}				\
				plat/common/plat_gicv2.c			\
				${AX_PLAT_SOC}/ax620e_gicv2.c

PLAT_BL_COMMON_SOURCES	:=	plat/common/aarch64/crash_console_helpers.S

BL31_SOURCES		+=	${AX_GIC_SOURCES}				\
				drivers/ti/uart/aarch64/16550_console.S		\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				lib/cpus/aarch64/cortex_a53.S			\
				plat/common/plat_psci_common.c			\
				${AX_PLAT_SOC}/drivers/pmu/pmu.c		\
				${AX_PLAT_SOC}/ax620e_bl31_setup.c 		\
				${AX_PLAT_SOC}/ax620e_topology.c		\
				${AX_PLAT_SOC}/drivers/soc/soc.c 		\
				${AX_PLAT_SOC}/ax620e_pm.c			\
				${AX_PLAT_SOC}/ax620e_pwrc.c			\
				${AX_PLAT_SOC}/drivers/wakeup/wakeup.c 		\
				${AX_PLAT_SOC}/drivers/wakeup/wakeup_source.c 	\
				${AX_PLAT_SOC}/drivers/flash_sys/flash_sys.c 	\
				${AX_PLAT_SOC}/drivers/isp_sys/isp_sys.c 	\
				${AX_PLAT_SOC}/drivers/mm_sys/mm_sys.c 		\
				${AX_PLAT_SOC}/drivers/npu_sys/npu_sys.c 	\
				${AX_PLAT_SOC}/drivers/vpu_sys/vpu_sys.c 	\
				${AX_PLAT_SOC}/drivers/chip_top/chip_top.c 	\
				${AX_PLAT_SOC}/drivers/periph_sys/periph_sys.c  \
				${AX_PLAT_SOC}/drivers/cpu_sys/cpu_sys.c 	\
				${AX_PLAT_SOC}/drivers/ddr_sys/ddr_sys.c 	\
				${AX_PLAT_SOC}/drivers/firewall/firewall.c 	\
				${AX_PLAT_SOC}/drivers/sema/sema.c 	\
				${AX_PLAT_SOC}/drivers/timestamp/timestamp.c 	\
				${AX_PLAT_SOC}/aarch64/ax620e_helpers.S		\
				${AX_PLAT_SOC}/aarch64/ax620e_on_ram_func.S

BL31_CPPFLAGS	+=	${EXT_CFLAGS}
HW_ASSISTED_COHERENCY := 0
USE_COHERENT_MEM    := 0

# Do not enable SVE
ENABLE_SVE_FOR_NS	:= 0
ENABLE_ASSERTIONS	:= 0

COLD_BOOT_SINGLE_CPU := 1

#SEPARATE_CODE_AND_RODATA  todo
#ERRATA_A55_1530923 := 1
ERRATA_A53_1530924 := 1

#spin_lock problem but useless
#ARM_ARCH_MAJOR          := 8
#ARM_ARCH_MINOR          := 4
#USE_SPINLOCK_CAS := 1

ARM_LINUX_KERNEL_AS_BL33 := 1
$(eval $(call assert_boolean,ARM_LINUX_KERNEL_AS_BL33))
$(eval $(call add_define,ARM_LINUX_KERNEL_AS_BL33))
