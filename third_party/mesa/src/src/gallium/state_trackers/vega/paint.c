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

#include "paint.h"

#include "matrix.h"
#include "image.h"

#include "pipe/p_compiler.h"
#include "util/u_inlines.h"

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_sampler.h"

#include "cso_cache/cso_context.h"

struct vg_paint {
   struct vg_object base;

   VGPaintType type;

   struct {
      VGfloat color[4];
      VGint colori[4];
   } solid;

   struct {
      VGColorRampSpreadMode spread;
      VGuint color_data[1024];
      struct {
         VGfloat  coords[4];
         VGint  coordsi[4];
      } linear;
      struct {
         VGfloat vals[5];
         VGint valsi[5];
      } radial;
      struct pipe_sampler_view *sampler_view;
      struct pipe_sampler_state sampler;

      VGfloat *ramp_stops;
      VGint *ramp_stopsi;
      VGint    num_stops;

      VGboolean color_ramps_premultiplied;
   } gradient;

   struct {
      struct pipe_sampler_view *sampler_view;
      VGTilingMode tiling_mode;
      struct pipe_sampler_state sampler;
   } pattern;

   /* XXX next 3 all unneded? */
   struct pipe_resource *cbuf;
   struct pipe_shader_state fs_state;
   void *fs;
};

static INLINE VGuint mix_pixels(VGuint p1, VGuint a, VGuint p2, VGuint b)
{
   VGuint t = (p1 & 0xff00ff) * a + (p2 & 0xff00ff) * b;
   t >>= 8; t &= 0xff00ff;

   p1 = ((p1 >> 8) & 0xff00ff) * a + ((p2 >> 8) & 0xff00ff) * b;
   p1 &= 0xff00ff00; p1 |= t;

   return p1;
}

static INLINE VGuint float4_to_argb(const VGfloat *clr)
{
   return float_to_ubyte(clr[3]) << 24 |
      float_to_ubyte(clr[0]) << 16 |
      float_to_ubyte(clr[1]) << 8 |
      float_to_ubyte(clr[2]) << 0;
}

static INLINE void create_gradient_data(const VGfloat *ramp_stops,
                                        VGint num,
                                        VGuint *data,
                                        VGint size)
{
   VGint i;
   VGint pos = 0;
   VGfloat fpos = 0, incr = 1.f / size;
   VGuint last_color;

   while (fpos < ramp_stops[0]) {
      data[pos] = float4_to_argb(ramp_stops + 1);
      fpos += incr;
      ++pos;
   }

   for (i = 0; i < num - 1; ++i) {
      VGint rcur  = 5 * i;
      VGint rnext = 5 * (i + 1);
      VGfloat delta = 1.f/(ramp_stops[rnext] - ramp_stops[rcur]);
      while (fpos < ramp_stops[rnext] && pos < size) {
         VGint dist = 256 * ((fpos - ramp_stops[rcur]) * delta);
         VGint idist = 256 - dist;
         VGuint current_color = float4_to_argb(ramp_stops + rcur + 1);
         VGuint next_color = float4_to_argb(ramp_stops + rnext + 1);
         data[pos] = mix_pixels(current_color, idist,
                                next_color, dist);
         fpos += incr;
         ++pos;
      }
   }

   last_color = float4_to_argb(ramp_stops + ((num - 1) * 5 + 1));
   while (pos < size) {
      data[pos] = last_color;
      ++pos;
   }
   data[size-1] = last_color;
}

static INLINE struct pipe_resource *create_gradient_texture(struct vg_paint *p)
{
   struct pipe_context *pipe = p->base.ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_resource *tex = 0;
   struct pipe_resource templ;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_1D;
   templ.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   templ.last_level = 0;
   templ.width0 = 1024;
   templ.height0 = 1;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.bind = PIPE_BIND_SAMPLER_VIEW;

   tex = screen->resource_create(screen, &templ);

   { /* upload color_data */
      struct pipe_transfer *transfer =
         pipe_get_transfer(p->base.ctx->pipe, tex, 0, 0,
                           PIPE_TRANSFER_WRITE, 0, 0, 1024, 1);
      void *map = pipe->transfer_map(pipe, transfer);
      memcpy(map, p->gradient.color_data, sizeof(VGint)*1024);
      pipe->transfer_unmap(pipe, transfer);
      pipe->transfer_destroy(pipe, transfer);
   }

   return tex;
}

