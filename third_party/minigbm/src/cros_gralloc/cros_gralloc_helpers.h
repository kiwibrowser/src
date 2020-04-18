/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CROS_GRALLOC_HELPERS_H
#define CROS_GRALLOC_HELPERS_H

#include "../drv.h"
#include "cros_gralloc_handle.h"
#include "cros_gralloc_types.h"

#include <system/graphics.h>
#include <system/window.h>

constexpr uint32_t cros_gralloc_magic = 0xABCDDCBA;
constexpr uint32_t handle_data_size =
    ((sizeof(struct cros_gralloc_handle) - offsetof(cros_gralloc_handle, fds[0])) / sizeof(int));

uint32_t cros_gralloc_convert_format(int32_t format);

cros_gralloc_handle_t cros_gralloc_convert_handle(buffer_handle_t handle);

int32_t cros_gralloc_sync_wait(int32_t acquire_fence);

__attribute__((format(printf, 4, 5))) void cros_gralloc_log(const char *prefix, const char *file,
							    int line, const char *format, ...);

#define cros_gralloc_error(...)                                                                    \
	do {                                                                                       \
		cros_gralloc_log("CROS_GRALLOC_ERROR", __FILE__, __LINE__, __VA_ARGS__);           \
	} while (0)

#endif
