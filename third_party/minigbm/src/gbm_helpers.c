/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>
#include <stdio.h>

#include "drv.h"
#include "gbm.h"

uint64_t gbm_convert_usage(uint32_t usage)
{
	uint64_t use_flags = BO_USE_NONE;

	if (usage & GBM_BO_USE_SCANOUT)
		use_flags |= BO_USE_SCANOUT;
	if (usage & GBM_BO_USE_CURSOR)
		use_flags |= BO_USE_CURSOR;
	if (usage & GBM_BO_USE_CURSOR_64X64)
		use_flags |= BO_USE_CURSOR_64X64;
	if (usage & GBM_BO_USE_RENDERING)
		use_flags |= BO_USE_RENDERING;
	if (usage & GBM_BO_USE_TEXTURING)
		use_flags |= BO_USE_TEXTURE;
	if (usage & GBM_BO_USE_LINEAR)
		use_flags |= BO_USE_LINEAR;
	if (usage & GBM_BO_USE_CAMERA_WRITE)
		use_flags |= BO_USE_CAMERA_WRITE;
	if (usage & GBM_BO_USE_CAMERA_READ)
		use_flags |= BO_USE_CAMERA_READ;
	if (usage & GBM_BO_USE_PROTECTED)
		use_flags |= BO_USE_PROTECTED;
	if (usage & GBM_BO_USE_SW_READ_OFTEN)
		use_flags |= BO_USE_SW_READ_OFTEN;
	if (usage & GBM_BO_USE_SW_READ_RARELY)
		use_flags |= BO_USE_SW_READ_RARELY;
	if (usage & GBM_BO_USE_SW_WRITE_OFTEN)
		use_flags |= BO_USE_SW_WRITE_OFTEN;
	if (usage & GBM_BO_USE_SW_WRITE_RARELY)
		use_flags |= BO_USE_SW_WRITE_RARELY;

	return use_flags;
}
