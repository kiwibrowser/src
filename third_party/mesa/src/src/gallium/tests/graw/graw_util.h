
#include "state_tracker/graw.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"

#include "util/u_box.h"    
#include "util/u_debug.h"
#include "util/u_draw_quad.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"


struct graw_info
{
   struct pipe_screen *screen;
   struct pipe_context *ctx;
   struct pipe_resource *color_buf[PIPE_MAX_COLOR_BUFS], *zs_buf;
   struct pipe_surface *color_surf[PIPE_MAX_COLOR_BUFS], *zs_surf;
   void *window;
};



static INLINE boolean
graw_util_create_window(struct graw_info *info,
                        int width, int height,
                        int num_cbufs, bool zstencil_buf)
{
   static const enum pipe_format formats[] = {
      PIPE_FORMAT_R8G8B8A8_UNORM,
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_NONE
   };
   enum pipe_format format;
   struct pipe_resource resource_temp;
   struct pipe_surface surface_temp;
   int i;

   memset(info, 0, sizeof(*info));

   /* It's hard to say whether window or screen should be created
    * first.  Different environments would prefer one or the other.
    *
    * Also, no easy way of querying supported formats if the screen
    * cannot be created first.
    */
   for (i = 0; info->window == NULL && formats[i] != PIPE_FORMAT_NONE; i++) {
      info->screen = graw_create_window_and_screen(0, 0, width, height,
                                                   formats[i],
                                                   &info->window);
      format = formats[i];
   }
   if (!info->screen || !info->window) {
      debug_printf("graw: Failed to create screen/window\n");
      return FALSE;
   }
   
   info->ctx = info->screen->context_create(info->screen, NULL);
   if (info->ctx == NULL) {
      debug_printf("graw: Failed to create context\n");
      return FALSE;
   }

   for (i = 0; i < num_cbufs; i++) {
      /* create color texture */
      resource_temp.target = PIPE_TEXTURE_2D;
      resource_temp.format = format;
      resource_temp.width0 = width;
      resource_temp.height0 = height;
      resource_temp.depth0 = 1;
      resource_temp.array_size = 1;
      resource_temp.last_level = 0;
      resource_temp.nr_samples = 1;
      resource_temp.bind = (PIPE_BIND_RENDER_TARGET |
                            PIPE_BIND_DISPLAY_TARGET);
      info->color_buf[i] = info->screen->resource_create(info->screen,
                                                         &resource_temp);
      if (info->color_buf[i] == NULL) {
         debug_printf("graw: Failed to create color texture\n");
         return FALSE;
      }

      /* create color surface */
      surface_temp.format = resource_temp.format;
      surface_temp.usage = PIPE_BIND_RENDER_TARGET;
      surface_temp.u.tex.level = 0;
      surface_temp.u.tex.first_layer = 0;
      surface_temp.u.tex.last_layer = 0;
      info->color_surf[i] = info->ctx->create_surface(info->ctx,
                                                      info->color_buf[i],
                                                      &surface_temp);
      if (info->color_surf[i] == NULL) {
         debug_printf("graw: Failed to get color surface\n");
         return FALSE;
      }
   }

   /* create Z texture (XXX try other Z/S formats if needed) */
   resource_temp.target = PIPE_TEXTURE_2D;
   resource_temp.format = PIPE_FORMAT_S8_UINT_Z24_UNORM;
   resource_temp.width0 = width;
   resource_temp.height0 = height;
   resource_temp.depth0 = 1;
   resource_temp.array_size = 1;
   resource_temp.last_level = 0;
   resource_temp.nr_samples = 1;
   resource_temp.bind = PIPE_BIND_DEPTH_STENCIL;
   info->zs_buf = info->screen->resource_create(info->screen, &resource_temp);
   if (!info->zs_buf) {
      debug_printf("graw: Failed to create Z texture\n");
      return FALSE;
   }

   /* create z surface */
   surface_temp.format = resource_temp.format;
   surface_temp.usage = PIPE_BIND_DEPTH_STENCIL;
   surface_temp.u.tex.level = 0;
   surface_temp.u.tex.first_layer = 0;
   surface_temp.u.tex.last_layer = 0;
   info->zs_surf = info->ctx->create_surface(info->ctx,
                                             info->zs_buf,
                                             &surface_temp);
   if (info->zs_surf == NULL) {
      debug_printf("graw: Failed to get Z surface\n");
      return FALSE;
   }

   {
      struct pipe_framebuffer_state fb;
      memset(&fb, 0, sizeof fb);
      fb.nr_cbufs = num_cbufs;
      fb.width = width;
      fb.height = height;
      for (i = 0; i < num_cbufs; i++)
         fb.cbufs[i] = info->color_surf[i];
      fb.zsbuf = info->zs_surf;
      info->ctx->set_framebuffer_state(info->ctx, &fb);
   }

   return TRUE;
}


