#ifndef MC20E_E907_INTERRUPT_H
#define MC20E_E907_INTERRUPT_H

#include <core_rv32.h>
#include <rthw.h>
#include <stdbool.h>

struct MC20E_RISCV_INT_REG
{
    __IOM uint32_t int_mask_riscv0;
    __OM uint32_t int_mask_riscv0_set;
    __OM uint32_t int_mask_riscv0_clr;
    __IOM uint32_t int_mask_riscv1;
    __OM uint32_t int_mask_riscv1_set;
    __OM uint32_t int_mask_riscv1_clr;
    __IOM uint32_t int_mask_riscv2;
    __OM uint32_t int_mask_riscv2_set;
    __OM uint32_t int_mask_riscv2_clr;
    __IOM uint32_t int_mask_riscv3;
    __OM uint32_t int_mask_riscv3_set;
    __OM uint32_t int_mask_riscv3_clr;

    __I uint32_t int_raw_riscv0;
    __I uint32_t int_raw_riscv1;
    __I uint32_t int_raw_riscv2;
    __I uint32_t int_raw_riscv3;

    __I uint32_t int_sta_riscv0;
    __I uint32_t int_sta_riscv1;
    __I uint32_t int_sta_riscv2;
    __I uint32_t int_sta_riscv3;

    __IOM uint32_t riscv_err_resp2int_ctrl;
    __OM uint32_t riscv_err_resp2int_ctrl_set;
    __OM uint32_t riscv_err_resp2int_ctrl_clr;
    __I uint32_t riscv_err_resp2int_status;

    __I uint32_t riscv_reset_status;
};

