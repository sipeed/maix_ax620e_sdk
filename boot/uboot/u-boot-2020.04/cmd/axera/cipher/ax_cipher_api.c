/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <common.h>
#include "ax_base_type.h"
#include "ax_cipher_api.h"
#include <string.h>
#include <linux/dma-mapping.h>
#include "ax_base_type.h"
#include "eip130_drv.h"
#include "ax_cipher_api.h"
#include "ax_cipher_token_crypto.h"

#define HASH_RESULT_MAX_LEN_IN_WORD      (16)
#define HASH_MAX_BLOCK_SIZE         (0x100000)
#define CE_KEY_DATA_VHEADER       (4)
typedef struct AX_CIPHER_CHN_S {
    AX_BOOL bIsUsed;
    AX_CIPHER_CTRL_S stCipherCtrl;
} AX_CIPHER_CHN_S;

typedef struct AX_HASH_INFO_S {
    AX_U8 bIsUsed;
    AX_U8 bIsFirst;
    AX_U8 digistLen;
    AX_U8 bIsMac;
    AX_S32 hashType;
    AX_U32 keyLen;
    AX_U64 u64TotalDataLen;
    AX_U32 u32LastBlockSize;
    AX_U32 u32BlockSize;
    AX_U8  u8LastBlock[HASH_RESULT_MAX_LEN_IN_WORD * 2 * 4];
    AX_U32 u32ShaVal[HASH_RESULT_MAX_LEN_IN_WORD];
    AX_U32 u32MacKey[HASH_RESULT_MAX_LEN_IN_WORD];
} HASH_INFO_S;

#define AX_CIPHER_MAX_CHN 4
#define HASH_CHANNAL_MAX_NUM         (8)
#define AX_CIPHER_RANDOM_MAX_BLK_SIZE (0xFFFF)
#define AX_CIPHER_CRYPTO_MAX_SIZE 0xFFF00 //for AES-ICM max block size must be little than 1MB
#define CE_DATA_SIZE_B2WB(x)  (4*((x+31)/32))
#define CE_DATA_SIZE_VWB(x)   (4 + CE_DATA_SIZE_B2WB(x))
#define CE_DATA_WORD_B2WB(x)  (((x+31)/32))
#define CE_DATA_SIZE_B2B(x)   ((x+7)/8)

static AX_CIPHER_CHN_S   s_cipherChn[AX_CIPHER_MAX_CHN];
static HASH_INFO_S   s_cipherHashData[HASH_CHANNAL_MAX_NUM];
static AX_U32 trngConfiged = 0;
AX_S32 AX_CIPHER_Init(AX_VOID)
{
    return CeInit();
}
AX_S32 AX_CIPHER_DeInit(AX_VOID)
{
    CeDisable();
    return 0;
}
typedef struct {
    AX_VOID *virAddr;
    AX_VOID *pyhyAddr;
    AX_U32 size;
} AX_CIPHER_CMM_T;
#define AX_PRINT                            printf
#define AX_CIPHER_DBG(fmt...)               //AX_PRINT(fmt)
#define AX_CIPHER_ERR(fmt...)               AX_PRINT(fmt)
#define AX_CIPHER_INFO(fmt...)              AX_PRINT(fmt)

AX_S32 AX_CIPHER_CMM_Alloc(AX_S32 size, AX_S32 align, AX_CIPHER_CMM_T *cmm)
{
    if (!cmm) {
        return AX_CIPHER_INVALID_PARAMETER;
    }
    cmm->virAddr = memalign(align, size + align - 1);
    if (!cmm->virAddr) {
        AX_CIPHER_ERR("Alloc memory failed, size:%x, align:%x!\n", size, align);
        return AX_CIPHER_NO_MEMORY;
    }
    cmm->pyhyAddr = cmm->virAddr;
    cmm->size = size;
    return AX_CIPHER_SUCCESS;
}
AX_VOID AX_CIPHER_CMM_Free(AX_CIPHER_CMM_T *cmm)
{
    if (cmm->virAddr) {
        free(cmm->virAddr);
        cmm->virAddr = 0;
        cmm->pyhyAddr = 0;
    }
}
AX_VOID AX_CIPHER_CacheFlush(AX_VOID *buf, AX_S32 size)
{
    flush_cache((unsigned long)buf, (size  + ARCH_DMA_MINALIGN - 1) / ARCH_DMA_MINALIGN * ARCH_DMA_MINALIGN);
}
AX_VOID AX_CIPHER_CacheInvalid(AX_VOID *buf, AX_S32 size)
{
    flush_cache((unsigned long)buf, (size  + ARCH_DMA_MINALIGN - 1) / ARCH_DMA_MINALIGN * ARCH_DMA_MINALIGN);
    //invalidate_dcache_range(buf, size);
}
AX_VOID AX_CIPHER_MemoryCpy(AX_VOID *dst, AX_VOID *src, AX_S32 size)
{
    memcpy(dst, src, size);
}

static int CipherParamCheck(AX_CIPHER_ALGO_E Algorithm)
{
    switch (Algorithm) {
    case AX_CIPHER_ALGO_CIPHER_AES:
        break;
    default:
        return AX_CIPHER_INVALID_PARAMETER;
    }
    return AX_CIPHER_SUCCESS;
}

