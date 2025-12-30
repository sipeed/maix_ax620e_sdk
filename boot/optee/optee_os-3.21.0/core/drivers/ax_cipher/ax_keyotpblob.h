/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_KEYOTPBLOB_H__
#define __AX_KEYOTPBLOB_H__

typedef struct {
	void *key;
	void *label;
	uint64_t policy;
	uint32_t key_len;
	uint32_t label_size;
} KEY_OTP_BLOB_CONTEXT;

int32_t ax_cipher_keyblob_wrap(KEY_OTP_BLOB_CONTEXT * keyBlobContext, void * assetData, int32_t assetDataLen, void * keyBlob, int32_t * keyBlobSize);
int32_t ax_cipher_keyblob_unwrap(KEY_OTP_BLOB_CONTEXT * keyBlobContext, void * keyBlob, int32_t keyBlobSize, void * assetData, int32_t * assetDataLen);

#endif
