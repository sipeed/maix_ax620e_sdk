/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_CIPHER_H__
#define __AX_CIPHER_H__

#include "ax_base_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/** Cipher algorithm */
typedef enum {
    AX_CIPHER_ALGO_HASH_SHA1    = 0,          // SHA-1
    AX_CIPHER_ALGO_HASH_SHA224  = 1,          // SHA-224
    AX_CIPHER_ALGO_HASH_SHA256  = 2,          // SHA-256
    AX_CIPHER_ALGO_HASH_SHA384  = 3,          // SHA-384
    AX_CIPHER_ALGO_HASH_SHA512  = 4,          // SHA-512
    AX_CIPHER_ALGO_MAC_HMAC_SHA1 = 5,         // HMAC-SHA-1
    AX_CIPHER_ALGO_MAC_HMAC_SHA224 = 6,       // HMAC-SHA-224
    AX_CIPHER_ALGO_MAC_HMAC_SHA256 = 7,       // HMAC-SHA-256
    AX_CIPHER_ALGO_MAC_HMAC_SHA384 = 8,       // HMAC-SHA-384
    AX_CIPHER_ALGO_MAC_HMAC_SHA512 = 9,       // HMAC-SHA-512
    AX_CIPHER_ALGO_MAC_AES_CMAC = 10,         // AES-CMAC
    AX_CIPHER_ALGO_MAC_AES_CBC_MAC = 11,      // AES-CBC-MAC
    AX_CIPHER_ALGO_CIPHER_AES = 12,           // AES
    AX_CIPHER_ALGO_CIPHER_DES = 13,           // DES
    AX_CIPHER_ALG_INVALID = 0xffffffff,
} AX_CIPHER_ALGO_E;
typedef enum {
    // (Block)Cipher modes
    AX_CIPHER_MODE_CIPHER_ECB = 0,        // ECB
    AX_CIPHER_MODE_CIPHER_CBC,            // CBC
    AX_CIPHER_MODE_CIPHER_CTR,            // CTR
    AX_CIPHER_MODE_CIPHER_ICM,            // ICM
    AX_CIPHER_MODE_CIPHER_F8,             // F8
    AX_CIPHER_MODE_CIPHER_CCM,            // CCM
    AX_CIPHER_MODE_CIPHER_XTS,            // XTS
    AX_CIPHER_MODE_CIPHER_GCM,            // GCM
    AX_CIPHER_MODE_CIPHER_MAX,            // must be last
} AX_CIPHER_MODE_E;

typedef enum AX_CIPHER_RSA_SIGN_SCHEME_E {
    AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA1 = 0x0,
    AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA224,
    AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA256,
    AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA1,
    AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA224,
    AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA256,
    AX_CIPHER_RSA_SIGN_SCHEME_INVALID  = 0xffffffff,
} AX_CIPHER_RSA_SIGN_SCHEME_E;
typedef struct {
    AX_U8 *hmacKey;
    AX_U32 hmackeyLen;
    AX_CIPHER_ALGO_E hashType;
} AX_CIPHER_HASH_CTL_S;
typedef struct {
    AX_U32 hashBits;
    AX_U32 modulusBits;
    AX_U8 *modulusData;
    AX_U32 privateExponentBytes;
    AX_U8 *exponentData;
    AX_CIPHER_RSA_SIGN_SCHEME_E enScheme;
} AX_CIPHER_RSA_PRIVATE_KEY;

typedef struct {
    AX_U32 hashBits;
    AX_U32 modulusBits;
    AX_U8 *modulusData;
    AX_U32 publicExponentBytes;
    AX_U8 *exponentData;
    AX_CIPHER_RSA_SIGN_SCHEME_E enScheme;
} AX_CIPHER_RSA_PUBLIC_KEY;
typedef struct {
    AX_U8 *data;
    AX_U32 len;             // Data size in bytes
} AX_CIPHER_SIG_DATA_S;

