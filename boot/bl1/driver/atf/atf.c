#include "atf.h"

bl_params_t atf_bl_params;
bl_params_node_t atf_bl33_params;
entry_point_info_t atf_bl33_ep_info;
#ifdef OPTEE_BOOT
bl_params_node_t atf_bl32_params;
entry_point_info_t atf_bl32_ep_info;
#endif

int atf_boot_prepare(unsigned long uboot_addr, unsigned long optee_addr)
{
	atf_bl_params.h.type = PARAM_BL_PARAMS;
	atf_bl_params.h.version = VERSION_2;
	atf_bl_params.head = &atf_bl33_params;

	atf_bl33_params.image_id = BL33_IMAGE_ID;
#ifdef OPTEE_BOOT
	atf_bl33_params.next_params_info = &atf_bl32_params;
#else
	atf_bl33_params.next_params_info = NULL;
#endif
	atf_bl33_params.ep_info = &atf_bl33_ep_info;

	atf_bl33_ep_info.pc = (unsigned long)uboot_addr;
	atf_bl33_ep_info.spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	SET_PARAM_HEAD(&atf_bl33_ep_info, PARAM_EP, VERSION_1, EP_NON_SECURE);

#ifdef OPTEE_BOOT
	atf_bl32_params.image_id = BL32_IMAGE_ID;
	atf_bl32_params.next_params_info = NULL;
	atf_bl32_params.ep_info = &atf_bl32_ep_info;

	atf_bl32_ep_info.pc = optee_addr;
	atf_bl32_ep_info.spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	SET_PARAM_HEAD(&atf_bl32_ep_info, PARAM_EP, VERSION_1, EP_SECURE);
#endif

	return 0;
}