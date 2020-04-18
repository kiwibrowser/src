/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

struct drv_array {
	void **items;
	uint32_t size;
	uint32_t item_size;
	uint32_t allocations;
};

struct drv_array *drv_array_init(uint32_t item_size)
{
	struct drv_array *array;

	array = calloc(1, sizeof(*array));

	/* Start with a power of 2 number of allocations. */
	array->allocations = 2;
	array->items = calloc(array->allocations, sizeof(*array->items));
	array->item_size = item_size;
	return array;
}

void *drv_array_append(struct drv_array *array, void *data)
{
	void *item;

	if (array->size >= array->allocations) {
		void **new_items = NULL;
		array->allocations *= 2;
		new_items = realloc(array->items, array->allocations * sizeof(*array->items));
		assert(new_items);
		array->items = new_items;
	}

	item = calloc(1, array->item_size);
	memcpy(item, data, array->item_size);
	array->items[array->size] = item;
	array->size++;
	return item;
}

void drv_array_remove(struct drv_array *array, uint32_t idx)
{
	uint32_t i;

	assert(array);
	assert(idx < array->size);

	free(array->items[idx]);
	array->items[idx] = NULL;

	for (i = idx + 1; i < array->size; i++)
		array->items[i - 1] = array->items[i];

	array->size--;
	if ((DIV_ROUND_UP(array->allocations, 2) > array->size) && array->allocations > 2) {
		void **new_items = NULL;
		array->allocations = DIV_ROUND_UP(array->allocations, 2);
		new_items = realloc(array->items, array->allocations * sizeof(*array->items));
		assert(new_items);
		array->items = new_items;
	}
}

void *drv_array_at_idx(struct drv_array *array, uint32_t idx)
{
	assert(idx < array->size);
	return array->items[idx];
}

uint32_t drv_array_size(struct drv_array *array)
{
	return array->size;
}

void drv_array_destroy(struct drv_array *array)
{
	uint32_t i;

	for (i = 0; i < array->size; i++)
		free(array->items[i]);

	free(array->items);
	free(array);
}
