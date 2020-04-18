/*
 * Copyright © 2012, 2013 Thierry Reding
 * Copyright © 2013 Erik Faye-Lund
 * Copyright © 2014 NVIDIA Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __DRM_TEGRA_H__
#define __DRM_TEGRA_H__ 1

#include <stdint.h>
#include <stdlib.h>

struct drm_tegra_bo;
struct drm_tegra;

int drm_tegra_new(struct drm_tegra **drmp, int fd);
void drm_tegra_close(struct drm_tegra *drm);

int drm_tegra_bo_new(struct drm_tegra_bo **bop, struct drm_tegra *drm,
		     uint32_t flags, uint32_t size);
int drm_tegra_bo_wrap(struct drm_tegra_bo **bop, struct drm_tegra *drm,
		      uint32_t handle, uint32_t flags, uint32_t size);
struct drm_tegra_bo *drm_tegra_bo_ref(struct drm_tegra_bo *bo);
void drm_tegra_bo_unref(struct drm_tegra_bo *bo);
int drm_tegra_bo_get_handle(struct drm_tegra_bo *bo, uint32_t *handle);
int drm_tegra_bo_map(struct drm_tegra_bo *bo, void **ptr);
int drm_tegra_bo_unmap(struct drm_tegra_bo *bo);

int drm_tegra_bo_get_flags(struct drm_tegra_bo *bo, uint32_t *flags);
int drm_tegra_bo_set_flags(struct drm_tegra_bo *bo, uint32_t flags);

struct drm_tegra_bo_tiling {
	uint32_t mode;
	uint32_t value;
};

int drm_tegra_bo_get_tiling(struct drm_tegra_bo *bo,
			    struct drm_tegra_bo_tiling *tiling);
int drm_tegra_bo_set_tiling(struct drm_tegra_bo *bo,
			    const struct drm_tegra_bo_tiling *tiling);

#endif /* __DRM_TEGRA_H__ */
