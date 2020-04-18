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

#include "mask.h"

#include "path.h"
#include "image.h"
#include "shaders_cache.h"
#include "renderer.h"
#include "asm_util.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_surface.h"
#include "util/u_sampler.h"

struct vg_mask_layer {
   struct vg_object base;

   VGint width;
   VGint height;

   struct pipe_sampler_view *sampler_view;
};

static INLINE VGboolean
intersect_rectangles(VGint dwidth, VGint dheight,
                     VGint swidth, VGint sheight,
                     VGint tx, VGint ty,
                     VGint twidth, VGint theight,
                     VGint *offsets,
                     VGint *location)
{
   if (tx + twidth <= 0 || tx >= dwidth)
      return VG_FALSE;
   if (ty + theight <= 0 || ty >= dheight)
      return VG_FALSE;

   offsets[0] = 0;
   offsets[1] = 0;
   location[0] = tx;
   location[1] = ty;

   if (tx < 0) {
      offsets[0] -= tx;
      location[0] = 0;

      location[2] = MIN2(tx + swidth, MIN2(dwidth, tx + twidth));
      offsets[2] = location[2];
   } else {
      offsets[2] = MIN2(twidth, MIN2(dwidth - tx, swidth ));
      location[2] = offsets[2];
   }

   if (ty < 0) {
      offsets[1] -= ty;
      location[1] = 0;

      location[3] = MIN2(ty + sheight, MIN2(dheight, ty + theight));
      offsets[3] = location[3];
   } else {
      offsets[3] = MIN2(theight, MIN2(dheight - ty, sheight));
      location[3] = offsets[3];
   }

   return VG_TRUE;
}

#if DEBUG_MASKS
static void read_alpha_mask(void * data, VGint dataStride,
                            VGImageFormat dataFormat,
                            VGint sx, VGint sy,
                            VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_context *pipe = ctx->pipe;

   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct st_renderbuffer *strb = stfb->alpha_mask;

   VGfloat temp[VEGA_MAX_IMAGE_WIDTH][4];
   VGfloat *df = (VGfloat*)temp;
   VGint y = (stfb->height - sy) - 1, yStep = -1;
   VGint i;
   VGubyte *dst = (VGubyte *)data;
   VGint xoffset = 0, yoffset = 0;

   if (sx < 0) {
      xoffset = -sx;
      xoffset *= _vega_size_for_format(dataFormat);
      width += sx;
      sx = 0;
   }
   if (sy < 0) {
      yoffset = -sy;
      height += sy;
      sy = 0;
      y = (stfb->height - sy) - 1;
      yoffset *= dataStride;
   }

   {
      struct pipe_surface *surf;

      surf = pipe->create_surface(pipe, strb->texture,  0, 0, 0,
                                  PIPE_BIND_TRANSFER_READ);

      /* Do a row at a time to flip image data vertically */
      for (i = 0; i < height; i++) {
#if 0
         debug_printf("%d-%d  == %d\n", sy, height, y);
#endif
         pipe_get_tile_rgba(surf, sx, y, width, 1, df);
         y += yStep;
         _vega_pack_rgba_span_float(ctx, width, temp, dataFormat,
                                    dst + yoffset + xoffset);
         dst += dataStride;
      }

      pipe_surface_reference(&surf, NULL);
   }
}

void save_alpha_to_file(const char *filename)
{
   struct vg_context *ctx = vg_current_context();
   struct st_framebuffer *stfb = ctx->draw_buffer;
   VGint *data;
   int i, j;

   data = malloc(sizeof(int) * stfb->width * stfb->height);
   read_alpha_mask(data, stfb->width * sizeof(int),
                   VG_sRGBA_8888,
                   0, 0, stfb->width, stfb->height);
   fprintf(stderr, "/*---------- start */\n");
   fprintf(stderr, "const int image_width = %d;\n",
           stfb->width);
   fprintf(stderr, "const int image_height = %d;\n",
           stfb->height);
   fprintf(stderr, "const int image_data = {\n");
   for (i = 0; i < stfb->height; ++i) {
      for (j = 0; j < stfb->width; ++j) {
         int rgba = data[i * stfb->height + j];
         int argb = 0;
         argb = (rgba >> 8);
         argb |= ((rgba & 0xff) << 24);
         fprintf(stderr, "0x%x, ", argb);
      }
      fprintf(stderr, "\n");
   }
   fprintf(stderr, "};\n");
   fprintf(stderr, "/*---------- end */\n");
}
#endif

