#
# SPDX-License-Identifier:     GPL-2.0+
#
ifneq ($(cpu_clk),)
KBUILD_CPPFLAGS += -DCFG_CPU_CLK_CMDLINE
KBUILD_CPPFLAGS += -DCPU_CLK=$(cpu_clk)
endif

obj-y += ax620e.o
obj-y += timer.o
obj-y += board.o
obj-y += chip_config.o
obj-y += pll_config.o
obj-$(CONFIG_PWM_AXERA) += common/
