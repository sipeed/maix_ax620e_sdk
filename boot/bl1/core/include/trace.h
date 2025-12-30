#ifndef _TRACE_H_
#define _TRACE_H_

#define TRACE_MEM_BASE		0x3000

typedef enum {
	/*BOOT_READ_HEADER = (1 << 0),
	BOOT_CE_BYPASS = (1 << 1),
	BOOT_CE_INIT = (1 << 2),
	BOOT_READ_IMAGE = (1 << 3),
	BOOT_FLASH_CLK_BW_DEFAULT_INIT_FAIL = (1 << 8),
	BOOT_FLASH_CLK_BW_PERROMANCE_SWITCH_FAIL = (1 << 9),
	BOOT_FLASH_CLK_BW_DEFAULT_SWITCH_FAIL = (1 << 10),
	BOOT_READ_ORIGIN_HEADER_FAIL = (1 << 11),
	BOOT_VERIFY_ORIGIN_HEADER_FAIL = (1 << 12),
	BOOT_READ_BACKUP_HEADER_FAIL = (1 << 13),
	BOOT_VERIFY_BACKUP_HEADER_FAIL = (1 << 14),
	BOOT_READ_ORIGIN_CE_FW_FAIL = (1 << 15),
	BOOT_VERIFY_ORIGIN_CE_FW_FAIL = (1 << 16),
	BOOT_READ_BACKUP_CE_FW_FAIL = (1 << 17),
	BOOT_VERIFY_BACKUP_CE_FW_FAIL = (1 << 18),
	BOOT_READ_ORIGIN_IMAGE_FAIL = (1 << 19),
	BOOT_VERIFY_ORIGIN_IMAGE_FAIL = (1 << 20),
	BOOT_READ_BACKUP_IMAGE_FAIL = (1 << 21),
	BOOT_VERIFY_BACKUP_IMAGE_FAIL = (1 << 22),
	BOOT_SECURE_INIT_FAIL = (1 << 23),
	BOOT_VERIFY_PUBKEY_FAIL = (1 << 24),
	BOOT_CIPHER_SHA_FAIL = (1 << 25),
	BOOT_AES_DECRYPTO_FAIL = (1 << 26),
	BOOT_RISCV_CHECKSUM_FAIL = (1 << 27),*/
	BOOT_RISCV_CHECKSUM_PASS = (1 << 28),

}BOOT_DEBUG_FLAG;

enum AX_LOG_LEVEL {
    AX_LOG_LEVEL_OFF       = 0, // uart log disabled
    AX_LOG_LEVEL_EMERGE    = 1, // System is unusable
    AX_LOG_LEVEL_CRITICAL  = 2, // Critical conditions
    AX_LOG_LEVEL_ERROR     = 3, // Error conditions
    AX_LOG_LEVEL_WARNING   = 4, // Warning conditions
    AX_LOG_LEVEL_NOTICE    = 5, // Normal but significant condition
    AX_LOG_LEVEL_INFO      = 6, // Informational
    AX_LOG_LEVEL_DEBUG     = 7, // Debug-level messages
    AX_LOG_LEVEL_VERBOSE   = 8  // Verbose-level messages
};
extern enum AX_LOG_LEVEL ax_log_threhold;
extern enum AX_LOG_LEVEL ax_log_level;

void ax_print_str(char * string);
void ax_print_num(unsigned long int unum, unsigned int radix);

#define AX_LOG_STR_ERROR(format, ...) \
    ax_log_level = AX_LOG_LEVEL_ERROR; \
    ax_print_str(format); \
    ax_log_level = AX_LOG_LEVEL_DEBUG;

#define AX_LOG_HEX_ERROR(format, ...) \
    ax_log_level = AX_LOG_LEVEL_ERROR; \
    ax_print_str("0x"); \
    ax_print_num(format, 16); \
    ax_log_level = AX_LOG_LEVEL_DEBUG;


#endif
