/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "i915_state_inlines.h"
#include "i915_context.h"
#include "i915_reg.h"
#include "i915_state.h"
#include "i915_resource.h"


/*
 * A note about min_lod & max_lod.
 *
 * There is a circular dependancy between the sampler state
 * and the map state to be submitted to hw.
 *
 * Two condition must be meet:
 * min_lod =< max_lod == true
 * max_lod =< last_level == true
 *
 *
 * This is all fine and dandy if it were for the fact that max_lod
 * is set on the map state instead of the sampler state. That is
 * the max_lod we submit on map is:
 * max_lod = MIN2(last_level, max_lod);
 *
 * So we need to update the map state when we change samplers and
 * we need to change the sampler state when map state is changed.
 * The first part is done by calling update_texture in update_samplers
 * and the second part is done else where in code tracking the state
 * changes.
 */

static void update_map(struct i915_context *i915,
                       uint unit,
                       const struct i915_texture *tex,
                       const struct i915_sampler_state *sampler,
                       const struct pipe_sampler_view* view,
                       uint state[2]);



/***********************************************************************
 * Samplers
 */

/**
 * Compute i915 texture sampling state.
 *
 * Recalculate all state from scratch.  Perhaps not the most
 * efficient, but this has gotten complex enough that we need
 * something which is understandable and reliable.
 * \param state  returns the 3 words of compute state
 */
static void update_sampler(struct i915_context *i915,
                           uint unit,
                           const struct i915_sampler_state *sampler,
                           const struct i915_texture *tex,
                           unsigned state[3])
{
   const struct pipe_resource *pt = &tex->b.b;
   unsigned minlod, lastlod;

   state[0] = sampler->state[0];
   state[1] = sampler->state[1];
   state[2] = sampler->state[2];

   if (pt->format == PIPE_FORMAT_UYVY ||
       pt->format == PIPE_FORMAT_YUYV)
      state[0] |= SS2_COLORSPACE_CONVERSION;

   /* 3D textures don't seem to respect the border color.
    * Fallback if there's ever a danger that they might refer to
    * it.  
    * 
    * Effectively this means fallback on 3D clamp or
    * clamp_to_border.
    *
    * XXX: Check if this is true on i945.  
    * XXX: Check if this bug got fixed in release silicon.
    */
#if 0
   {
      const unsigned ws = sampler->templ->wrap_s;
      const unsigned wt = sampler->templ->wrap_t;
      const unsigned wr = sampler->templ->wrap_r;
      if (pt->target == PIPE_TEXTURE_3D &&
          (sampler->templ->min_img_filter != PIPE_TEX_FILTER_NEAREST ||
           sampler->templ->mag_img_filter != PIPE_TEX_FILTER_NEAREST) &&
          (ws == PIPE_TEX_WRAP_CLAMP ||
           wt == PIPE_TEX_WRAP_CLAMP ||
           wr == PIPE_TEX_WRAP_CLAMP ||
           ws == PIPE_TEX_WRAP_CLAMP_TO_BORDER ||
           wt == PIPE_TEX_WRAP_CLAMP_TO_BORDER || 
           wr == PIPE_TEX_WRAP_CLAMP_TO_BORDER)) {
         if (i915->conformance_mode > 0) {
            assert(0);
            /*             sampler->fallback = true; */
            /* TODO */
         }
      }
   }
#endif

   /* See note at the top of file */
   minlod = sampler->minlod;
   lastlod = pt->last_level << 4;

   if (lastlod < minlod) {
      minlod = lastlod;
   }

   state[1] |= (sampler->minlod << SS3_MIN_LOD_SHIFT);
   state[1] |= (unit << SS3_TEXTUREMAP_INDEX_SHIFT);
}

