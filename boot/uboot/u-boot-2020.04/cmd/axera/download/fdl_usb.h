#ifndef __USB_H__
#define __USB_H__


/* important!
in current, ep-out trb has some problem with dcache when usb dma update it
if define DMA_ALLOC_COHERENT, ep-out trb use dma_alloc_coherent to fix dcache bug of ep-out trb
otherwise, we need to call disable_dcache in usb_open() to fix it
by the way, ep-in trb and send & recv buf with dcache are ok!
*/
#define DMA_ALLOC_COHERENT

//#define DISABLE_DCACHE


int usb_recv(u8* buf, u32 len, u32 timeout_ms);
int usb_send(unsigned char *buf, u32 len, u32 timeout_ms);
int axera_usb_init(void);

#endif