/* setup mask shader */
static void *setup_mask_operation(VGMaskOperation operation)
{
   struct vg_context *ctx = vg_current_context();
   void *shader = 0;

   switch (operation) {
   case VG_UNION_MASK: {
      if (!ctx->mask.union_fs) {
         ctx->mask.union_fs = shader_create_from_text(ctx->pipe,
                                                      union_mask_asm,
                                                      200,
                                                      PIPE_SHADER_FRAGMENT);
      }
      shader = ctx->mask.union_fs->driver;
   }
      break;
   case VG_INTERSECT_MASK: {
      if (!ctx->mask.intersect_fs) {
         ctx->mask.intersect_fs = shader_create_from_text(ctx->pipe,
                                                          intersect_mask_asm,
                                                          200,
                                                          PIPE_SHADER_FRAGMENT);
      }
      shader = ctx->mask.intersect_fs->driver;
   }
      break;
   case VG_SUBTRACT_MASK: {
      if (!ctx->mask.subtract_fs) {
         ctx->mask.subtract_fs = shader_create_from_text(ctx->pipe,
                                                         subtract_mask_asm,
                                                         200,
                                                         PIPE_SHADER_FRAGMENT);
      }
      shader = ctx->mask.subtract_fs->driver;
   }
      break;
   case VG_SET_MASK: {
      if (!ctx->mask.set_fs) {
         ctx->mask.set_fs = shader_create_from_text(ctx->pipe,
                                                    set_mask_asm,
                                                    200,
                                                    PIPE_SHADER_FRAGMENT);
      }
      shader = ctx->mask.set_fs->driver;
   }
      break;
   default:
         assert(0);
      break;
   }

   return shader;
}

static void mask_resource_fill(struct pipe_resource *dst,
                               int x, int y, int width, int height,
                               VGfloat coverage)
{
   struct vg_context *ctx = vg_current_context();
   VGfloat fs_consts[12] = {
      0.0f, 0.0f, 0.0f, 0.0f, /* not used */
      0.0f, 0.0f, 0.0f, 0.0f, /* not used */
      0.0f, 0.0f, 0.0f, coverage /* color */
   };
   void *fs;

   if (x < 0) {
      width += x;
      x = 0;
   }
   if (y < 0) {
      height += y;
      y = 0;
   }

   fs = shaders_cache_fill(ctx->sc, VEGA_SOLID_FILL_SHADER);

   if (renderer_filter_begin(ctx->renderer, dst, VG_FALSE, ~0,
            NULL, NULL, 0, fs, (const void *) fs_consts, sizeof(fs_consts))) {
      renderer_filter(ctx->renderer, x, y, width, height, 0, 0, 0, 0);
      renderer_filter_end(ctx->renderer);
   }

#if DEBUG_MASKS
   save_alpha_to_file(0);
#endif
}


static void mask_using_texture(struct pipe_sampler_view *sampler_view,
                               VGboolean is_layer,
                               VGMaskOperation operation,
                               VGint x, VGint y,
                               VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_sampler_view *dst_view = vg_get_surface_mask(ctx);
   struct pipe_resource *dst = dst_view->texture;
   struct pipe_resource *texture = sampler_view->texture;
   const struct pipe_sampler_state *samplers[2];
   struct pipe_sampler_view *views[2];
   struct pipe_sampler_state sampler;
   VGint offsets[4], loc[4];
   const VGfloat ones[4] = {1.f, 1.f, 1.f, 1.f};
   void *fs;

   if (!intersect_rectangles(dst->width0, dst->height0,
                             texture->width0, texture->height0,
                             x, y, width, height,
                             offsets, loc))
      return;
#if 0
   debug_printf("Offset = [%d, %d, %d, %d]\n", offsets[0],
                offsets[1], offsets[2], offsets[3]);
   debug_printf("Locati = [%d, %d, %d, %d]\n", loc[0],
                loc[1], loc[2], loc[3]);
#endif


   sampler = ctx->mask.sampler;
   sampler.normalized_coords = 1;
   samplers[0] = &sampler;
   views[0] = sampler_view;

   /* prepare our blend surface */
   samplers[1] = &ctx->mask.sampler;
   views[1] = vg_prepare_blend_surface_from_mask(ctx);

   fs = setup_mask_operation(operation);

   if (renderer_filter_begin(ctx->renderer, dst, VG_FALSE,
            ~0, samplers, views, 2, fs, (const void *) ones, sizeof(ones))) {
      /* layer should be flipped when used as a texture */
      if (is_layer) {
         offsets[1] += offsets[3];
         offsets[3] = -offsets[3];
      }
      renderer_filter(ctx->renderer,
            loc[0], loc[1], loc[2], loc[3],
            offsets[0], offsets[1], offsets[2], offsets[3]);
      renderer_filter_end(ctx->renderer);
   }
}


#ifdef OPENVG_VERSION_1_1

struct vg_mask_layer * mask_layer_create(VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_mask_layer *mask = 0;

   mask = CALLOC_STRUCT(vg_mask_layer);
   vg_init_object(&mask->base, ctx, VG_OBJECT_MASK);
   mask->width = width;
   mask->height = height;

   {
      struct pipe_resource pt;
      struct pipe_context *pipe = ctx->pipe;
      struct pipe_screen *screen = ctx->pipe->screen;
      struct pipe_sampler_view view_templ;
      struct pipe_sampler_view *view = NULL;
      struct pipe_resource *texture;

      memset(&pt, 0, sizeof(pt));
      pt.target = PIPE_TEXTURE_2D;
      pt.format = PIPE_FORMAT_B8G8R8A8_UNORM;
      pt.last_level = 0;
      pt.width0 = width;
      pt.height0 = height;
      pt.depth0 = 1;
      pt.array_size = 1;
      pt.bind = PIPE_BIND_SAMPLER_VIEW;

      texture = screen->resource_create(screen, &pt);

      if (texture) {
         u_sampler_view_default_template(&view_templ, texture, texture->format);
         view = pipe->create_sampler_view(pipe, texture, &view_templ);
      }
      pipe_resource_reference(&texture, NULL);
      mask->sampler_view = view;
   }

   vg_context_add_object(ctx, &mask->base);

   return mask;
}