AX_S32 AX_CIPHER_CreateHandle(AX_CIPHER_HANDLE *phCipher, const AX_CIPHER_CTRL_S *pstCipherCtrl)
{
    int i;
    if ((phCipher == 0) || (pstCipherCtrl == 0)) {
        return AX_CIPHER_INVALID_PARAMETER;
    }
    for (i = 0; i < AX_CIPHER_MAX_CHN; i++) {
        if (s_cipherChn[i].bIsUsed == AX_FALSE) {
            break;
        }
    }
    if (CipherParamCheck(pstCipherCtrl->alg) != AX_CIPHER_SUCCESS) {
        return AX_CIPHER_INVALID_PARAMETER;
    }
    memset(&s_cipherChn[i], 0, sizeof(s_cipherChn[i]));
    if (i < AX_CIPHER_MAX_CHN) {
        s_cipherChn[i].bIsUsed = AX_TRUE;
        s_cipherChn[i].stCipherCtrl = *pstCipherCtrl;
        *phCipher = i;
        return 0;
    }
    return AX_CIPHER_FULL_ERROR;
}
AX_S32 AX_CIPHER_CryptoBlock(AX_CIPHER_CHN_S *phCipher, AX_U8 *szSrcAddr, AX_U8 *szDestAddr, AX_U32 byteLength,
                             AX_BOOL encrypt)
{
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    AX_CIPHER_TokenModeCipher mode;
    switch (phCipher->stCipherCtrl.workMode) {
    case AX_CIPHER_MODE_CIPHER_ECB:
        mode = TOKEN_MODE_CIPHER_ECB;
        break;
    case AX_CIPHER_MODE_CIPHER_CBC:
        mode = TOKEN_MODE_CIPHER_CBC;
        break;
    case AX_CIPHER_MODE_CIPHER_CTR:
        mode = TOKEN_MODE_CIPHER_CTR;
        break;
    case AX_CIPHER_MODE_CIPHER_ICM:
        mode = TOKEN_MODE_CIPHER_ICM;
        break;
    case AX_CIPHER_MODE_CIPHER_F8:
        mode = TOKEN_MODE_CIPHER_F8;
        break;
    case AX_CIPHER_MODE_CIPHER_CCM:
        mode = TOKEN_MODE_CIPHER_CCM;
        break;
    case AX_CIPHER_MODE_CIPHER_XTS:
        mode = TOKEN_MODE_CIPHER_XTS;
        break;
    case AX_CIPHER_MODE_CIPHER_GCM:
        mode = TOKEN_MODE_CIPHER_GCM;
        break;
    default:
        return AX_CIPHER_INVALID_PARAMETER;
    }
    Eip130Token_Command_Crypto_Operation(&CommandToken, EIP130TOKEN_CRYPTO_ALGO_AES, (AX_U8)mode, encrypt, byteLength);
    Eip130Token_Command_Crypto_SetDataAddresses(&CommandToken, (AX_ULONG)szSrcAddr, byteLength, (AX_ULONG)szDestAddr,
            byteLength);
    Eip130Token_Command_Crypto_CopyKey(&CommandToken, phCipher->stCipherCtrl.pKey, phCipher->stCipherCtrl.keySize);
    Eip130Token_Command_Crypto_SetKeyLength(&CommandToken, phCipher->stCipherCtrl.keySize);
    if (mode != TOKEN_MODE_CIPHER_ECB) {
        // From token
        Eip130Token_Command_Crypto_CopyIV(&CommandToken, phCipher->stCipherCtrl.pIV);
    }
    return Eip130PhysicalTokenExchange((AX_U32 *)&CommandToken, (AX_U32 *)&ResultToken, 1);
}

