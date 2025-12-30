#include "config.h"
#ifdef TEST_USB_IN_HAPS

#include "fdl_usb.h"
#include "printf.h"
#include "timer.h"

#define RECV_LEN 4096
unsigned char recv_buf[RECV_LEN] __attribute__ ((aligned(8)));


void test_usb(void)
{
    int recv_actual = 0;
    int ret = 0;

    info("test fdl1 usb dev...\r\n");
    debug("recv_buf addr: %p\r\n", recv_buf);

    ret = axera_usb_init();
    if(ret) {
        err("usb init failed\r\n");
        return;
    }

    while(1){
        recv_actual = usb_recv(recv_buf, RECV_LEN, 0);
        if(recv_actual > 0)
            usb_send(recv_buf, recv_actual, 1000);
    }

    return ;
}

int main(void)
{
    print_init();
    timer_init();

    test_usb();
}

#endif