void mask_layer_destroy(struct vg_mask_layer *layer)
{
   struct vg_context *ctx = vg_current_context();

   vg_context_remove_object(ctx, &layer->base);
   pipe_sampler_view_reference(&layer->sampler_view, NULL);
   FREE(layer);
}

void mask_layer_fill(struct vg_mask_layer *layer,
                     VGint x, VGint y,
                     VGint width, VGint height,
                     VGfloat value)
{
   mask_resource_fill(layer->sampler_view->texture,
                      x, y, width, height, value);
}

void mask_copy(struct vg_mask_layer *layer,
               VGint sx, VGint sy,
               VGint dx, VGint dy,
               VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_sampler_view *src = vg_get_surface_mask(ctx);
   struct pipe_surface *surf, surf_tmpl;

   /* get the destination surface */
   u_surface_default_template(&surf_tmpl, layer->sampler_view->texture,
                              PIPE_BIND_RENDER_TARGET);
   surf = ctx->pipe->create_surface(ctx->pipe, layer->sampler_view->texture,
                                    &surf_tmpl);
   if (surf && renderer_copy_begin(ctx->renderer, surf, VG_FALSE, src)) {
      /* layer should be flipped when used as a texture */
      sy += height;
      height = -height;

      renderer_copy(ctx->renderer,
            dx, dy, width, height,
            sx, sy, width, height);
      renderer_copy_end(ctx->renderer);
   }

   pipe_surface_reference(&surf, NULL);
}

static void mask_layer_render_to(struct vg_mask_layer *layer,
                                 struct path *path,
                                 VGbitfield paint_modes)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_sampler_view *view = vg_get_surface_mask(ctx);
   struct matrix *mat = &ctx->state.vg.path_user_to_surface_matrix;
   struct pipe_surface *surf, surf_tmpl;
   u_surface_default_template(&surf_tmpl, view->texture,
                              PIPE_BIND_RENDER_TARGET);
   surf = pipe->create_surface(pipe, view->texture, &surf_tmpl);

   renderer_validate_for_mask_rendering(ctx->renderer, surf, mat);

   if (paint_modes & VG_FILL_PATH) {
      path_fill(path);
   }

   if (paint_modes & VG_STROKE_PATH){
      path_stroke(path);
   }

   pipe_surface_reference(&surf, NULL);
}

void mask_render_to(struct path *path,
                    VGbitfield paint_modes,
                    VGMaskOperation operation)
{
   struct vg_context *ctx = vg_current_context();
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct vg_mask_layer *temp_layer;
   VGint width, height;

   width = stfb->width;
   height = stfb->height;

   temp_layer = mask_layer_create(width, height);
   mask_layer_fill(temp_layer, 0, 0, width, height, 0.0f);

   mask_layer_render_to(temp_layer, path, paint_modes);

   mask_using_layer(temp_layer, operation, 0, 0, width, height);

   mask_layer_destroy(temp_layer);
}

void mask_using_layer(struct vg_mask_layer *layer,
                      VGMaskOperation operation,
                      VGint x, VGint y,
                      VGint width, VGint height)
{
   mask_using_texture(layer->sampler_view, VG_TRUE, operation,
                      x, y, width, height);
}

VGint mask_layer_width(struct vg_mask_layer *layer)
{
   return layer->width;
}

VGint mask_layer_height(struct vg_mask_layer *layer)
{
   return layer->height;
}


#endif

void mask_using_image(struct vg_image *image,
                      VGMaskOperation operation,
                      VGint x, VGint y,
                      VGint width, VGint height)
{
   mask_using_texture(image->sampler_view, VG_FALSE, operation,
                      x, y, width, height);
}

void mask_fill(VGint x, VGint y, VGint width, VGint height,
               VGfloat value)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_sampler_view *view = vg_get_surface_mask(ctx);

#if DEBUG_MASKS
   debug_printf("mask_fill(%d, %d, %d, %d) with  rgba(%f, %f, %f, %f)\n",
                x, y, width, height,
                0.0f, 0.0f, 0.0f, value);
#endif

   mask_resource_fill(view->texture, x, y, width, height, value);
}

VGint mask_bind_samplers(struct pipe_sampler_state **samplers,
                         struct pipe_sampler_view **sampler_views)
{
   struct vg_context *ctx = vg_current_context();

   if (ctx->state.vg.masking) {
      samplers[1] = &ctx->mask.sampler;
      sampler_views[1] = vg_get_surface_mask(ctx);
      return 1;
   } else
      return 0;
}