AX_S32 AX_CIPHER_Crypto(AX_CIPHER_HANDLE pCipher, AX_U8 *szSrcAddr, AX_U8 *szDestAddr, AX_U32 byteLength,
                        AX_BOOL encrypt)
{
    AX_CIPHER_CHN_S *phCipher = (AX_CIPHER_CHN_S *)&s_cipherChn[pCipher];
    AX_S32 algoBlockSize = 16;
    AX_S32 blkCnt = 0;
    AX_S32 blkSize = 0;
    AX_CIPHER_CMM_T cmm;
    AX_CIPHER_CMM_T cmmDst;
    void *phyAddr;
    void *virAddr;
    int ret;
    if ((byteLength % algoBlockSize) != 0) {
        return AX_CIPHER_INVALID_PARAMETER;
    }
    if (byteLength > AX_CIPHER_CRYPTO_MAX_SIZE) {
        blkSize = AX_CIPHER_CRYPTO_MAX_SIZE;
    } else {
        blkSize = byteLength;
    }
    blkCnt = byteLength / blkSize;
    ret = AX_CIPHER_CMM_Alloc(blkSize, ARCH_DMA_MINALIGN, &cmm);
    if (ret < 0) {
        AX_CIPHER_ERR("Alloc memory failed!\n");
        return AX_CIPHER_NO_MEMORY;
    }
    ret = AX_CIPHER_CMM_Alloc(blkSize, ARCH_DMA_MINALIGN, &cmmDst);
    if (ret < 0) {
        AX_CIPHER_ERR("Alloc memory failed!\n");
        AX_CIPHER_CMM_Free(&cmm);
        return AX_CIPHER_NO_MEMORY;
    }
    AX_CIPHER_DBG("Crypto, src:%lx, dst:%lx, byteLength:%x, blkCnt:%d\n", (AX_ULONG)szSrcAddr, (AX_ULONG)szDestAddr,
                  byteLength, blkCnt);
    phyAddr = cmm.pyhyAddr;
    virAddr = cmm.virAddr;
    while (blkCnt--) {
        AX_CIPHER_MemoryCpy(virAddr, szSrcAddr, blkSize);
        AX_CIPHER_CacheFlush(virAddr, blkSize);
        AX_CIPHER_DBG("Crypto, src:%lx, dst:%lx, len: %\n", (AX_ULONG)szSrcAddr, (AX_ULONG)szDestAddr, blkSize);
        AX_CIPHER_CacheInvalid(cmmDst.virAddr, blkSize);
        ret = AX_CIPHER_CryptoBlock(phCipher, phyAddr, cmmDst.pyhyAddr, blkSize, encrypt);
        if (ret < 0) {
            AX_CIPHER_CMM_Free(&cmm);
            AX_CIPHER_CMM_Free(&cmmDst);
            AX_CIPHER_ERR("CryptoBlock failed ret: %d, szSrcAddr:%lx, szDestAddr:%lx,byteLength:%x\n", ret, (AX_ULONG)szSrcAddr,
                          (AX_ULONG)szDestAddr,
                          byteLength);
            return ret;
        }
        AX_CIPHER_MemoryCpy(szDestAddr, cmmDst.virAddr, blkSize);
        szSrcAddr += blkSize;
        byteLength -= blkSize;
        szDestAddr += blkSize;
    }
    if (byteLength) {
        AX_CIPHER_DBG("Crypto, src:%lx, dst:%lx, byteLength: %x\n", (AX_ULONG)szSrcAddr, (AX_ULONG)szDestAddr, byteLength);
        AX_CIPHER_MemoryCpy(virAddr, szSrcAddr, byteLength);
        AX_CIPHER_CacheFlush(virAddr, byteLength);
        AX_CIPHER_CacheInvalid(szDestAddr, byteLength);
        AX_CIPHER_MemoryCpy(szDestAddr, cmmDst.virAddr, byteLength);
        ret = AX_CIPHER_CryptoBlock(phCipher, phyAddr, szDestAddr, byteLength, encrypt);
        if (ret < 0) {
            AX_CIPHER_ERR("CryptoBlock failed ret: %d, szSrcAddr:%lx, szDestAddr:%lx,byteLength:%x\n", ret, (AX_ULONG)szSrcAddr,
                          (AX_ULONG)szDestAddr,
                          byteLength);
        }
    }
    AX_CIPHER_CMM_Free(&cmm);
    AX_CIPHER_CMM_Free(&cmmDst);
    return ret;
}
AX_S32 AX_CIPHER_Encrypt(AX_CIPHER_HANDLE pCipher, AX_U8 *szSrcAddr, AX_U8 *szDestAddr, AX_U32 byteLength)
{
    return AX_CIPHER_Crypto(pCipher, szSrcAddr, szDestAddr, byteLength, AX_TRUE);
}
AX_S32 AX_CIPHER_Decrypt(AX_CIPHER_HANDLE pCipher, AX_U8 *szSrcAddr, AX_U8 *szDestAddr, AX_U32 byteLength)
{
    return AX_CIPHER_Crypto(pCipher, szSrcAddr, szDestAddr, byteLength, AX_FALSE);
}
AX_S32 AX_CIPHER_DestroyHandle(AX_CIPHER_HANDLE pCipher)
{
    if (pCipher < AX_CIPHER_MAX_CHN) {
        s_cipherChn[pCipher].bIsUsed = AX_FALSE;
        return AX_CIPHER_SUCCESS;
    }
    return AX_CIPHER_INVALID_PARAMETER;
}
AX_S32 AX_CIPHER_HashInit(AX_CIPHER_HASH_CTL_S *pstHashCtl, AX_CIPHER_HANDLE *pHashHandle)
{
    int i;
    HASH_INFO_S *hash;
    AX_U8 digistLen;
    for (i = 0; i < HASH_CHANNAL_MAX_NUM; i++) {
        if (!s_cipherHashData[i].bIsUsed) {
            break;
        }
    }
    if (i >= HASH_CHANNAL_MAX_NUM) {
        return AX_CIPHER_NO_MEMORY;
    }
    hash = &s_cipherHashData[i];
    memset(hash, 0, sizeof(HASH_INFO_S));
    switch (pstHashCtl->hashType) {
    case AX_CIPHER_ALGO_HASH_SHA1:
        hash->hashType = EIP130TOKEN_HASH_ALGORITHM_SHA1;
        digistLen = 20;
        break;
    case AX_CIPHER_ALGO_HASH_SHA224:
        hash->hashType = EIP130TOKEN_HASH_ALGORITHM_SHA224;
        digistLen = 28;
        break;
    case AX_CIPHER_ALGO_HASH_SHA256:
        hash->hashType = EIP130TOKEN_HASH_ALGORITHM_SHA256;
        digistLen = 32;
        break;
    case AX_CIPHER_ALGO_HASH_SHA512:
        hash->hashType = EIP130TOKEN_HASH_ALGORITHM_SHA512;
        digistLen = 64;
        break;
    case AX_CIPHER_ALGO_MAC_HMAC_SHA1:
        hash->hashType = EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA1;
        hash->bIsMac = true;
        hash->keyLen = pstHashCtl->hmackeyLen;
        memcpy(hash->u32MacKey, pstHashCtl->hmacKey, pstHashCtl->hmackeyLen);
        digistLen = 20;
        break;
    case AX_CIPHER_ALGO_MAC_HMAC_SHA224:
        hash->hashType = EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA224;
        hash->bIsMac = true;
        hash->keyLen = pstHashCtl->hmackeyLen;
        memcpy(hash->u32MacKey, pstHashCtl->hmacKey, pstHashCtl->hmackeyLen);
        digistLen = 28;
        break;
    case AX_CIPHER_ALGO_MAC_HMAC_SHA256:
        hash->hashType = EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA256;
        hash->bIsMac = true;
        hash->keyLen = pstHashCtl->hmackeyLen;
        memcpy(hash->u32MacKey, pstHashCtl->hmacKey, pstHashCtl->hmackeyLen);
        digistLen = 32;
        break;
    case AX_CIPHER_ALGO_MAC_HMAC_SHA512:
        hash->hashType = EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA512;
        hash->bIsMac = true;
        hash->keyLen = pstHashCtl->hmackeyLen;
        memcpy(hash->u32MacKey, pstHashCtl->hmacKey, pstHashCtl->hmackeyLen);
        digistLen = 64;
        break;
    default:
        return AX_CIPHER_INVALID_ALGORITHM;
    }
    hash->bIsUsed = true;
    *pHashHandle = i;
    hash->bIsFirst = true;
    hash->digistLen = digistLen;
    if (pstHashCtl->hashType == AX_CIPHER_ALGO_HASH_SHA512 ||
            pstHashCtl->hashType == AX_CIPHER_ALGO_MAC_HMAC_SHA512) {
        hash->u32BlockSize = 128;
    } else {
        hash->u32BlockSize = 64;
    }
    return AX_CIPHER_SUCCESS;
}
AX_S32 AX_CIPHER_HashUpdateBlock(HASH_INFO_S *hash, AX_U8 *inputData, AX_U32 inPutLen)
{
    int ret;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    bool fInitWithDefault;
    if (hash->bIsFirst) {
        fInitWithDefault = true;
    } else {
        fInitWithDefault = false;
    }
    memset(&CommandToken, 0, sizeof(CommandToken));
    hash->u64TotalDataLen += inPutLen;
    if (hash->bIsMac) {
        Eip130Token_Command_Mac(&CommandToken, (AX_U8)hash->hashType, fInitWithDefault, false, (AX_ULONG)inputData, inPutLen);
        Eip130Token_Command_Mac_CopyMAC(&CommandToken, (AX_U8 *)hash->u32ShaVal, sizeof(hash->u32ShaVal));
        Eip130Token_Command_Mac_CopyKey(&CommandToken, (AX_U8 *)hash->u32MacKey, hash->keyLen);
    } else {
        Eip130Token_Command_Hash(&CommandToken, (AX_U8)hash->hashType, fInitWithDefault, false, (AX_ULONG)inputData, inPutLen);
        Eip130Token_Command_Hash_CopyDigest(&CommandToken, (AX_U8 *)hash->u32ShaVal, sizeof(hash->u32ShaVal));

    }
    ret = Eip130PhysicalTokenExchange((AX_U32 *)&CommandToken, (AX_U32 *)&ResultToken, 1);
    if (ret == AX_CIPHER_SUCCESS) {
        Eip130Token_Result_Hash_CopyDigest(&ResultToken, HASH_RESULT_MAX_LEN_IN_WORD * 4,
                                           (AX_U8 *)hash->u32ShaVal);
        hash->bIsFirst = false;
    }
    return ret;
}
AX_S32 AX_CIPHER_HashUpdate(AX_CIPHER_HANDLE handle, AX_U8 *inputData, AX_U32 inPutLen)
{
    HASH_INFO_S *hash;
    AX_S32 handleLen = 0;
    AX_S32 blkLen = 0;
    AX_CIPHER_CMM_T cmm;
    AX_VOID *phyAddr = NULL;
    AX_VOID *virAddr = NULL;
    AX_S32 ret;
    AX_S32 isFirst = AX_TRUE;
    AX_BOOL blkAlloc = AX_FALSE;
    if (handle >= HASH_CHANNAL_MAX_NUM) {
        return AX_CIPHER_INVALID_PARAMETER;
    }
    hash = &s_cipherHashData[handle];
    AX_CIPHER_DBG("inPutLen: %x, u32LastBlockSize: %x, u32BlockSize: %d\n", inPutLen, hash->u32LastBlockSize,
                  hash->u32BlockSize);
    if ((inPutLen + hash->u32LastBlockSize) <= hash->u32BlockSize) {
        AX_CIPHER_MemoryCpy(hash->u8LastBlock + hash->u32LastBlockSize, inputData, inPutLen);
        hash->u32LastBlockSize += inPutLen;
        return AX_CIPHER_SUCCESS;
    }
    inPutLen += hash->u32LastBlockSize;
    if ((inPutLen % hash->u32BlockSize) == 0) {
        handleLen = inPutLen - hash->u32BlockSize;
    } else {
        handleLen = inPutLen - (inPutLen % hash->u32BlockSize);
    }
    if (handleLen > HASH_MAX_BLOCK_SIZE) {
        blkLen = HASH_MAX_BLOCK_SIZE;
    } else {
        blkLen = handleLen;
    }
    if (blkLen > 0) {
        ret = AX_CIPHER_CMM_Alloc(blkLen, ARCH_DMA_MINALIGN, &cmm);
        if (ret < 0) {
            AX_CIPHER_ERR("hash malloc memory failed!\n");
            return AX_CIPHER_NO_MEMORY;
        }
        blkAlloc = AX_TRUE;
        phyAddr = cmm.pyhyAddr;
        virAddr = cmm.virAddr;
        AX_CIPHER_MemoryCpy(virAddr, hash->u8LastBlock, hash->u32LastBlockSize);
        AX_CIPHER_MemoryCpy(virAddr + hash->u32LastBlockSize, inputData, blkLen - hash->u32LastBlockSize);
    }
    while (handleLen) {
        AX_CIPHER_CacheFlush(virAddr, blkLen);
        ret = AX_CIPHER_HashUpdateBlock(hash, phyAddr, blkLen);
        if (ret < 0) {
            AX_CIPHER_CMM_Free(&cmm);
            return AX_CIPHER_OPERATION_FAILED;
        }
        handleLen -= blkLen;
        inPutLen -= blkLen;
        if (isFirst == AX_TRUE) {
            inputData += blkLen - hash->u32LastBlockSize;
            isFirst = AX_FALSE;
        } else {
            inputData += blkLen;
        }
        if (handleLen > HASH_MAX_BLOCK_SIZE) {
            blkLen = HASH_MAX_BLOCK_SIZE;
        } else {
            blkLen = handleLen;
        }
        AX_CIPHER_MemoryCpy(virAddr, inputData, blkLen);
    }
    if (blkAlloc) {
        AX_CIPHER_CMM_Free(&cmm);
    }
    AX_CIPHER_MemoryCpy(hash->u8LastBlock, inputData, inPutLen);
    hash->u32LastBlockSize = inPutLen;
    return AX_CIPHER_SUCCESS;
}
AX_S32 AX_CIPHER_HashFinal(AX_CIPHER_HANDLE handle, AX_U8 *inputData, AX_U32 inPutLen, AX_U8 *outPutHash)
{
    int ret;
    HASH_INFO_S *hash;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    bool fInitWithDefault;
    AX_CIPHER_CMM_T cmm;
    void *phyAddr;
    void *virAddr;
    if ((inPutLen != 0) && (!inputData)) {
        return AX_CIPHER_INVALID_PARAMETER;
    }
    if (handle >= HASH_CHANNAL_MAX_NUM) {
        return AX_CIPHER_INVALID_PARAMETER;
    }
    hash = &s_cipherHashData[handle];

    ret = AX_CIPHER_HashUpdate(handle, inputData, inPutLen);
    if (ret < 0) {
        return ret;
    }
    ret = AX_CIPHER_CMM_Alloc(hash->u32LastBlockSize, ARCH_DMA_MINALIGN, &cmm);
    if (ret < 0) {
        AX_CIPHER_ERR("hash malloc memory failed!\n");
        return AX_CIPHER_NO_MEMORY;
    }
    phyAddr = cmm.pyhyAddr;
    virAddr = cmm.virAddr;
    AX_CIPHER_MemoryCpy(virAddr, hash->u8LastBlock, hash->u32LastBlockSize);
    if (hash->bIsFirst) {
        fInitWithDefault = true;
    } else {
        fInitWithDefault = false;
    }
    AX_CIPHER_CacheFlush(virAddr, hash->u32LastBlockSize);
    memset(&CommandToken, 0, sizeof(CommandToken));
    hash->u64TotalDataLen += hash->u32LastBlockSize;
    if (hash->bIsMac) {
        Eip130Token_Command_Mac(&CommandToken, (AX_U8)hash->hashType, fInitWithDefault, true, (AX_ULONG)phyAddr,
                                hash->u32LastBlockSize);
        Eip130Token_Command_Mac_CopyMAC(&CommandToken, (AX_U8 *)hash->u32ShaVal, sizeof(hash->u32ShaVal));
        Eip130Token_Command_Mac_CopyKey(&CommandToken, (AX_U8 *)hash->u32MacKey, hash->keyLen);
        Eip130Token_Command_Mac_SetTotalMessageLength(&CommandToken, hash->u64TotalDataLen);
    } else {
        Eip130Token_Command_Hash(&CommandToken, (AX_U8)hash->hashType, fInitWithDefault, true, (AX_ULONG)phyAddr,
                                 hash->u32LastBlockSize);
        Eip130Token_Command_Hash_CopyDigest(&CommandToken, (AX_U8 *)hash->u32ShaVal, sizeof(hash->u32ShaVal));
        Eip130Token_Command_Hash_SetTotalMessageLength(&CommandToken, hash->u64TotalDataLen);
    }
    ret = Eip130PhysicalTokenExchange((AX_U32 *)&CommandToken, (AX_U32 *)&ResultToken, 1);
    if (ret == AX_CIPHER_SUCCESS && !(ResultToken.W[0] & (1 << 31))) {
        Eip130Token_Result_Hash_CopyDigest(&ResultToken, hash->digistLen, outPutHash);
    } else {
        AX_CIPHER_ERR("%s token exchange error ResultToken.W[0]=0x%x!\n", __func__, ResultToken.W[0]);
        ret = AX_CIPHER_OPERATION_FAILED;
    }
    hash->bIsUsed = false;
    AX_CIPHER_CMM_Free(&cmm);
    return ret;
}
AX_U32 AX_CIPHER_TrngConfig(
    AX_U8  AutoSeed,
    AX_U16 SampleCycles,
    AX_U8  SampleDiv,
    AX_U8  NoiseBlocks,
    AX_BOOL   fReseed)
{
    int ret;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    memset(&CommandToken, 0, sizeof(CommandToken));
    memset(&ResultToken, 0, sizeof(ResultToken));
    // Configure
    Eip130Token_Command_TRNG_Configure(
        &CommandToken, AutoSeed, SampleCycles,
        SampleDiv, NoiseBlocks);
    if (fReseed) {
        // RRD = Reseed post-processor
        CommandToken.W[2] |= BIT_1;
    }
    CommandToken.W[0] |= 1;
    ret = Eip130PhysicalTokenExchange((AX_U32 *)&CommandToken, (AX_U32 *)&ResultToken, 1);
    if ((ret < 0) || (ResultToken.W[0] & (1 << 31))) {
        return AX_CIPHER_INTERNAL_ERROR;
    }
    return AX_CIPHER_SUCCESS;
}
AX_U32 AX_CIPHER_GetRandomNumber(AX_U32 *pRandomNumber, AX_U32 size)
{
    int ret;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    AX_CIPHER_CMM_T cmm;
    void *phyAddr;
    void *virAddr;
    if (!pRandomNumber || !size || (size > AX_CIPHER_RANDOM_MAX_BLK_SIZE)) {
        return AX_CIPHER_INVALID_PARAMETER;
    }
    if (!trngConfiged) {
        ret = AX_CIPHER_TrngConfig(0, 1, 0, 8, 1);
        if (ret < 0) {
            return AX_CIPHER_INTERNAL_ERROR;
        }
        trngConfiged = 1;
    }
    ret = AX_CIPHER_CMM_Alloc(size, ARCH_DMA_MINALIGN, &cmm);
    if (ret < 0) {
        AX_CIPHER_ERR("Alloc memory failed!\n");
        return AX_CIPHER_NO_MEMORY;
    }
    phyAddr = cmm.pyhyAddr;
    virAddr = cmm.virAddr;
    AX_CIPHER_DBG("GetRandomNumber, phyAddr:%lx, virAddr:%lx\n", (AX_ULONG)phyAddr, (AX_ULONG)virAddr);
    AX_CIPHER_CacheInvalid(virAddr, size);
    Eip130Token_Command_RandomNumber_Generate(&CommandToken, size, (AX_ULONG)phyAddr);
    ret = Eip130PhysicalTokenExchange((AX_U32 *)&CommandToken, (AX_U32 *)&ResultToken, 1);
    if ((ret < 0) || (ResultToken.W[0] & (1 << 31))) {
        AX_CIPHER_CMM_Free(&cmm);
        return AX_CIPHER_INTERNAL_ERROR;
    }
    AX_CIPHER_MemoryCpy(pRandomNumber, virAddr, size);
    AX_CIPHER_CMM_Free(&cmm);
    return AX_CIPHER_SUCCESS;
}
static AX_S32 AX_CIPHER_AssetAlloc(AX_U64 AssetPolicy, AX_U32 AssetSize, AX_U32 *AssetId_p)
{
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    int ret;
    memset(&CommandToken, 0, sizeof(CommandToken));
    Eip130Token_Command_AssetCreate(&CommandToken, AssetPolicy, AssetSize);
    ret = Eip130PhysicalTokenExchange((AX_U32 *)&CommandToken, (AX_U32 *)&ResultToken, 1);
    if ((ret < 0) || (ResultToken.W[0] & (1 << 31))) {
        AX_CIPHER_ERR("%s token exchange error ResultToken.W[0]=0x%x!\n", __func__, ResultToken.W[0]);
        return AX_CIPHER_INTERNAL_ERROR;
    }
    *AssetId_p = ResultToken.W[1];
    return AX_CIPHER_SUCCESS;
}
static AX_S32 AX_CIPHER_AssetFree(AX_U32 AssetId)
{
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    int ret;
    memset(&CommandToken, 0, sizeof(CommandToken));
    Eip130Token_Command_AssetDelete(&CommandToken, AssetId);
    ret = Eip130PhysicalTokenExchange((AX_U32 *)&CommandToken, (AX_U32 *)&ResultToken, 1);
    if ((ret < 0) || (ResultToken.W[0] & (1 << 31))) {
        return AX_CIPHER_INTERNAL_ERROR;
    }
    return AX_CIPHER_SUCCESS;
}

