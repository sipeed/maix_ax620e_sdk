/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_MAP_H
#define _AX_MAP_H

#include "rbtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef void* ax_map_handle;
typedef struct rb_node ax_map_node_t;
typedef enum {
	ax_map_type_ss = 0,
	ax_map_type_si = 1,
	ax_map_type_ii = 2,
} ax_map_type_e;

typedef struct _ax_ss_node_t {
    ax_map_node_t node;
    char *key;
    char *val;
}ax_map_ssnd_t;

typedef struct _ax_si_node_t {
    ax_map_node_t node;
    char *key;
    int   val;
}ax_map_sind_t;

typedef struct _ax_ii_node_t {
    ax_map_node_t node;
    int key;
    int val;
}ax_map_iind_t;


ax_map_handle  ax_map_create(ax_map_type_e type);
void           ax_map_destory(ax_map_handle root);

ax_map_ssnd_t* ax_map_ss_get(ax_map_handle root, const char *key);
int            ax_map_ss_put(ax_map_handle root, const char* key, const char* val);

ax_map_sind_t* ax_map_si_get(ax_map_handle root, const char *key);
int            ax_map_si_put(ax_map_handle root, const char* key, int val);

ax_map_iind_t* ax_map_ii_get(ax_map_handle root, int key);
int            ax_map_ii_put(ax_map_handle root, int key, int val);

void           ax_map_erase(ax_map_handle root, ax_map_node_t* node);

void*          ax_map_first(ax_map_handle root);
void*          ax_map_last(ax_map_handle root);

void*          ax_map_next(ax_map_handle root, ax_map_node_t *node);
void*          ax_map_prev(ax_map_handle root, ax_map_node_t *node);

#endif  //_AX_MAP_H