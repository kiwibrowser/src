/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef HELPERS_H
#define HELPERS_H

#include "drv.h"
#include "helpers_array.h"

uint32_t drv_size_from_format(uint32_t format, uint32_t stride, uint32_t height, size_t plane);
int drv_bo_from_format(struct bo *bo, uint32_t stride, uint32_t aligned_height, uint32_t format);
int drv_dumb_bo_create(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
		       uint64_t use_flags);
int drv_dumb_bo_destroy(struct bo *bo);
int drv_gem_bo_destroy(struct bo *bo);
int drv_prime_bo_import(struct bo *bo, struct drv_import_fd_data *data);
void *drv_dumb_bo_map(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags);
int drv_bo_munmap(struct bo *bo, struct vma *vma);
int drv_mapping_destroy(struct bo *bo);
int drv_get_prot(uint32_t map_flags);
uintptr_t drv_get_reference_count(struct driver *drv, struct bo *bo, size_t plane);
void drv_increment_reference_count(struct driver *drv, struct bo *bo, size_t plane);
void drv_decrement_reference_count(struct driver *drv, struct bo *bo, size_t plane);
uint32_t drv_log_base2(uint32_t value);
int drv_add_combination(struct driver *drv, uint32_t format, struct format_metadata *metadata,
			uint64_t usage);
void drv_add_combinations(struct driver *drv, const uint32_t *formats, uint32_t num_formats,
			  struct format_metadata *metadata, uint64_t usage);
void drv_modify_combination(struct driver *drv, uint32_t format, struct format_metadata *metadata,
			    uint64_t usage);
struct drv_array *drv_query_kms(struct driver *drv);
int drv_modify_linear_combinations(struct driver *drv);
uint64_t drv_pick_modifier(const uint64_t *modifiers, uint32_t count,
			   const uint64_t *modifier_order, uint32_t order_count);
#endif
