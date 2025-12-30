/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _MAP_H
#define _MAP_H

#include "ax_map.h"

typedef struct rb_root map_root_t;
typedef struct rb_node map_node_t;

typedef struct _ax_map_root {
    map_root_t    root;
    ax_map_type_e type;
}ax_map_root_t;


ax_map_handle ax_map_create(ax_map_type_e type) {
	ax_map_root_t * root = (ax_map_root_t *)malloc(sizeof(ax_map_root_t));
	if (root) {
		root->root = RB_ROOT;
		root->type = type;
	}
	return (ax_map_handle)root;
}

void ax_map_destory(ax_map_handle root) {
    if (!root) {
        return;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;
	ax_map_node_t* free_node = NULL;
    ax_map_node_t* tmp_node = NULL;

    for(free_node = rb_first(&proot->root); free_node; ) {
        tmp_node = rb_next(free_node);
        ax_map_erase(root, free_node);
        free_node = tmp_node;
    }

}

ax_map_ssnd_t* ax_map_ss_get(ax_map_handle root, const char *key) {
    if (!root || !key) {
        return NULL;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;

    ax_map_node_t *node = proot->root.rb_node;
    while (node) {
        ax_map_ssnd_t *data = container_of(node, ax_map_ssnd_t, node);

        //compare between the key with the keys in map
        int cmp = strcmp(key, data->key);
        if (cmp < 0) {
            node = node->rb_left;
        }else if (cmp > 0) {
            node = node->rb_right;
        }else {
            return data;
        }
    }
    return NULL;
}

int ax_map_ss_put(ax_map_handle root, const char* key, const char* val) {
    if (!root || !key || !val) {
        return -1;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;

    ax_map_ssnd_t *data = (ax_map_ssnd_t*)malloc(sizeof(ax_map_ssnd_t));
    if (!data) {
        return -1;
    }

    data->key = (char*)malloc((strlen(key)+1)*sizeof(char));
    if (!data->key) {
        free(data);
        return -1;
    }
    strcpy(data->key, key);

    data->val = (char*)malloc((strlen(val)+1)*sizeof(char));
    if (!data->val) {
        free(data->key);
        free(data);
        return -1;
    }
    strcpy(data->val, val);

    map_node_t **new_node = &(proot->root.rb_node), *parent = NULL;
    while (*new_node) {
        ax_map_ssnd_t *this_node = container_of(*new_node, ax_map_ssnd_t, node);
        int result = strcmp(key, this_node->key);
        parent = *new_node;

        if (result < 0) {
            new_node = &((*new_node)->rb_left);
        }else if (result > 0) {
            new_node = &((*new_node)->rb_right);
        }else {
            // key is existed and set new value
            free(this_node->val);
            this_node->val = data->val;
            free(data->key);
            free(data);
            return 0;
        }
    }

    rb_link_node(&data->node, parent, new_node);
    rb_insert_color(&data->node, root);

    return 0;
}

ax_map_sind_t* ax_map_si_get(ax_map_handle root, const char *key) {
    if (!root || !key) {
        return NULL;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;

    ax_map_node_t *node = proot->root.rb_node;
    while (node) {
        ax_map_sind_t *data = container_of(node, ax_map_sind_t, node);

        //compare between the key with the keys in map
        int cmp = strcmp(key, data->key);
        if (cmp < 0) {
            node = node->rb_left;
        }else if (cmp > 0) {
            node = node->rb_right;
        }else {
            return data;
        }
    }
    return NULL;
}

int ax_map_si_put(ax_map_handle root, const char* key, int val) {
    if (!root || !key) {
        return -1;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;

    ax_map_sind_t *data = (ax_map_sind_t*)malloc(sizeof(ax_map_sind_t));
    if (!data) {
        return -1;
    }

    data->key = (char*)malloc((strlen(key)+1)*sizeof(char));
    if (!data->key) {
        free(data);
        return -1;
    }
    strcpy(data->key, key);

    data->val = val;

    map_node_t **new_node = &(proot->root.rb_node), *parent = NULL;
    while (*new_node) {
        ax_map_sind_t *this_node = container_of(*new_node, ax_map_sind_t, node);
        int result = strcmp(key, this_node->key);
        parent = *new_node;

        if (result < 0) {
            new_node = &((*new_node)->rb_left);
        }else if (result > 0) {
            new_node = &((*new_node)->rb_right);
        }else {
            // key is existed and set new value
            this_node->val = data->val;
            free(data->key);
            free(data);
            return 0;
        }
    }

    rb_link_node(&data->node, parent, new_node);
    rb_insert_color(&data->node, root);
    return 0;
}

ax_map_iind_t* ax_map_ii_get(ax_map_handle root, int key) {
    if (!root) {
        return NULL;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;

    ax_map_node_t *node = proot->root.rb_node;
    while (node) {
        ax_map_iind_t *data = container_of(node, ax_map_iind_t, node);

        //compare between the key with the keys in map
        int cmp = key - data->key;
        if (cmp < 0) {
            node = node->rb_left;
        }else if (cmp > 0) {
            node = node->rb_right;
        }else {
            return data;
        }
    }
    return NULL;
}

int ax_map_ii_put(ax_map_handle root, int key, int val) {
    if (!root || !key) {
        return -1;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;

    ax_map_iind_t *data = (ax_map_iind_t*)malloc(sizeof(ax_map_iind_t));
    if (!data) {
        return -1;
    }

    data->key = key;
    data->val = val;

    map_node_t **new_node = &(proot->root.rb_node), *parent = NULL;
    while (*new_node) {
        ax_map_iind_t *this_node = container_of(*new_node, ax_map_iind_t, node);
        int result = key - this_node->key;
        parent = *new_node;

        if (result < 0) {
            new_node = &((*new_node)->rb_left);
        }else if (result > 0) {
            new_node = &((*new_node)->rb_right);
        }else {
            // key is existed and set new value
            this_node->val = data->val;
            free(data);
            return 0;
        }
    }

    rb_link_node(&data->node, parent, new_node);
    rb_insert_color(&data->node, root);
    return 0;
}

void ax_map_erase(ax_map_handle root, ax_map_node_t* node) {
    if (!root || !node) {
        return;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;
    rb_erase(node, &proot->root);

    if (proot->type == ax_map_type_ss) {
        ax_map_ssnd_t *node2free = rb_entry(node, ax_map_ssnd_t, node);
        if (node2free) {
            if(node2free->key) free(node2free->key);
            node2free->key = NULL;
            if(node2free->val) free(node2free->val);
            node2free->val = NULL;

            free(node2free);
        }

    } else if (proot->type == ax_map_type_si) {
        ax_map_sind_t *node2free = rb_entry(node, ax_map_sind_t, node);
        if (node2free) {
            if(node2free->key) free(node2free->key);
            node2free->key = NULL;

            free(node2free);
        }
    } else if (proot->type == ax_map_type_ii) {
        ax_map_iind_t *node2free = rb_entry(node, ax_map_iind_t, node);
        if (node2free) {
            free(node2free);
        }
    }
}

void* ax_map_first(ax_map_handle root) {
    if (!root) {
        return NULL;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;

    map_node_t *node = rb_first(&proot->root);
    if (proot->type == ax_map_type_ss) {
        return (void*)rb_entry(node, ax_map_ssnd_t, node);
    } else if(proot->type == ax_map_type_si) {
        return (void*)rb_entry(node, ax_map_sind_t, node);
    } else if(proot->type == ax_map_type_ii) {
        return (void*)rb_entry(node, ax_map_iind_t, node);
    } else {
        return NULL;
    }
}

void* ax_map_last(ax_map_handle root) {
    if (!root) {
        return NULL;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;

    map_node_t *node = rb_last(&proot->root);
    if (proot->type == ax_map_type_ss) {
        return (void*) rb_entry(node, ax_map_ssnd_t, node);
    } else if(proot->type == ax_map_type_si) {
        return (void*)rb_entry(node, ax_map_sind_t, node);
    } else if(proot->type == ax_map_type_ii) {
        return (void*)rb_entry(node, ax_map_iind_t, node);
    } else {
        return NULL;
    }
}

void* ax_map_next(ax_map_handle root, ax_map_node_t* node) {
    if (!root || !node) {
        return NULL;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;

    ax_map_node_t *next = rb_next(node);
    if (proot->type == ax_map_type_ss) {
        return (void*) rb_entry(next, ax_map_ssnd_t, node);
    } else if(proot->type == ax_map_type_si) {
        return (void*)rb_entry(next, ax_map_sind_t, node);
    } else if(proot->type == ax_map_type_ii) {
        return (void*)rb_entry(next, ax_map_iind_t, node);
    } else {
        return NULL;
    }

}

void* ax_map_prev(ax_map_handle root, ax_map_node_t* node) {
    if (!root || !node) {
        return NULL;
    }
	ax_map_root_t *proot = (ax_map_root_t*)root;

    ax_map_node_t *prev = rb_prev(node);
    if (proot->type == ax_map_type_ss) {
        return (void*) rb_entry(prev, ax_map_ssnd_t, node);
    } else if(proot->type == ax_map_type_si) {
        return (void*)rb_entry(prev, ax_map_sind_t, node);
    } else if(proot->type == ax_map_type_ii) {
        return (void*)rb_entry(prev, ax_map_iind_t, node);
    } else {
        return NULL;
    }
}

#endif  //_MAP_H