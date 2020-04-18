/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef DRV_MARVELL

#include "drv_priv.h"
#include "helpers.h"
#include "util.h"

static const uint32_t render_target_formats[] = { DRM_FORMAT_ARGB8888, DRM_FORMAT_XRGB8888 };

static int marvell_init(struct driver *drv)
{
	drv_add_combinations(drv, render_target_formats, ARRAY_SIZE(render_target_formats),
			     &LINEAR_METADATA, BO_USE_RENDER_MASK);

	return drv_add_linear_combinations(drv, render_target_formats,
					   ARRAY_SIZE(render_target_formats));
}

const struct backend backend_marvell = {
	.name = "marvell",
	.init = marvell_init,
	.bo_create = drv_dumb_bo_create,
	.bo_destroy = drv_dumb_bo_destroy,
	.bo_import = drv_prime_bo_import,
	.bo_map = drv_dumb_bo_map,
	.bo_unmap = drv_bo_munmap,
};

#endif
