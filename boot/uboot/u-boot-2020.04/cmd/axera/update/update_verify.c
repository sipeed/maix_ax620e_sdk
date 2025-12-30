#include <common.h>
#include <asm/arch/boot_mode.h>
#include "../secureboot/secureboot.h"

#define SIZE_SPL_IMG_PADDING			(128 * 1024)

extern struct boot_mode_info boot_info_data;

#if defined(CONFIG_AXERA_SECURE_BOOT) && defined(CONFIG_CMD_AXERA_CIPHER)
int update_verify_image(const char *part_name, const char *pfile)
{
	struct img_header *img_header;
	struct spl_header *spl_header;
	struct rsa_key *rsa_key;
	u32 key_bits;
	int ret = 0;
	char *pkey;
	char public_key[396] = {0};

	if (is_secure_enable() == 0) {
		return 0;
	}

	if (!strcmp(part_name, "spl")) {
		spl_header = (struct spl_header *)pfile;

		/* verify pubkey hash */
		pkey = (char *)&spl_header->key_n_header;
		if (public_key_verify(pkey, sizeof(struct rsa_key)) < 0) {
			printf("spl image public key verify failed\n");
			return -1;
		}

		/* verify image */
		memset(public_key, 0, 396);
		key_bits = (spl_header->capability & RSA_3072_MODE) ? 3072 : 2048;
		memcpy((void *)(public_key),
				(void *)&spl_header->key_n_header,
				(4 + key_bits / 8));
		memcpy((void *)(public_key + (4 + 3072 / 8)),
				(void *)((char *)&spl_header->key_n_header + 4 + key_bits / 8),
				8);
		rsa_key = (struct rsa_key *)public_key;
		ret = secure_verify(part_name,
							rsa_key,
							(char *)spl_header + sizeof(*spl_header),
							(char *)spl_header->signature,
							spl_header->img_size,
							key_bits);
		if (ret != 0) {
			pr_err("secure_verify spl ret:%d\n", ret);
			return -1;
		} else
			printf("secure_verify spl ret:%d\n", ret);

		if (boot_info_data.storage_sel != STORAGE_TYPE_NOR) {
			spl_header = (struct spl_header *)(pfile + SIZE_SPL_IMG_PADDING);

			/* verify pubkey hash */
			pkey = (char *)&spl_header->key_n_header;
			if (public_key_verify(pkey, sizeof(struct rsa_key)) < 0) {
				printf("spl_bak image public key verify failed\n");
				return -1;
			}

			/*verify back spl */
			memset(public_key, 0, 396);

			key_bits = (spl_header->capability & RSA_3072_MODE) ? 3072 : 2048;
			memcpy((void *)(public_key),
					(void *)&spl_header->key_n_header,
					(4 + key_bits / 8));
			memcpy((void *)(public_key + (4 + 3072 / 8)),
					(void *)((char *)&spl_header->key_n_header + 4 + key_bits / 8),
					8);
			rsa_key = (struct rsa_key *)public_key;
			ret = secure_verify((char *)part_name,
								rsa_key,
								(char *)spl_header + sizeof(*spl_header),
								(char *)spl_header->signature,
								spl_header->img_size,
								key_bits);
			printf("secure_verify spl_bak ret:%d\n", ret);
		}
	} else if (!strcmp(part_name, "uboot")
		|| !strcmp(part_name, "uboot_b")
		|| !strcmp(part_name, "atf")
		|| !strcmp(part_name, "atf_b")
		|| !strcmp(part_name, "optee")
		|| !strcmp(part_name, "optee_b")
		|| !strcmp(part_name, "dtb")
		|| !strcmp(part_name, "dtb_b")
		|| !strcmp(part_name, "kernel")
		|| !strcmp(part_name, "kernel_b")) {
		img_header = (struct img_header *)pfile;
		key_bits = (img_header->capability & RSA_3072_MODE) ? 3072 : 2048;

		/* verify pubkey hash */
		/* copy key_key_n_header and key(2048 or 3072) in bytes */
		memset(public_key, 0, 396);
		memcpy((void *)public_key,
				(void *)&img_header->pub_key.key_n_header,
				(4 + key_bits / 8));
		/* copy key_e_header & rsa_key_e*/
		memcpy((void *)public_key + 4 + key_bits / 8,
				(void *)&img_header->pub_key.key_e_header,
				8);
		if (public_key_verify(public_key, sizeof(struct rsa_key)) < 0) {
			printf("%s image public key verify failed\n", part_name);
			return -1;
		}

		/* verify image */
		rsa_key = (struct rsa_key *)&img_header->pub_key;
		ret = secure_verify((char *)part_name,
							rsa_key,
							(char *)img_header + sizeof(*img_header),
							(char *)img_header->signature.signature,
							img_header->img_size,
							key_bits);
		printf("secure_verify %s ret:%d\n", part_name, ret);
	}

	return ret;
}
#else
int update_verify_image(const char *part_name, const char *pfile)
{
	(void)part_name;
	(void)pfile;

	return 0;
}
#endif

