#include <common.h>
#include <command.h>
#include <fdl_channel.h>
#include <cpu_func.h>

#include "fdl_usb.h"


static int Usb_Open(struct FDL_ChannelHandler  *channel)
{
#ifdef DISABLE_DCACHE
	printf("warning: disable dcache!!\n");
	dcache_disable();
#endif
	axera_usb_init();
	return 0;
}


static int __attribute__((unused)) Usb_Close(struct FDL_ChannelHandler  *channel)
{
#ifdef DISABLE_DCACHE
	dcache_enable();
#endif
	return 0;
}


static int Usb_Read(struct FDL_ChannelHandler  *channel, u8 *buf, u32 len)
{
	return usb_recv((unsigned char *)buf, len, 0);
}


static u32 Usb_Write(struct FDL_ChannelHandler *channel, const u8 *buf, u32 len)
{
	usb_send((unsigned char *)buf, len, 0);
	return len;
}


struct FDL_ChannelHandler gUsbChannel = {
	.channel = DL_CHAN_USB,
	.open = Usb_Open,
	.read = Usb_Read,
	.write = Usb_Write,
};


/*test fdl usb data transfer function*/
void test_fdl_usb(void)
{
	#define RECV_LEN 10000
	u8 recv_buff[RECV_LEN] = {0};
	int  len = 0;
	u8 test_str[] = "hello, i'm fdl2 usb device";

	printf("test fdl usb\n");
	gUsbChannel.open(&gUsbChannel);
	gUsbChannel.write(&gUsbChannel, test_str, sizeof(test_str));

	while(1)
	{
		len = gUsbChannel.read(&gUsbChannel, recv_buff, RECV_LEN);
		if(len == -1)
			return;

		if(len > 0)
		{
			recv_buff[len] = 0;
			printf("usb recv %d byte data.\n", len);
			printf("%s", recv_buff);
			printf("\r\n");
			printf("send %d byte data to host\n", len);
			gUsbChannel.write(&gUsbChannel, recv_buff, len);
		}
	}
}


static int do_testfdlusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	test_fdl_usb();
	return 0;
}


U_BOOT_CMD(
	testusb, 1, 0, do_testfdlusb,
	"test fdl usb2 device data transfer function, loopback mode",
	""
);