static INLINE struct pipe_sampler_view *create_gradient_sampler_view(struct vg_paint *p)
{
   struct pipe_context *pipe = p->base.ctx->pipe;
   struct pipe_resource *texture;
   struct pipe_sampler_view view_templ;
   struct pipe_sampler_view *view;

   texture = create_gradient_texture(p);

   if (!texture)
      return NULL;

   u_sampler_view_default_template(&view_templ, texture, texture->format);
   view = pipe->create_sampler_view(pipe, texture, &view_templ);
   /* want the texture to go away if the view is freed */
   pipe_resource_reference(&texture, NULL);

   return view;
}

struct vg_paint * paint_create(struct vg_context *ctx)
{
   struct vg_paint *paint = CALLOC_STRUCT(vg_paint);
   const VGfloat default_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
   const VGfloat def_ling[] = {0.0f, 0.0f, 1.0f, 0.0f};
   const VGfloat def_radg[] = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
   vg_init_object(&paint->base, ctx, VG_OBJECT_PAINT);
   vg_context_add_object(ctx, &paint->base);

   paint->type = VG_PAINT_TYPE_COLOR;
   memcpy(paint->solid.color, default_color,
          4 * sizeof(VGfloat));
   paint->gradient.spread = VG_COLOR_RAMP_SPREAD_PAD;
   memcpy(paint->gradient.linear.coords, def_ling,
          4 * sizeof(VGfloat));
   memcpy(paint->gradient.radial.vals, def_radg,
          5 * sizeof(VGfloat));

   paint->gradient.sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   paint->gradient.sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   paint->gradient.sampler.min_img_filter = PIPE_TEX_MIPFILTER_NEAREST;
   paint->gradient.sampler.mag_img_filter = PIPE_TEX_MIPFILTER_NEAREST;
   paint->gradient.sampler.normalized_coords = 1;

   memcpy(&paint->pattern.sampler,
          &paint->gradient.sampler,
          sizeof(struct pipe_sampler_state));

   return paint;
}

void paint_destroy(struct vg_paint *paint)
{
   struct vg_context *ctx = paint->base.ctx;
   pipe_sampler_view_reference(&paint->gradient.sampler_view, NULL);
   if (paint->pattern.sampler_view)
      pipe_sampler_view_reference(&paint->pattern.sampler_view, NULL);
   if (ctx)
      vg_context_remove_object(ctx, &paint->base);

   free(paint->gradient.ramp_stopsi);
   free(paint->gradient.ramp_stops);
   FREE(paint);
}

void paint_set_color(struct vg_paint *paint,
                     const VGfloat *color)
{
   paint->solid.color[0] = color[0];
   paint->solid.color[1] = color[1];
   paint->solid.color[2] = color[2];
   paint->solid.color[3] = color[3];

   paint->solid.colori[0] = FLT_TO_INT(color[0]);
   paint->solid.colori[1] = FLT_TO_INT(color[1]);
   paint->solid.colori[2] = FLT_TO_INT(color[2]);
   paint->solid.colori[3] = FLT_TO_INT(color[3]);
}

static INLINE void paint_color_buffer(struct vg_paint *paint, void *buffer)
{
   VGfloat *map = (VGfloat*)buffer;
   memcpy(buffer, paint->solid.color, 4 * sizeof(VGfloat));
   map[4] = 0.f;
   map[5] = 1.f;
   map[6] = 2.f;
   map[7] = 4.f;
}

