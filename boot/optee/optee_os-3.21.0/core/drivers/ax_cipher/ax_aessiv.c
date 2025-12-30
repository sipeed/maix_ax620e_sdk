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
#include <string.h>
#include <stdlib.h>
#include "ax_base_type.h"
#include "ax_cipher_api.h"
#include "ax_base_type.h"
#include "eip130_drv.h"
#include "ax_cipher_api.h"
#include "ax_cipher_token_crypto.h"
#include <mm/core_mmu.h>
#include <mm/core_memprot.h>
#include <util.h>
#include "ax_aessiv.h"
static void aessiv_xor(uint8_t * buffer, const uint8_t * xor_value)
{
	int i;
	for (i = 0; i < (128 / 8); i++) {
		buffer[i] ^= xor_value[i];
	}
}

static void aessiv_bit_shift_left(uint8_t * buffer)
{
	int i;
	for (i = 0; i < ((128 / 8) - 1); i++) {
		buffer[i] = (uint8_t) ((buffer[i] << 1) | ((buffer[i + 1] >> 7) & 1));
	}
	buffer[((128 / 8) - 1)] = (uint8_t) (buffer[((128 / 8) - 1)] << 1);
}

static void aessiv_dbl(uint8_t * buffer)
{
	const uint8_t aessiv_xorBlock[(128 / 8)] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
	};
	bool XorNeeded = ((buffer[0] >> 7) == 1);

	aessiv_bit_shift_left(buffer);
	if (XorNeeded) {
		aessiv_xor(buffer, aessiv_xorBlock);
	}
}

static int aessiv_s2v(aessiv_context * context, const uint8_t * data, const size_t dataSize, uint8_t * mac)
{
	uint8_t zeroBlock[(128 / 8)] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	uint8_t *dataBuf = NULL;
	size_t dataBufSize = 0;
	int status;
	unsigned int i;
	status = ax_cipher_encrypt_mac_blk(context->key, context->key_size / 2, zeroBlock, sizeof(zeroBlock), mac, 16);
	if (status != AX_CIPHER_SUCCESS) {
		ax_cipher_err("ax_cipher_encrypt_mac_blk failed, %x\n", status);
		return -1;
	}
	for (i = 0; i < context->ad_list_count; i++) {
		uint8_t localMac[(128 / 8)];
		int ADsize = 0;
		aessiv_dbl(mac);
		ADsize = (int)(context->ad_list[i + 1] - context->ad_list[i]);
		status = ax_cipher_encrypt_mac_blk(context->key, context->key_size / 2, context->ad_list[i], ADsize, localMac, 16);
		if (status != AX_CIPHER_SUCCESS) {
			ax_cipher_err("MAC FAIL\n");
			return -1;
		}
		aessiv_xor(mac, localMac);
	}
	if (dataSize >= (128 / 8)) {
		dataBufSize = dataSize;
		dataBuf = malloc(dataBufSize);
		if (dataBuf == NULL) {
			return -1;
		}

		memcpy(dataBuf, data, dataSize);
		aessiv_xor(&dataBuf[dataBufSize - 16], mac);
	} else {
		dataBufSize = (128 / 8);
		dataBuf = malloc(dataBufSize);
		if (dataBuf == NULL) {
			return -1;
		}

		aessiv_dbl(mac);
		memcpy(dataBuf, data, dataSize);
		dataBuf[dataSize] = 0x80;	// Pad
		for (i = (unsigned int)(dataSize + 1); i < (128 / 8); i++) {
			dataBuf[i] = 0x00;
		}
		aessiv_xor(dataBuf, mac);
	}
	status = ax_cipher_encrypt_mac_blk(context->key, context->key_size / 2, dataBuf, dataBufSize, mac, 16);
	if (status != AX_CIPHER_SUCCESS) {
		free(dataBuf);
		return -1;
	}
	free(dataBuf);
	return 0;
}