static void *AX_CIPHER_ReverseMemCpy(void *Dest, const void *Src, size_t Size)
{
    AX_U8 *dp = Dest;
    const AX_U8 *sp = Src;
    sp += (Size - 1);
    while (Size--) {
        *dp++ = *sp--;
    }
    return Dest;
}

static void AX_CIPHER_BigIntToHW(
    const AX_U8 *BigInt_p,
    const AX_U32 ModulusSizeBits,
    const AX_U8 BeginItem,
    const AX_U8 Items,
    AX_U8 *const Blob_p,
    AX_U32 CopySize)
{
    AX_U8 *ptr = (AX_U8 *)Blob_p;
    AX_U32 RemainingLength = (AX_U32)CE_DATA_SIZE_B2WB(ModulusSizeBits);

    // Convert big integer from application to HW format
    // - Initialize header
    *ptr++ = (AX_U8)(ModulusSizeBits);
    *ptr++ = (AX_U8)(ModulusSizeBits >> 8);
    *ptr++ = (AX_U8)(BeginItem);
    *ptr++ = (AX_U8)(Items);
    AX_CIPHER_ReverseMemCpy(ptr, BigInt_p, CopySize);
    RemainingLength -= CopySize;
    if (RemainingLength != 0) {
        ptr += CopySize;
        memset(ptr, 0, RemainingLength);
    }
}
static void AX_CIPHER_BigIntFromHW(const AX_U8 *const Blob_p, const AX_U32 ModulusSizeBits, AX_U8 *const Data_p)
{
    AX_U32 Size = (AX_U32)CE_DATA_SIZE_B2B(ModulusSizeBits);
    AX_CIPHER_ReverseMemCpy(Data_p, (Blob_p + 4), Size);
}
static AX_S32 AX_CIPHER_AssetLoad(AX_U8 *Modulus_p, AX_U32 modulusBits, AX_U8 *Exponent_p, AX_U32 ExponentSizeBits,
                                  AX_U32 AssetId, AX_U32 AssetSize)
{
    int ret;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    AX_CIPHER_CMM_T cmm;
    AX_VOID *phyAddr;
    AX_VOID *virAddr;

    memset(&CommandToken, 0, sizeof(CommandToken));
    ret = AX_CIPHER_CMM_Alloc(AssetSize, ARCH_DMA_MINALIGN, &cmm);
    if (ret < 0) {
        AX_CIPHER_ERR("%s, malloc memory failed!\n", __func__);
        return AX_CIPHER_NO_MEMORY;
    }
    phyAddr = cmm.pyhyAddr;
    virAddr = cmm.virAddr;
    AX_CIPHER_BigIntToHW(Modulus_p, modulusBits, 0, 2, virAddr, modulusBits / 8);
    AX_CIPHER_BigIntToHW(Exponent_p, ExponentSizeBits, 1, 2, virAddr +  CE_DATA_SIZE_VWB(modulusBits),
                         ExponentSizeBits / 8);

    AX_CIPHER_CacheFlush(virAddr, AssetSize);
    Eip130Token_Command_AssetLoad_Plaintext(&CommandToken, AssetId);
    Eip130Token_Command_AssetLoad_SetInput(&CommandToken, (AX_ULONG)phyAddr, AssetSize);
    ret = Eip130PhysicalTokenExchange((AX_U32 *)&CommandToken, (AX_U32 *)&ResultToken, 1);
    if ((ret < 0) || (ResultToken.W[0] & (1 << 31))) {
        AX_CIPHER_CMM_Free(&cmm);
        AX_CIPHER_ERR("%s token exchange error ResultToken.W[0]=0x%x!\n", __func__, ResultToken.W[0]);
        return AX_CIPHER_INTERNAL_ERROR;
    }
    AX_CIPHER_CMM_Free(&cmm);
    return AX_CIPHER_SUCCESS;
}
static AX_S32 AX_Hash_Compute(AX_CIPHER_HASH_CTL_S *hash_ctl, AX_U8 *msg, AX_U32 msgBytes, AX_U8 *hash, int hashLen)
{
    AX_CIPHER_HANDLE handle;
    AX_U32 hashResult[HASH_RESULT_MAX_LEN_IN_WORD];
    AX_S32 ret;
    ret = AX_CIPHER_HashInit(hash_ctl, &handle);
    if (ret) {
        return ret;
    }
    ret = AX_CIPHER_HashFinal(handle, msg, msgBytes, (AX_U8 *)hashResult);
    if (ret) {
        return ret;
    }
    memcpy(hash, hashResult, hashLen);
    return AX_CIPHER_SUCCESS;
}
AX_S32 AX_CIPHER_RsaVerify(AX_CIPHER_RSA_PUBLIC_KEY *key, AX_U8 *msg, AX_U32 msgBytes, AX_CIPHER_SIG_DATA_S *sig)
{
    AX_U32 AssetId;
    AX_U64 AssetPolicy = 0;
    AX_U32 AssetSize;
    AX_U32 hashResult[HASH_RESULT_MAX_LEN_IN_WORD];
    AX_CIPHER_HASH_CTL_S hash_ctl;
    AX_CIPHER_ALGO_E hashType;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    AX_U32 HashDataSize;
    AX_U32 SaltSize;
    AX_U32 method;
    AX_CIPHER_CMM_T cmm;
    int ret;
    switch (key->enScheme) {
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA1:
        AssetPolicy |= CE_POLICY_SHA1 | CE_POLICY_PK_RSA_PKCS1_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA1;
        HashDataSize = 20;
        break;
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA224:
        AssetPolicy |= CE_POLICY_SHA224 | CE_POLICY_PK_RSA_PKCS1_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA224;
        HashDataSize = 28;
        break;
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA256:
        AssetPolicy |= CE_POLICY_SHA256 | CE_POLICY_PK_RSA_PKCS1_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA256;
        HashDataSize = 32;
        break;
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA1:
        AssetPolicy |= CE_POLICY_SHA1 | CE_POLICY_PK_RSA_PSS_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA1;
        HashDataSize = 20;
        break;
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA224:
        AssetPolicy |= CE_POLICY_SHA224 | CE_POLICY_PK_RSA_PSS_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA224;
        HashDataSize = 28;
        break;
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA256:
        AssetPolicy |= CE_POLICY_SHA256 | CE_POLICY_PK_RSA_PSS_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA256;
        HashDataSize = 32;
        break;
    default:
        return AX_CIPHER_INVALID_PARAMETER;
    }
    if (key->modulusBits != (sig->len * 8)) {
        return AX_CIPHER_INVALID_PARAMETER;
    }
    if ((key->enScheme == AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA1)
        || (key->enScheme == AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA224)
        || (key->enScheme == AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA256)) {
        SaltSize = HashDataSize;
        method = CETOKEN_PKASSET_RSA_PSS_VERIFY;
    } else {
        SaltSize = 0;
        method = CETOKEN_PKASSET_RSA_PKCS1V1_5_VERIFY;
    }
    AssetSize = CE_DATA_SIZE_VWB(key->modulusBits) + CE_DATA_SIZE_VWB(key->publicExponentBytes * 8);
    AssetPolicy |= CE_POLICY_PUBLIC_KEY;
    ret = AX_CIPHER_AssetAlloc(AssetPolicy, AssetSize, &AssetId);
    if (ret < 0) {
        AX_CIPHER_ERR("%s asset alloc error!\n", __func__);
        return AX_CIPHER_INTERNAL_ERROR;
    }

    ret = AX_CIPHER_AssetLoad(key->modulusData, key->modulusBits, key->exponentData, key->publicExponentBytes * 8, AssetId,
                        AssetSize);
    if (ret < 0) {
        AX_CIPHER_AssetFree(AssetId);
        AX_CIPHER_ERR("%s asset load error!\n", __func__);
        return AX_CIPHER_INTERNAL_ERROR;
    }
    memset(&hash_ctl, 0, sizeof(hash_ctl));
    hash_ctl.hashType = hashType;
    ret = AX_Hash_Compute(&hash_ctl, msg, msgBytes, (AX_U8 *)hashResult, HashDataSize);
    if (ret < 0) {
        AX_CIPHER_AssetFree(AssetId);
        AX_CIPHER_ERR("%s hash compute error!\n", __func__);
        return AX_CIPHER_INTERNAL_ERROR;
    }
    ret = AX_CIPHER_CMM_Alloc(CE_DATA_SIZE_VWB(key->modulusBits), ARCH_DMA_MINALIGN, &cmm);
    if (ret < 0) {
        AX_CIPHER_ERR("%s malloc memory failed!\n", __func__);
        AX_CIPHER_AssetFree(AssetId);
        return AX_CIPHER_NO_MEMORY;
    }
    AX_CIPHER_BigIntToHW(sig->data, key->modulusBits, 0, 1, cmm.virAddr, key->modulusBits / 8);
    AX_CIPHER_CacheFlush(cmm.virAddr, CE_DATA_SIZE_VWB(key->modulusBits));
    memset(&CommandToken, 0, sizeof(CommandToken));
    memset(&ResultToken, 0, sizeof(ResultToken));
    Eip130Token_Command_Pk_Asset_Command(&CommandToken, method,
                                         CE_DATA_WORD_B2WB(key->modulusBits), CE_DATA_WORD_B2WB(key->publicExponentBytes * 8),
                                         (AX_U8)SaltSize, AssetId, 0,  0, 0, 0,
                                         (AX_ULONG)cmm.pyhyAddr, (AX_ULONG)key->modulusBits / 8 + 4);
    for (int i = 0; i < (HashDataSize / 4); i++) {
        CommandToken.W[12 + i] = hashResult[i];
    }
    CommandToken.W[3] |= HashDataSize | (1 << 30);
    ret = Eip130PhysicalTokenExchange((AX_U32 *)&CommandToken, (AX_U32 *)&ResultToken, 1);
    AX_CIPHER_CMM_Free(&cmm);
    if ((ret < 0) || (ResultToken.W[0] & (1 << 31))) {
        AX_CIPHER_AssetFree(AssetId);
        return AX_CIPHER_INTERNAL_ERROR;
    }
    return AX_CIPHER_AssetFree(AssetId);
}
AX_S32 AX_CIPHER_RsaSign(AX_CIPHER_RSA_PRIVATE_KEY *key, AX_U8 *msg, AX_U32 msgBytes, AX_CIPHER_SIG_DATA_S *sig)
{
    int ret;
    AX_U32 AssetId;
    AX_U64 AssetPolicy = 0;
    AX_U32 AssetSize;
    AX_U32 hashResult[HASH_RESULT_MAX_LEN_IN_WORD];
    AX_CIPHER_HASH_CTL_S hash_ctl;
    AX_CIPHER_ALGO_E hashType;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    AX_U32 HashDataSize;
    AX_U32 SaltSize;
    AX_U32 method;
    AX_CIPHER_CMM_T cmm;
    switch (key->enScheme) {
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA1:
        AssetPolicy |= CE_POLICY_SHA1 | CE_POLICY_PK_RSA_PKCS1_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA1;
        HashDataSize = 20;
        break;
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA224:
        AssetPolicy |= CE_POLICY_SHA224 | CE_POLICY_PK_RSA_PKCS1_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA224;
        HashDataSize = 28;
        break;
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA256:
        AssetPolicy |= CE_POLICY_SHA256 | CE_POLICY_PK_RSA_PKCS1_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA256;
        HashDataSize = 32;
        break;
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA1:
        AssetPolicy |= CE_POLICY_SHA1 | CE_POLICY_PK_RSA_PSS_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA1;
        HashDataSize = 20;
        break;
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA224:
        AssetPolicy |= CE_POLICY_SHA224 | CE_POLICY_PK_RSA_PSS_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA224;
        HashDataSize = 28;
        break;
    case AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA256:
        AssetPolicy |= CE_POLICY_SHA256 | CE_POLICY_PK_RSA_PSS_SIGN;
        hashType = AX_CIPHER_ALGO_HASH_SHA256;
        HashDataSize = 32;
        break;
    default:
        return AX_CIPHER_INVALID_PARAMETER;
    }
    if ((key->enScheme == AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA1)
        || (key->enScheme == AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA224)
        || (key->enScheme == AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA256)) {
        SaltSize = HashDataSize;
        method = CETOKEN_PKASSET_RSA_PSS_SIGN;
    } else {
        SaltSize = 0;
        method = CETOKEN_PKASSET_RSA_PKCS1V1_5_SIGN;
    }
    AssetSize = CE_DATA_SIZE_VWB(key->modulusBits) + CE_DATA_SIZE_VWB(key->privateExponentBytes * 8);
    ret = AX_CIPHER_AssetAlloc(AssetPolicy, AssetSize, &AssetId);
    if (ret < 0) {
        return AX_CIPHER_INTERNAL_ERROR;
    }
    AX_CIPHER_AssetLoad(key->modulusData, key->modulusBits, key->exponentData, key->privateExponentBytes * 8, AssetId,
                        AssetSize);
    ret = AX_CIPHER_CMM_Alloc(CE_DATA_SIZE_VWB(key->modulusBits), ARCH_DMA_MINALIGN, &cmm);
    if (ret < 0) {
        AX_CIPHER_ERR("hash malloc memory failed!\n");
        AX_CIPHER_AssetFree(AssetId);
        return AX_CIPHER_NO_MEMORY;
    }
    memset(&hash_ctl, 0, sizeof(hash_ctl));
    hash_ctl.hashType = hashType;
    AX_Hash_Compute(&hash_ctl, msg, msgBytes, (AX_U8 *)hashResult, HashDataSize);
    memset(&CommandToken, 0, sizeof(CommandToken));
    memset(&ResultToken, 0, sizeof(ResultToken));
    AX_CIPHER_CacheInvalid(cmm.virAddr, CE_DATA_SIZE_VWB(key->modulusBits));
    Eip130Token_Command_Pk_Asset_Command(&CommandToken, method,
                                         CE_DATA_WORD_B2WB(key->modulusBits), CE_DATA_WORD_B2WB(key->privateExponentBytes * 8),
                                         (AX_U8)SaltSize, AssetId, 0,  0, 0, 0,
                                         (AX_ULONG)cmm.pyhyAddr, (AX_U16)key->modulusBits / 8 + 4);
    for (int i = 0; i < (HashDataSize / 4); i++) {
        CommandToken.W[12 + i] = hashResult[i];
    }
    CommandToken.W[3] |= HashDataSize | (1 << 30);
    ret = Eip130PhysicalTokenExchange((AX_U32 *)&CommandToken, (AX_U32 *)&ResultToken, 1);
    if ((ret < 0) || (ResultToken.W[0] & (1 << 31))) {
        AX_CIPHER_AssetFree(AssetId);
        AX_CIPHER_CMM_Free(&cmm);
        return AX_CIPHER_INTERNAL_ERROR;
    }
    AX_CIPHER_BigIntFromHW(cmm.virAddr, key->modulusBits, sig->data);
    AX_CIPHER_CMM_Free(&cmm);
    return AX_CIPHER_AssetFree(AssetId);
}

