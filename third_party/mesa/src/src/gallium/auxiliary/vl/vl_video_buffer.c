/**************************************************************************
 *
 * Copyright 2011 Christian König.
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

#include <assert.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_sampler.h"
#include "util/u_memory.h"

#include "vl_video_buffer.h"

const enum pipe_format const_resource_formats_YV12[3] = {
   PIPE_FORMAT_R8_UNORM,
   PIPE_FORMAT_R8_UNORM,
   PIPE_FORMAT_R8_UNORM
};

const enum pipe_format const_resource_formats_NV12[3] = {
   PIPE_FORMAT_R8_UNORM,
   PIPE_FORMAT_R8G8_UNORM,
   PIPE_FORMAT_NONE
};

const enum pipe_format const_resource_formats_YUVA[3] = {
   PIPE_FORMAT_R8G8B8A8_UNORM,
   PIPE_FORMAT_NONE,
   PIPE_FORMAT_NONE
};

const enum pipe_format const_resource_formats_VUYA[3] = {
   PIPE_FORMAT_B8G8R8A8_UNORM,
   PIPE_FORMAT_NONE,
   PIPE_FORMAT_NONE
};

const enum pipe_format const_resource_formats_YUYV[3] = {
   PIPE_FORMAT_R8G8_R8B8_UNORM,
   PIPE_FORMAT_NONE,
   PIPE_FORMAT_NONE
};

const enum pipe_format const_resource_formats_UYVY[3] = {
   PIPE_FORMAT_G8R8_B8R8_UNORM,
   PIPE_FORMAT_NONE,
   PIPE_FORMAT_NONE
};

const unsigned const_resource_plane_order_YUV[3] = {
   0,
   1,
   2
};

const unsigned const_resource_plane_order_YVU[3] = {
   0,
   2,
   1
};

const enum pipe_format *
vl_video_buffer_formats(struct pipe_screen *screen, enum pipe_format format)
{
   switch(format) {
   case PIPE_FORMAT_YV12:
      return const_resource_formats_YV12;

   case PIPE_FORMAT_NV12:
      return const_resource_formats_NV12;

   case PIPE_FORMAT_R8G8B8A8_UNORM:
      return const_resource_formats_YUVA;

   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return const_resource_formats_VUYA;

   case PIPE_FORMAT_YUYV:
      return const_resource_formats_YUYV;

   case PIPE_FORMAT_UYVY:
      return const_resource_formats_UYVY;

   default:
      return NULL;
   }
}

const unsigned *
vl_video_buffer_plane_order(enum pipe_format format)
{
   switch(format) {
   case PIPE_FORMAT_YV12:
      return const_resource_plane_order_YVU;

   case PIPE_FORMAT_NV12:
   case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_YUYV:
   case PIPE_FORMAT_UYVY:
      return const_resource_plane_order_YUV;

   default:
      return NULL;
   }
}

static enum pipe_format
vl_video_buffer_surface_format(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   /* a subsampled formats can't work as surface use RGBA instead */
   if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED)
      return PIPE_FORMAT_R8G8B8A8_UNORM;

   return format;
}

boolean
vl_video_buffer_is_format_supported(struct pipe_screen *screen,
                                    enum pipe_format format,
                                    enum pipe_video_profile profile)
{
   const enum pipe_format *resource_formats;
   unsigned i;

   resource_formats = vl_video_buffer_formats(screen, format);
   if (!resource_formats)
      return false;

   for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
      enum pipe_format format = resource_formats[i];

      if (format == PIPE_FORMAT_NONE)
         continue;

      /* we at least need to sample from it */
      if (!screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_SAMPLER_VIEW))
         return false;

      format = vl_video_buffer_surface_format(format);
      if (!screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET))
         return false;
   }

   return true;
}

unsigned
vl_video_buffer_max_size(struct pipe_screen *screen)
{
   uint32_t max_2d_texture_level;

   max_2d_texture_level = screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS);

   return 1 << (max_2d_texture_level-1);
}

void
vl_video_buffer_set_associated_data(struct pipe_video_buffer *vbuf,
                                    struct pipe_video_decoder *vdec,
                                    void *associated_data,
                                    void (*destroy_associated_data)(void *))
{
   vbuf->decoder = vdec;

   if (vbuf->associated_data == associated_data)
      return;

   if (vbuf->associated_data)
      vbuf->destroy_associated_data(vbuf->associated_data);

   vbuf->associated_data = associated_data;
   vbuf->destroy_associated_data = destroy_associated_data;
}

void *
vl_video_buffer_get_associated_data(struct pipe_video_buffer *vbuf,
                                    struct pipe_video_decoder *vdec)
{
   if (vbuf->decoder == vdec)
      return vbuf->associated_data;
   else
      return NULL;
}

