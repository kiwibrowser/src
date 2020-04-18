#ifndef RADEON_DRM_PUBLIC_H
#define RADEON_DRM_PUBLIC_H

#include "pipe/p_defines.h"

struct radeon_winsys;

struct radeon_winsys *radeon_drm_winsys_create(int fd);

#endif
