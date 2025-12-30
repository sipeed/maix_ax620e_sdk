/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_CIPHER_TOKEN_CRYPTO_H__
#define __AX_CIPHER_TOKEN_CRYPTO_H__
#define CE_POLICY_SHA1                       0x0000000000000001ULL
#define CE_POLICY_SHA224                     0x0000000000000002ULL
#define CE_POLICY_SHA256                     0x0000000000000004ULL
#define CE_POLICY_SHA384                     0x0000000000000008ULL
#define CE_POLICY_SHA512                     0x0000000000000010ULL
#define CE_POLICY_CMAC                       0x0000000000000020ULL
#define CE_POLICY_POLY1305                   0x0000000000000040ULL

/** Asset policies related to symmetric cipher algorithms */
#define CE_POLICY_ALGO_CIPHER_MASK           0x0000000000000300ULL
#define CE_POLICY_ALGO_CIPHER_AES            0x0000000000000100ULL
#define CE_POLICY_ALGO_CIPHER_TRIPLE_DES     0x0000000000000200ULL
#define CE_POLICY_ALGO_CIPHER_CHACHA20       0x0000000000002000ULL
#define CE_POLICY_ALGO_CIPHER_SM4            0x0000000000004000ULL
#define CE_POLICY_ALGO_CIPHER_ARIA           0x0000000000008000ULL

/** Asset policies related to symmetric cipher modes */
#define CE_POLICY_MODE1                      0x0000000000010000ULL
#define CE_POLICY_MODE2                      0x0000000000020000ULL
#define CE_POLICY_MODE3                      0x0000000000040000ULL
#define CE_POLICY_MODE4                      0x0000000000080000ULL
#define CE_POLICY_MODE5                      0x0000000000100000ULL
#define CE_POLICY_MODE6                      0x0000000000200000ULL
#define CE_POLICY_MODE7                      0x0000000000400000ULL
#define CE_POLICY_MODE8                      0x0000000000800000ULL
#define CE_POLICY_MODE9                      0x0000000001000000ULL
#define CE_POLICY_MODE10                     0x0000000002000000ULL

/** Asset policies specialized per symmetric cipher algorithm */
#define CE_POLICY_AES_MODE_ECB               (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE1)
#define CE_POLICY_AES_MODE_CBC               (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE2)
#define CE_POLICY_AES_MODE_CTR               (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE4)
#define CE_POLICY_AES_MODE_CTR32             (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE4)
#define CE_POLICY_AES_MODE_ICM               (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE5)
#define CE_POLICY_AES_MODE_CCM               (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE7|CE_POLICY_CMAC)
#define CE_POLICY_AES_MODE_F8                (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE8)
#define CE_POLICY_AES_MODE_XTS               (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE9)
#define CE_POLICY_AES_MODE_GCM               (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE10)

#define CE_POLICY_3DES_MODE_ECB              (CE_POLICY_ALGO_CIPHER_TRIPLE_DES|CE_POLICY_MODE1)
#define CE_POLICY_3DES_MODE_CBC              (CE_POLICY_ALGO_CIPHER_TRIPLE_DES|CE_POLICY_MODE2)

#define CE_POLICY_CHACHA20_ENCRYPT           (CE_POLICY_ALGO_CIPHER_CHACHA20)
#define CE_POLICY_CHACHA20_AEAD              (CE_POLICY_ALGO_CIPHER_CHACHA20|CE_POLICY_POLY1305)

#define CE_POLICY_SM4_MODE_ECB               (CE_POLICY_ALGO_CIPHER_SM4|CE_POLICY_MODE1)
#define CE_POLICY_SM4_MODE_CBC               (CE_POLICY_ALGO_CIPHER_SM4|CE_POLICY_MODE2)
#define CE_POLICY_SM4_MODE_CTR               (CE_POLICY_ALGO_CIPHER_SM4|CE_POLICY_MODE4)

#define CE_POLICY_ARIA_MODE_ECB              (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE1)
#define CE_POLICY_ARIA_MODE_CBC              (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE2)
#define CE_POLICY_ARIA_MODE_CTR              (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE4)
#define CE_POLICY_ARIA_MODE_CTR32            (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE4)
#define CE_POLICY_ARIA_MODE_ICM              (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE5)
#define CE_POLICY_ARIA_MODE_CCM              (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE7|CE_POLICY_CMAC)
#define CE_POLICY_ARIA_MODE_GCM              (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE10)