static void update_samplers(struct i915_context *i915)
{
   uint unit;

   i915->current.sampler_enable_nr = 0;
   i915->current.sampler_enable_flags = 0x0;

   for (unit = 0; unit < i915->num_fragment_sampler_views && unit < i915->num_samplers;
        unit++) {
      /* determine unit enable/disable by looking for a bound texture */
      /* could also examine the fragment program? */
      if (i915->fragment_sampler_views[unit]) {
         struct i915_texture *texture = i915_texture(i915->fragment_sampler_views[unit]->texture);

         update_sampler(i915,
                        unit,
                        i915->sampler[unit],          /* sampler state */
                        texture,                      /* texture */
                        i915->current.sampler[unit]); /* the result */
         update_map(i915,
                    unit,
                    texture,                             /* texture */
                    i915->sampler[unit],                 /* sampler state */
                    i915->fragment_sampler_views[unit],  /* sampler view */
                    i915->current.texbuffer[unit]);      /* the result */

         i915->current.sampler_enable_nr++;
         i915->current.sampler_enable_flags |= (1 << unit);
      }
   }

   i915->hardware_dirty |= I915_HW_SAMPLER | I915_HW_MAP;
}

struct i915_tracked_state i915_hw_samplers = {
   "samplers",
   update_samplers,
   I915_NEW_SAMPLER | I915_NEW_SAMPLER_VIEW
};


/***********************************************************************
 * Sampler views
 */

static uint translate_texture_format(enum pipe_format pipeFormat,
                                     const struct pipe_sampler_view* view)
{
   if ( (view->swizzle_r != PIPE_SWIZZLE_RED ||
         view->swizzle_g != PIPE_SWIZZLE_GREEN ||
         view->swizzle_b != PIPE_SWIZZLE_BLUE ||
         view->swizzle_a != PIPE_SWIZZLE_ALPHA ) &&
         pipeFormat != PIPE_FORMAT_Z24_UNORM_S8_UINT &&
         pipeFormat != PIPE_FORMAT_Z24X8_UNORM )
      debug_printf("i915: unsupported texture swizzle for format %d\n", pipeFormat);

   switch (pipeFormat) {
   case PIPE_FORMAT_L8_UNORM:
      return MAPSURF_8BIT | MT_8BIT_L8;
   case PIPE_FORMAT_I8_UNORM:
      return MAPSURF_8BIT | MT_8BIT_I8;
   case PIPE_FORMAT_A8_UNORM:
      return MAPSURF_8BIT | MT_8BIT_A8;
   case PIPE_FORMAT_L8A8_UNORM:
      return MAPSURF_16BIT | MT_16BIT_AY88;
   case PIPE_FORMAT_B5G6R5_UNORM:
      return MAPSURF_16BIT | MT_16BIT_RGB565;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      return MAPSURF_16BIT | MT_16BIT_ARGB1555;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      return MAPSURF_16BIT | MT_16BIT_ARGB4444;
   case PIPE_FORMAT_B10G10R10A2_UNORM:
      return MAPSURF_32BIT | MT_32BIT_ARGB2101010;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_SRGB:
      return MAPSURF_32BIT | MT_32BIT_ARGB8888;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      return MAPSURF_32BIT | MT_32BIT_XRGB8888;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      return MAPSURF_32BIT | MT_32BIT_ABGR8888;
   case PIPE_FORMAT_R8G8B8X8_UNORM:
      return MAPSURF_32BIT | MT_32BIT_XBGR8888;
   case PIPE_FORMAT_YUYV:
      return (MAPSURF_422 | MT_422_YCRCB_NORMAL);
   case PIPE_FORMAT_UYVY:
      return (MAPSURF_422 | MT_422_YCRCB_SWAPY);
#if 0
   case PIPE_FORMAT_RGB_FXT1:
   case PIPE_FORMAT_RGBA_FXT1:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_FXT1);
#endif
   case PIPE_FORMAT_Z16_UNORM:
      return (MAPSURF_16BIT | MT_16BIT_L16);
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT1_RGB:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT1);
   case PIPE_FORMAT_DXT3_RGBA:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT2_3);
   case PIPE_FORMAT_DXT5_RGBA:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT4_5);
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
      {
         if ( view->swizzle_r == PIPE_SWIZZLE_RED &&
              view->swizzle_g == PIPE_SWIZZLE_RED &&
              view->swizzle_b == PIPE_SWIZZLE_RED &&
              view->swizzle_a == PIPE_SWIZZLE_ONE)
            return (MAPSURF_32BIT | MT_32BIT_xA824);
         if ( view->swizzle_r == PIPE_SWIZZLE_RED &&
              view->swizzle_g == PIPE_SWIZZLE_RED &&
              view->swizzle_b == PIPE_SWIZZLE_RED &&
              view->swizzle_a == PIPE_SWIZZLE_RED)
            return (MAPSURF_32BIT | MT_32BIT_xI824);
         if ( view->swizzle_r == PIPE_SWIZZLE_ZERO &&
              view->swizzle_g == PIPE_SWIZZLE_ZERO &&
              view->swizzle_b == PIPE_SWIZZLE_ZERO &&
              view->swizzle_a == PIPE_SWIZZLE_RED)
            return (MAPSURF_32BIT | MT_32BIT_xL824);
         debug_printf("i915: unsupported depth swizzle %d %d %d %d\n",
                      view->swizzle_r,
                      view->swizzle_g,
                      view->swizzle_b,
                      view->swizzle_a);
         return (MAPSURF_32BIT | MT_32BIT_xL824);
      }
   default:
      debug_printf("i915: translate_texture_format() bad image format %x\n",
                   pipeFormat);
      assert(0);
      return 0;
   }
}