typedef enum {
    AX_CIPHER_RSA_ENC_SCHEME_NO_PADDING,
    AX_CIPHER_RSA_ENC_SCHEME_PKCS1_V1_5,
    AX_CIPHER_RSA_ENC_SCHEME_OAEP,
} AX_CIPHER_RSA_ENC_SCHEME_E;

typedef struct {
    AX_CIPHER_RSA_ENC_SCHEME_E enScheme;
    AX_CIPHER_RSA_PUBLIC_KEY pubKey;
} AX_CIPHER_RSA_PUB_ENC_S;
typedef struct {
    AX_CIPHER_RSA_ENC_SCHEME_E enScheme;
    AX_CIPHER_RSA_PRIVATE_KEY priKey;
} AX_CIPHER_RSA_PRI_ENC_S;

typedef enum {
    AX_CIPHER_SUCCESS = 0,                    /** No error */
    AX_CIPHER_INVALID_TOKEN = 0x80020001,             /** Invalid token */
    AX_CIPHER_INVALID_PARAMETER = 0x80020002,         /** Invalid parameter */
    AX_CIPHER_INVALID_KEYSIZE = 0x80020003,           /** Invalid key size */
    AX_CIPHER_INVALID_LENGTH = 0x80020004,            /** Invalid length */
    AX_CIPHER_INVALID_LOCATION = 0x80020005,          /** Invalid location */
    AX_CIPHER_CLOCK_ERROR = 0x80020006,               /** Clock error */
    AX_CIPHER_ACCESS_ERROR = 0x80020007,              /** Access error */
    AX_CIPHER_UNWRAP_ERROR = 0x80020008,             /** Unwrap error */
    AX_CIPHER_DATA_OVERRUN_ERROR = 0x80020009,       /** Data overrun error */
    AX_CIPHER_ASSET_CHECKSUM_ERROR = 0x8002000A,     /** Asset checksum error */
    AX_CIPHER_INVALID_ASSET = 0x8002000B,            /** Invalid Asset */
    AX_CIPHER_FULL_ERROR = 0x8002000C,               /** Full/Overflow error */
    AX_CIPHER_INVALID_ADDRESS = 0x8002000D,          /** Invalid address */
    AX_CIPHER_INVALID_MODULUS = 0x8002000E,          /** Invalid Modulus */
    AX_CIPHER_VERIFY_ERROR = 0x8002000F,             /** Verify error */
    AX_CIPHER_INVALID_STATE = 0x80020010,            /** Invalid state */
    AX_CIPHER_OTP_WRITE_ERROR = 0x80020011,          /** OTP write error */
    AX_CIPHER_ASSET_EXPIRED = 0x80020012,            /** Asset expired error */
    AX_CIPHER_COPROCESSOR_ERROR = 0x80020013,        /** Coprocessor error */
    AX_CIPHER_PANIC_ERROR = 0x80020014,              /** Panic error */
    AX_CIPHER_TRNG_SHUTDOWN_ERROR = 0x80020015,      /** Too many FROs shutdown */
    AX_CIPHER_DRBG_STUCK_ERROR = 0x80020016,         /** Stuck DRBG */
    AX_CIPHER_UNSUPPORTED = 0x80020017,             /** Not supported */
    AX_CIPHER_NOT_INITIALIZED = 0x80020018,         /** Not initialized yet */
    AX_CIPHER_BAD_ARGUMENT = 0x80020019,            /** Wrong use; not depending on configuration */
    AX_CIPHER_INVALID_ALGORITHM = 0x8002001A,       /** Invalid algorithm code */
    AX_CIPHER_INVALID_MODE = 0x8002001B,            /** Invalid mode code */
    AX_CIPHER_BUFFER_TOO_SMALL = 0x8002001C,        /** Provided buffer too small for intended use */
    AX_CIPHER_NO_MEMORY = 0x8002001D,               /** No memory */
    AX_CIPHER_OPERATION_FAILED = 0x8002001E,        /** Operation failed */
    AX_CIPHER_TIMEOUT_ERROR = 0x8002001F,           /** Token or data timeout error */
    AX_CIPHER_INTERNAL_ERROR = 0x80020020,          /** Internal error */
    AX_CIPHER_LOAD_KEY_ERROR = 0x80020021,          /** load key error */
} AX_CIPHER_STS;


