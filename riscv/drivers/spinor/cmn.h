/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __CMN_H
#define __CMN_H

#include <rtdevice.h>
#include "soc.h"

typedef unsigned long long int  u64;
typedef unsigned int            u32;
typedef unsigned short          u16;
typedef unsigned char			u8;

#define UNUSED(x)	(void)x

#define BITS_PER_INT				32	/*64-bit CPU, sizeof(int)=4 */
#define BITS_PER_LONG				64	/*64-bit CPU, sizeof(long)=8 */

#define BIT(nr)						(1U << (nr))
#define BIT_UL(nr)					(1UL << (nr))
#define BIT_MASK(nr)				(1U << ((nr) % BITS_PER_INT))
#define BIT_UL_MASK(nr)				(1UL << ((nr) % BITS_PER_LONG))
#define BIT_UL_WORD(nr)				((nr) / BITS_PER_LONG)
#define BITS_PER_BYTE				8

/*
 * Create a contiguous bitmask starting at bit position @l and ending at
 * position @h. For example
 * GENMASK_UL(39, 21) gives us the 64bit vector 0x000000ffffe00000.
 */
#define GENMASK(h, l) \
	(((~0U) << (l)) & (~0U >> (BITS_PER_INT - 1 - (h))))

#define GENMASK_ULL(h, l) \
	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#endif
