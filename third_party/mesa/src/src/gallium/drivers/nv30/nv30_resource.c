/*
 * Copyright 2012 Red Hat Inc.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Ben Skeggs
 *
 */

#include "util/u_format.h"
#include "util/u_inlines.h"

#include "nv30_screen.h"
#include "nv30_context.h"
#include "nv30_resource.h"
#include "nv30_transfer.h"

static struct pipe_resource *
nv30_resource_create(struct pipe_screen *pscreen,
                     const struct pipe_resource *tmpl)
{
   switch (tmpl->target) {
   case PIPE_BUFFER:
      return nouveau_buffer_create(pscreen, tmpl);
   default:
      return nv30_miptree_create(pscreen, tmpl);
   }
}

static struct pipe_resource *
nv30_resource_from_handle(struct pipe_screen *pscreen,
                          const struct pipe_resource *tmpl,
                          struct winsys_handle *handle)
{
   if (tmpl->target == PIPE_BUFFER)
      return NULL;
   else
      return nv30_miptree_from_handle(pscreen, tmpl, handle);
}

void
nv30_resource_screen_init(struct pipe_screen *pscreen)
{
   pscreen->resource_create = nv30_resource_create;
   pscreen->resource_from_handle = nv30_resource_from_handle;
   pscreen->resource_get_handle = u_resource_get_handle_vtbl;
   pscreen->resource_destroy = u_resource_destroy_vtbl;
}

void
nv30_resource_init(struct pipe_context *pipe)
{
   pipe->get_transfer = u_get_transfer_vtbl;
   pipe->transfer_map = u_transfer_map_vtbl;
   pipe->transfer_flush_region = u_transfer_flush_region_vtbl;
   pipe->transfer_unmap = u_transfer_unmap_vtbl;
   pipe->transfer_destroy = u_transfer_destroy_vtbl;
   pipe->transfer_inline_write = u_transfer_inline_write_vtbl;
   pipe->create_surface = nv30_miptree_surface_new;
   pipe->surface_destroy = nv30_miptree_surface_del;
   pipe->resource_copy_region = nv30_resource_copy_region;
   pipe->resource_resolve = nv30_resource_resolve;
}