static INLINE void paint_linear_gradient_buffer(struct vg_paint *paint,
                                                const struct matrix *inv,
                                                void *buffer)
{
   VGfloat *map = (VGfloat*)buffer;
   VGfloat dd;

   map[0] = paint->gradient.linear.coords[2] - paint->gradient.linear.coords[0];
   map[1] = paint->gradient.linear.coords[3] - paint->gradient.linear.coords[1];
   dd = (map[0] * map[0] + map[1] * map[1]);

   map[2] = (dd > 0.0f) ? 1.f / dd : 0.f;
   map[3] = 1.f;

   map[4] = 0.f;
   map[5] = 1.f;
   map[6] = 2.f;
   map[7] = 4.f;
   {
      struct matrix mat;
      matrix_load_identity(&mat);
      /* VEGA_LINEAR_GRADIENT_SHADER expects the first point to be at (0, 0) */
      matrix_translate(&mat, -paint->gradient.linear.coords[0], -paint->gradient.linear.coords[1]);
      matrix_mult(&mat, inv);

      map[8]  = mat.m[0]; map[9]  = mat.m[3]; map[10] = mat.m[6]; map[11] = 0.f;
      map[12] = mat.m[1]; map[13] = mat.m[4]; map[14] = mat.m[7]; map[15] = 0.f;
      map[16] = mat.m[2]; map[17] = mat.m[5]; map[18] = mat.m[8]; map[19] = 0.f;
   }
#if 0
   debug_printf("Coords  (%f, %f, %f, %f)\n",
                map[0], map[1], map[2], map[3]);
#endif
}


static INLINE void paint_radial_gradient_buffer(struct vg_paint *paint,
                                                const struct matrix *inv,
                                                void *buffer)
{
   const VGfloat *center = &paint->gradient.radial.vals[0];
   const VGfloat *focal = &paint->gradient.radial.vals[2];
   VGfloat rr = paint->gradient.radial.vals[4];
   VGfloat *map = (VGfloat*)buffer;
   VGfloat dd, new_focal[2];

   rr *= rr;

   map[0] = center[0] - focal[0];
   map[1] = center[1] - focal[1];
   dd = map[0] * map[0] + map[1] * map[1];

   /* focal point must lie inside the circle */
   if (0.998f * rr < dd) {
      VGfloat scale;

      scale = (dd > 0.0f) ? sqrt(0.998f * rr / dd) : 0.0f;
      map[0] *= scale;
      map[1] *= scale;

      new_focal[0] = center[0] - map[0];
      new_focal[1] = center[1] - map[1];
      dd = map[0] * map[0] + map[1] * map[1];
      focal = new_focal;
   }

   map[2] = (rr > dd) ? rr - dd : 1.0f;
   map[3] = 1.f;

   map[4] = 0.f;
   map[5] = 1.f;
   map[6] = 2.f;
   map[7] = 4.f;

   {
      struct matrix mat;
      matrix_load_identity(&mat);
      matrix_translate(&mat, -focal[0], -focal[1]);
      matrix_mult(&mat, inv);

      map[8]  = mat.m[0]; map[9]  = mat.m[3]; map[10] = mat.m[6]; map[11] = 0.f;
      map[12] = mat.m[1]; map[13] = mat.m[4]; map[14] = mat.m[7]; map[15] = 0.f;
      map[16] = mat.m[2]; map[17] = mat.m[5]; map[18] = mat.m[8]; map[19] = 0.f;
   }

#if 0
   debug_printf("Coords  (%f, %f, %f, %f)\n",
                map[0], map[1], map[2], map[3]);
#endif
}


static INLINE void  paint_pattern_buffer(struct vg_paint *paint,
                                         const struct matrix *inv,
                                         void *buffer)
{
   VGfloat *map = (VGfloat *)buffer;
   memcpy(map, paint->solid.color, 4 * sizeof(VGfloat));

   map[4] = 0.f;
   map[5] = 1.f;
   map[6] = paint->pattern.sampler_view->texture->width0;
   map[7] = paint->pattern.sampler_view->texture->height0;
   {
      struct matrix mat;

      memcpy(&mat, inv, sizeof(*inv));

      map[8]  = mat.m[0]; map[9]  = mat.m[3]; map[10] = mat.m[6]; map[11] = 0.f;
      map[12] = mat.m[1]; map[13] = mat.m[4]; map[14] = mat.m[7]; map[15] = 0.f;
      map[16] = mat.m[2]; map[17] = mat.m[5]; map[18] = mat.m[8]; map[19] = 0.f;
   }
}

void paint_set_type(struct vg_paint *paint, VGPaintType type)
{
   paint->type = type;
}

