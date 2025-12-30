/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "config.h"

#ifdef RUN_IN_UBOOT
#include <common.h>
#include <linux/io.h>
#include <cpu_func.h>
#include <console.h>
#else
#include "printf.h"
#endif

#include "fdl_usb_driver.h"
#include "fdl_usb.h"

#ifdef DMA_ALLOC_COHERENT
#include <asm/dma-mapping.h>
#endif

#ifdef RUN_IN_UBOOT
#define NOOP(...)
#define debug    NOOP
#define info   NOOP
#define err    printf
#endif


#define USB2_BASE_ADDR	DWC3_REGISTER_BASE


#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))
#define lower_32_bits(n) ((u32)(n))


static dwc3_device_t g_dwc3_dev;
static int usb_out_len;
static int usb_in_len;
static usb3_dev_ep_t dwc3_epn_out, dwc3_epn_in;

#ifndef DMA_ALLOC_COHERENT
static dwc3_trb_t	d_epn_out_trb __attribute__((aligned (64)));
static dwc3_trb_t	d_epn_in_trb[2] __attribute__((aligned (64)));
#endif
char __attribute((aligned (64))) event_buffer[DWC3_EVENT_BUFFERS_SIZE];


static void usb_receive(dwc3_device_t *dev, dma_addr_t addr, int len);


#ifdef RUN_IN_UBOOT
static void dwc3_flush_cache(long addr, long length)
{
	unsigned long start = rounddown((unsigned long)addr, ARCH_DMA_MINALIGN);
	unsigned long end = roundup((unsigned long)addr + length,
				    ARCH_DMA_MINALIGN);

	debug("flush cache: start:%p, end:%p\n", start, end);
	flush_dcache_range(start, end);
}

static void dwc3_invalidate_cache(long addr, long length)
{
	unsigned long start = rounddown((unsigned long)addr, ARCH_DMA_MINALIGN);
	unsigned long end = roundup((unsigned long)addr + length,
				    ARCH_DMA_MINALIGN);

	debug("invalidate cache: start:%p, end:%p\n", start, end);
	invalidate_dcache_range(start, end);
}
#endif


int dwc3_send_ep_cmd(dwc3_device_t *dev, unsigned ep, unsigned cmd,
		 struct dwc3_gadget_ep_cmd_params *params)
{
	u32 timeout = 5000;
	u32 reg;

	writel(params->param0, (void *)dev->reg_base + DWC3_DEPCMDPAR0(ep));
	writel(params->param1, (void *)dev->reg_base + DWC3_DEPCMDPAR1(ep));

	writel(cmd | DWC3_DEPCMD_CMDACT, (void *)dev->reg_base + DWC3_DEPCMD(ep));
	do {
		reg = readl((void *)dev->reg_base + DWC3_DEPCMD(ep));
		if (!(reg & DWC3_DEPCMD_CMDACT)) {
			return 0;
		}
		timeout--;
	} while (timeout > 0);

	err("send ep cmd timeout\r\n");
	return -1;
}

static void dwc3_prepare_one_trb(struct dwc3_trb *trb, dma_addr_t dma,
		unsigned length, u32 type, unsigned last)
{
	trb->size = DWC3_TRB_SIZE_LENGTH(length);
	trb->bpl = lower_32_bits(dma);
	trb->bph = upper_32_bits(dma);
	trb->ctrl = type;

	if (last)
		trb->ctrl |= DWC3_TRB_CTRL_LST;

	trb->ctrl |= DWC3_TRB_CTRL_HWO;

#ifdef RUN_IN_UBOOT
	/* Clean + Invalidate the buffers */
	// flush_dcache_range((long)trb, (long)trb + sizeof(*trb));
	// flush_dcache_range((long)dma, (long)dma + length);
	dwc3_flush_cache((long)trb, sizeof(*trb));
	dwc3_flush_cache((long)dma, length);
#endif
}

