/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "vg_context.h"

#include "paint.h"
#include "renderer.h"
#include "shaders_cache.h"
#include "shader.h"
#include "vg_manager.h"
#include "api.h"
#include "mask.h"
#include "handle.h"

#include "pipe/p_context.h"
#include "util/u_inlines.h"

#include "cso_cache/cso_context.h"

#include "util/u_memory.h"
#include "util/u_blit.h"
#include "util/u_sampler.h"
#include "util/u_surface.h"
#include "util/u_format.h"

struct vg_context *_vg_context = 0;

struct vg_context * vg_current_context(void)
{
   return _vg_context;
}

/**
 * A depth/stencil rb will be needed regardless of what the visual says.
 */
static boolean
choose_depth_stencil_format(struct vg_context *ctx)
{
   struct pipe_screen *screen = ctx->pipe->screen;
   enum pipe_format formats[] = {
      PIPE_FORMAT_Z24_UNORM_S8_UINT,
      PIPE_FORMAT_S8_UINT_Z24_UNORM,
      PIPE_FORMAT_NONE
   };
   enum pipe_format *fmt;

   for (fmt = formats; *fmt != PIPE_FORMAT_NONE; fmt++) {
      if (screen->is_format_supported(screen, *fmt,
               PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL))
         break;
   }

   ctx->ds_format = *fmt;

   return (ctx->ds_format != PIPE_FORMAT_NONE);
}

void vg_set_current_context(struct vg_context *ctx)
{
   _vg_context = ctx;
   api_make_dispatch_current((ctx) ? ctx->dispatch : NULL);
}

struct vg_context * vg_create_context(struct pipe_context *pipe,
                                      const void *visual,
                                      struct vg_context *share)
{
   struct vg_context *ctx;

   ctx = CALLOC_STRUCT(vg_context);

   ctx->pipe = pipe;
   if (!choose_depth_stencil_format(ctx)) {
      FREE(ctx);
      return NULL;
   }

   ctx->dispatch = api_create_dispatch();

   vg_init_state(&ctx->state.vg);
   ctx->state.dirty = ALL_DIRTY;

   ctx->cso_context = cso_create_context(pipe);

   ctx->default_paint = paint_create(ctx);
   ctx->state.vg.stroke_paint = ctx->default_paint;
   ctx->state.vg.fill_paint = ctx->default_paint;


   ctx->mask.sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->mask.sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->mask.sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->mask.sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   ctx->mask.sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   ctx->mask.sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   ctx->mask.sampler.normalized_coords = 0;

   ctx->blend_sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->blend_sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->blend_sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->blend_sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   ctx->blend_sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   ctx->blend_sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   ctx->blend_sampler.normalized_coords = 0;

   vg_set_error(ctx, VG_NO_ERROR);

   ctx->owned_objects[VG_OBJECT_PAINT] = cso_hash_create();
   ctx->owned_objects[VG_OBJECT_IMAGE] = cso_hash_create();
   ctx->owned_objects[VG_OBJECT_MASK] = cso_hash_create();
   ctx->owned_objects[VG_OBJECT_FONT] = cso_hash_create();
   ctx->owned_objects[VG_OBJECT_PATH] = cso_hash_create();

   ctx->renderer = renderer_create(ctx);
   ctx->sc = shaders_cache_create(ctx);
   ctx->shader = shader_create(ctx);

   ctx->blit = util_create_blit(ctx->pipe, ctx->cso_context);

   return ctx;
}