void paint_set_ramp_stops(struct vg_paint *paint, const VGfloat *stops,
                          int num)
{
   const VGfloat default_stops[] = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                    1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
   VGint i;
   const VGint num_stops = num / 5;
   VGfloat last_coord;

   paint->gradient.num_stops = num;
   if (num) {
      free(paint->gradient.ramp_stops);
      paint->gradient.ramp_stops = malloc(sizeof(VGfloat)*num);
      memcpy(paint->gradient.ramp_stops, stops, sizeof(VGfloat)*num);
   } else
      return;

   /* stops must be in increasing order. the last stop is 1.0. if the
    * first one is bigger than 1 then the whole sequence is invalid*/
   if (stops[0] > 1) {
      stops = default_stops;
      num = 10;
   }
   last_coord = stops[0];
   for (i = 1; i < num_stops; ++i) {
      VGint idx = 5 * i;
      VGfloat coord = stops[idx];
      if (!floatsEqual(last_coord, coord) && coord < last_coord) {
         stops = default_stops;
         num = 10;
         break;
      }
      last_coord = coord;
   }

   create_gradient_data(stops, num / 5, paint->gradient.color_data,
                        1024);

   if (paint->gradient.sampler_view) {
      pipe_sampler_view_reference(&paint->gradient.sampler_view, NULL);
      paint->gradient.sampler_view = NULL;
   }

   paint->gradient.sampler_view = create_gradient_sampler_view(paint);
}

void paint_set_colori(struct vg_paint *p,
                      VGuint rgba)
{
   p->solid.color[0] = ((rgba >> 24) & 0xff) / 255.f;
   p->solid.color[1] = ((rgba >> 16) & 0xff) / 255.f;
   p->solid.color[2] = ((rgba >>  8) & 0xff) / 255.f;
   p->solid.color[3] = ((rgba >>  0) & 0xff) / 255.f;
}

VGuint paint_colori(struct vg_paint *p)
{
#define F2B(f) (float_to_ubyte(f))

   return ((F2B(p->solid.color[0]) << 24) |
           (F2B(p->solid.color[1]) << 16) |
           (F2B(p->solid.color[2]) << 8)  |
           (F2B(p->solid.color[3]) << 0));
#undef F2B
}

void paint_set_linear_gradient(struct vg_paint *paint,
                               const VGfloat *coords)
{
   memcpy(paint->gradient.linear.coords, coords, sizeof(VGfloat) * 4);
}

void paint_set_spread_mode(struct vg_paint *paint,
                           VGint mode)
{
   paint->gradient.spread = mode;
   switch(mode) {
   case VG_COLOR_RAMP_SPREAD_PAD:
      paint->gradient.sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      break;
   case VG_COLOR_RAMP_SPREAD_REPEAT:
      paint->gradient.sampler.wrap_s = PIPE_TEX_WRAP_REPEAT;
      break;
   case VG_COLOR_RAMP_SPREAD_REFLECT:
      paint->gradient.sampler.wrap_s = PIPE_TEX_WRAP_MIRROR_REPEAT;
      break;
   }
}

VGColorRampSpreadMode paint_spread_mode(struct vg_paint *paint)
{
   return paint->gradient.spread;
}

void paint_set_radial_gradient(struct vg_paint *paint,
                               const VGfloat *values)
{
   memcpy(paint->gradient.radial.vals, values, sizeof(VGfloat) * 5);
}

void paint_set_pattern(struct vg_paint *paint,
                       struct vg_image *img)
{
   if (paint->pattern.sampler_view)
      pipe_sampler_view_reference(&paint->pattern.sampler_view, NULL);

   paint->pattern.sampler_view = NULL;
   pipe_sampler_view_reference(&paint->pattern.sampler_view,
                               img->sampler_view);
}

