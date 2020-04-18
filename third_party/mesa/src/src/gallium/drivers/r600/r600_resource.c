/*
 * Copyright 2010 Marek Olšák <maraeo@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "r600_pipe.h"

static struct pipe_resource *r600_resource_create(struct pipe_screen *screen,
						const struct pipe_resource *templ)
{
	if (templ->target == PIPE_BUFFER) {
		if (templ->bind & PIPE_BIND_GLOBAL) {
		    return r600_compute_global_buffer_create(screen, templ);
		}
		else {
		    return r600_buffer_create(screen, templ, 4096);
		}
	} else {
		return r600_texture_create(screen, templ);
	}
}

static struct pipe_resource *r600_resource_from_handle(struct pipe_screen * screen,
							const struct pipe_resource *templ,
							struct winsys_handle *whandle)
{
	if (templ->target == PIPE_BUFFER) {
		return NULL;
	} else {
		return r600_texture_from_handle(screen, templ, whandle);
	}
}

void r600_resource_destroy(struct pipe_screen *screen, struct pipe_resource *res)
{
	if (res->target == PIPE_BUFFER && (res->bind & PIPE_BIND_GLOBAL)) {
		r600_compute_global_buffer_destroy(screen, res);
	} else {
		u_resource_destroy_vtbl(screen, res);
	}
}

void r600_init_screen_resource_functions(struct pipe_screen *screen)
{
	screen->resource_create = r600_resource_create;
	screen->resource_from_handle = r600_resource_from_handle;
	screen->resource_get_handle = u_resource_get_handle_vtbl;
	screen->resource_destroy = r600_resource_destroy;
}

void r600_init_context_resource_functions(struct r600_context *r600)
{
	r600->context.get_transfer = u_get_transfer_vtbl;
	r600->context.transfer_map = u_transfer_map_vtbl;
	r600->context.transfer_flush_region = u_default_transfer_flush_region;
	r600->context.transfer_unmap = u_transfer_unmap_vtbl;
	r600->context.transfer_destroy = u_transfer_destroy_vtbl;
	r600->context.transfer_inline_write = u_default_transfer_inline_write;
}
