#include "cmn.h"
#include "timer.h"
#include "eip130_drv.h"
#include "chip_reg.h"

#define EIP130_REG_BASE (0x4900000)
#define EIP130_MAILBOX_IN_BASE (EIP130_REG_BASE)
#define EIP130_MAILBOX_OUT_BASE                 0x0000
#define EIP130_MAILBOX_SPACING_BYTES            0x0400  // Actual mailbox size independent
#define EIP130_FIRMWARE_RAM_BASE                (0x4000 + EIP130_REG_BASE)
#define EIP130_MAILBOX_STAT (0x3F00 + EIP130_REG_BASE)
#define EIP130_MAILBOX_CTRL (0x3F00 + EIP130_REG_BASE)
#define EIP130_MAILBOX_LOCKOUT (0x3f10 + EIP130_REG_BASE)
#define EIP130_MODULE_STATUS (0x3FE0 + EIP130_REG_BASE)
#define EIP130_CRC24_BUSY                       (1 << 8)
#define EIP130_CRC24_OK                         (1 << 9)
#define EIP130_FIRMWARE_WRITTEN                 (1 << 20)
#define EIP130_FIRMWARE_CHECKS_DONE             (1 << 22)
#define EIP130_FIRMWARE_ACCEPTED                (1 << 23)
#define EIP130_FATAL_ERROR                      (1 << 31)
#define EIP130_WAIT_TIMEOUT         (200000)
#define EIP130_RAM_SIZE             (0x17800 - 0x4000)
static unsigned int aes_token[] = {
    0x01009e4d, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
static unsigned int hash_token[] = {
    0x02009e4d, 0x00000000, 0x00000004, 0x00100000,
    0x00000000, 0x00000004, 0x00000003, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
static unsigned int rsa_asset_creat[] = {
    0x170016fe, 0x00000000, 0x80000004, 0x00000200,
    0x0000010c, 0x00000000, 0x00000000
};
static unsigned int rsa_load_asset_cmd[] = {
    0x2704751f, 0x00000000, 0x00015004, 0x0800010c,
    0x0118bd70, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000
};
static unsigned int rsa_verify_cmd[64] = {
    0x1904357f, 0x00000000, 0x01400009, 0x40000020,
    0x00015004, 0x00000000, 0x00000000, 0x01040000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000
};
static unsigned int result_token[64];

#ifdef SECURE_BOOT_TEST
static int eip130_mailbox_link(const u8 mailbox_nr)
{
    unsigned int set_value;
    unsigned int get_value;

    if (mailbox_nr < 1 || mailbox_nr > 4) {
        return EIP130_PARAM_FAIL;
    }

    set_value = 4 << ((mailbox_nr - 1) * 4);
    writel(set_value, EIP130_MAILBOX_CTRL);

    get_value = readl(EIP130_MAILBOX_STAT);
    if ((get_value & set_value) != set_value) {
        return EIP130_LINK_FAIL;    // Link failed
    }
    return EIP130_SUCCESS;
}

static int eip130_mailbox_unlink(int mailbox_nr)
{
    unsigned int set_value;
    unsigned int get_value;

    if (mailbox_nr < 1 || mailbox_nr > 8) {
        return EIP130_LINK_FAIL;
    }
    // Unlink mailbox
    set_value = 8 << ((mailbox_nr - 1) * 4);
    writel(set_value, EIP130_MAILBOX_CTRL);
    get_value = readl(EIP130_MAILBOX_STAT);
    set_value >>= 1;     // Adapt for linked check
    // Check if the mailbox is still linked
    if ((get_value & set_value) != 0) {
        return EIP130_LINK_FAIL;    // Unlinking failed
    }
    return EIP130_SUCCESS;  // Success
}

static int eip130_mailbox_can_write_token(int mailbox_nr)
{
    int val;
    unsigned int bit;
    if (mailbox_nr < 1 || mailbox_nr > 8) {
        return EIP130_PARAM_FAIL;
    }
    bit = 1 << ((mailbox_nr - 1) * 4);
    val = readl(EIP130_MAILBOX_STAT);
    if ((val & bit) == 0) {
        return 1;
    }
    return 0;
}
#endif

static int eip130_mailbox_can_read_token(int mailbox_nr)
{
    int val;
    int bit;
    bit = 2 << ((mailbox_nr - 1) * 4);
    val = readl(EIP130_MAILBOX_STAT);
    if ((val & bit) == 0) {
        return 0;
    }
    return 1;
}

static void eip130_write_array(unsigned long addr, unsigned int *data, unsigned int cnt,
                               int mailbox_nr)
{
    unsigned int *ptr = (unsigned int *)addr;
    int i;
    for (i = 0; i < cnt; i++) {
        ptr[i] = data[i];
    }
}

#ifdef SECURE_BOOT_TEST
static void eip130_enable()
{
    // enalbe ce clks
    //cnt_clk
    writel(BIT(0), PERI_SYS_GLB_CLK_EB0_SET);
    // aclk
    writel(BIT(1), PERI_SYS_GLB_CLK_EB1_SET);
    // pclk
    writel(BIT(12), PERI_SYS_GLB_CLK_EB2_SET);

    // release ce rst
    // mam_rst
    writel(BIT(4), PERI_SYS_GLB_CLK_RST0_CLR);
    //soft_rst, prst, arst, cnt_rst
    writel((BIT(5)|BIT(6)|BIT(7)|BIT(8)), PERI_SYS_GLB_CLK_RST0_CLR);

    // pclk_top_sel[1:0] switch to 2'b11 : cpll_208m
    writel(BIT(1) | BIT(0), COMM_SYS_GLB_CLK_MUX2_SET);

    // sel clk_ce_bus_sel to 416M [1:0] 2'b10: cpll_312m
    writel(BIT(1), PERI_SYS_GLB_CLK_MUX0_SET);
}
#endif

static void EIP130_RegisterWriteMailboxControl(unsigned int value,
        int mailbox_nr)
{
    writel(value, EIP130_MAILBOX_CTRL);
}

static int eip130_mailbox_write_and_submit_token(unsigned int *command_token_p,
        int mailbox_nr, int size)
{
    unsigned long mailbox_addr = EIP130_MAILBOX_IN_BASE;
    if (mailbox_nr < 1 || mailbox_nr > 8) {
        return EIP130_PARAM_FAIL;
    }

    mailbox_addr +=
        (unsigned long)(EIP130_MAILBOX_SPACING_BYTES * (mailbox_nr - 1));
    eip130_write_array(mailbox_addr, command_token_p, size, mailbox_nr);
    EIP130_RegisterWriteMailboxControl(1, 1);
    return EIP130_SUCCESS;
}

#ifdef SECURE_BOOT_TEST
static void eip130_write_module_status(unsigned int value)
{
    writel(value, EIP130_MODULE_STATUS);
}

static unsigned int eip130_read_modulestatus()
{
    return readl(EIP130_MODULE_STATUS);
}

static int eip130_register_write_mailbox_lockout(unsigned int value)
{
    writel(value, EIP130_MAILBOX_LOCKOUT);
    return EIP130_SUCCESS;
}
#endif
static int eip130_mailbox_read_token(unsigned int *result_token_p, int mailbox_nr)
{
    if (!eip130_mailbox_can_read_token(mailbox_nr)) {
        return -3;
    }
    eip130_write_array((unsigned long) result_token_p,
                       (unsigned int *)EIP130_MAILBOX_IN_BASE, 64,
                       mailbox_nr);
    EIP130_RegisterWriteMailboxControl(2, 1);

    return 0;       // Success
}

static int eip130_waitfor_result_token(int mailbox_nr)
{
    int i = 0;
    // Poll for output token available with sleep
    while (i < EIP130_WAIT_TIMEOUT) {
        if (eip130_mailbox_can_read_token(mailbox_nr)) {
            return EIP130_SUCCESS;
        }
        i++;
    }

    return EIP130_TIMEOUT_FAIL;
}

int eip130_physical_token_exchange(u32 *command_token_p,
                                   u32 *result_token_p,
                                   u32 mailbox_nr)
{
    int funcres;
    eip130_mailbox_write_and_submit_token(command_token_p, mailbox_nr, 64);
    do {
        // Wait for the result token to be available
        funcres = eip130_waitfor_result_token(mailbox_nr);
        if (funcres != 0) {
            return EIP130_HW_FAIL;
        }
        // Read the result token from the OUT mailbox
        funcres = eip130_mailbox_read_token(result_token_p, mailbox_nr);
        if (funcres != 0) {
            return EIP130_HW_FAIL;
        }
    } while ((command_token_p[0] & 0xffff) != (result_token_p[0] & 0xffff));
    return 0;
}

#ifdef SECURE_BOOT_TEST
static int eip130_firmwareload(unsigned long fwAddr, int size, int use_dma_copy_fw)
{
    int i;
    int value;
    int padding;
    int nLoadRetries = 3;
    int rc;
    unsigned int *paddingAddr;
    int mailbox_nr = 1;
    for (; nLoadRetries > 0; nLoadRetries--) {
        int n_retries;

        // Link mailbox
        // Note: The HW is expected to come out of reset state, so mailbox
        //       linking should be possible!
        rc = eip130_mailbox_link(mailbox_nr);
        if (rc < 0) {
            return rc;                  // General error
        }

        // Check if mailbox is ready for the token
        // Note: The HW is expected to come out of reset state, so direct
        //       mailbox use should be possible!
        if (!eip130_mailbox_can_write_token(mailbox_nr)) {
            eip130_mailbox_unlink(mailbox_nr); // Unlink mailbox
            return -1;                  // General error
        }

        // Write RAM-based Firmware Code Validation and Boot token (image header)
        // Note: The first 256 bytes of the firmware image
        rc = eip130_mailbox_write_and_submit_token((unsigned int *)fwAddr, mailbox_nr, 64);
        eip130_mailbox_unlink(mailbox_nr); // Unlink mailbox
        if (rc < 0) {
            return rc;                  // General error
        }

        // Write firmware code to FWRAM (image data)
        // Note: The firmware code is located directly behind the RAM-based
        //       Firmware Code Validation and Boot token in the firmware image.
        if (use_dma_copy_fw) {
            dma_memcpy(EIP130_FIRMWARE_RAM_BASE, fwAddr + 256, (size - 256));
            padding = EIP130_RAM_SIZE - size - 256;
            dma_memset((EIP130_FIRMWARE_RAM_BASE + size - 256), 0, padding);
        } else {
            eip130_write_array(EIP130_FIRMWARE_RAM_BASE, (unsigned int *)(fwAddr + 256), (size - 256) / 4, 1);
            padding = EIP130_RAM_SIZE - size - 256;
            padding = padding / 4;
            paddingAddr = (unsigned int *)(EIP130_FIRMWARE_RAM_BASE + (unsigned long)(size - 256));
            for (i = 0; i < padding; i++) {
                paddingAddr[i] = 0;
            }
        }
        // Report that the firmware is written
        eip130_write_module_status(EIP130_FIRMWARE_WRITTEN);
        value = eip130_read_modulestatus();
        if (((value & EIP130_CRC24_OK) == 0) ||
            ((value & EIP130_FATAL_ERROR) != 0)) {
            return -5;                  // Hardware error
        }
        if ((value & EIP130_FIRMWARE_WRITTEN) == 0) {
            return -1;                  // General error (Is HW active?)
        }

        // Check if firmware is accepted
        for (n_retries = 0x7FFFFF; n_retries && ((value & EIP130_FIRMWARE_CHECKS_DONE) == 0); n_retries--) {
            value = eip130_read_modulestatus();
        }
        if ((value & EIP130_FIRMWARE_CHECKS_DONE) == 0) {
            return -3;                  // Timeout (Is HW active?)
        }
        if ((value & EIP130_FIRMWARE_ACCEPTED) == EIP130_FIRMWARE_ACCEPTED) {
            return 0;                   // Firmware is accepted (started)
        }
    }
    return -4;                          // Firmware load failed
}
#endif

int eip130_sha256(const char *data, char *output, int size)
{
    char *ptr;
    int i;
    int ret;
    u64 data_addr = (unsigned long)data;
    u16 token_id;
    token_id = (hash_token[0]&0xFFFF) + 1;
    hash_token[0] = (hash_token[0]&0xFFFF0000) + token_id;

    hash_token[2] = size;
    hash_token[5] = size;
    hash_token[24] = size;
    hash_token[3] = (u32)(data_addr & 0xffffffff);
    hash_token[4] = (u32)((data_addr >> 32) & 0xffffffff);
    hash_token[6] = 3;
    for (i = 0; i < 8; i++) {
        hash_token[8 + i] = 0;
    }

    ret = eip130_physical_token_exchange((unsigned int *)hash_token, (unsigned int *)result_token, 1);
    if (ret != 0) {
        return ret;
    }

    if (result_token[0] & (1 << 31)) {
        return EIP130_HASH_COMPUTE_FAIL;
    }
    ptr = (char *)&result_token[2];
    for (i = 0; i < 32; i++) {
        output[i] = ptr[i];
    }
    return EIP130_SUCCESS;
}
int eip130_sha256_init(const char *data, char *output, int size)
{
    char *ptr;
    int i;
    int ret;
    u64 data_addr = (unsigned long)data;
    hash_token[2] = size;
    hash_token[5] = size;
    hash_token[6] = 0x23;
    hash_token[24] = 0;
    hash_token[3] = (u32)((u64)data_addr&0xffffffff);
    hash_token[4] = (u32)(((u64)data_addr>>32) & 0xffffffff);
    for (i = 0; i < 8; i++) {
        hash_token[8 + i] = 0;
    }
    ret = eip130_physical_token_exchange((unsigned int *)hash_token, (unsigned int *)result_token, 1);
    if (ret != 0) {
        return ret;
    }
    if (result_token[0] & (1 << 31)) {
        return EIP130_HASH_COMPUTE_FAIL;
    }
    ptr = (char *)&result_token[2];
    for (i = 0; i < 32; i++) {
        output[i] = ptr[i];
    }
    return EIP130_SUCCESS;
}
int eip130_sha256_update(const char *data, char *output, int size, int *digest)
{
    char *ptr;
    int i;
    int ret;
    u64 data_addr = (unsigned long)data;
    hash_token[2] = size;
    hash_token[5] = size;
    hash_token[6] = 0x33;
    hash_token[24] = 0;
    hash_token[3] = (u32)((u64)data_addr&0xffffffff);
    hash_token[4] = (u32)(((u64)data_addr>>32) & 0xffffffff);
    for (i = 0; i < 8; i++) {
        hash_token[8 + i] = digest[i];
    }
    ret = eip130_physical_token_exchange((unsigned int *)hash_token, (unsigned int *)result_token, 1);
    if (ret != 0) {
        return ret;
    }
    if (result_token[0] & (1 << 31)) {
        return EIP130_HASH_COMPUTE_FAIL;
    }
    ptr = (char *)&result_token[2];
    for (i = 0; i < 32; i++) {
        output[i] = ptr[i];
    }
    return EIP130_SUCCESS;
}
int eip130_sha256_final(const char *data, char *output, int size, int *digest, int all_size)
{
    char *ptr;
    int i;
    int ret;
    u64 data_addr = (unsigned long)data;
    hash_token[2] = size;
    hash_token[5] = size;
    hash_token[6] = 0x13;
    hash_token[24] = all_size;
    hash_token[3] = (u32)((u64)data_addr&0xffffffff);
    hash_token[4] = (u32)(((u64)data_addr>>32) & 0xffffffff);
    for (i = 0; i < 8; i++) {
        hash_token[8 + i] = digest[i];
    }
    ret = eip130_physical_token_exchange((unsigned int *)hash_token, (unsigned int *)result_token, 1);
    if (ret != 0) {
        return ret;
    }
    if (result_token[0] & (1 << 31)) {
        return EIP130_HASH_COMPUTE_FAIL;
    }
    ptr = (char *)&result_token[2];
    for (i = 0; i < 32; i++) {
        output[i] = ptr[i];
    }
    return EIP130_SUCCESS;
}
int eip130_rsa_verify(char *key, char *signature, int *hash, int key_bits)
{
    int as_id;
    int ret = 0;
    int i;
    u64 key_addr = (unsigned long)key;
    u64 signature_addr = (unsigned long)signature;
    if (key_bits == 3072) {
        rsa_asset_creat[4] = 0x0000018c;
    } else {
        rsa_asset_creat[4] = 0x0000010c;
    }
    ret = eip130_physical_token_exchange((unsigned int *)rsa_asset_creat, result_token, 1);
    as_id = result_token[1];
    if (ret != 0) {
        return -1;
    }
    rsa_load_asset_cmd[2] = as_id;
    if (key_bits == 3072) {
        rsa_load_asset_cmd[3] = 0x0800018c;
    } else {
        rsa_load_asset_cmd[3] = 0x0800010c;
    }
    rsa_load_asset_cmd[4] = (u32)(key_addr&0xffffffff);
    rsa_load_asset_cmd[5] = (u32)((key_addr>>32) & 0xffffffff);
    ret = eip130_physical_token_exchange(rsa_load_asset_cmd, result_token, 1);
    if (ret != 0) {
        return -1;
    }

    rsa_verify_cmd[4] = as_id;
    rsa_verify_cmd[10] = (u32)(signature_addr & 0xffffffff);
    rsa_verify_cmd[11] = (u32)((signature_addr>>32) & 0xffffffff);;
    for (i = 0; i < 8; i++) {
        rsa_verify_cmd[12 + i] = hash[i];
    }
    if (key_bits == 3072) {
        rsa_verify_cmd[2] = 0x01600009;
        rsa_verify_cmd[7] = 0x01840000;
    } else {
        rsa_verify_cmd[2] = 0x01400009;
        rsa_verify_cmd[7] = 0x01040000;
    }
    ret = eip130_physical_token_exchange(rsa_verify_cmd, result_token, 1);
    if (ret != 0) {
        return -1;
    }
    if (result_token[0] & (1 << 31)) {
        return EIP130_RSA_VERIFY_FAIL;
    }
    return EIP130_SUCCESS;
}

#ifdef SECURE_BOOT_TEST
int eip130_init(unsigned long fw_addr, int size, int use_dma_copy_fw)
{
    int i;
    int ret;

    eip130_enable();
    udelay(1000);
    for (i = 1; i < 5; i++) {
        ret = eip130_mailbox_link(i);
        if (ret != 0) {
            return EIP130_LINK_FAIL;
        }
    }
    ret = eip130_mailbox_can_write_token(1);
    if (ret != 1) {
        return EIP130_WRITE_FAIL;
    }
    ret = eip130_firmwareload(fw_addr, size, use_dma_copy_fw);
    if (ret != 0) {
        return EIP130_FW_LOAD_FAIL;
    }
    ret = eip130_mailbox_link(1);
    if (ret != EIP130_SUCCESS) {
        return EIP130_LINK_FAIL;
    }
    if (eip130_register_write_mailbox_lockout(0) != EIP130_SUCCESS) {
        return EIP130_LOCKOUT_FAIL;
    }
    return EIP130_SUCCESS;
}
#endif

static int aes_decrypt(unsigned long src_addr, unsigned long dst_addr, u32 size, int *key)
{
    int ret;
    int i;
    aes_token[2] = size;
    aes_token[3] = (u32)((u64)src_addr & 0xffffffff);
    aes_token[4] = (u32)(((u64)src_addr >> 32) & 0xffffffff);
    aes_token[5] = size;
    aes_token[6] = (u32)((u64)dst_addr & 0xffffffff);
    aes_token[7] = (u32)(((u64)dst_addr >> 32) & 0xffffffff);
    aes_token[8] = size;
    aes_token[11] = (3 << 16);//256bit
    for (i = 0; i < 8; i++) {
        aes_token[17 + i] = key[i];
    }
    ret = eip130_physical_token_exchange((u32 *)aes_token, (u32 *)result_token, 1);
    if (ret != 0) {
        return ret;
    }
    return 0;
}
int aes_ecb_decrypto(int *key, unsigned long src_addr, unsigned long dst_addr, int size)
{
    int ret;
    ret = aes_decrypt(src_addr, dst_addr, size, key);
    return ret;
}