void paint_set_pattern_tiling(struct vg_paint *paint,
                              VGTilingMode mode)
{
   paint->pattern.tiling_mode = mode;

   switch(mode) {
   case VG_TILE_FILL:
      paint->pattern.sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
      paint->pattern.sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
      break;
   case VG_TILE_PAD:
      paint->pattern.sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      paint->pattern.sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      break;
   case VG_TILE_REPEAT:
      paint->pattern.sampler.wrap_s = PIPE_TEX_WRAP_REPEAT;
      paint->pattern.sampler.wrap_t = PIPE_TEX_WRAP_REPEAT;
      break;
   case VG_TILE_REFLECT:
      paint->pattern.sampler.wrap_s = PIPE_TEX_WRAP_MIRROR_REPEAT;
      paint->pattern.sampler.wrap_t = PIPE_TEX_WRAP_MIRROR_REPEAT;
      break;
   default:
      debug_assert("!Unknown tiling mode");
   }
}

void paint_get_color(struct vg_paint *paint,
                     VGfloat *color)
{
   color[0] = paint->solid.color[0];
   color[1] = paint->solid.color[1];
   color[2] = paint->solid.color[2];
   color[3] = paint->solid.color[3];
}

void paint_ramp_stops(struct vg_paint *paint, VGfloat *stops,
                      int num)
{
   memcpy(stops, paint->gradient.ramp_stops, sizeof(VGfloat)*num);
}

void paint_linear_gradient(struct vg_paint *paint,
                           VGfloat *coords)
{
   memcpy(coords, paint->gradient.linear.coords, sizeof(VGfloat)*4);
}

void paint_radial_gradient(struct vg_paint *paint,
                           VGfloat *coords)
{
   memcpy(coords, paint->gradient.radial.vals, sizeof(VGfloat)*5);
}

int paint_num_ramp_stops(struct vg_paint *paint)
{
   return paint->gradient.num_stops;
}

VGPaintType paint_type(struct vg_paint *paint)
{
   return paint->type;
}

void paint_set_coloriv(struct vg_paint *paint,
                      const VGint *color)
{
   paint->solid.color[0] = color[0];
   paint->solid.color[1] = color[1];
   paint->solid.color[2] = color[2];
   paint->solid.color[3] = color[3];

   paint->solid.colori[0] = color[0];
   paint->solid.colori[1] = color[1];
   paint->solid.colori[2] = color[2];
   paint->solid.colori[3] = color[3];
}

void paint_get_coloriv(struct vg_paint *paint,
                      VGint *color)
{
   color[0] = paint->solid.colori[0];
   color[1] = paint->solid.colori[1];
   color[2] = paint->solid.colori[2];
   color[3] = paint->solid.colori[3];
}

void paint_set_color_ramp_premultiplied(struct vg_paint *paint,
                                        VGboolean set)
{
   paint->gradient.color_ramps_premultiplied = set;
}

VGboolean paint_color_ramp_premultiplied(struct vg_paint *paint)
{
   return paint->gradient.color_ramps_premultiplied;
}

void paint_set_ramp_stopsi(struct vg_paint *paint, const VGint *stops,
                           int num)
{
   if (num) {
      free(paint->gradient.ramp_stopsi);
      paint->gradient.ramp_stopsi = malloc(sizeof(VGint)*num);
      memcpy(paint->gradient.ramp_stopsi, stops, sizeof(VGint)*num);
   }
}

void paint_ramp_stopsi(struct vg_paint *paint, VGint *stops,
                       int num)
{
   memcpy(stops, paint->gradient.ramp_stopsi, sizeof(VGint)*num);
}

void paint_set_linear_gradienti(struct vg_paint *paint,
                                const VGint *coords)
{
   memcpy(paint->gradient.linear.coordsi, coords, sizeof(VGint) * 4);
}

void paint_linear_gradienti(struct vg_paint *paint,
                            VGint *coords)
{
   memcpy(coords, paint->gradient.linear.coordsi, sizeof(VGint)*4);
}

void paint_set_radial_gradienti(struct vg_paint *paint,
                                const VGint *values)
{
   memcpy(paint->gradient.radial.valsi, values, sizeof(VGint) * 5);
}

void paint_radial_gradienti(struct vg_paint *paint,
                            VGint *coords)
{
   memcpy(coords, paint->gradient.radial.valsi, sizeof(VGint)*5);
}

VGTilingMode paint_pattern_tiling(struct vg_paint *paint)
{
   return paint->pattern.tiling_mode;
}

