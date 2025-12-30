#include <common.h>
#include <pwm.h>
#include <command.h>
#include <env.h>
#include <errno.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <dm/uclass.h>

int do_pwm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    struct udevice *dev;
    uint dev_number = 0;
    uint chan_number = 0;
    uint period = 0;
    uint duty = 0;

    if (argc < 5) {
        debug("usage: pwm <device number(start with 0)> \
                <channel number(start with 0)> <period> <duty>\n");
        return -1;
    }

    dev_number = simple_strtoul(argv[1], NULL, 10);
    chan_number = simple_strtoul(argv[2], NULL, 10);
    period = simple_strtoul(argv[3], NULL, 10);
    duty = simple_strtoul(argv[4], NULL, 10);

    uclass_get_device(UCLASS_PWM, dev_number, &dev);
    pwm_set_config(dev, chan_number, period, duty);
    pwm_set_enable(dev, chan_number, true);

    return 0;
}

U_BOOT_CMD(
	pwm,	5,	1,	do_pwm,
	"pwm config", "pwm <device number(start with 0)> \
        <channel number(start with 0)> <period> <duty>\n"
);
