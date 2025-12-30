#ifndef __ATF_H_
#define __ATF_H_

#include "cmn.h"
#include <stdint.h>

#define ARM_BL31_PLAT_PARAM_VAL	0x0f1e2d3c4b5a6978
#define PARAM_BL_PARAMS		0x05
#define VERSION_2		0x02
#define BL33_IMAGE_ID		5
#define BL32_IMAGE_ID		4

#define MODE_RW_SHIFT		0x4
#define MODE_RW_MASK		0x1
#define MODE_RW_64		0x0
#define MODE_RW_32		0x1

#define MODE_EL_SHIFT		0x2
#define MODE_EL_MASK		0x3
#define MODE_EL3		0x3
#define MODE_EL2		0x2
#define MODE_EL1		0x1
#define MODE_EL0		0x0

#define MODE_SP_SHIFT		0x0
#define MODE_SP_MASK		0x1
#define MODE_SP_EL0		0x0
#define MODE_SP_ELX		0x1

#define SPSR_DAIF_SHIFT		6
#define SPSR_DAIF_MASK		0x0f

#define SPSR_64(el, sp, daif)		\
	(MODE_RW_64 << MODE_RW_SHIFT |	\
	 ((el) & MODE_EL_MASK) << MODE_EL_SHIFT |	\
	 ((sp) & MODE_SP_MASK) << MODE_SP_SHIFT |	\
	 ((daif) & SPSR_DAIF_MASK) << SPSR_DAIF_SHIFT)

#define DAIF_FIQ_BIT		(1 << 0)
#define DAIF_IRQ_BIT		(1 << 1)
#define DAIF_ABT_BIT		(1 << 2)
#define DAIF_DBG_BIT		(1 << 3)
#define DISABLE_ALL_EXCEPTIONS \
		(DAIF_FIQ_BIT | DAIF_IRQ_BIT | DAIF_ABT_BIT | DAIF_DBG_BIT)

#define EP_SECURITY_MASK	0x21
#define EP_SECURITY_SHIFT	0
#define EP_SECURE		0x0
#define EP_NON_SECURE		0x1

/* Param header types */
#define PARAM_EP		0x01
#define VERSION_1		0x1

typedef unsigned long		size_t;

typedef struct param_header {
	uint8_t type;		/* type of the structure */
	uint8_t version;	/* version of this structure */
	uint16_t size;		/* size of this structure in bytes */
	uint32_t attr;		/* attributes: unused bits SBZ */
} param_header_t;

typedef struct atf_image_info {
	param_header_t h;
	uintptr_t image_base;   /* physical address of base of image */
	uint32_t image_size;    /* bytes read from image file */
	uint32_t image_max_size;
} atf_image_info_t;

typedef struct aapcs64_params {
	uint64_t arg0;
	uint64_t arg1;
	uint64_t arg2;
	uint64_t arg3;
	uint64_t arg4;
	uint64_t arg5;
	uint64_t arg6;
	uint64_t arg7;
} aapcs64_params_t;

typedef struct entry_point_info {
	param_header_t h;
	uintptr_t pc;
	uint32_t spsr;
	aapcs64_params_t args;
} entry_point_info_t;

typedef struct bl_params_node {
	unsigned int image_id;
	atf_image_info_t *image_info;
	entry_point_info_t *ep_info;
	struct bl_params_node *next_params_info;
} bl_params_node_t;

typedef struct bl_params {
	param_header_t h;
	bl_params_node_t *head;
} bl_params_t;

#define SET_PARAM_HEAD(_p, _type, _ver, _attr) do { \
	(_p)->h.type = (uint8_t)(_type); \
	(_p)->h.version = (uint8_t)(_ver); \
	(_p)->h.size = (uint16_t)sizeof(*_p); \
	(_p)->h.attr = (uint32_t)(_attr) ; \
	} while (0)
/* for atf boot end */

typedef void (*atf_boot_fn)(u64 arg0, u64 arg1, u64 arg2, u64 arg3);

extern bl_params_t atf_bl_params;
extern bl_params_node_t atf_bl33_params;
extern entry_point_info_t atf_bl33_ep_info;
int atf_boot_prepare(unsigned long uboot_addr, unsigned long optee_addr);
#endif
