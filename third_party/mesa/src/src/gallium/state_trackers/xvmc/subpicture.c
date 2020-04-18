/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include <X11/Xlibint.h>
#include <X11/extensions/XvMClib.h>

#include "pipe/p_screen.h"
#include "pipe/p_video_decoder.h"
#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_sampler.h"
#include "util/u_rect.h"
#include "vl/vl_winsys.h"

#include "xvmc_private.h"

#define FOURCC_RGB 0x0000003
#define FOURCC_AI44 0x34344941
#define FOURCC_IA44 0x34344149

static enum pipe_format XvIDToPipe(int xvimage_id)
{
   switch (xvimage_id) {
      case FOURCC_RGB:
         return PIPE_FORMAT_B8G8R8X8_UNORM;

      case FOURCC_AI44:
         return PIPE_FORMAT_A4R4_UNORM;

      case FOURCC_IA44:
         return PIPE_FORMAT_R4A4_UNORM;

      default:
         XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized Xv image ID 0x%08X.\n", xvimage_id);
         return PIPE_FORMAT_NONE;
   }
}

static unsigned NumPaletteEntries4XvID(int xvimage_id)
{
   switch (xvimage_id) {
      case FOURCC_RGB:
         return 0;

      case FOURCC_AI44:
      case FOURCC_IA44:
         return 16;

      default:
         XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized Xv image ID 0x%08X.\n", xvimage_id);
         return 0;
   }
}

static int PipeToComponentOrder(enum pipe_format format, char *component_order)
{
   assert(component_order);

   switch (format) {
      case PIPE_FORMAT_B8G8R8X8_UNORM:
         return 0;

      case PIPE_FORMAT_R4A4_UNORM:
      case PIPE_FORMAT_A4R4_UNORM:
         component_order[0] = 'Y';
         component_order[1] = 'U';
         component_order[2] = 'V';
         component_order[3] = 'A';
         return 4;

      default:
         XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized PIPE_FORMAT 0x%08X.\n", format);
         component_order[0] = 0;
         component_order[1] = 0;
         component_order[2] = 0;
         component_order[3] = 0;
         return 0;
   }
}

static Status Validate(Display *dpy, XvPortID port, int surface_type_id, int xvimage_id)
{
   XvImageFormatValues *subpictures;
   int num_subpics;
   unsigned int i;

   subpictures = XvMCListSubpictureTypes(dpy, port, surface_type_id, &num_subpics);
   if (num_subpics < 1) {
      if (subpictures)
         XFree(subpictures);
      return BadMatch;
   }
   if (!subpictures)
      return BadAlloc;

   for (i = 0; i < num_subpics; ++i) {
      if (subpictures[i].id == xvimage_id) {
         XVMC_MSG(XVMC_TRACE, "[XvMC] Found requested subpicture format.\n" \
                              "[XvMC]   port=%u\n" \
                              "[XvMC]   surface id=0x%08X\n" \
                              "[XvMC]   image id=0x%08X\n" \
                              "[XvMC]   type=%08X\n" \
                              "[XvMC]   byte order=%08X\n" \
                              "[XvMC]   bits per pixel=%u\n" \
                              "[XvMC]   format=%08X\n" \
                              "[XvMC]   num planes=%d\n",
                              port, surface_type_id, xvimage_id, subpictures[i].type, subpictures[i].byte_order,
                              subpictures[i].bits_per_pixel, subpictures[i].format, subpictures[i].num_planes);
         if (subpictures[i].type == XvRGB) {
            XVMC_MSG(XVMC_TRACE, "[XvMC]   depth=%d\n" \
                                 "[XvMC]   red mask=0x%08X\n" \
                                 "[XvMC]   green mask=0x%08X\n" \
                                 "[XvMC]   blue mask=0x%08X\n",
                                 subpictures[i].depth, subpictures[i].red_mask,
                                 subpictures[i].green_mask, subpictures[i].blue_mask);
         }
         else if (subpictures[i].type == XvYUV) {
            XVMC_MSG(XVMC_TRACE, "[XvMC]   y sample bits=0x%08X\n" \
                                 "[XvMC]   u sample bits=0x%08X\n" \
                                 "[XvMC]   v sample bits=0x%08X\n" \
                                 "[XvMC]   horz y period=%u\n" \
                                 "[XvMC]   horz u period=%u\n" \
                                 "[XvMC]   horz v period=%u\n" \
                                 "[XvMC]   vert y period=%u\n" \
                                 "[XvMC]   vert u period=%u\n" \
                                 "[XvMC]   vert v period=%u\n",
                                 subpictures[i].y_sample_bits, subpictures[i].u_sample_bits, subpictures[i].v_sample_bits,
                                 subpictures[i].horz_y_period, subpictures[i].horz_u_period, subpictures[i].horz_v_period,
                                 subpictures[i].vert_y_period, subpictures[i].vert_u_period, subpictures[i].vert_v_period);
         }
         break;
      }
   }

   XFree(subpictures);

   return i < num_subpics ? Success : BadMatch;
}