void vg_destroy_context(struct vg_context *ctx)
{
   struct pipe_resource **cbuf = &ctx->mask.cbuf;

   util_destroy_blit(ctx->blit);
   renderer_destroy(ctx->renderer);
   shaders_cache_destroy(ctx->sc);
   shader_destroy(ctx->shader);
   paint_destroy(ctx->default_paint);

   if (*cbuf)
      pipe_resource_reference(cbuf, NULL);

   if (ctx->mask.union_fs)
      vg_shader_destroy(ctx, ctx->mask.union_fs);
   if (ctx->mask.intersect_fs)
      vg_shader_destroy(ctx, ctx->mask.intersect_fs);
   if (ctx->mask.subtract_fs)
      vg_shader_destroy(ctx, ctx->mask.subtract_fs);
   if (ctx->mask.set_fs)
      vg_shader_destroy(ctx, ctx->mask.set_fs);

   cso_release_all(ctx->cso_context);
   cso_destroy_context(ctx->cso_context);

   cso_hash_delete(ctx->owned_objects[VG_OBJECT_PAINT]);
   cso_hash_delete(ctx->owned_objects[VG_OBJECT_IMAGE]);
   cso_hash_delete(ctx->owned_objects[VG_OBJECT_MASK]);
   cso_hash_delete(ctx->owned_objects[VG_OBJECT_FONT]);
   cso_hash_delete(ctx->owned_objects[VG_OBJECT_PATH]);

   api_destroy_dispatch(ctx->dispatch);

   FREE(ctx);
}

void vg_init_object(struct vg_object *obj, struct vg_context *ctx, enum vg_object_type type)
{
   obj->type = type;
   obj->ctx = ctx;
   obj->handle = create_handle(obj);
}

/** free object resources, but not the object itself */
void vg_free_object(struct vg_object *obj)
{
   obj->type = 0;
   obj->ctx = NULL;
   destroy_handle(obj->handle);
}

VGboolean vg_context_is_object_valid(struct vg_context *ctx,
                                enum vg_object_type type,
                                VGHandle handle)
{
    if (ctx) {
       struct cso_hash *hash = ctx->owned_objects[type];
       if (!hash)
          return VG_FALSE;
       return cso_hash_contains(hash, (unsigned) handle);
    }
    return VG_FALSE;
}

void vg_context_add_object(struct vg_context *ctx,
                           struct vg_object *obj)
{
    if (ctx) {
       struct cso_hash *hash = ctx->owned_objects[obj->type];
       if (!hash)
          return;
       cso_hash_insert(hash, (unsigned) obj->handle, obj);
    }
}

void vg_context_remove_object(struct vg_context *ctx,
                              struct vg_object *obj)
{
   if (ctx) {
      struct cso_hash *hash = ctx->owned_objects[obj->type];
      if (!hash)
         return;
      cso_hash_take(hash, (unsigned) obj->handle);
   }
}

static struct pipe_resource *
create_texture(struct pipe_context *pipe, enum pipe_format format,
                    VGint width, VGint height)
{
   struct pipe_resource templ;

   memset(&templ, 0, sizeof(templ));

   if (format != PIPE_FORMAT_NONE) {
      templ.format = format;
   }
   else {
      templ.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   }

   templ.target = PIPE_TEXTURE_2D;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.last_level = 0;

   if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_ZS, 1)) {
      templ.bind = PIPE_BIND_DEPTH_STENCIL;
   } else {
      templ.bind = (PIPE_BIND_DISPLAY_TARGET |
                    PIPE_BIND_RENDER_TARGET |
                    PIPE_BIND_SAMPLER_VIEW);
   }

   return pipe->screen->resource_create(pipe->screen, &templ);
}

static struct pipe_sampler_view *
create_tex_and_view(struct pipe_context *pipe, enum pipe_format format,
                    VGint width, VGint height)
{
   struct pipe_resource *texture;
   struct pipe_sampler_view view_templ;
   struct pipe_sampler_view *view;

   texture = create_texture(pipe, format, width, height);

   if (!texture)
      return NULL;

   u_sampler_view_default_template(&view_templ, texture, texture->format);
   view = pipe->create_sampler_view(pipe, texture, &view_templ);
   /* want the texture to go away if the view is freed */
   pipe_resource_reference(&texture, NULL);

   return view;
}

