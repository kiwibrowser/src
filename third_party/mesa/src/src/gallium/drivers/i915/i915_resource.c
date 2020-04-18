#include "util/u_debug.h"

#include "i915_resource.h"
#include "i915_context.h"
#include "i915_screen.h"


static struct pipe_resource *
i915_resource_create(struct pipe_screen *screen,
                     const struct pipe_resource *template)
{
   if (template->target == PIPE_BUFFER)
      return i915_buffer_create(screen, template);
   else
      return i915_texture_create(screen, template, FALSE);

}

static struct pipe_resource *
i915_resource_from_handle(struct pipe_screen * screen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle)
{
   if (template->target == PIPE_BUFFER)
      return NULL;
   else
      return i915_texture_from_handle(screen, template, whandle);
}


void
i915_init_resource_functions(struct i915_context *i915 )
{
   i915->base.get_transfer = u_get_transfer_vtbl;
   i915->base.transfer_map = u_transfer_map_vtbl;
   i915->base.transfer_flush_region = u_transfer_flush_region_vtbl;
   i915->base.transfer_unmap = u_transfer_unmap_vtbl;
   i915->base.transfer_destroy = u_transfer_destroy_vtbl;
   i915->base.transfer_inline_write = u_transfer_inline_write_vtbl;
}

void
i915_init_screen_resource_functions(struct i915_screen *is)
{
   is->base.resource_create = i915_resource_create;
   is->base.resource_from_handle = i915_resource_from_handle;
   is->base.resource_get_handle = u_resource_get_handle_vtbl;
   is->base.resource_destroy = u_resource_destroy_vtbl;
}