static void
upload_sampler(struct pipe_context *pipe, struct pipe_sampler_view *dst,
               const struct pipe_box *dst_box, const void *src, unsigned src_stride,
               unsigned src_x, unsigned src_y)
{
   struct pipe_transfer *transfer;
   void *map;

   transfer = pipe->get_transfer(pipe, dst->texture, 0, PIPE_TRANSFER_WRITE, dst_box);
   if (!transfer)
      return;

   map = pipe->transfer_map(pipe, transfer);
   if (map) {
      util_copy_rect(map, dst->texture->format, transfer->stride, 0, 0,
                     dst_box->width, dst_box->height,
                     src, src_stride, src_x, src_y);

      pipe->transfer_unmap(pipe, transfer);
   }

   pipe->transfer_destroy(pipe, transfer);
}

PUBLIC
Status XvMCCreateSubpicture(Display *dpy, XvMCContext *context, XvMCSubpicture *subpicture,
                            unsigned short width, unsigned short height, int xvimage_id)
{
   XvMCContextPrivate *context_priv;
   XvMCSubpicturePrivate *subpicture_priv;
   struct pipe_context *pipe;
   struct pipe_resource tex_templ, *tex;
   struct pipe_sampler_view sampler_templ;
   Status ret;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Creating subpicture %p.\n", subpicture);

   assert(dpy);

   if (!context)
      return XvMCBadContext;

   context_priv = context->privData;
   pipe = context_priv->pipe;

   if (!subpicture)
      return XvMCBadSubpicture;

   if (width > context_priv->subpicture_max_width ||
       height > context_priv->subpicture_max_height)
      return BadValue;

   ret = Validate(dpy, context->port, context->surface_type_id, xvimage_id);
   if (ret != Success)
      return ret;

   subpicture_priv = CALLOC(1, sizeof(XvMCSubpicturePrivate));
   if (!subpicture_priv)
      return BadAlloc;

   memset(&tex_templ, 0, sizeof(tex_templ));
   tex_templ.target = PIPE_TEXTURE_2D;
   tex_templ.format = XvIDToPipe(xvimage_id);
   tex_templ.last_level = 0;
   if (pipe->screen->get_video_param(pipe->screen,
                                     PIPE_VIDEO_PROFILE_UNKNOWN,
                                     PIPE_VIDEO_CAP_NPOT_TEXTURES)) {
      tex_templ.width0 = width;
      tex_templ.height0 = height;
   }
   else {
      tex_templ.width0 = util_next_power_of_two(width);
      tex_templ.height0 = util_next_power_of_two(height);
   }
   tex_templ.depth0 = 1;
   tex_templ.array_size = 1;
   tex_templ.usage = PIPE_USAGE_DYNAMIC;
   tex_templ.bind = PIPE_BIND_SAMPLER_VIEW;
   tex_templ.flags = 0;

   tex = pipe->screen->resource_create(pipe->screen, &tex_templ);

   memset(&sampler_templ, 0, sizeof(sampler_templ));
   u_sampler_view_default_template(&sampler_templ, tex, tex->format);

   subpicture_priv->sampler = pipe->create_sampler_view(pipe, tex, &sampler_templ);
   pipe_resource_reference(&tex, NULL);
   if (!subpicture_priv->sampler) {
      FREE(subpicture_priv);
      return BadAlloc;
   }

   subpicture_priv->context = context;
   subpicture->subpicture_id = XAllocID(dpy);
   subpicture->context_id = context->context_id;
   subpicture->xvimage_id = xvimage_id;
   subpicture->width = width;
   subpicture->height = height;
   subpicture->num_palette_entries = NumPaletteEntries4XvID(xvimage_id);
   subpicture->entry_bytes = PipeToComponentOrder(tex_templ.format, subpicture->component_order);
   subpicture->privData = subpicture_priv;

   if (subpicture->num_palette_entries > 0) {
      tex_templ.target = PIPE_TEXTURE_1D;
      tex_templ.format = PIPE_FORMAT_R8G8B8X8_UNORM;
      tex_templ.width0 = subpicture->num_palette_entries;
      tex_templ.height0 = 1;
      tex_templ.usage = PIPE_USAGE_STATIC;

      tex = pipe->screen->resource_create(pipe->screen, &tex_templ);

      memset(&sampler_templ, 0, sizeof(sampler_templ));
      u_sampler_view_default_template(&sampler_templ, tex, tex->format);
      sampler_templ.swizzle_a = PIPE_SWIZZLE_ONE;
      subpicture_priv->palette = pipe->create_sampler_view(pipe, tex, &sampler_templ);
      pipe_resource_reference(&tex, NULL);
      if (!subpicture_priv->sampler) {
         FREE(subpicture_priv);
         return BadAlloc;
      }
   }

   SyncHandle();

   XVMC_MSG(XVMC_TRACE, "[XvMC] Subpicture %p created.\n", subpicture);

   return Success;
}

