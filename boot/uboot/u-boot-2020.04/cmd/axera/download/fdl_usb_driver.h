/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRIVERS_USB_DWC3_GADGET_H
#define __DRIVERS_USB_DWC3_GADGET_H

#include <common.h>
#include <asm/io.h>


#define EP_OUT_NUM	2
#define EP_IN_NUM	2
#define DWC3_ENDPOINTS_NUM	(EP_OUT_NUM+EP_IN_NUM)
#define DWC3_EVENT_SIZE		4	/* bytes */
#define DWC3_EVENT_MAX_NUM	(8*DWC3_ENDPOINTS_NUM)	/*it can NOT too small, or will overflow */
#define DWC3_EVENT_BUFFERS_SIZE	(DWC3_EVENT_SIZE * DWC3_EVENT_MAX_NUM)


#define DWC3_GLOBALS_REGS_START	0xc100
#define DWC3_DEV_REG_OFFSET		0xC700
/* Device Registers */
#define DWC3_DCFG		0xc700
#define DWC3_DCTL		0xc704
#define DWC3_DEVTEN		0xc708
#define DWC3_DSTS		0xc70c

#define DWC3_OUT_EP_REG_OFFSET	0xC800
#define DWC3_IN_EP_REG_OFFSET	0xC810

#define DWC3_GEVNTADRLO(n)	(0xc400 + (n * 0x10))
#define DWC3_GEVNTADRHI(n)	(0xc404 + (n * 0x10))
#define DWC3_GEVNTSIZ(n)	(0xc408 + (n * 0x10))
#define DWC3_GEVNTCOUNT(n)	(0xc40c + (n * 0x10))
#define DWC3_GEVNTCOUNT_MASK	0xffff
/* Global Event Size Registers */
#define DWC3_GEVNTSIZ_INTMASK		(1 << 31)
#define DWC3_GEVNTSIZ_SIZE(n)		((n) & 0xffff)

#define DWC3_DSTS_CONNECTSPD		(7 << 0)

/* TRB Length, PCM and Status */
#define DWC3_TRB_SIZE_MASK	(0x00ffffff)
#define DWC3_TRB_SIZE_LENGTH(n)	((n) & DWC3_TRB_SIZE_MASK)

/* TRB Control */
#define DWC3_TRB_CTRL_HWO		(1 << 0)
#define DWC3_TRB_CTRL_LST		(1 << 1)
#define DWC3_TRB_CTRL_CHN		(1 << 2)
#define DWC3_TRB_CTRL_CSP		(1 << 3)
#define DWC3_TRB_CTRL_TRBCTL(n)		(((n) & 0x3f) << 4)
#define DWC3_TRB_CTRL_IOC		(1 << 11)
#define DWC3_TRBCTL_NORMAL		DWC3_TRB_CTRL_TRBCTL(1)

#define DWC3_DEPCMDPAR2(n)	(0xc800 + (n * 0x10))
#define DWC3_DEPCMDPAR1(n)	(0xc804 + (n * 0x10))
#define DWC3_DEPCMDPAR0(n)	(0xc808 + (n * 0x10))
#define DWC3_DEPCMD(n)		(0xc80c + (n * 0x10))

#define DWC3_DEPCMD_CMDACT		(1 << 10)
#define DWC3_DEPCMD_CMDIOC		(1 << 8)

#define USB_SPEED_HIGH		0
#define USB_SPEED_FULL		1
#define USB_SPEED_SUPER	    4

#define EVENTCNT_CNT_BITS 0x0000ffff
#define DEPEVT_EPNUM_BITS 0x0000003e
#define DEPEVT_EPNUM_SHIFT 1
#define DEPEVT_INTTYPE_BITS 0x000003c0
#define DEPEVT_INTTYPE_SHIFT 6
#define DEPEVT_XFER_CMPL 1
#define DEPEVT_XFER_IN_PROG 2

#define DWC3_DEPCMD_DEPSTARTCFG		(0x09 << 0)
#define DWC3_DEPCMD_ENDTRANSFER		(0x08 << 0)
#define DWC3_DEPCMD_UPDATETRANSFER	(0x07 << 0)
#define DWC3_DEPCMD_STARTTRANSFER	(0x06 << 0)

#define EVENT_NON_EP_BIT 0x01

typedef struct dwc3_trb {
	u32		bpl;
	u32		bph;
	u32		size;
	u32		ctrl;
} dwc3_trb_t;

struct dwc3_gadget_ep_cmd_params {
	u32	param2;
	u32	param1;
	u32	param0;
};

struct dwc3_event_buffer {
	void			*buf;
	unsigned		length;
	unsigned int		lpos;
	unsigned int		count;
	unsigned int		flags;
#define DWC3_EVENT_PENDING	(1UL << 0)
	dma_addr_t		dma;
};

struct dwc3_device;

typedef struct usb_ep {
	struct dwc3_device *dev;
	u8 num;
	u8 type;
	u16 maxpacket;
	unsigned int xfer_started	: 1;
	unsigned int is_in		: 1;
	unsigned int active		: 1;
	dwc3_trb_t *dma_desc;
} usb_ep_t;

typedef struct usb3_dev_ep {
	usb_ep_t ep;
} usb3_dev_ep_t;

typedef struct dwc3_device {
	unsigned long reg_base;
	u8 speed;
	u32 recv_len;
	u32 send_len;
	dma_addr_t recv_buf;
#define EVENT_BUF_SIZE	64	// size in dwords
	u32 *event_ptr; // save pending event addr
	u32 *event_buf;
	u32 event_size;
	usb3_dev_ep_t *out_ep, *in_ep;
} dwc3_device_t;

#endif
