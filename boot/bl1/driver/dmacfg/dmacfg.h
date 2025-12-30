#ifndef __DMACFG_H__
#define __DMACFG_H__
#include "cmn.h"
#include "chip_reg.h"

typedef struct {
	unsigned long long addr_base:56;
	unsigned long long cmd:8;	//1
} ax_dmacfg_start_info_t;

typedef struct {
	unsigned long long llp:54;
	unsigned long long lli_ioc:1;
	unsigned long long lli_last:1;
	unsigned long long cmd:8;	//0xf
} ax_dmacfg_end_info_t;

typedef struct {
	unsigned long long reserve[16];
} ax_dmacfg_reserve_t;

typedef struct {
	unsigned long long data:32;
	unsigned long long offset:20;
	unsigned long long :4;
	unsigned long long cmd:8;	//2
} ax_dmacfg_write_once_info_t;

typedef struct {
	unsigned long long offset:20;
	unsigned long long tr_width:2;
	unsigned long long inc_len:16;
	unsigned long long dinc:1;
	unsigned long long :17;
	unsigned long long cmd:8;	//3
} ax_dmacfg_writethrough_info_t;

typedef struct {
	unsigned long long data0:32;
	unsigned long long data1:32;
} ax_dmacfg_writethrough_data_t;

typedef struct {
	unsigned long long offset:20;
	unsigned long long tr_width:2;
	unsigned long long inc_len:16;
	unsigned long long sinc:1;
	unsigned long long dinc:1;
	unsigned long long :16;
	unsigned long long cmd:8;	//4
	unsigned long long r_addr:56;
	unsigned long long jump_cmd:8;	//5
} ax_dmacfg_write_jump_info_t;

typedef struct {
	ax_dmacfg_start_info_t start_info;
	ax_dmacfg_write_jump_info_t write_jump_info;
	ax_dmacfg_end_info_t end_info;
	ax_dmacfg_reserve_t reserve;
} ax_dmacfg_write_jump_buf_t;

typedef struct {
	unsigned long long w_addr:37;
	unsigned long long tr_width:2;
	unsigned long long inc_len:16;
	unsigned long long sinc:1;
	unsigned long long r_cmd:8;	//6
	unsigned long long r_addr:56;
	unsigned long long jump_cmd:8;	//5
} ax_dmacfg_read_and_write_info_t;

typedef struct {
	ax_dmacfg_start_info_t start_info;
	ax_dmacfg_read_and_write_info_t read_and_write_info;
	ax_dmacfg_end_info_t end_info;
	ax_dmacfg_reserve_t reserve;
} ax_dmacfg_read_write_buf_t;

#endif