PUBLIC
Status XvMCClearSubpicture(Display *dpy, XvMCSubpicture *subpicture, short x, short y,
                           unsigned short width, unsigned short height, unsigned int color)
{
   XvMCSubpicturePrivate *subpicture_priv;
   XvMCContextPrivate *context_priv;
   struct pipe_context *pipe;
   struct pipe_sampler_view *dst;
   struct pipe_box dst_box = {x, y, 0, width, height, 1};
   struct pipe_transfer *transfer;
   union util_color uc;
   void *map;

   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   /* Convert color to float */
   util_format_read_4f(PIPE_FORMAT_B8G8R8A8_UNORM,
                       uc.f, 1, &color, 4,
                       0, 0, 1, 1);

   subpicture_priv = subpicture->privData;
   context_priv = subpicture_priv->context->privData;
   pipe = context_priv->pipe;
   dst = subpicture_priv->sampler;

   /* TODO: Assert clear rect is within bounds? Or clip? */
   transfer = pipe->get_transfer(pipe, dst->texture, 0, PIPE_TRANSFER_WRITE, &dst_box);
   if (!transfer)
      return XvMCBadSubpicture;

   map = pipe->transfer_map(pipe, transfer);
   if (map) {
      util_fill_rect(map, dst->texture->format, transfer->stride, 0, 0,
                     dst_box.width, dst_box.height, &uc);

      pipe->transfer_unmap(pipe, transfer);
   }

   pipe->transfer_destroy(pipe, transfer);

   return Success;
}

PUBLIC
Status XvMCCompositeSubpicture(Display *dpy, XvMCSubpicture *subpicture, XvImage *image,
                               short srcx, short srcy, unsigned short width, unsigned short height,
                               short dstx, short dsty)
{
   XvMCSubpicturePrivate *subpicture_priv;
   XvMCContextPrivate *context_priv;
   struct pipe_context *pipe;
   struct pipe_box dst_box = {dstx, dsty, 0, width, height, 1};
   unsigned src_stride;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Compositing subpicture %p.\n", subpicture);

   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   assert(image);

   if (subpicture->xvimage_id != image->id)
      return BadMatch;

   /* No planar support for now */
   if (image->num_planes != 1)
      return BadMatch;

   subpicture_priv = subpicture->privData;
   context_priv = subpicture_priv->context->privData;
   pipe = context_priv->pipe;

   /* clipping should be done by upload_sampler and regardles what the documentation
   says image->pitches[0] doesn't seems to be in bytes, so don't use it */
   src_stride = image->width * util_format_get_blocksize(subpicture_priv->sampler->texture->format);
   upload_sampler(pipe, subpicture_priv->sampler, &dst_box, image->data, src_stride, srcx, srcy);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Subpicture %p composited.\n", subpicture);

   return Success;
}