static void
vg_context_update_surface_mask_view(struct vg_context *ctx,
                                    uint width, uint height)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct pipe_sampler_view *old_sampler_view = stfb->surface_mask_view;
   struct pipe_context *pipe = ctx->pipe;

   if (old_sampler_view &&
       old_sampler_view->texture->width0 == width &&
       old_sampler_view->texture->height0 == height)
      return;

   /*
     we use PIPE_FORMAT_B8G8R8A8_UNORM because we want to render to
     this texture and use it as a sampler, so while this wastes some
     space it makes both of those a lot simpler
   */
   stfb->surface_mask_view = create_tex_and_view(pipe,
         PIPE_FORMAT_B8G8R8A8_UNORM, width, height);

   if (!stfb->surface_mask_view) {
      if (old_sampler_view)
         pipe_sampler_view_reference(&old_sampler_view, NULL);
      return;
   }

   /* XXX could this call be avoided? */
   vg_validate_state(ctx);

   /* alpha mask starts with 1.f alpha */
   mask_fill(0, 0, width, height, 1.f);

   /* if we had an old surface copy it over */
   if (old_sampler_view) {
      struct pipe_box src_box;
      u_box_origin_2d(MIN2(old_sampler_view->texture->width0,
                           stfb->surface_mask_view->texture->width0),
                      MIN2(old_sampler_view->texture->height0,
                           stfb->surface_mask_view->texture->height0),
                      &src_box);

      pipe->resource_copy_region(pipe,
                                 stfb->surface_mask_view->texture,
                                 0, 0, 0, 0,
                                 old_sampler_view->texture,
                                 0, &src_box);
   }

   /* Free the old texture
    */
   if (old_sampler_view)
      pipe_sampler_view_reference(&old_sampler_view, NULL);
}

static void
vg_context_update_blend_texture_view(struct vg_context *ctx,
                                     uint width, uint height)
{
   struct pipe_context *pipe = ctx->pipe;
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct pipe_sampler_view *old = stfb->blend_texture_view;

   if (old &&
       old->texture->width0 == width &&
       old->texture->height0 == height)
      return;

   stfb->blend_texture_view = create_tex_and_view(pipe,
         PIPE_FORMAT_B8G8R8A8_UNORM, width, height);

   pipe_sampler_view_reference(&old, NULL);
}

static boolean
vg_context_update_depth_stencil_rb(struct vg_context * ctx,
                                   uint width, uint height)
{
   struct st_renderbuffer *dsrb = ctx->draw_buffer->dsrb;
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_surface surf_tmpl;

   if ((dsrb->width == width && dsrb->height == height) && dsrb->texture)
      return FALSE;

   /* unreference existing ones */
   pipe_surface_reference(&dsrb->surface, NULL);
   pipe_resource_reference(&dsrb->texture, NULL);
   dsrb->width = dsrb->height = 0;

   dsrb->texture = create_texture(pipe, dsrb->format, width, height);
   if (!dsrb->texture)
      return TRUE;

   memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   u_surface_default_template(&surf_tmpl, dsrb->texture,
                              PIPE_BIND_DEPTH_STENCIL);
   dsrb->surface = pipe->create_surface(pipe,
                                        dsrb->texture,
                                        &surf_tmpl);
   if (!dsrb->surface) {
      pipe_resource_reference(&dsrb->texture, NULL);
      return TRUE;
   }

   dsrb->width = width;
   dsrb->height = height;

   assert(dsrb->surface->width == width);
   assert(dsrb->surface->height == height);

   return TRUE;
}