void
vl_vide_buffer_template(struct pipe_resource *templ,
                        const struct pipe_video_buffer *tmpl,
                        enum pipe_format resource_format,
                        unsigned depth, unsigned usage, unsigned plane)
{
   memset(templ, 0, sizeof(*templ));
   templ->target = depth > 1 ? PIPE_TEXTURE_3D : PIPE_TEXTURE_2D;
   templ->format = resource_format;
   templ->width0 = tmpl->width;
   templ->height0 = tmpl->height;
   templ->depth0 = depth;
   templ->array_size = 1;
   templ->bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   templ->usage = usage;

   if (plane > 0) {
      if (tmpl->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
         templ->width0 /= 2;
         templ->height0 /= 2;
      } else if (tmpl->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422) {
         templ->height0 /= 2;
      }
   }
}

static void
vl_video_buffer_destroy(struct pipe_video_buffer *buffer)
{
   struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   unsigned i;

   assert(buf);

   for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
      pipe_sampler_view_reference(&buf->sampler_view_planes[i], NULL);
      pipe_sampler_view_reference(&buf->sampler_view_components[i], NULL);
      pipe_resource_reference(&buf->resources[i], NULL);
   }

   for (i = 0; i < VL_NUM_COMPONENTS * 2; ++i)
      pipe_surface_reference(&buf->surfaces[i], NULL);

   vl_video_buffer_set_associated_data(buffer, NULL, NULL, NULL);

   FREE(buffer);
}

static struct pipe_sampler_view **
vl_video_buffer_sampler_view_planes(struct pipe_video_buffer *buffer)
{
   struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   struct pipe_sampler_view sv_templ;
   struct pipe_context *pipe;
   unsigned i;

   assert(buf);

   pipe = buf->base.context;

   for (i = 0; i < buf->num_planes; ++i ) {
      if (!buf->sampler_view_planes[i]) {
         memset(&sv_templ, 0, sizeof(sv_templ));
         u_sampler_view_default_template(&sv_templ, buf->resources[i], buf->resources[i]->format);

         if (util_format_get_nr_components(buf->resources[i]->format) == 1)
            sv_templ.swizzle_r = sv_templ.swizzle_g = sv_templ.swizzle_b = sv_templ.swizzle_a = PIPE_SWIZZLE_RED;

         buf->sampler_view_planes[i] = pipe->create_sampler_view(pipe, buf->resources[i], &sv_templ);
         if (!buf->sampler_view_planes[i])
            goto error;
      }
   }

   return buf->sampler_view_planes;

error:
   for (i = 0; i < buf->num_planes; ++i )
      pipe_sampler_view_reference(&buf->sampler_view_planes[i], NULL);

   return NULL;
}

static struct pipe_sampler_view **
vl_video_buffer_sampler_view_components(struct pipe_video_buffer *buffer)
{
   struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   struct pipe_sampler_view sv_templ;
   struct pipe_context *pipe;
   const enum pipe_format *sampler_format;
   const unsigned *plane_order;
   unsigned i, j, component;

   assert(buf);

   pipe = buf->base.context;

   sampler_format = vl_video_buffer_formats(pipe->screen, buf->base.buffer_format);
   plane_order = vl_video_buffer_plane_order(buf->base.buffer_format);

   for (component = 0, i = 0; i < buf->num_planes; ++i ) {
      struct pipe_resource *res = buf->resources[plane_order[i]];
      const struct util_format_description *desc = util_format_description(res->format);
      unsigned nr_components = util_format_get_nr_components(res->format);
      if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED)
         nr_components = 3;

      for (j = 0; j < nr_components && component < VL_NUM_COMPONENTS; ++j, ++component) {
         if (buf->sampler_view_components[component])
            continue;

         memset(&sv_templ, 0, sizeof(sv_templ));
         u_sampler_view_default_template(&sv_templ, res, sampler_format[plane_order[i]]);
         sv_templ.swizzle_r = sv_templ.swizzle_g = sv_templ.swizzle_b = PIPE_SWIZZLE_RED + j;
         sv_templ.swizzle_a = PIPE_SWIZZLE_ONE;
         buf->sampler_view_components[component] = pipe->create_sampler_view(pipe, res, &sv_templ);
         if (!buf->sampler_view_components[component])
            goto error;
      }
   }
   assert(component == VL_NUM_COMPONENTS);

   return buf->sampler_view_components;

error:
   for (i = 0; i < VL_NUM_COMPONENTS; ++i )
      pipe_sampler_view_reference(&buf->sampler_view_components[i], NULL);

   return NULL;
}