/** Asset policies related to Algorithm/cipher/MAC operations */
#define CE_POLICY_MAC_GENERATE               0x0000000004000000ULL
#define CE_POLICY_MAC_VERIFY                 0x0000000008000000ULL
#define CE_POLICY_ENCRYPT                    0x0000000010000000ULL
#define CE_POLICY_DECRYPT                    0x0000000020000000ULL

/** Asset policies related to temporary values
 *  Note that the CE_POLICY_TEMP_MAC should be used for intermediate
 *  hash digest as well. */
#define CE_POLICY_TEMP_IV                    0x0001000000000000ULL
#define CE_POLICY_TEMP_COUNTER               0x0002000000000000ULL
#define CE_POLICY_TEMP_MAC                   0x0004000000000000ULL
#define CE_POLICY_TEMP_AUTH_STATE            0x0010000000000000ULL

/** Asset policy related to monotonic counter */
#define CE_POLICY_MONOTONIC                  0x0000000100000000ULL

/** Asset policies related to key derive functionality */
#define CE_POLICY_TRUSTED_ROOT_KEY           0x0000000200000000ULL
#define CE_POLICY_TRUSTED_KEY_DERIVE         0x0000000400000000ULL
#define CE_POLICY_KEY_DERIVE                 0x0000000800000000ULL

/** Asset policies related to AES key wrap functionality\n
 *  Note: Must be combined with operations bits */
#define CE_POLICY_TRUSTED_WRAP               0x0000001000000000ULL
#define CE_POLICY_AES_WRAP                   0x0000002000000000ULL

/** Asset policies related to PK operations */
#define CE_POLICY_PUBLIC_KEY                 0x0000000080000000ULL
#define CE_POLICY_PK_RSA_OAEP_WRAP           0x0000004000000000ULL
#define CE_POLICY_PK_RSA_PKCS1_WRAP          0x0000010000000000ULL
#define CE_POLICY_PK_RSA_PKCS1_SIGN          0x0000020000000000ULL
#define CE_POLICY_PK_RSA_PSS_SIGN            0x0000040000000000ULL
#define CE_POLICY_PK_DSA_SIGN                0x0000080000000000ULL
#define CE_POLICY_PK_ECC_ECDSA_SIGN          0x0000100000000000ULL
#define CE_POLICY_PK_DH_KEY                  0x0000200000000000ULL
#define CE_POLICY_PK_ECDH_KEY                0x0000400000000000ULL
#define CE_POLICY_PUBLIC_KEY_PARAM           0x0000800000000000ULL

#define CE_POLICY_PK_ECC_ELGAMAL_ENC         (CE_POLICY_PK_ECC_ECDSA_SIGN|CE_POLICY_PK_ECDH_KEY)

/** Asset policies related to Authentication */
#define CE_POLICY_EMMC_AUTH_KEY              0x0400000000000000ULL
#define CE_POLICY_AUTH_KEY                   0x8000000000000000ULL

/** Asset policies related to the domain */
#define CE_POLICY_SOURCE_NON_SECURE          0x0100000000000000ULL
#define CE_POLICY_CROSS_DOMAIN               0x0200000000000000ULL

/** Asset policies related to general purpose data that can or must be used
 *  in an operation */
#define CE_POLICY_PRIVATE_DATA               0x0800000000000000ULL
#define CE_POLICY_PUBLIC_DATA                0x1000000000000000ULL

/** Asset policies related to export functionality */
#define CE_POLICY_EXPORT                     0x2000000000000000ULL
#define CE_POLICY_TRUSTED_EXPORT             0x4000000000000000ULL

#define CETOKEN_PKASSET_RSA_PKCS1V1_5_SIGN 8
#define CETOKEN_PKASSET_RSA_PKCS1V1_5_VERIFY 9
#define CETOKEN_PKASSET_RSA_PSS_SIGN 0xc
#define CETOKEN_PKASSET_RSA_PSS_VERIFY 0xd