PUBLIC
Status XvMCDestroySubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   XvMCSubpicturePrivate *subpicture_priv;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Destroying subpicture %p.\n", subpicture);

   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   subpicture_priv = subpicture->privData;
   pipe_sampler_view_reference(&subpicture_priv->sampler, NULL);
   pipe_sampler_view_reference(&subpicture_priv->palette, NULL);
   FREE(subpicture_priv);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Subpicture %p destroyed.\n", subpicture);

   return Success;
}

PUBLIC
Status XvMCSetSubpicturePalette(Display *dpy, XvMCSubpicture *subpicture, unsigned char *palette)
{
   XvMCSubpicturePrivate *subpicture_priv;
   XvMCContextPrivate *context_priv;
   struct pipe_context *pipe;
   struct pipe_box dst_box = {0, 0, 0, 0, 1, 1};

   assert(dpy);
   assert(palette);

   if (!subpicture)
      return XvMCBadSubpicture;

   subpicture_priv = subpicture->privData;
   context_priv = subpicture_priv->context->privData;
   pipe = context_priv->pipe;

   dst_box.width = subpicture->num_palette_entries;

   upload_sampler(pipe, subpicture_priv->palette, &dst_box, palette, 0, 0, 0);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Palette of Subpicture %p set.\n", subpicture);

   return Success;
}

PUBLIC
Status XvMCBlendSubpicture(Display *dpy, XvMCSurface *target_surface, XvMCSubpicture *subpicture,
                           short subx, short suby, unsigned short subw, unsigned short subh,
                           short surfx, short surfy, unsigned short surfw, unsigned short surfh)
{
   struct u_rect src_rect = {subx, subx + subw, suby, suby + subh};
   struct u_rect dst_rect = {surfx, surfx + surfw, surfy, surfy + surfh};

   XvMCSurfacePrivate *surface_priv;
   XvMCSubpicturePrivate *subpicture_priv;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Associating subpicture %p with surface %p.\n", subpicture, target_surface);

   assert(dpy);

   if (!target_surface)
      return XvMCBadSurface;

   if (!subpicture)
      return XvMCBadSubpicture;

   if (target_surface->context_id != subpicture->context_id)
      return BadMatch;

   /* TODO: Verify against subpicture independent scaling */

   surface_priv = target_surface->privData;
   subpicture_priv = subpicture->privData;

   /* TODO: Assert rects are within bounds? Or clip? */
   subpicture_priv->src_rect = src_rect;
   subpicture_priv->dst_rect = dst_rect;

   surface_priv->subpicture = subpicture;
   subpicture_priv->surface = target_surface;

   return Success;
}

PUBLIC
Status XvMCBlendSubpicture2(Display *dpy, XvMCSurface *source_surface, XvMCSurface *target_surface,
                            XvMCSubpicture *subpicture, short subx, short suby, unsigned short subw, unsigned short subh,
                            short surfx, short surfy, unsigned short surfw, unsigned short surfh)
{
   assert(dpy);

   if (!source_surface || !target_surface)
      return XvMCBadSurface;

   if (!subpicture)
      return XvMCBadSubpicture;

   if (source_surface->context_id != subpicture->context_id)
      return BadMatch;

   if (source_surface->context_id != subpicture->context_id)
      return BadMatch;

   /* TODO: Assert rects are within bounds? Or clip? */

   return Success;
}

PUBLIC
Status XvMCSyncSubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   return Success;
}

PUBLIC
Status XvMCFlushSubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   return Success;
}

PUBLIC
Status XvMCGetSubpictureStatus(Display *dpy, XvMCSubpicture *subpicture, int *status)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   assert(status);

   /* TODO */
   *status = 0;

   return Success;
}
