#ifndef _BOARD_H_
#define _BOARD_H_

// #define VPLL0_CONFIG_850M

u32 get_boot_mode(u32 chip_mode);
u32 get_boot_voltage(void);
int chip_init(void);
void wtd_enable(u8 enable);
void pin_power_detect();
u32 get_boot_index(void);
void set_boot_index(int val);
u32 get_ota_flag(void);
u32 get_boot_time(void);
void set_boot_time(int val);
u32 get_emmc_voltage(void);
int generic_timer_init(void);
void get_misc_info(void);
void set_pwm_volt(void);
#endif