static int aessiv_aesctr(aessiv_context * context, uint8_t * IV, const uint8_t * inData, const size_t inSize, uint8_t * out_data)
{
	int status;
	size_t InSize;
	uint8_t tmpIV[(128 / 8)];
	AX_CIPHER_HANDLE handler;
	ax_cipher_ctrl_s ctrl;
	memcpy(tmpIV, IV, sizeof(tmpIV));
	tmpIV[8] &= 0x7F;
	tmpIV[12] &= 0x7F;
	ctrl.alg = AX_CIPHER_ALGO_CIPHER_AES;
	ctrl.workMode = AX_CIPHER_MODE_CIPHER_CTR;
	ctrl.pKey = (uint8_t *) context->key + (context->key_size / 2);
	ctrl.keySize = context->key_size / 2;
	ctrl.pIV = (uint8_t *) tmpIV;
	status = ax_cipher_create_handle(&handler, &ctrl);
	if (status != AX_CIPHER_SUCCESS) {
		ax_cipher_err("ax_cipher_create_handle\n");
		return -1;
	}

	InSize = (inSize + ((128 / 8) - 1)) & (size_t) ~ ((128 / 8) - 1);
	if (InSize == inSize) {
		status = ax_cipher_encrypt(handler, (uint8_t *) inData, out_data, InSize);
	} else {
		uint8_t *inDataBuf;
		uint8_t *outDataBuf;

		inDataBuf = malloc(InSize);
		if (inDataBuf == NULL) {
			ax_cipher_destroy_handle(handler);
			return -1;
		}
		outDataBuf = malloc(InSize);
		if (outDataBuf == NULL) {
			free(inDataBuf);
			ax_cipher_destroy_handle(handler);
			return -1;
		}

		memcpy(inDataBuf, inData, inSize);
		if ((InSize - inSize) > 0) {
			memset(&inDataBuf[inSize], 0, (InSize - inSize));
		}
		status = ax_cipher_encrypt(handler, (uint8_t *) inData, out_data, InSize);
		if (status == AX_CIPHER_SUCCESS) {
			memcpy(out_data, outDataBuf, inSize);
		}

		free(inDataBuf);
		free(outDataBuf);
	}
	if (status != AX_CIPHER_SUCCESS) {
		ax_cipher_destroy_handle(handler);
		return -1;
	}
	ax_cipher_destroy_handle(handler);
	return 0;
}

int ax_cipher_aessiv_init(aessiv_context * context, const bool verbose)
{
	memset(context, 0, sizeof(aessiv_context));
	context->verbose = verbose;
	return 0;
}

int ax_cipher_aessiv_set_key(aessiv_context * context, const uint8_t * key, const size_t key_size)
{
	if (key_size > sizeof(context->key)) {
		return -1;
	}

	memcpy(context->key, key, key_size);
	context->key_size = (unsigned int)key_size;
	return 0;
}

int ax_cipher_aessiv_set_ad(aessiv_context * context, const uint8_t * AD, const size_t ADSize)
{
	uint8_t *begin = NULL;

	if (context->ad_list_count == 0) {
		if (ADSize > sizeof(context->ad_buffer)) {
			return -1;
		}

		begin = context->ad_buffer;
		context->ad_list[context->ad_list_count] = begin;
	} else {
		begin = context->ad_list[context->ad_list_count];
		if (ADSize > (sizeof(context->ad_buffer) - (unsigned int)(begin - context->ad_buffer))) {
			return -1;
		}
		if (context->ad_list_count == ((sizeof(context->ad_list) / sizeof(context->ad_list[0])) - 1)) {
			return -1;
		}
	}
	memcpy(begin, AD, ADSize);
	context->ad_list_count++;
	context->ad_list[context->ad_list_count] = begin + ADSize;

	return 0;
}

int ax_cipher_aessiv_encrypt(aessiv_context * context, const uint8_t * inData, const size_t inSize, uint8_t * out_data, size_t * outSize)
{
	uint8_t V[(128 / 8)];

	if ((inSize + (128 / 8)) > *outSize) {
		return -1;
	}

	if (aessiv_s2v(context, inData, inSize, V) < 0) {
		return -1;
	}

	if (aessiv_aesctr(context, V, inData, inSize, &out_data[(128 / 8)])) {
		return -1;
	}

	memcpy(out_data, V, (128 / 8));
	*outSize = inSize + (128 / 8);
	return 0;
}

int ax_cipher_aessiv_decrypt(aessiv_context * context, const uint8_t * inData, const size_t inSize, uint8_t * out_data, size_t * outSize)
{
	uint8_t V[(128 / 8)];
	uint8_t *C;
	uint8_t *C1;
	*outSize = inSize - sizeof(V);
	C = malloc(inSize - sizeof(V));
	if (!C) {
		ax_cipher_err("AESSIV_Decrypt malloc C failed\n");
		return -1;
	}
	C1 = malloc(inSize);
	if (!C1) {
		free(C);
		ax_cipher_err("AESSIV_Decrypt malloc C1 failed\n");
		return -1;
	}
	memcpy(V, inData, sizeof(V));
	memcpy(C, inData + sizeof(V), *outSize);
	if (aessiv_aesctr(context, V, C, *outSize, C1) < 0) {
		free(C);
		free(C1);
		ax_cipher_err("inSize: %ld\n", (long int)inSize);
		return -1;
	}
	memcpy(out_data, C1, *outSize);
	free(C);
	free(C1);
	return 0;
}
