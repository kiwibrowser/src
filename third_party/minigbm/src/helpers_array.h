/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

struct drv_array;

struct drv_array *drv_array_init(uint32_t item_size);

/* The data will be copied and appended to the array. */
void *drv_array_append(struct drv_array *array, void *data);

/* The data at the specified index will be freed -- the array will shrink. */
void drv_array_remove(struct drv_array *array, uint32_t idx);

void *drv_array_at_idx(struct drv_array *array, uint32_t idx);

uint32_t drv_array_size(struct drv_array *array);

/* The array and all associated data will be freed. */
void drv_array_destroy(struct drv_array *array);
