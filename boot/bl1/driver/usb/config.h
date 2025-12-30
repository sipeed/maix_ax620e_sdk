#ifndef __CONFIG_H_
#define __CONFIG_H_


/* important!
in current, ep-out trb has some problem with dcache when usb dma update it
if define DMA_ALLOC_COHERENT, ep-out trb use dma_alloc_coherent to fix dcache bug of ep-out trb
otherwise, we need to call disable_dcache in usb_open() to fix it
by the way, ep-in trb and send & recv buf with dcache are ok!
*/
// #define DMA_ALLOC_COHERENT

//#define DISABLE_DCACHE


// test usb device raw data transfer in haps
// #define TEST_USB_IN_HAPS  Y
// #define RUN_IN_UBOOT

//dwc3 regsiter base addr
#define DWC3_REGISTER_BASE	0x8000000


#define ZEBU	1
#define HAPS	2
#define ASIC	3

// ZEBU or HAPS or ASIC
#define	TEST_PLATFORM	ASIC

#endif
