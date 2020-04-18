/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GBM_PRIV_H
#define GBM_PRIV_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include "drv.h"
#include "gbm.h"

struct gbm_device {
	struct driver *drv;
};

struct gbm_surface {
};

struct gbm_bo {
	struct gbm_device *gbm;
	struct bo *bo;
	uint32_t gbm_format;
	void *user_data;
	void (*destroy_user_data)(struct gbm_bo *, void *);
};

#endif