typedef AX_U64                  AX_CIPHER_HANDLE;
typedef struct {
    AX_CIPHER_ALGO_E alg;        /**< Cipher algorithm */
    AX_CIPHER_MODE_E workMode;   /**< Operating mode */
    AX_U8 *pKey;               /**< Key input */
    AX_U32 keySize;                /**< Key size */
    AX_U8 *pIV;                /**< Initialization vector (IV) */
} AX_CIPHER_CTRL_S;

AX_S32 AX_CIPHER_Init(AX_VOID);
AX_S32 AX_CIPHER_DeInit(AX_VOID);
AX_S32 AX_CIPHER_CreateHandle(AX_CIPHER_HANDLE *phCipher, const AX_CIPHER_CTRL_S *pstCipherCtrl);
AX_S32 AX_CIPHER_Encrypt(AX_CIPHER_HANDLE pCipher, AX_U8 *szSrcAddr, AX_U8 *szDestAddr, AX_U32 byteLength);
AX_S32 AX_CIPHER_Decrypt(AX_CIPHER_HANDLE pCipher, AX_U8 *szSrcAddr, AX_U8 *szDestAddr, AX_U32 byteLength);
AX_S32 AX_CIPHER_DestroyHandle(AX_CIPHER_HANDLE pCipher);
AX_S32 AX_CIPHER_RsaVerify(AX_CIPHER_RSA_PUBLIC_KEY *key, AX_U8 *msg, AX_U32 msgBytes, AX_CIPHER_SIG_DATA_S *sig);
AX_S32 AX_CIPHER_RsaSign(AX_CIPHER_RSA_PRIVATE_KEY *key, AX_U8 *msg, AX_U32 msgBytes, AX_CIPHER_SIG_DATA_S *sig);
AX_U32 AX_CIPHER_RsaPublicEncrypt(AX_CIPHER_RSA_PUB_ENC_S *pRsaEnc, AX_U8 *pInput, AX_U32 inLen, AX_U8 *pOutput,
                                  AX_U32 *pOutLen);
AX_U32 AX_CIPHER_RsaPublicDecrypt(AX_CIPHER_RSA_PUB_ENC_S *pRsaDec, AX_U8 *pInput, AX_U32 inLen, AX_U8 *pOutput,
                                  AX_U32 *pOutLen);
AX_U32 AX_CIPHER_RsaPrivateDecrypt(AX_CIPHER_RSA_PRI_ENC_S *pRsaDec, AX_U8 *pInput, AX_U32 inLen, AX_U8 *pOutput,
                                   AX_U32 *pOutLen);
AX_U32 AX_CIPHER_RsaPrivateEncrypt(AX_CIPHER_RSA_PRI_ENC_S *pRsaEnc, AX_U8 *pInput, AX_U32 inLen, AX_U8 *pOutput,
                                   AX_U32 *pOutLen);
AX_S32 AX_CIPHER_HashInit(AX_CIPHER_HASH_CTL_S *pstHashCtl, AX_CIPHER_HANDLE *pHashHandle);
AX_S32 AX_CIPHER_HashUpdate(AX_CIPHER_HANDLE handle, AX_U8 *inputData, AX_U32 inPutLen);
AX_S32 AX_CIPHER_HashFinal(AX_CIPHER_HANDLE handle, AX_U8 *inputData, AX_U32 inPutLen, AX_U8 *outPutHash);
AX_U32 AX_CIPHER_GetRandomNumber(AX_U32 *pRandomNumber, AX_U32 size);
//

void *allocDdrBuffer(AX_ADDR size);
void *freeDdrBuffer(AX_ADDR size);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __AX_CIPHER_H__ */
