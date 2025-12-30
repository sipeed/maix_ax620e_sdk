#ifndef __USB_H__
#define __USB_H__


int usb_recv(unsigned char* buf, unsigned int len, unsigned int timeout_ms);
int usb_send(const unsigned char *buf, unsigned int len, unsigned int timeout_ms);
int axera_usb_init(void);

#endif