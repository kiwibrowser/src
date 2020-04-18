
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "nouveau/nouveau_screen.h"

#include "nv50_resource.h"

static struct pipe_resource *
nv50_resource_create(struct pipe_screen *screen,
                     const struct pipe_resource *templ)
{
   switch (templ->target) {
   case PIPE_BUFFER:
      return nouveau_buffer_create(screen, templ);
   default:
      return nv50_miptree_create(screen, templ);
   }
}

static struct pipe_resource *
nv50_resource_from_handle(struct pipe_screen * screen,
                          const struct pipe_resource *templ,
                          struct winsys_handle *whandle)
{
   if (templ->target == PIPE_BUFFER)
      return NULL;
   else
      return nv50_miptree_from_handle(screen, templ, whandle);
}

struct pipe_surface *
nv50_surface_from_buffer(struct pipe_context *pipe,
			 struct pipe_resource *pbuf,
			 const struct pipe_surface *templ)
{
   struct nv50_surface *sf = CALLOC_STRUCT(nv50_surface);
   if (!sf)
      return NULL;

   pipe_reference_init(&sf->base.reference, 1);
   pipe_resource_reference(&sf->base.texture, pbuf);

   sf->base.format = templ->format;
   sf->base.usage = templ->usage;
   sf->base.u.buf.first_element = templ->u.buf.first_element;
   sf->base.u.buf.last_element = templ->u.buf.last_element;

   sf->offset =
      templ->u.buf.first_element * util_format_get_blocksize(sf->base.format);

   sf->offset &= ~0x7f; /* FIXME: RT_ADDRESS requires 128 byte alignment */

   sf->width = templ->u.buf.last_element - templ->u.buf.first_element + 1;
   sf->height = 1;
   sf->depth = 1;

   sf->base.width = sf->width;
   sf->base.height = sf->height;

   sf->base.context = pipe;
   return &sf->base;
}

static struct pipe_surface *
nv50_surface_create(struct pipe_context *pipe,
		    struct pipe_resource *pres,
		    const struct pipe_surface *templ)
{
   if (unlikely(pres->target == PIPE_BUFFER))
      return nv50_surface_from_buffer(pipe, pres, templ);
   return nv50_miptree_surface_new(pipe, pres, templ);
}

void
nv50_surface_destroy(struct pipe_context *pipe, struct pipe_surface *ps)
{
   struct nv50_surface *s = nv50_surface(ps);

   pipe_resource_reference(&ps->texture, NULL);

   FREE(s);
}

void
nv50_init_resource_functions(struct pipe_context *pcontext)
{
   pcontext->get_transfer = u_get_transfer_vtbl;
   pcontext->transfer_map = u_transfer_map_vtbl;
   pcontext->transfer_flush_region = u_transfer_flush_region_vtbl;
   pcontext->transfer_unmap = u_transfer_unmap_vtbl;
   pcontext->transfer_destroy = u_transfer_destroy_vtbl;
   pcontext->transfer_inline_write = u_transfer_inline_write_vtbl;
   pcontext->create_surface = nv50_surface_create;
   pcontext->surface_destroy = nv50_surface_destroy;
}

void
nv50_screen_init_resource_functions(struct pipe_screen *pscreen)
{
   pscreen->resource_create = nv50_resource_create;
   pscreen->resource_from_handle = nv50_resource_from_handle;
   pscreen->resource_get_handle = u_resource_get_handle_vtbl;
   pscreen->resource_destroy = u_resource_destroy_vtbl;
}
