/**
  * Copyright (C) 2011 by Tobias Thiel
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  * THE SOFTWARE.
  */

#include "ax_opal_queue.h"
#include "queue_internal.h"

AX_OPAL_QUEUE_T *opal_queue_create(void) {
	AX_OPAL_QUEUE_T *q = (AX_OPAL_QUEUE_T *)malloc(sizeof(AX_OPAL_QUEUE_T));
	if(q != NULL) {
		q->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
		if(q->mutex == NULL) {
			free(q);
			return NULL;
		}
		pthread_mutex_init(q->mutex, NULL);

		q->cond_get = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
		if(q->cond_get == NULL) {
			pthread_mutex_destroy(q->mutex);
			free(q->mutex);
			free(q);
			return NULL;
		}
		pthread_cond_init(q->cond_get, NULL);

		q->cond_put = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
		if(q->cond_put == NULL) {
			pthread_cond_destroy(q->cond_get);
			free(q->cond_get);
			pthread_mutex_destroy(q->mutex);
			free(q->mutex);
			free(q);
			return NULL;
		}
		pthread_cond_init(q->cond_put, NULL);

		q->first_el = NULL;
		q->last_el = NULL;
		q->num_els = 0;
		q->max_els = 0;
		q->new_data = 1;
		q->sort = 0;
		q->asc_order = 1;
		q->cmp_el = NULL;
	}

	return q;
}

AX_OPAL_QUEUE_T *opal_queue_create_limited(uintX_t max_elements) {
	AX_OPAL_QUEUE_T *q = opal_queue_create();
	if(q != NULL)
		q->max_els = max_elements;

	return q;
}

AX_OPAL_QUEUE_T *opal_queue_create_sorted(int8_t asc, int (*cmp)(void *, void *)) {
	if(cmp == NULL)
		return NULL;

	AX_OPAL_QUEUE_T *q = opal_queue_create();
	if(q != NULL) {
		q->sort = 1;
		q->asc_order = asc;
		q->cmp_el = cmp;
	}

	return q;
}

AX_OPAL_QUEUE_T *opal_queue_create_limited_sorted(uintX_t max_elements, int8_t asc, int (*cmp)(void *, void *)) {
	if(cmp == NULL)
		return NULL;

	AX_OPAL_QUEUE_T *q = opal_queue_create();
	if(q != NULL) {
		q->max_els = max_elements;
		q->sort = 1;
		q->asc_order = asc;
		q->cmp_el = cmp;
	}

	return q;
}

int8_t opal_queue_destroy(AX_OPAL_QUEUE_T *q) {
	if(q == NULL)
		return Q_ERR_INVALID;
	return opal_queue_destroy_internal(q, 0, NULL);
}

int8_t opal_queue_destroy_complete(AX_OPAL_QUEUE_T *q, void (*ff)(void *)) {
	if(q == NULL)
		return Q_ERR_INVALID;
	return opal_queue_destroy_internal(q, 1, ff);
}

int8_t opal_queue_flush(AX_OPAL_QUEUE_T *q) {
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;

	int8_t r = opal_queue_flush_internal(q, 0, NULL);

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return r;
}

int8_t opal_queue_flush_complete(AX_OPAL_QUEUE_T *q, void (*ff)(void *)) {
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;

	int8_t r = opal_queue_flush_internal(q, 1, ff);

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return r;
}

uintX_t opal_queue_elements(AX_OPAL_QUEUE_T *q) {
	uintX_t ret = UINTX_MAX;
	if(q == NULL)
		return ret;
	if (0 != opal_queue_lock_internal(q))
		return ret;

	ret = q->num_els;

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return ret;
}

int8_t opal_queue_empty(AX_OPAL_QUEUE_T *q) {
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;

	uint8_t ret;
	if(q->first_el == NULL || q->last_el == NULL)
		ret = 1;
	else
		ret = 0;

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return ret;
}

int8_t opal_queue_set_new_data(AX_OPAL_QUEUE_T *q, uint8_t v) {
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;
	q->new_data = v;
	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;

	if(q->new_data == 0) {
		// notify waiting threads, when new data isn't accepted
		pthread_cond_broadcast(q->cond_get);
		pthread_cond_broadcast(q->cond_put);
	}

	return Q_OK;
}

uint8_t opal_queue_get_new_data(AX_OPAL_QUEUE_T *q) {
	if(q == NULL)
		return 0;
	if (0 != opal_queue_lock_internal(q))
		return 0;

	uint8_t r = q->new_data;

	if (0 != opal_queue_unlock_internal(q))
		return 0;
	return r;
}

int8_t opal_queue_put(AX_OPAL_QUEUE_T *q, void *el) {
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;

	int8_t r = opal_queue_put_internal(q, el, NULL);

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return r;
}

int8_t opal_queue_put_wait(AX_OPAL_QUEUE_T *q, void *el) {
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;

	int8_t r = opal_queue_put_internal(q, el, pthread_cond_wait);

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return r;
}

int8_t opal_queue_get(AX_OPAL_QUEUE_T *q, void **e) {
	*e = NULL;
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;

	int8_t r = opal_queue_get_internal(q, e, NULL, NULL, NULL);

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return r;
}

int8_t opal_queue_get_wait(AX_OPAL_QUEUE_T *q, void **e) {
	*e = NULL;
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;

	int8_t r = opal_queue_get_internal(q, e, pthread_cond_wait, NULL, NULL);

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return r;
}

int8_t opal_queue_get_filtered(AX_OPAL_QUEUE_T *q, void **e, int (*cmp)(void *, void *), void *cmpel) {
	*e = NULL;
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;

	int8_t r = opal_queue_get_internal(q, e, NULL, cmp, cmpel);

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return r;
}

int8_t opal_queue_flush_put(AX_OPAL_QUEUE_T *q, void (*ff)(void *), void *e) {
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;

	int8_t r = opal_queue_flush_internal(q, 0, NULL);
	r = opal_queue_put_internal(q, e, NULL);

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return r;
}

int8_t opal_queue_flush_complete_put(AX_OPAL_QUEUE_T *q, void (*ff)(void *), void *e) {
	if(q == NULL)
		return Q_ERR_INVALID;
	if (0 != opal_queue_lock_internal(q))
		return Q_ERR_LOCK;

	int8_t r = opal_queue_flush_internal(q, 1, ff);
	r = opal_queue_put_internal(q, e, NULL);

	if (0 != opal_queue_unlock_internal(q))
		return Q_ERR_LOCK;
	return r;
}
