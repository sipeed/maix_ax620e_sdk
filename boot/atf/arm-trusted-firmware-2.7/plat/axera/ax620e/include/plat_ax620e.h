/*
 * Copyright (c) 2013-2018, AX650 Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_AX620E_H
#define PLAT_AX620E_H


#if defined(IMAGE_BL31)
/*
 * Use this macro to instantiate lock before it is used in below
 * ax650_lock_xxx() macros
 */
#define AX620E_INSTANTIATE_LOCK	static DEFINE_BAKERY_LOCK(ax620e_lock)
#define AX620E_LOCK_GET_INSTANCE	(&ax620e_lock)
#define AX620E_SCMI_INSTANTIATE_LOCK	spinlock_t ax620e_scmi_lock

#define AX620E_SCMI_LOCK_GET_INSTANCE	(&ax620e_scmi_lock)

/*
 * These are wrapper macros to the Coherent Memory Bakery Lock API.
 */
#define ax620e_lock_init()		bakery_lock_init(&ax620e_lock)
#define ax620e_lock_get()		bakery_lock_get(&ax620e_lock)
#define ax620e_lock_release()		bakery_lock_release(&ax620e_lock)


#endif


#ifndef __ASSEMBLER__

#include <stdint.h>

/*******************************************************************************
 * Function & variable prototypes
 ******************************************************************************/


/*
 * Mandatory functions required in AX650 standard platforms
 */
void plat_ax620e_gic_driver_init(void);
void plat_ax620e_gic_init(void);
void plat_ax620e_gic_cpuif_enable(void);
void plat_ax620e_gic_cpuif_disable(void);
void plat_ax620e_gic_redistif_on(void);
void plat_ax620e_gic_redistif_off(void);
void plat_ax620e_gic_pcpu_init(void);
void plat_ax620e_gic_save(void);
void plat_ax620e_gic_resume(void);

#endif /*__ASSEMBLER__*/

#endif /* AX620E_PWRC_H */
