/*
 * Copyright (c) 2013-2018, AX650 Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef AX620E_PWRC_H
#define AX620E_PWRC_H

#include <stdint.h>
#include <pmu.h>

/*
 * Use this macro to instantiate lock before it is used in below
 * ax620E_lock_xxx() macros
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
#define ax620e_lock_release()	bakery_lock_release(&ax620e_lock)

/*******************************************************************************
 * Function & variable prototypes
 ******************************************************************************/
void ax620e_pwrc_write_ppoffr(u_register_t mpidr);
void ax620e_pwrc_write_pponr(u_register_t mpidr);
unsigned int ax620e_pwrc_read_psysr(u_register_t mpidr);
void plat_ax620e_pwrc_setup(void);
void ax620e_sys_pwrdwn();


#endif /* AX620E_PWRC_H */
