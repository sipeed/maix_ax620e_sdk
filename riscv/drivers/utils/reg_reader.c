#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static void print_usage()
{
    rt_kprintf("Usage:reg_reader r/w addr [value]\n"
                "      r/w:        read reg or write reg\n"
                "      addr:       reg addr, start with '0x'\n"
                "      value:      write value, start with '0x'\n");
}

static rt_uint8_t ascii_to_hex(rt_uint8_t number)
{
    if(number >= '0' && number <= '9') {
        return (number - 0x30);
    } else if(number >= 'a' && number <= 'f') {
        return ((number - 'a') + 10);
    } else if(number >= 'A' && number <= 'F') {
        return ((number - 'A') + 10);
    }
    return 0;
}

static rt_int32_t string_to_hexbuff(char* str, rt_uint8_t *hexbuff, rt_int32_t len)
{
    if(str == RT_NULL || hexbuff == RT_NULL) {
        rt_kprintf("str or hexbuff is null\n");
        return -1;
    }
    if((len * 2) < rt_strlen(str + 2)) {
        rt_kprintf("len is minus\n");
        return -1;
    }

    char high = 0;
    char low = 0;
    rt_int32_t  count = 0;
    char *p = RT_NULL;
    rt_int32_t  sl = 0;
    char tmp[256] = {0};

    p = str + 2;
    sl = rt_strlen(p);
    if(sl % 2) {
        rt_snprintf(tmp, 256, "0%s", p);
        p = tmp;
        sl = sl + 1;
    }
    rt_kprintf("src string = %s\n", p);
    while(count < (sl / 2)) {
        high = ascii_to_hex(*p);
        low = ascii_to_hex(*(++p));
        hexbuff[count] = ((high & 0x0f) << 4 | (low & 0x0f));
        p++;
        count++;
    }

    return count;
}

#define REG32_READ(addr)                               *((volatile rt_uint32_t *)(addr));
#define REG32_WRITE(addr, data)                        *((volatile rt_uint32_t *)(addr)) = data;

int reg_reader(int argc, char * argv[])
{
    uint32_t phy_addr = 0;
    uint32_t val = 0;
    uint8_t tmp[4] = {0};
    rt_int32_t is_write = 0;
    uint32_t data = 0;
    int32_t count = 0;

    if(argc < 3) {
        print_usage();
        return 0;
    }
    if(rt_strncmp(argv[1], "w", 10) == 0) {
        is_write = 1;
    }
    string_to_hexbuff(argv[2], tmp, 4);
    phy_addr = (tmp[0] << 24) + (tmp[1] << 16) + (tmp[2] << 8) + tmp[3];

    if(is_write == 0) {
        val = REG32_READ(phy_addr);
        rt_kprintf("addr 0x%lX, val 0x%lX\n", phy_addr, val);
    }else {
        count = string_to_hexbuff(argv[3], tmp, 4);
        if(count == 4) {
            data = (tmp[0] << 24) + (tmp[1] << 16) + (tmp[2] << 8) + tmp[3];
        } else if(count == 3) {
            data = (tmp[0] << 16) + (tmp[1] << 8) + tmp[2];
        } else if(count == 2) {
            data = (tmp[0] << 8) + tmp[1];
        } else {
            data = tmp[0];
        }
        rt_kprintf("write addr=0x%x, val=0x%x\n",  phy_addr, data);
        REG32_WRITE(phy_addr, data);
    }
    return 0;
}

MSH_CMD_EXPORT(reg_reader, r/w reg value);