static INLINE void
graw_util_default_state(struct graw_info *info, boolean depth_test)
{
   {
      struct pipe_blend_state blend;
      void *handle;
      memset(&blend, 0, sizeof blend);
      blend.rt[0].colormask = PIPE_MASK_RGBA;
      handle = info->ctx->create_blend_state(info->ctx, &blend);
      info->ctx->bind_blend_state(info->ctx, handle);
   }

   {
      struct pipe_depth_stencil_alpha_state depthStencilAlpha;
      void *handle;
      memset(&depthStencilAlpha, 0, sizeof depthStencilAlpha);
      depthStencilAlpha.depth.enabled = depth_test;
      depthStencilAlpha.depth.writemask = 1;
      depthStencilAlpha.depth.func = PIPE_FUNC_LESS;
      handle = info->ctx->create_depth_stencil_alpha_state(info->ctx,
                                                           &depthStencilAlpha);
      info->ctx->bind_depth_stencil_alpha_state(info->ctx, handle);
   }

   {
      struct pipe_rasterizer_state rasterizer;
      void *handle;
      memset(&rasterizer, 0, sizeof rasterizer);
      rasterizer.cull_face = PIPE_FACE_NONE;
      rasterizer.gl_rasterization_rules = 1;
      handle = info->ctx->create_rasterizer_state(info->ctx, &rasterizer);
      info->ctx->bind_rasterizer_state(info->ctx, handle);
   }
}


static INLINE void
graw_util_viewport(struct graw_info *info,
                   float x, float y,
                   float width, float height,
                   float near, float far)
{
   float z = near;
   float half_width = width / 2.0f;
   float half_height = height / 2.0f;
   float half_depth = (far - near) / 2.0f;
   struct pipe_viewport_state vp;

   vp.scale[0] = half_width;
   vp.scale[1] = half_height;
   vp.scale[2] = half_depth;
   vp.scale[3] = 1.0f;

   vp.translate[0] = half_width + x;
   vp.translate[1] = half_height + y;
   vp.translate[2] = half_depth + z;
   vp.translate[3] = 0.0f;

   info->ctx->set_viewport_state(info->ctx, &vp);
}


static INLINE void
graw_util_flush_front(const struct graw_info *info)
{
   info->screen->flush_frontbuffer(info->screen, info->color_buf[0],
                                   0, 0, info->window);
}


static INLINE struct pipe_resource *
graw_util_create_tex2d(const struct graw_info *info,
                       int width, int height, enum pipe_format format,
                       const void *data)
{ 
   const int row_stride = width * util_format_get_blocksize(format);
   const int image_bytes = row_stride * height;
   struct pipe_resource temp, *tex;
   struct pipe_box box;

   temp.target = PIPE_TEXTURE_2D;
   temp.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   temp.width0 = width;
   temp.height0 = height;
   temp.depth0 = 1;
   temp.last_level = 0;
   temp.array_size = 1;
   temp.nr_samples = 1;
   temp.bind = PIPE_BIND_SAMPLER_VIEW;
   
   tex = info->screen->resource_create(info->screen, &temp);
   if (!tex) {
      debug_printf("graw: failed to create texture\n");
      return NULL;
   }

   u_box_2d(0, 0, width, height, &box);

   info->ctx->transfer_inline_write(info->ctx,
                                    tex,
                                    0,
                                    PIPE_TRANSFER_WRITE,
                                    &box,
                                    data,
                                    row_stride,
                                    image_bytes);

   /* Possibly read back & compare against original data:
    */
#if 0
   {
      struct pipe_transfer *t;
      uint32_t *ptr;
      t = pipe_get_transfer(info->ctx, samptex,
                            0, 0, /* level, layer */
                            PIPE_TRANSFER_READ,
                            0, 0, SIZE, SIZE); /* x, y, width, height */

      ptr = info->ctx->transfer_map(info->ctx, t);

      if (memcmp(ptr, tex2d, sizeof tex2d) != 0) {
         assert(0);
         exit(9);
      }

      info->ctx->transfer_unmap(info->ctx, t);

      info->ctx->transfer_destroy(info->ctx, t);
   }
#endif

   return tex;
}


static INLINE void *
graw_util_create_simple_sampler(const struct graw_info *info,
                                unsigned wrap_mode,
                                unsigned img_filter)
{
   struct pipe_sampler_state sampler_desc;
   void *sampler;

   memset(&sampler_desc, 0, sizeof sampler_desc);
   sampler_desc.wrap_s =
   sampler_desc.wrap_t =
   sampler_desc.wrap_r = wrap_mode;
   sampler_desc.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler_desc.min_img_filter =
   sampler_desc.mag_img_filter = img_filter;
   sampler_desc.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler_desc.compare_func = 0;
   sampler_desc.normalized_coords = 1;
   sampler_desc.max_anisotropy = 0;
   
   sampler = info->ctx->create_sampler_state(info->ctx, &sampler_desc);

   return sampler;
}


static INLINE struct pipe_sampler_view *
graw_util_create_simple_sampler_view(const struct graw_info *info,
                                     struct pipe_resource *texture)
{
   struct pipe_sampler_view sv_temp;
   struct pipe_sampler_view *sv;

   memset(&sv_temp, 0, sizeof(sv_temp));
   sv_temp.format = texture->format;
   sv_temp.texture = texture;
   sv_temp.swizzle_r = PIPE_SWIZZLE_RED;
   sv_temp.swizzle_g = PIPE_SWIZZLE_GREEN;
   sv_temp.swizzle_b = PIPE_SWIZZLE_BLUE;
   sv_temp.swizzle_a = PIPE_SWIZZLE_ALPHA;

   sv = info->ctx->create_sampler_view(info->ctx, texture, &sv_temp);

   return sv;
}