VGint paint_bind_samplers(struct vg_paint *paint, struct pipe_sampler_state **samplers,
                          struct pipe_sampler_view **sampler_views)
{
   struct vg_context *ctx = vg_current_context();

   switch(paint->type) {
   case VG_PAINT_TYPE_LINEAR_GRADIENT:
   case VG_PAINT_TYPE_RADIAL_GRADIENT: {
      if (paint->gradient.sampler_view) {
         paint->gradient.sampler.min_img_filter = image_sampler_filter(ctx);
         paint->gradient.sampler.mag_img_filter = image_sampler_filter(ctx);
         samplers[0] = &paint->gradient.sampler;
         sampler_views[0] = paint->gradient.sampler_view;
         return 1;
      }
   }
      break;
   case VG_PAINT_TYPE_PATTERN: {
      memcpy(paint->pattern.sampler.border_color.f,
             ctx->state.vg.tile_fill_color,
             sizeof(VGfloat) * 4);
      paint->pattern.sampler.min_img_filter = image_sampler_filter(ctx);
      paint->pattern.sampler.mag_img_filter = image_sampler_filter(ctx);
      samplers[0] = &paint->pattern.sampler;
      sampler_views[0] = paint->pattern.sampler_view;
      return 1;
   }
      break;
   default:
      break;
   }
   return 0;
}

void paint_resolve_type(struct vg_paint *paint)
{
   if (paint->type == VG_PAINT_TYPE_PATTERN &&
       !paint->pattern.sampler_view) {
      paint->type = VG_PAINT_TYPE_COLOR;
   }
}

VGboolean paint_is_degenerate(struct vg_paint *paint)
{
   VGboolean degen;
   VGfloat *vals;


   switch (paint->type) {
   case VG_PAINT_TYPE_LINEAR_GRADIENT:
      vals = paint->gradient.linear.coords;
      /* two points are coincident */
      degen = (floatsEqual(vals[0], vals[2]) &&
               floatsEqual(vals[1], vals[3]));
      break;
   case VG_PAINT_TYPE_RADIAL_GRADIENT:
      vals = paint->gradient.radial.vals;
      /* radius <= 0 */
      degen = (vals[4] <= 0.0f);
      break;
   case VG_PAINT_TYPE_COLOR:
   case VG_PAINT_TYPE_PATTERN:
   default:
      degen = VG_FALSE;
      break;
   }

   return degen;
}

VGint paint_constant_buffer_size(struct vg_paint *paint)
{
   switch(paint->type) {
   case VG_PAINT_TYPE_COLOR:
      return 8 * sizeof(VGfloat);/*4 color + 4 constants (0.f,1.f,2.f,4.f)*/
      break;
   case VG_PAINT_TYPE_LINEAR_GRADIENT:
      return 20 * sizeof(VGfloat);
      break;
   case VG_PAINT_TYPE_RADIAL_GRADIENT:
      return 20 * sizeof(VGfloat);
      break;
   case VG_PAINT_TYPE_PATTERN:
      return 20 * sizeof(VGfloat);
      break;
   default:
      debug_printf("Uknown paint type: %d\n", paint->type);
   }

   return 0;
}

void paint_fill_constant_buffer(struct vg_paint *paint,
                                const struct matrix *mat,
                                void *buffer)
{
   switch(paint->type) {
   case VG_PAINT_TYPE_COLOR:
      paint_color_buffer(paint, buffer);
      break;
   case VG_PAINT_TYPE_LINEAR_GRADIENT:
      paint_linear_gradient_buffer(paint, mat, buffer);
      break;
   case VG_PAINT_TYPE_RADIAL_GRADIENT:
      paint_radial_gradient_buffer(paint, mat, buffer);
      break;
   case VG_PAINT_TYPE_PATTERN:
      paint_pattern_buffer(paint, mat, buffer);
      break;

   default:
      abort();
   }
}

VGboolean paint_is_opaque(struct vg_paint *paint)
{
   /* TODO add other paint types and make sure PAINT_DIRTY gets set */
   return (paint->type == VG_PAINT_TYPE_COLOR &&
           floatsEqual(paint->solid.color[3], 1.0f));
}