/*
 * uboot dram size is 11MB, but SAMPLE_CIPHER_AES_BigPacket function
 * allocated mem is greater than 11MB. So wo need to specify a piece
 * of memory in the ddr for SAMPLE_CIPHER_AES_BigPacket
 */
#define ALLOC_DDR_BASE          (0x42000000)
#define DDR_ALLOC_BUFFER_SIZE   (128 << 20)
#define BUFFER_ALIGN            (8)
static unsigned long gCurrentOffset = 0;

void *allocDdrBuffer(AX_ADDR size)
{
    unsigned long currAddr;
    if (ALLOC_DDR_BASE + gCurrentOffset + size > ALLOC_DDR_BASE + DDR_ALLOC_BUFFER_SIZE) {
        printf("%s failed\n", __func__);
        return NULL;
    }

    currAddr = ALLOC_DDR_BASE + gCurrentOffset;

    if (size % BUFFER_ALIGN) {
        size = (size / BUFFER_ALIGN + 1) * BUFFER_ALIGN;
    }
    gCurrentOffset += size;

    return (void *)(currAddr);
}

void *freeDdrBuffer(AX_ADDR size)
{
    if (size % BUFFER_ALIGN) {
        size = (size / BUFFER_ALIGN + 1) * BUFFER_ALIGN;
    }

    if (size > gCurrentOffset) {
        printf("%s error\n", __func__);
        return NULL;
    }
    gCurrentOffset -= size;

    return (void *)(ALLOC_DDR_BASE + gCurrentOffset);
}