typedef enum
{
    EIP130TOKEN_CRYPTO_ALGO_AES      = 0,
    EIP130TOKEN_CRYPTO_ALGO_DES      = 1,
    EIP130TOKEN_CRYPTO_ALGO_3DES     = 2,
    EIP130TOKEN_CRYPTO_ALGO_CHACHA20 = 7,
    EIP130TOKEN_CRYPTO_ALGO_SM4      = 8,
    EIP130TOKEN_CRYPTO_ALGO_ARIA     = 9,
}EIP130_TOKEN_CRYPTO_ALGO_E;
typedef enum
{
	TOKEN_MODE_CIPHER_ECB = 0,
	TOKEN_MODE_CIPHER_CBC,
	TOKEN_MODE_CIPHER_CTR,
	TOKEN_MODE_CIPHER_ICM,
	TOKEN_MODE_CIPHER_F8,
	TOKEN_MODE_CIPHER_CCM,
	TOKEN_MODE_CIPHER_XTS,
	TOKEN_MODE_CIPHER_GCM,
	TOKEN_MODE_CIPHER_CHACHA20_ENC = 0,
	TOKEN_MODE_CIPHER_CHACHA20_AEAD,
} AX_CIPHER_TokenModeCipher;
enum
{
    EIP130TOKEN_HASH_ALGORITHM_SHA1 = 1,
    EIP130TOKEN_HASH_ALGORITHM_SHA224,
    EIP130TOKEN_HASH_ALGORITHM_SHA256,
    EIP130TOKEN_HASH_ALGORITHM_SHA384,
    EIP130TOKEN_HASH_ALGORITHM_SHA512,
};
enum
{
    EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA1 = 1,
    EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA224,
    EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA256,
    EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA384,
    EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA512,
};
void Eip130Token_Command_WriteByteArray(
        Eip130Token_Command_t * const CommandToken,
        unsigned int StartWord,
        const AX_U8 * Data,
        const unsigned int DataLenInBytes)
{
    const AX_U8 * const Stop = Data + DataLenInBytes;

    if (CommandToken == 0 || Data == 0)
    {
        return;
    }
    while (Data < Stop)
    {
        AX_U32 W;
        if (StartWord >= EIP130TOKEN_RESULT_WORDS)
        {
            return;
        }
        // LSB-first
        W = (AX_U32)(*Data++);
        if (Data < Stop)
        {
            W |= (AX_U32)((*Data++) << 8);
            if (Data < Stop)
            {
                W |= (AX_U32)((*Data++) << 16);
                if (Data < Stop)
                {
                    W |= (AX_U32)((*Data++) << 24);
                }
            }
        }
        // Write word
        CommandToken->W[StartWord++] = W;
    }
}
void
Eip130Token_Result_ReadByteArray(
        const Eip130Token_Result_t * const ResultToken_p,
        unsigned int StartWord,
        unsigned int DestLenOutBytes,
        AX_U8 * Dest)
{
    AX_U8 * const Stop = Dest + DestLenOutBytes;

    if (ResultToken_p == 0 || Dest == 0)
    {
        return;
    }
    while (Dest < Stop)
    {
        AX_U32 W;

        if (StartWord >= EIP130TOKEN_RESULT_WORDS)
        {
            return;
        }
        // Read word
        W = ResultToken_p->W[StartWord++];

        // LSB-first
        *Dest++ = (AX_U8)W;
        if (Dest < Stop)
        {
            W >>= 8;
            *Dest++ = (AX_U8)W;
            if (Dest)
            {
                W >>= 8;
                *Dest++ = (AX_U8)W;
                if (Dest < Stop)
                {
                    W >>= 8;
                    *Dest++ = (AX_U8)W;
                }
            }
        }
    }
}