static inline uint32_t
ms3_tiling_bits(enum i915_winsys_buffer_tile tiling)
{
         uint32_t tiling_bits = 0;

         switch (tiling) {
         case I915_TILE_Y:
            tiling_bits |= MS3_TILE_WALK_Y;
         case I915_TILE_X:
            tiling_bits |= MS3_TILED_SURFACE;
         case I915_TILE_NONE:
            break;
         }

         return tiling_bits;
}

static void update_map(struct i915_context *i915,
                       uint unit,
                       const struct i915_texture *tex,
                       const struct i915_sampler_state *sampler,
                       const struct pipe_sampler_view* view,
                       uint state[2])
{
   const struct pipe_resource *pt = &tex->b.b;
   uint format, pitch;
   const uint width = pt->width0, height = pt->height0, depth = pt->depth0;
   const uint num_levels = pt->last_level;
   unsigned max_lod = num_levels * 4;

   assert(tex);
   assert(width);
   assert(height);
   assert(depth);

   format = translate_texture_format(pt->format, view);
   i915->current.sampler_srgb[unit] = ( pt->format == PIPE_FORMAT_B8G8R8A8_SRGB ||
                                        pt->format == PIPE_FORMAT_L8_SRGB );
   pitch = tex->stride;

   assert(format);
   assert(pitch);

   /* MS3 state */
   state[0] =
      (((height - 1) << MS3_HEIGHT_SHIFT)
       | ((width - 1) << MS3_WIDTH_SHIFT)
       | format
       | ms3_tiling_bits(tex->tiling));

   /*
    * XXX When min_filter != mag_filter and there's just one mipmap level,
    * set max_lod = 1 to make sure i915 chooses between min/mag filtering.
    */

   /* See note at the top of file */
   if (max_lod > (sampler->maxlod >> 2))
      max_lod = sampler->maxlod >> 2;

   /* MS4 state */
   state[1] =
      ((((pitch / 4) - 1) << MS4_PITCH_SHIFT)
       | MS4_CUBE_FACE_ENA_MASK
       | ((max_lod) << MS4_MAX_LOD_SHIFT)
       | ((depth - 1) << MS4_VOLUME_DEPTH_SHIFT));
}

static void update_maps(struct i915_context *i915)
{
   uint unit;

   for (unit = 0; unit < i915->num_fragment_sampler_views && unit < i915->num_samplers;
        unit++) {
      /* determine unit enable/disable by looking for a bound texture */
      /* could also examine the fragment program? */
      if (i915->fragment_sampler_views[unit]) {
         struct i915_texture *texture = i915_texture(i915->fragment_sampler_views[unit]->texture);

         update_map(i915,
                    unit,
                    texture,                            /* texture */
                    i915->sampler[unit],                /* sampler state */
                    i915->fragment_sampler_views[unit], /* sampler view */
                    i915->current.texbuffer[unit]);
      }
   }

   i915->hardware_dirty |= I915_HW_MAP;
}

struct i915_tracked_state i915_hw_sampler_views = {
   "sampler_views",
   update_maps,
   I915_NEW_SAMPLER_VIEW
};