static struct pipe_surface **
vl_video_buffer_surfaces(struct pipe_video_buffer *buffer)
{
   struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   struct pipe_surface surf_templ;
   struct pipe_context *pipe;
   unsigned i, j, depth, surf;

   assert(buf);

   pipe = buf->base.context;

   depth = buffer->interlaced ? 2 : 1;
   for (i = 0, surf = 0; i < depth; ++i ) {
      for (j = 0; j < VL_NUM_COMPONENTS; ++j, ++surf) {
         assert(surf < (VL_NUM_COMPONENTS * 2));

         if (!buf->resources[j]) {
            pipe_surface_reference(&buf->surfaces[surf], NULL);
            continue;
         }

         if (!buf->surfaces[surf]) {
            memset(&surf_templ, 0, sizeof(surf_templ));
            surf_templ.format = vl_video_buffer_surface_format(buf->resources[j]->format);
            surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
            surf_templ.u.tex.first_layer = surf_templ.u.tex.last_layer = i;
            buf->surfaces[surf] = pipe->create_surface(pipe, buf->resources[j], &surf_templ);
            if (!buf->surfaces[surf])
               goto error;
         }
      }
   }

   return buf->surfaces;

error:
   for (i = 0; i < (VL_NUM_COMPONENTS * 2); ++i )
      pipe_surface_reference(&buf->surfaces[i], NULL);

   return NULL;
}

struct pipe_video_buffer *
vl_video_buffer_create(struct pipe_context *pipe,
                       const struct pipe_video_buffer *tmpl)
{
   const enum pipe_format *resource_formats;
   struct pipe_video_buffer templat, *result;
   bool pot_buffers;

   assert(pipe);
   assert(tmpl->width > 0 && tmpl->height > 0);

   pot_buffers = !pipe->screen->get_video_param
   (
      pipe->screen,
      PIPE_VIDEO_PROFILE_UNKNOWN,
      PIPE_VIDEO_CAP_NPOT_TEXTURES
   );

   resource_formats = vl_video_buffer_formats(pipe->screen, tmpl->buffer_format);
   if (!resource_formats)
      return NULL;

   templat = *tmpl;
   templat.width = pot_buffers ? util_next_power_of_two(tmpl->width)
                 : align(tmpl->width, VL_MACROBLOCK_WIDTH);
   templat.height = pot_buffers ? util_next_power_of_two(tmpl->height)
                  : align(tmpl->height, VL_MACROBLOCK_HEIGHT);

   if (tmpl->interlaced)
      templat.height /= 2;

   result = vl_video_buffer_create_ex
   (
      pipe, &templat, resource_formats,
      tmpl->interlaced ? 2 : 1, PIPE_USAGE_STATIC
   );


   if (result && tmpl->interlaced)
      result->height *= 2;

   return result;
}

struct pipe_video_buffer *
vl_video_buffer_create_ex(struct pipe_context *pipe,
                          const struct pipe_video_buffer *tmpl,
                          const enum pipe_format resource_formats[VL_NUM_COMPONENTS],
                          unsigned depth, unsigned usage)
{
   struct pipe_resource res_tmpl;
   struct pipe_resource *resources[VL_NUM_COMPONENTS];
   unsigned i;

   assert(pipe);

   memset(resources, 0, sizeof resources);

   vl_vide_buffer_template(&res_tmpl, tmpl, resource_formats[0], depth, usage, 0);
   resources[0] = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   if (!resources[0])
      goto error;

   if (resource_formats[1] == PIPE_FORMAT_NONE) {
      assert(resource_formats[2] == PIPE_FORMAT_NONE);
      return vl_video_buffer_create_ex2(pipe, tmpl, resources);
   }

   vl_vide_buffer_template(&res_tmpl, tmpl, resource_formats[1], depth, usage, 1);
   resources[1] = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   if (!resources[1])
      goto error;

   if (resource_formats[2] == PIPE_FORMAT_NONE)
      return vl_video_buffer_create_ex2(pipe, tmpl, resources);

   vl_vide_buffer_template(&res_tmpl, tmpl, resource_formats[2], depth, usage, 2);
   resources[2] = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   if (!resources[2])
      goto error;

   return vl_video_buffer_create_ex2(pipe, tmpl, resources);

error:
   for (i = 0; i < VL_NUM_COMPONENTS; ++i)
      pipe_resource_reference(&resources[i], NULL);

   return NULL;
}

struct pipe_video_buffer *
vl_video_buffer_create_ex2(struct pipe_context *pipe,
                           const struct pipe_video_buffer *tmpl,
                           struct pipe_resource *resources[VL_NUM_COMPONENTS])
{
   struct vl_video_buffer *buffer;
   unsigned i;

   buffer = CALLOC_STRUCT(vl_video_buffer);

   buffer->base = *tmpl;
   buffer->base.context = pipe;
   buffer->base.destroy = vl_video_buffer_destroy;
   buffer->base.get_sampler_view_planes = vl_video_buffer_sampler_view_planes;
   buffer->base.get_sampler_view_components = vl_video_buffer_sampler_view_components;
   buffer->base.get_surfaces = vl_video_buffer_surfaces;
   buffer->num_planes = 0;

   for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
      buffer->resources[i] = resources[i];
      if (resources[i])
         buffer->num_planes++;
   }

   return &buffer->base;
}
