/*
 * Copyright (C) 2009 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __NOUVEAU_DRIVER_H__
#define __NOUVEAU_DRIVER_H__

#include "main/imports.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/formats.h"
#include "main/state.h"
#include "utils.h"
#include "dri_util.h"

#undef NDEBUG
#include <assert.h>

#include <libdrm/nouveau.h>
#include "nouveau_screen.h"
#include "nouveau_state.h"
#include "nouveau_surface.h"
#include "nouveau_local.h"

#define DRIVER_AUTHOR	"Nouveau"

struct nouveau_driver {
	struct gl_context *(*context_create)(struct nouveau_screen *screen,
				     const struct gl_config *visual,
				     struct gl_context *share_ctx);
	void (*context_destroy)(struct gl_context *ctx);

	void (*surface_copy)(struct gl_context *ctx,
			     struct nouveau_surface *dst,
			     struct nouveau_surface *src,
			     int dx, int dy, int sx, int sy, int w, int h);
	void (*surface_fill)(struct gl_context *ctx,
			     struct nouveau_surface *dst,
			     unsigned mask, unsigned value,
			     int dx, int dy, int w, int h);

	nouveau_state_func *emit;
	int num_emit;
};

#define nouveau_error(format, ...) \
	fprintf(stderr, "%s: " format, __func__, ## __VA_ARGS__)

void
nouveau_clear(struct gl_context *ctx, GLbitfield buffers);

void
nouveau_span_functions_init(struct gl_context *ctx);

void
nouveau_driver_functions_init(struct dd_function_table *functions);

void
nouveau_texture_functions_init(struct dd_function_table *functions);

#endif
