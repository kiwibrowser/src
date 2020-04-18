/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
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

#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_debug.h"
#include "util/u_rect.h"
#include "vl/vl_defines.h"

#include "vdpau_private.h"

/**
 * Create a VdpVideoSurface.
 */
VdpStatus
vlVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type,
                        uint32_t width, uint32_t height,
                        VdpVideoSurface *surface)
{
   struct pipe_context *pipe;
   vlVdpSurface *p_surf;
   VdpStatus ret;

   if (!(width && height)) {
      ret = VDP_STATUS_INVALID_SIZE;
      goto inv_size;
   }

   if (!vlCreateHTAB()) {
      ret = VDP_STATUS_RESOURCES;
      goto no_htab;
   }

   p_surf = CALLOC(1, sizeof(vlVdpSurface));
   if (!p_surf) {
      ret = VDP_STATUS_RESOURCES;
      goto no_res;
   }

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev) {
      ret = VDP_STATUS_INVALID_HANDLE;
      goto inv_device;
   }

   p_surf->device = dev;
   pipe = dev->context;

   pipe_mutex_lock(dev->mutex);
   memset(&p_surf->templat, 0, sizeof(p_surf->templat));
   p_surf->templat.buffer_format = pipe->screen->get_video_param
   (
      pipe->screen,
      PIPE_VIDEO_PROFILE_UNKNOWN,
      PIPE_VIDEO_CAP_PREFERED_FORMAT
   );
   p_surf->templat.chroma_format = ChromaToPipe(chroma_type);
   p_surf->templat.width = width;
   p_surf->templat.height = height;
   p_surf->templat.interlaced = pipe->screen->get_video_param
   (
      pipe->screen,
      PIPE_VIDEO_PROFILE_UNKNOWN,
      PIPE_VIDEO_CAP_PREFERS_INTERLACED
   );
   p_surf->video_buffer = pipe->create_video_buffer(pipe, &p_surf->templat);
   vlVdpVideoSurfaceClear(p_surf);
   pipe_mutex_unlock(dev->mutex);

   *surface = vlAddDataHTAB(p_surf);
   if (*surface == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
   }

   return VDP_STATUS_OK;

no_handle:
   p_surf->video_buffer->destroy(p_surf->video_buffer);

inv_device:
   FREE(p_surf);

no_res:
no_htab:
inv_size:
   return ret;
}

/**
 * Destroy a VdpVideoSurface.
 */
VdpStatus
vlVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
   vlVdpSurface *p_surf;

   p_surf = (vlVdpSurface *)vlGetDataHTAB((vlHandle)surface);
   if (!p_surf)
      return VDP_STATUS_INVALID_HANDLE;

   pipe_mutex_lock(p_surf->device->mutex);
   if (p_surf->video_buffer)
      p_surf->video_buffer->destroy(p_surf->video_buffer);
   pipe_mutex_unlock(p_surf->device->mutex);

   FREE(p_surf);
   return VDP_STATUS_OK;
}

/**
 * Retrieve the parameters used to create a VdpVideoSurface.
 */
VdpStatus
vlVdpVideoSurfaceGetParameters(VdpVideoSurface surface,
                               VdpChromaType *chroma_type,
                               uint32_t *width, uint32_t *height)
{
   if (!(width && height && chroma_type))
      return VDP_STATUS_INVALID_POINTER;

   vlVdpSurface *p_surf = vlGetDataHTAB(surface);
   if (!p_surf)
      return VDP_STATUS_INVALID_HANDLE;

   if (p_surf->video_buffer) {
      *width = p_surf->video_buffer->width;
      *height = p_surf->video_buffer->height;
      *chroma_type = PipeToChroma(p_surf->video_buffer->chroma_format);
   } else {
      *width = p_surf->templat.width;
      *height = p_surf->templat.height;
      *chroma_type = PipeToChroma(p_surf->templat.chroma_format);
   }

   return VDP_STATUS_OK;
}

static void
vlVdpVideoSurfaceSize(vlVdpSurface *p_surf, int component,
                      unsigned *width, unsigned *height)
{
   *width = p_surf->templat.width;
   *height = p_surf->templat.height;

   if (component > 0) {
      if (p_surf->templat.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
         *width /= 2;
         *height /= 2;
      } else if (p_surf->templat.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422) {
         *height /= 2;
      }
      if (p_surf->templat.interlaced)
         *height /= 2;
   }
}

/**
 * Copy image data from a VdpVideoSurface to application memory in a specified
 * YCbCr format.
 */
