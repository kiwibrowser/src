/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CROS_GRALLOC_BUFFER_H
#define CROS_GRALLOC_BUFFER_H

#include "../drv.h"
#include "cros_gralloc_helpers.h"

class cros_gralloc_buffer
{
      public:
	cros_gralloc_buffer(uint32_t id, struct bo *acquire_bo,
			    struct cros_gralloc_handle *acquire_handle);
	~cros_gralloc_buffer();

	uint32_t get_id() const;

	/* The new reference count is returned by both these functions. */
	int32_t increase_refcount();
	int32_t decrease_refcount();

	int32_t lock(const struct rectangle *rect, uint32_t map_flags,
		     uint8_t *addr[DRV_MAX_PLANES]);
	int32_t unlock();

      private:
	cros_gralloc_buffer(cros_gralloc_buffer const &);
	cros_gralloc_buffer operator=(cros_gralloc_buffer const &);

	uint32_t id_;
	struct bo *bo_;
	struct cros_gralloc_handle *hnd_;

	int32_t refcount_;
	int32_t lockcount_;
	uint32_t num_planes_;

	struct mapping *lock_data_[DRV_MAX_PLANES];
};

#endif
