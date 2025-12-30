/**********************************************************************************
 *
 * Copyright (c) 2019-2023 AXERA Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of AXERA Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of AXERA Technology Co., Ltd.
 *
 **********************************************************************************/

OUTPUT_FORMAT("elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)
MEMORY
{
	iRom (rx) : ORIGIN = 0x03000400, LENGTH = 0x100000  /* 0x100000 1M */
}

SECTIONS
{
	. = ALIGN(4);
	.text :
	{
		*start.o
		*(.text)
	} > iRom

	. = ALIGN(4);
	.rodata :
	{
		*(.rodata)
		*(.rodata.str1.4)
		*(.rodata.str1.8)
		. = ALIGN(4);
		_ro_data_end = . ;
	} > iRom

	. = ALIGN(4);
	.data :
	{
		_data_load_addr = LOADADDR(.data);
		_data_start = . ;
		*(.data)
		. = ALIGN(4);
		_data_end = . ;
		_size_data = _data_end - _data_start;
	} > iRom

	. = ALIGN(4);
	_bss_start = . ;
	.bss :
	{
		*(.bss)
		. = ALIGN(4);
	} > iRom
	_bss_end = . ;
	_size_bss = _bss_end - _bss_start;
	. = ALIGN(4);
	.stack :
	{
		_stackbottom = . ;
	} > iRom
}