static void dwc3_process_event_buf(dwc3_device_t *dwc,
                                 int ep, u32 event)
{
	int is_in;
	usb3_dev_ep_t *dwc3_ep;

	is_in = ep & 1;
	dwc3_ep = is_in ? dwc->in_ep : dwc->out_ep;
	debug("dwc3_process_event_buf\r\n");

	switch (event & DEPEVT_INTTYPE_BITS) {
	case DEPEVT_XFER_CMPL << DEPEVT_INTTYPE_SHIFT:
		dwc3_ep->ep.xfer_started = 0;
		if (dwc3_ep == dwc->out_ep) {
			dwc3_trb_t *trb = dwc3_ep->ep.dma_desc;
			debug("ep out interrupt: DEPEVT_XFER_CMPL\n");
			// debug("ep-out trb addr: %p, trb size:%ld, recv buf addr:%p\n",
				// trb, sizeof(*trb), (void *)(trb->bpl + ((uintptr_t)trb->bph << 32)));
			debug("ep-out trb addr: %p, trb size:%ld\n", trb, sizeof(*trb));
#ifdef RUN_IN_UBOOT
			dwc3_invalidate_cache((long)trb, sizeof(dwc3_trb_t));
#endif
			usb_out_len = dwc->recv_len - (trb->size & DWC3_TRB_SIZE_MASK);
#ifdef RUN_IN_UBOOT
			if(usb_out_len > 0)
				dwc3_invalidate_cache((uintptr_t)(trb->bpl + ((uintptr_t)trb->bph << 32)), usb_out_len);
#endif
			debug("recv actual:%d bytes\r\n", usb_out_len);

			if(usb_out_len == 0)	// receive a zero-length packet from host
				usb_receive(dwc, dwc->recv_buf, dwc->recv_len); //receive again!
		}
		if (dwc3_ep == dwc->in_ep) {
			debug("ep in interrupt: DEPEVT_XFER_CMPL\r\n");
			dwc3_trb_t *trb = dwc3_ep->ep.dma_desc;
			debug("ep-in trb addr: %p, trb size:%ld\n", trb, sizeof(*trb));
#ifdef RUN_IN_UBOOT
			dwc3_invalidate_cache((long)trb, sizeof(dwc3_trb_t));
#endif
			debug("send, remain data:%d Bytes\r\n", (trb->size & DWC3_TRB_SIZE_MASK));
			usb_in_len = dwc->send_len - (trb->size & DWC3_TRB_SIZE_MASK);
		}
		break;
	case DEPEVT_XFER_IN_PROG << DEPEVT_INTTYPE_SHIFT:
		debug("ep interrupt: DEPEVT_XFER_IN_PROG\r\n");
		break;
	default:
		break;
	}
}

static u32 get_eventbuf_event(dwc3_device_t *dev, int size)
{
	u32 event;

#ifdef RUN_IN_UBOOT
	// invalidate_dcache_range((long)dev->event_buf, ((long)dev->event_buf) + DWC3_EVENT_BUFFERS_SIZE);
	dwc3_invalidate_cache((long)dev->event_buf, DWC3_EVENT_BUFFERS_SIZE);
#endif

	event = *(u32 *)(dev->event_ptr);
	if(event)
		(dev->event_ptr) ++;

	if (dev->event_ptr >= dev->event_buf + size)
		dev->event_ptr = dev->event_buf;

	debug("current event ptr:%p, event:%x\n", dev->event_ptr, event);
	return event;
}

static void dwc3_handle_event(dwc3_device_t *dwc, u32 buf)
{
	u32 count, i;
	u32 event;
	int ep;

	count = readl((void *)dwc->reg_base + DWC3_GEVNTCOUNT(buf));
	count &= DWC3_GEVNTCOUNT_MASK;

	if (!count)
		return;

	for (i = 0; i < count; i += 4) {
		debug("get a usb event\n");

		event = get_eventbuf_event(dwc, dwc->event_size);
		writel(4, (void *)dwc->reg_base + DWC3_GEVNTCOUNT(buf));

		if (event == 0)
			continue;

		if (event & EVENT_NON_EP_BIT) {
			/* do nothing */
		} else {
			ep = event >> DEPEVT_EPNUM_SHIFT & DEPEVT_EPNUM_BITS >> DEPEVT_EPNUM_SHIFT;
			dwc3_process_event_buf(dwc, ep, event);
		}

	}
}