VdpStatus
vlVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface,
                              VdpYCbCrFormat destination_ycbcr_format,
                              void *const *destination_data,
                              uint32_t const *destination_pitches)
{
   vlVdpSurface *vlsurface;
   struct pipe_context *pipe;
   enum pipe_format format;
   struct pipe_sampler_view **sampler_views;
   unsigned i, j;

   vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
      return VDP_STATUS_INVALID_HANDLE;

   pipe = vlsurface->device->context;
   if (!pipe)
      return VDP_STATUS_INVALID_HANDLE;

   format = FormatYCBCRToPipe(destination_ycbcr_format);
   if (format == PIPE_FORMAT_NONE)
       return VDP_STATUS_INVALID_Y_CB_CR_FORMAT;

   if (vlsurface->video_buffer == NULL || format != vlsurface->video_buffer->buffer_format)
      return VDP_STATUS_NO_IMPLEMENTATION; /* TODO We don't support conversion (yet) */

   pipe_mutex_lock(vlsurface->device->mutex);
   sampler_views = vlsurface->video_buffer->get_sampler_view_planes(vlsurface->video_buffer);
   if (!sampler_views) {
      pipe_mutex_unlock(vlsurface->device->mutex);
      return VDP_STATUS_RESOURCES;
   }

   for (i = 0; i < 3; ++i) {
      unsigned width, height;
      struct pipe_sampler_view *sv = sampler_views[i];
      if (!sv) continue;

      vlVdpVideoSurfaceSize(vlsurface, i, &width, &height);

      for (j = 0; j < sv->texture->depth0; ++j) {
         struct pipe_box box = {
            0, 0, j,
            width, height, 1
         };
         struct pipe_transfer *transfer;
         uint8_t *map;

         transfer = pipe->get_transfer(pipe, sv->texture, 0, PIPE_TRANSFER_READ, &box);
         if (transfer == NULL) {
            pipe_mutex_unlock(vlsurface->device->mutex);
            return VDP_STATUS_RESOURCES;
         }

         map = pipe_transfer_map(pipe, transfer);
         if (map == NULL) {
            pipe_transfer_destroy(pipe, transfer);
            pipe_mutex_unlock(vlsurface->device->mutex);
            return VDP_STATUS_RESOURCES;
         }

         util_copy_rect(destination_data[i] + destination_pitches[i] * j, sv->texture->format,
                        destination_pitches[i] * sv->texture->depth0, 0, 0,
                        box.width, box.height, map, transfer->stride, 0, 0);

         pipe_transfer_unmap(pipe, transfer);
         pipe_transfer_destroy(pipe, transfer);
      }
   }
   pipe_mutex_unlock(vlsurface->device->mutex);

   return VDP_STATUS_OK;
}

/**
 * Copy image data from application memory in a specific YCbCr format to
 * a VdpVideoSurface.
 */
VdpStatus
vlVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface,
                              VdpYCbCrFormat source_ycbcr_format,
                              void const *const *source_data,
                              uint32_t const *source_pitches)
{
   enum pipe_format pformat = FormatYCBCRToPipe(source_ycbcr_format);
   struct pipe_context *pipe;
   struct pipe_sampler_view **sampler_views;
   unsigned i, j;

   if (!vlCreateHTAB())
      return VDP_STATUS_RESOURCES;

   vlVdpSurface *p_surf = vlGetDataHTAB(surface);
   if (!p_surf)
      return VDP_STATUS_INVALID_HANDLE;

   pipe = p_surf->device->context;
   if (!pipe)
      return VDP_STATUS_INVALID_HANDLE;

   pipe_mutex_lock(p_surf->device->mutex);
   if (p_surf->video_buffer == NULL || pformat != p_surf->video_buffer->buffer_format) {

      /* destroy the old one */
      if (p_surf->video_buffer)
         p_surf->video_buffer->destroy(p_surf->video_buffer);

      /* adjust the template parameters */
      p_surf->templat.buffer_format = pformat;

      /* and try to create the video buffer with the new format */
      p_surf->video_buffer = pipe->create_video_buffer(pipe, &p_surf->templat);

      /* stil no luck? ok forget it we don't support it */
      if (!p_surf->video_buffer) {
         pipe_mutex_unlock(p_surf->device->mutex);
         return VDP_STATUS_NO_IMPLEMENTATION;
      }
      vlVdpVideoSurfaceClear(p_surf);
   }

   sampler_views = p_surf->video_buffer->get_sampler_view_planes(p_surf->video_buffer);
   if (!sampler_views) {
      pipe_mutex_unlock(p_surf->device->mutex);
      return VDP_STATUS_RESOURCES;
   }

   for (i = 0; i < 3; ++i) {
      unsigned width, height;
      struct pipe_sampler_view *sv = sampler_views[i];
      if (!sv || !source_pitches[i]) continue;

      vlVdpVideoSurfaceSize(p_surf, i, &width, &height);

      for (j = 0; j < sv->texture->depth0; ++j) {
         struct pipe_box dst_box = {
            0, 0, j,
            width, height, 1
         };

         pipe->transfer_inline_write(pipe, sv->texture, 0,
                                     PIPE_TRANSFER_WRITE, &dst_box,
                                     source_data[i] + source_pitches[i] * j,
                                     source_pitches[i] * sv->texture->depth0,
                                     0);
      }
   }
   pipe_mutex_unlock(p_surf->device->mutex);

   return VDP_STATUS_OK;
}

/**
 * Helper function to initially clear the VideoSurface after (re-)creation
 */
void
vlVdpVideoSurfaceClear(vlVdpSurface *vlsurf)
{
   struct pipe_context *pipe = vlsurf->device->context;
   struct pipe_surface **surfaces;
   unsigned i;

   if (!vlsurf->video_buffer)
      return;

   surfaces = vlsurf->video_buffer->get_surfaces(vlsurf->video_buffer);
   for (i = 0; i < VL_MAX_SURFACES; ++i) {
      union pipe_color_union c = {};

      if (!surfaces[i])
         continue;

      if (i > !!vlsurf->templat.interlaced)
         c.f[0] = c.f[1] = c.f[2] = c.f[3] = 0.5f;

      pipe->clear_render_target(pipe, surfaces[i], &c, 0, 0,
                                surfaces[i]->width, surfaces[i]->height);
   }
   pipe->flush(pipe, NULL);
}
