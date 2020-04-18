/**********************************************************
 * Copyright 2008-2012 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_debug.h"

#include "svga_resource.h"
#include "svga_resource_buffer.h"
#include "svga_resource_texture.h"
#include "svga_context.h"
#include "svga_screen.h"


static struct pipe_resource *
svga_resource_create(struct pipe_screen *screen,
                     const struct pipe_resource *template)
{
   if (template->target == PIPE_BUFFER)
      return svga_buffer_create(screen, template);
   else
      return svga_texture_create(screen, template);
}


static struct pipe_resource *
svga_resource_from_handle(struct pipe_screen * screen,
                          const struct pipe_resource *template,
                          struct winsys_handle *whandle)
{
   if (template->target == PIPE_BUFFER)
      return NULL;
   else
      return svga_texture_from_handle(screen, template, whandle);
}


void
svga_init_resource_functions(struct svga_context *svga)
{
   svga->pipe.get_transfer = u_get_transfer_vtbl;
   svga->pipe.transfer_map = u_transfer_map_vtbl;
   svga->pipe.transfer_flush_region = u_transfer_flush_region_vtbl;
   svga->pipe.transfer_unmap = u_transfer_unmap_vtbl;
   svga->pipe.transfer_destroy = u_transfer_destroy_vtbl;
   svga->pipe.transfer_inline_write = u_transfer_inline_write_vtbl;
}

void
svga_init_screen_resource_functions(struct svga_screen *is)
{
   is->screen.resource_create = svga_resource_create;
   is->screen.resource_from_handle = svga_resource_from_handle;
   is->screen.resource_get_handle = u_resource_get_handle_vtbl;
   is->screen.resource_destroy = u_resource_destroy_vtbl;
}