void dwc3_handle_interrupt(dwc3_device_t *dev)
{
	dwc3_handle_event(dev, 0);
}

static void usb_receive(dwc3_device_t *dev, dma_addr_t addr, int len)
{
	usb3_dev_ep_t *ep = dev->out_ep;
	struct dwc3_gadget_ep_cmd_params params;

	if (ep->ep.xfer_started) {
		err("ep out transfer started\r\n");
		return;
	}

	dev->recv_len = len;
	dev->recv_buf = addr;  //save buf addr

	dwc3_prepare_one_trb(ep->ep.dma_desc, (dma_addr_t)addr,
	               len, DWC3_TRBCTL_NORMAL,
	               1);
	params.param0 = upper_32_bits((dma_addr_t)ep->ep.dma_desc);
	params.param1 = lower_32_bits((dma_addr_t)ep->ep.dma_desc);
	dwc3_send_ep_cmd(dev, ep->ep.num, DWC3_DEPCMD_STARTTRANSFER,
	                         &params);

	ep->ep.xfer_started = 1;
}

int usb_recv(u8* buf, u32 len, u32 timeout_ms)
{
	dwc3_device_t *dev = &g_dwc3_dev;
	unsigned int transfer_size = 0;

	if(len % dev->out_ep->ep.maxpacket == 0 && len != 0) {
		transfer_size = len;
	} else {
		transfer_size = (len / dev->out_ep->ep.maxpacket + 1) * dev->out_ep->ep.maxpacket;
	}

	info("usb recv: addr:%p, ep_maxpacket:%d, len:%d, transfer_size:%d\r\n",
		buf, dev->out_ep->ep.maxpacket, len, transfer_size);

	usb_out_len = 0;
	usb_receive(dev, (dma_addr_t)buf, transfer_size);

	while(1) {
		dwc3_handle_interrupt(dev);
		if (usb_out_len > 0) {
			info("receive %d bytes data from host\r\n", usb_out_len);
			return usb_out_len;
		}

#ifdef RUN_IN_UBOOT
		if(ctrlc()){
			printf("abort\r\n");
			free(dev->out_ep->ep.dma_desc);
			free(dev->in_ep->ep.dma_desc);
			return -1;
		}
#endif
	}
}


int usb_send(const unsigned char *buf, u32 len, u32 timeout_ms)
{
	dwc3_device_t *dev = &g_dwc3_dev;
	usb3_dev_ep_t *ep = dev->in_ep;
	struct dwc3_gadget_ep_cmd_params params;
	u8 zlp = 0;

	info("usb send: addr:%p, len:%d\r\n", buf, len);

	/* Zero-Length Packet check */
	zlp = (len && !(len % ep->ep.maxpacket)) ? 1 : 0;

	/* Fill in Bulk In TRB */
	dwc3_prepare_one_trb(ep->ep.dma_desc, (dma_addr_t)buf,
		len, DWC3_TRBCTL_NORMAL,
		zlp ? 0 : DWC3_TRB_CTRL_LST);

	if (zlp) {	//send zero-length packet to host
		debug("send zero-length packet\r\n");
		dwc3_prepare_one_trb(ep->ep.dma_desc + 1, 0,
			0, DWC3_TRBCTL_NORMAL,
			DWC3_TRB_CTRL_LST);
	}

	params.param0 = upper_32_bits((dma_addr_t)ep->ep.dma_desc);
	params.param1 = lower_32_bits((dma_addr_t)ep->ep.dma_desc);

	dwc3_send_ep_cmd(dev, ep->ep.num, DWC3_DEPCMD_STARTTRANSFER,
	                         &params);

	ep->ep.xfer_started = 1;

	usb_in_len = 0;
	dev->send_len = len;

	while(1) {
		dwc3_handle_interrupt(dev);
		if (usb_in_len > 0) {
			info("send %d bytes data to host\r\n", usb_in_len);
			return usb_in_len;
		}

#ifdef RUN_IN_UBOOT
		if(ctrlc()){
			printf("abort\r\n");
			free(dev->out_ep->ep.dma_desc);
			free(dev->in_ep->ep.dma_desc);
			return -1;
		}
#endif
	}

}