void vg_validate_state(struct vg_context *ctx)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;

   vg_manager_validate_framebuffer(ctx);

   if (vg_context_update_depth_stencil_rb(ctx, stfb->width, stfb->height))
      ctx->state.dirty |= DEPTH_STENCIL_DIRTY;

   /* blend state depends on fb format and paint color */
   if ((ctx->state.dirty & FRAMEBUFFER_DIRTY) ||
       (ctx->state.dirty & PAINT_DIRTY))
      ctx->state.dirty |= BLEND_DIRTY;

   renderer_validate(ctx->renderer, ctx->state.dirty,
         ctx->draw_buffer, &ctx->state.vg);

   ctx->state.dirty = 0;

   shader_set_masking(ctx->shader, ctx->state.vg.masking);
   shader_set_image_mode(ctx->shader, ctx->state.vg.image_mode);
   shader_set_color_transform(ctx->shader, ctx->state.vg.color_transform);
}

VGboolean vg_object_is_valid(VGHandle object, enum vg_object_type type)
{
   struct vg_object *obj = handle_to_object(object);
   if (obj && is_aligned(obj) && obj->type == type)
      return VG_TRUE;
   else
      return VG_FALSE;
}

void vg_set_error(struct vg_context *ctx,
                  VGErrorCode code)
{
   /*vgGetError returns the oldest error code provided by
    * an API call on the current context since the previous
    * call to vgGetError on that context (or since the creation
    of the context).*/
   if (ctx->_error == VG_NO_ERROR)
      ctx->_error = code;
}

static void vg_prepare_blend_texture(struct vg_context *ctx,
                                     struct pipe_sampler_view *src)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct pipe_surface *surf;
   struct pipe_surface surf_tmpl;

   vg_context_update_blend_texture_view(ctx, stfb->width, stfb->height);

   memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   u_surface_default_template(&surf_tmpl, stfb->blend_texture_view->texture,
                              PIPE_BIND_RENDER_TARGET);
   surf = ctx->pipe->create_surface(ctx->pipe,
                                    stfb->blend_texture_view->texture,
                                    &surf_tmpl);
   if (surf) {
      util_blit_pixels_tex(ctx->blit,
                           src, 0, 0, stfb->width, stfb->height,
                           surf, 0, 0, stfb->width, stfb->height,
                           0.0, PIPE_TEX_MIPFILTER_NEAREST);

      pipe_surface_reference(&surf, NULL);
   }
}

struct pipe_sampler_view *vg_prepare_blend_surface(struct vg_context *ctx)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_sampler_view *view;
   struct pipe_sampler_view view_templ;
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct st_renderbuffer *strb = stfb->strb;

   vg_validate_state(ctx);

   u_sampler_view_default_template(&view_templ, strb->texture, strb->texture->format);
   view = pipe->create_sampler_view(pipe, strb->texture, &view_templ);

   vg_prepare_blend_texture(ctx, view);

   pipe_sampler_view_reference(&view, NULL);

   return stfb->blend_texture_view;
}


struct pipe_sampler_view *vg_prepare_blend_surface_from_mask(struct vg_context *ctx)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;

   vg_validate_state(ctx);

   vg_context_update_surface_mask_view(ctx, stfb->width, stfb->height);
   vg_prepare_blend_texture(ctx, stfb->surface_mask_view);

   return stfb->blend_texture_view;
}

struct pipe_sampler_view *vg_get_surface_mask(struct vg_context *ctx)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;

   vg_context_update_surface_mask_view(ctx, stfb->width, stfb->height);

   return stfb->surface_mask_view;
}

/**
 * A transformation from window coordinates to paint coordinates.
 */
VGboolean vg_get_paint_matrix(struct vg_context *ctx,
                              const struct matrix *paint_to_user,
                              const struct matrix *user_to_surface,
                              struct matrix *mat)
{
   struct matrix tmp;

   /* get user-to-paint matrix */
   memcpy(mat, paint_to_user, sizeof(*paint_to_user));
   if (!matrix_invert(mat))
      return VG_FALSE;

   /* get surface-to-user matrix */
   memcpy(&tmp, user_to_surface, sizeof(*user_to_surface));
   if (!matrix_invert(&tmp))
      return VG_FALSE;

   matrix_mult(mat, &tmp);

   return VG_TRUE;
}
