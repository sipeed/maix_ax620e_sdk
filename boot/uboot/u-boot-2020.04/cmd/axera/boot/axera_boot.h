#ifndef __AXERA_BOOT_H__
#define __AXERA_BOOT_H__

#define HEADER_MAGIC		"AXERA!"
#define AXIMG_HEADER_SIZE	64
#define AXIMG_MAGIC_SIZE	6

typedef struct axera_image_header {
	unsigned char magic[AXIMG_MAGIC_SIZE];	//AXERA image file magic
	unsigned int img_size;	//image total size include header
	unsigned int raw_img_sz;		//raw img size not include header
	unsigned int raw_img_offset;	//raw img offset in image file
	unsigned char id[32];	//image file HASH
} axera_image_header_t;

#endif