static void dwc3_event_buffers_setup(dwc3_device_t *dwc3_dev)
{
	dma_addr_t evt_buf;

	evt_buf = (dma_addr_t) event_buffer;

	writel(lower_32_bits(evt_buf), (void *)dwc3_dev->reg_base + DWC3_GEVNTADRLO(0));
	writel(upper_32_bits(evt_buf), (void *)dwc3_dev->reg_base + DWC3_GEVNTADRHI(0));

	writel(DWC3_EVENT_BUFFERS_SIZE, (void *)dwc3_dev->reg_base + DWC3_GEVNTSIZ(0));
	writel(0, (void *)dwc3_dev->reg_base + DWC3_GEVNTCOUNT(0));

	dwc3_dev->event_buf = (u32 *)event_buffer;
	dwc3_dev->event_size = DWC3_EVENT_BUFFERS_SIZE >> 2;
	dwc3_dev->event_ptr = dwc3_dev->event_buf;

	debug("event buffer addr:%p, size:%d\n", event_buffer, DWC3_EVENT_BUFFERS_SIZE);
}

static void dwc3_ep_init(dwc3_device_t *dev)
{
	u32 dsts;
	u8 speed;
	usb3_dev_ep_t *ep_out;
	usb3_dev_ep_t *ep_in;
	u32 maxpacket;

	dsts = readl((void *)dev->reg_base + DWC3_DSTS);
	speed = dsts & DWC3_DSTS_CONNECTSPD;
	dev->speed = speed;
	switch(speed) {
	case USB_SPEED_HIGH:
		maxpacket = 512;
		break;
	case USB_SPEED_FULL:
		maxpacket = 64;
		break;
	case USB_SPEED_SUPER:
		maxpacket = 1024;
		break;
	default:
		maxpacket = 512;
	}

	ep_out = dev->out_ep = &dwc3_epn_out;
#ifdef DMA_ALLOC_COHERENT
	unsigned long dma_addr;
	ep_out->ep.dma_desc = (dwc3_trb_t *)dma_alloc_coherent(sizeof(dwc3_trb_t), &dma_addr);
#else
	ep_out->ep.dma_desc = &d_epn_out_trb;
#endif
	ep_out->ep.dev = dev;
	ep_out->ep.num = 0x2;
	ep_out->ep.maxpacket = maxpacket;
	ep_out->ep.xfer_started = 0;

	ep_in = dev->in_ep = &dwc3_epn_in;
#ifdef DMA_ALLOC_COHERENT
	ep_in->ep.dma_desc = (dwc3_trb_t *)dma_alloc_coherent(sizeof(dwc3_trb_t) * 2, &dma_addr);
#else
	ep_in->ep.dma_desc = &d_epn_in_trb[0];
#endif
	ep_in->ep.dev = dev;
	ep_in->ep.num = 0x3;
	ep_in->ep.maxpacket = maxpacket;
	ep_in->ep.xfer_started = 0;

	debug("ep-out trb addr:%p\n", (void *)ep_out->ep.dma_desc);
	debug("ep-in trb addr:%p\n", (void *)ep_in->ep.dma_desc);
}


int axera_usb_init(void)
{
	info("fdl usb init\r\n");

	dwc3_device_t *dwc3_dev = &g_dwc3_dev;

	dwc3_dev->reg_base = USB2_BASE_ADDR;

	dwc3_ep_init(dwc3_dev);
	dwc3_event_buffers_setup(dwc3_dev);

	usb_out_len = 0;

	return 0;
}