static inline void Eip130Token_Command_Crypto_Operation(
        Eip130Token_Command_t * const CommandToken,
        const AX_U8 Algorithm,
        const AX_U8 Mode,
        const bool fEncrypt,
        const AX_U32 DataLengthInBytes)
{
    CommandToken->W[0] = (EIP130TOKEN_OPCODE_ENCRYPTION << 24);
    CommandToken->W[2] = DataLengthInBytes;

    // Algorithm, Mode and direction
    CommandToken->W[11] = (MASK_4_BITS & Algorithm) + ((MASK_4_BITS & Mode) << 4);
    if (fEncrypt)
    {
        CommandToken->W[11] |= BIT_15;
    }
}
static inline void Eip130Token_Command_Crypto_SetDataAddresses(
        Eip130Token_Command_t * const CommandToken,
        const AX_U64 InputDataAddress,
        const AX_U32 InputDataLengthInBytes,
        const AX_U64 OutputDataAddress,
        const AX_U32 OutputDataLengthInBytes)
{
    CommandToken->W[3] = (AX_U32)(InputDataAddress);
    CommandToken->W[4] = (AX_U32)(InputDataAddress >> 32);
    CommandToken->W[5] = InputDataLengthInBytes;
    CommandToken->W[6] = (AX_U32)(OutputDataAddress);
    CommandToken->W[7] = (AX_U32)(OutputDataAddress >> 32);
    CommandToken->W[8] = OutputDataLengthInBytes;
}
static inline void Eip130Token_Command_Crypto_CopyKey(
        Eip130Token_Command_t * const CommandToken,
        const AX_U8 * const Key_p,
        const AX_U32 KeyLengthInBytes)
{
    Eip130Token_Command_WriteByteArray(CommandToken, 17, Key_p, KeyLengthInBytes);
}
static inline void Eip130Token_Command_Crypto_SetKeyLength(
        Eip130Token_Command_t * const CommandToken,
        const AX_U32 KeyLengthInBytes)
{
    AX_U32 CodedKeyLen = 0;
    // Coded key length only needed for AES and ARIA
    switch (KeyLengthInBytes)
    {
    case (128 / 8):
        CodedKeyLen = 1;
        break;

    case (192 / 8):
        CodedKeyLen = 2;
        break;

    case (256 / 8):
        CodedKeyLen = 3;
        break;

    default:
        break;
    }
    CommandToken->W[11] |= (CodedKeyLen << 16);
}
static inline void
Eip130Token_Command_Crypto_CopyIV(
        Eip130Token_Command_t * const CommandToken,
        const AX_U8 * const IV_p)
{
    Eip130Token_Command_WriteByteArray(CommandToken, 13, IV_p, 16);
}

static inline void
Eip130Token_Command_Hash(
        Eip130Token_Command_t * const CommandToken,
        const AX_U8 HashAlgo,
        const bool fInitWithDefault,
        const bool fFinalize,
        const AX_U64 InputDataAddress,
        const AX_U32 InputDataLengthInBytes)
{
    CommandToken->W[0] = (EIP130TOKEN_OPCODE_HASH << 24);
    CommandToken->W[2] = InputDataLengthInBytes;
    CommandToken->W[3] = (AX_U32)(InputDataAddress);
    CommandToken->W[4] = (AX_U32)(InputDataAddress >> 32);
    CommandToken->W[5] = InputDataLengthInBytes;
    CommandToken->W[6] = (MASK_4_BITS & HashAlgo);
    if (!fInitWithDefault)
    {
        CommandToken->W[6] |= BIT_4;
    }
    if (!fFinalize)
    {
        CommandToken->W[6] |= BIT_5;
    }
}

static inline void
Eip130Token_Command_Hash_SetTempDigestASID(Eip130Token_Command_t * const CommandToken, const AX_U32 AssetId)
{
    CommandToken->W[7] = AssetId;
}

static inline void
Eip130Token_Command_Hash_SetTotalMessageLength(
        Eip130Token_Command_t * const CommandToken,
        const AX_U64 TotalMessageLengthInBytes)
{
    CommandToken->W[24] = (AX_U32)(TotalMessageLengthInBytes);
    CommandToken->W[25] = (AX_U32)(TotalMessageLengthInBytes >> 32);
}


static inline void
Eip130Token_Command_Hash_CopyDigest(
        Eip130Token_Command_t * const CommandToken,
        const AX_U8 * const Digest_p,
        const AX_U32 DigestLenInBytes)
{
    Eip130Token_Command_WriteByteArray(CommandToken, 8, Digest_p, DigestLenInBytes);
}


static inline void
Eip130Token_Result_Hash_CopyDigest(
        Eip130Token_Result_t * const ResultToken,
        const AX_U32 DigestLenInBytes,
        AX_U8 * Digest)
{
    Eip130Token_Result_ReadByteArray(ResultToken, 2,
                                     DigestLenInBytes, Digest);
}