typedef enum MC20E_IRQn {
    INT_DDR_CTRL_INT                     	=   0,
    INT_SEC_INT_FIREWALL                        =   1,

    INT_LP_CTRL_INT                         	=   2,

    INT_REQ_REERVED0                            =   3,
    INT_REQ_REERVED1                            =   4,

    INT_REQ_ISP_IFE2RISCV                       =   5,
    INT_REQ_ISP_ITP2RISCV                       =   6,

    INT_REQ_REERVED2                            =   7,
    INT_REQ_REERVED3                            =   8,
    INT_REQ_REERVED4                            =   9,

    INT_REQ_NPU0                       		=   10,
    INT_REQ_NPU_TMR                       	=   11,
    INT_REQ_NPU_FAB_ERR                       	=   12,
    INT_REQ_NPU_EU_ERR0                       	=   13,
    INT_REQ_NPU_PSLV_ERR                       	=   14,

    INT_TMR_SYNC                       		=   15,

    INT_REQ_REERVED5                            =   16,
    INT_REQ_REERVED6                            =   17,

    INT_REQ_RISCV_RSP_ERR                       =   18,
    INT_REQ_COMMON_FAB                       	=   19,

    INT_RISCV_REMAP_IRQ                       	=   20,
    INT_RISCV_DBG_MNR_IRQ                       =   21,
    INT_RISCV_PERF_MNR_IRQ                      =   22,

    INT_REQ_SEMA                      		=   23,

    INT_WAKEUP_EIC                      	=   24,
    INT_TIMER32_INTR                      	=   25,
    INT_TIMER64_INTR                      	=   26,
    INT_WDT1_INTR                      		=   27,

    INT_DEB_GPIO_INT                      	=   28,
    INT_DEB_GPIO_LP_INT                      	=   29,
    INT_PMU_INT_REQ                      	=   30,

    INT_SW_INT_O_0				=   31,
    INT_SW_INT_O_1				=   32,
    INT_SW_INT_O_2				=   33,
    INT_SW_INT_O_3				=   34,

    INT_USB_WAKE_UP				=   35,

    INT_REQ_AXERA_DMA				=   38,
    INT_REQ_AXERA_DMA_CFG			=   39,

    INT_GZIPD_INT				=   40,
    INT_MAILBOX_INT2				=   41,

    INT_REQ_AHB_SSI_S				=   42,

    INT_JENC_VPU_INT				=   46,
    INT_VDEC_VPU_INT				=   47,
    INT_VENC_VPU_INT				=   48,

    INT_FAB_MM_INT				=   51,
    INT_DPU_MM_INT				=   52,
    INT_DPU_LITE_MM_INT				=   53,
    INT_TDP_MM_INT				=   54,
    INT_CDMA_MM_INT				=   55,

    INT_IVE_MM_INT				=   56,
    INT_VGP_MM_INT				=   57,
    INT_VPP_MM_INT				=   58,
    INT_GDC_MM_INT				=   59,
    INT_AXI2CSI_MM_INT				=   60,

    INT_REQ_AXERA_DMA_PER			=   64,
    INT_REQ_PERIPH_NSEC_GPIO0			=   65,
    INT_REQ_PERIPH_NSEC_GPIO1			=   66,
    INT_REQ_PERIPH_NSEC_GPIO2			=   67,
    INT_REQ_PERIPH_NSEC_GPIO3			=   68,

    INT_REQ_PERIPH_UART0			=   69,
    INT_REQ_PERIPH_UART1			=   70,
    INT_REQ_PERIPH_UART2			=   71,
    INT_REQ_PERIPH_UART3			=   72,
    INT_REQ_PERIPH_UART4			=   73,
    INT_REQ_PERIPH_UART5			=   74,

    INT_REQ_PERIPH_TMR0_0			=   75,
    INT_REQ_PERIPH_TMR0_1			=   76,
    INT_REQ_PERIPH_TMR0_2			=   77,
    INT_REQ_PERIPH_TMR0_3			=   78,

    INT_REQ_PERIPH_TMR1_0			=   79,
    INT_REQ_PERIPH_TMR1_1			=   80,
    INT_REQ_PERIPH_TMR1_2			=   81,
    INT_REQ_PERIPH_TMR1_3			=   82,

    INT_REQ_PERIPH_PWM0				=   83,
    INT_REQ_PERIPH_PWM1				=   84,
    INT_REQ_PERIPH_PWM2				=   85,

    INT_REQ_PERIPH_I2C0				=   86,
    INT_REQ_PERIPH_I2C1				=   87,
    INT_REQ_PERIPH_I2C2				=   88,
    INT_REQ_PERIPH_I2C3				=   89,
    INT_REQ_PERIPH_I2C4				=   90,
    INT_REQ_PERIPH_I2C5				=   91,
    INT_REQ_PERIPH_I2C6				=   92,
    INT_REQ_PERIPH_I2C7				=   93,

    INT_REQ_PERIPH_I2C_S0			=   94,
    INT_REQ_PERIPH_I2C_S1			=   95,
    INT_REQ_PERIPH_I2C_S			=   96,
    INT_REQ_PERIPH_I2C_TDM_S			=   97,
    INT_REQ_PERIPH_I2C_M			=   98,
    INT_REQ_PERIPH_I2C_TDM_M			=   99,

    INT_REQ_PERIPH_APB_SSI_M0			=  100,
    INT_REQ_PERIPH_APB_SSI_M1			=  101,
    INT_REQ_PERIPH_APB_SSI_M2			=  102,

    INT_REQ_PERIPH_TMR32_0			=  103,
    INT_REQ_PERIPH_TMR32_1			=  104,
    INT_REQ_PERIPH_TMR32_2			=  105,
    INT_REQ_PERIPH_TMR32_3			=  106,

    INT_REQ_PERIPH_TMR64_0			=  107,
    INT_REQ_PERIPH_TMR64_1			=  108,

    INT_REQ_PERIPH_SEC_GPIO0			=  109,
    INT_REQ_PERIPH_SEC_GPIO1			=  110,
    INT_REQ_PERIPH_SEC_GPIO2			=  111,
    INT_REQ_PERIPH_SEC_GPIO3			=  112,

    INT_REQ_MAX,

}MC20E_RISCV_IRQn_Type;

rt_isr_handler_t mc20e_e907_interrupt_install(int vector, rt_isr_handler_t handler, void *param, const char *name);
void mc20e_e907_interrupt_umask(int vector);
void mc20e_e907_interrupt_mask(int vector);
int mc20e_e907_interrupt_get_mask(int vector, bool *mask_state);
#endif