static inline void
Eip130Token_Command_Mac(
        Eip130Token_Command_t * const CommandToken,
        const AX_U8 MacAlgo,
        const bool fInit,
        const bool fFinalize,
        const AX_U64 InputDataAddress,
        const AX_U32 InputDataLengthInBytes)
{
    CommandToken->W[0] = (EIP130TOKEN_OPCODE_MAC << 24);
    CommandToken->W[2] = InputDataLengthInBytes;
    CommandToken->W[3] = (AX_U32)(InputDataAddress);
    CommandToken->W[4] = (AX_U32)(InputDataAddress >> 32);
    CommandToken->W[5] = (AX_U32)((InputDataLengthInBytes + 3) & (AX_U32)~3);
    CommandToken->W[6] = (MASK_4_BITS & MacAlgo);
    if (!fInit)
    {
        CommandToken->W[6] |= BIT_4;
    }
    if (!fFinalize)
    {
        CommandToken->W[6] |= BIT_5;
    }
}
static inline void
Eip130Token_Command_Mac_SetTotalMessageLength(
        Eip130Token_Command_t * const CommandToken,
        const AX_U64 TotalMessageLengthInBytes)
{
    CommandToken->W[24] = (AX_U32)(TotalMessageLengthInBytes);
    CommandToken->W[25] = (AX_U32)(TotalMessageLengthInBytes >> 32);
}
static inline void
Eip130Token_Command_Mac_SetASLoadKey(
        Eip130Token_Command_t * const CommandToken,
        const AX_U32 AssetId)
{
    CommandToken->W[6] |= BIT_8;
    CommandToken->W[28] = AssetId;
}
static inline void
Eip130Token_Command_Mac_SetASLoadMAC(
        Eip130Token_Command_t * const CommandToken,
        const AX_U32 AssetId)
{
    CommandToken->W[6] |= BIT_9;
    CommandToken->W[8] = AssetId;
}
static inline void
Eip130Token_Command_Mac_CopyKey(
        Eip130Token_Command_t * const CommandToken,
        const AX_U8 * const Key_p,
        const AX_U32 KeyLengthInBytes)
{
    CommandToken->W[6] |= ((MASK_8_BITS & KeyLengthInBytes) << 16);
    Eip130Token_Command_WriteByteArray(CommandToken, 28, Key_p, KeyLengthInBytes);
}
static inline void
Eip130Token_Command_Mac_CopyMAC(
        Eip130Token_Command_t * const CommandToken,
        const AX_U8 * const MAC_p,
        const AX_U32 MACLenInBytes)
{
    Eip130Token_Command_WriteByteArray(CommandToken, 8,
                                       MAC_p, MACLenInBytes);
}
static inline void
Eip130Token_Result_Mac_CopyMAC(
        Eip130Token_Result_t * const ResultToken_p,
        const AX_U32 MACLenInBytes,
        AX_U8 * MAC_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 2, MACLenInBytes, MAC_p);
}
static inline void
Eip130Token_Command_RandomNumber_Generate(
        Eip130Token_Command_t * const CommandToken,
        const AX_U16 NumberLengthInBytes,
        const AX_U64 OutputDataAddress)
{
    CommandToken->W[0] = (EIP130TOKEN_OPCODE_TRNG << 24) |
                           (EIP130TOKEN_SUBCODE_RANDOMNUMBER << 28);
    CommandToken->W[2] = NumberLengthInBytes;
    CommandToken->W[3] = (AX_U32)(OutputDataAddress);
    CommandToken->W[4] = (AX_U32)(OutputDataAddress >> 32);
}
static inline void
Eip130Token_Command_TRNG_Configure(
        Eip130Token_Command_t * const CommandToken,
        const AX_U8  AutoSeed,
        const AX_U16 SampleCycles,
        const AX_U8  SampleDiv,
        const AX_U8  NoiseBlocks)
{
    CommandToken->W[0] = (EIP130TOKEN_OPCODE_TRNG << 24) |
                           (EIP130TOKEN_SUBCODE_TRNGCONFIG << 28);
    CommandToken->W[2] = (AX_U32)((AX_U32)(AutoSeed << 8) | BIT_0);
    CommandToken->W[3] = (AX_U32)((SampleCycles << 16) |
                                      ((SampleDiv & 0x0F) << 8) |
                                      NoiseBlocks);
}
static inline void
Eip130Token_Command_AssetCreate(
        Eip130Token_Command_t * const CommandToken,
        const AX_U64 Policy,
        const AX_U32 LengthInBytes)
{
    CommandToken->W[0] = (EIP130TOKEN_OPCODE_ASSETMANAGEMENT << 24) |
                           (EIP130TOKEN_SUBCODE_ASSETCREATE << 28);
    CommandToken->W[2] = (AX_U32)(Policy & 0xffffffff);
    CommandToken->W[3] = (AX_U32)(Policy >> 32);
    CommandToken->W[4] = (LengthInBytes & MASK_10_BITS) | BIT_28;
    CommandToken->W[5] = 0;
    CommandToken->W[6] = 0;
}
static inline void
Eip130Token_Command_AssetDelete(
        Eip130Token_Command_t * const CommandToken,
        const AX_U32 AssetId)
{
    CommandToken->W[0] = (EIP130TOKEN_OPCODE_ASSETMANAGEMENT << 24) |
                           (EIP130TOKEN_SUBCODE_ASSETDELETE << 28);
    CommandToken->W[2] = AssetId;
}
static inline void
Eip130Token_Command_AssetLoad_Plaintext(
        Eip130Token_Command_t * const CommandToken,
        const AX_U32 AssetId)
{
    CommandToken->W[0] = (EIP130TOKEN_OPCODE_ASSETMANAGEMENT << 24) |
                           (EIP130TOKEN_SUBCODE_ASSETLOAD << 28);
    CommandToken->W[2] = AssetId;
    CommandToken->W[3] = BIT_27;     // Plaintext
    CommandToken->W[4] = 0;
    CommandToken->W[5] = 0;
    CommandToken->W[6] = 0;
    CommandToken->W[7] = 0;
    CommandToken->W[8] = 0;
}
static inline void
Eip130Token_Command_Pk_Asset_Command(
        Eip130Token_Command_t * const CommandToken,
        const AX_U8 Command,
        const AX_U8 Nwords,
        const AX_U8 Mwords,
        const AX_U8 OtherLen,
        const AX_U32 KeyAssetId,
        const AX_U32 ParamAssetId,
        const AX_U32 IOAssetId,
        const AX_U64 InputDataAddress,
        const AX_U16 InputDataLengthInBytes,
        const AX_U64 OutputDataAddress,       // or Signature address
        const AX_U16 OutputDataLengthInBytes) // or Signature length
{
    CommandToken->W[0]  = (EIP130TOKEN_OPCODE_PUBLIC_KEY << 24) |
                            (EIP130TOKEN_SUBCODE_PK_WITHASSETS << 28);
    CommandToken->W[2]  = (AX_U32)(Command | // PK operation to perform
                                       (Nwords << 16) |
                                       (Mwords << 24));
    CommandToken->W[3]  = (AX_U32)(OtherLen << 8);
    CommandToken->W[4]  = KeyAssetId; // asset containing x and y coordinates of pk
    CommandToken->W[5]  = ParamAssetId; // public key parameters:
                                          // p, a, b, n, base x, base y[, h]
    CommandToken->W[6]  = IOAssetId;
    CommandToken->W[7]  = ((MASK_12_BITS & OutputDataLengthInBytes) << 16 ) |
                             (MASK_12_BITS & InputDataLengthInBytes);
    CommandToken->W[8]  = (AX_U32)(InputDataAddress);
    CommandToken->W[9]  = (AX_U32)(InputDataAddress >> 32);
    CommandToken->W[10] = (AX_U32)(OutputDataAddress);
    CommandToken->W[11] = (AX_U32)(OutputDataAddress >> 32);
}
static inline void
Eip130Token_Command_AssetLoad_SetInput(
        Eip130Token_Command_t * const CommandToken,
        const AX_U64 DataAddress,
        const AX_U32 DataLengthInBytes)
{
    CommandToken->W[3] |= (DataLengthInBytes & MASK_10_BITS);
    CommandToken->W[4]  = (AX_U32)(DataAddress);
    CommandToken->W[5]  = (AX_U32)(DataAddress >> 32);
}
static inline void
Eip130Token_Command_Pk_Asset_SetAdditionalLength(
        Eip130Token_Command_t * const CommandToken,
        const AX_U64 AddLength)
{
    AX_U32 offset = ((CommandToken->W[3] & 0xFF) + 3) & (AX_U32)~3;
    CommandToken->W[3] &= (AX_U32)~0xFF;
    CommandToken->W[3] |= (offset + (2 * (AX_U32)sizeof(AX_U32)));
    CommandToken->W[12 + (offset / sizeof(AX_U32))] = (AX_U32)(AddLength);
    CommandToken->W[13 + (offset / sizeof(AX_U32))] = (AX_U32)(AddLength >> 32);
}

#endif