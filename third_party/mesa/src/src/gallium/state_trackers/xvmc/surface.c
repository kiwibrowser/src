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
#include <stdio.h>

#include <X11/Xlibint.h>

#include "pipe/p_video_decoder.h"
#include "pipe/p_video_state.h"
#include "pipe/p_state.h"

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "vl/vl_winsys.h"

#include "xvmc_private.h"

static void
MacroBlocksToPipe(XvMCContextPrivate *context,
                  XvMCSurfacePrivate *surface,
                  unsigned int xvmc_picture_structure,
                  const XvMCMacroBlock *xvmc_mb,
                  const XvMCBlockArray *xvmc_blocks,
                  struct pipe_mpeg12_macroblock *mb,
                  unsigned int num_macroblocks)
{
   unsigned int i, j, k;

   assert(xvmc_mb);
   assert(xvmc_blocks);
   assert(num_macroblocks);

   for (; num_macroblocks > 0; --num_macroblocks) {
      mb->base.codec = PIPE_VIDEO_CODEC_MPEG12;
      mb->x = xvmc_mb->x;
      mb->y = xvmc_mb->y;
      mb->macroblock_type = xvmc_mb->macroblock_type;

      switch (xvmc_picture_structure) {
      case XVMC_FRAME_PICTURE:
         mb->macroblock_modes.bits.frame_motion_type = xvmc_mb->motion_type;
         mb->macroblock_modes.bits.field_motion_type = 0;
         break;

      case XVMC_TOP_FIELD:
      case XVMC_BOTTOM_FIELD:
         mb->macroblock_modes.bits.frame_motion_type = 0;
         mb->macroblock_modes.bits.field_motion_type = xvmc_mb->motion_type;
         break;

      default:
         assert(0);
      }

      mb->macroblock_modes.bits.dct_type = xvmc_mb->dct_type;
      mb->motion_vertical_field_select = xvmc_mb->motion_vertical_field_select;

      for (i = 0; i < 2; ++i)
         for (j = 0; j < 2; ++j)
            for (k = 0; k < 2; ++k)
               mb->PMV[i][j][k] = xvmc_mb->PMV[i][j][k];

      mb->coded_block_pattern = xvmc_mb->coded_block_pattern;
      mb->blocks = xvmc_blocks->blocks + xvmc_mb->index * BLOCK_SIZE_SAMPLES;
      mb->num_skipped_macroblocks = 0;

      ++xvmc_mb;
      ++mb;
   }
}

static void
GetPictureDescription(XvMCSurfacePrivate *surface, struct pipe_mpeg12_picture_desc *desc)
{
   unsigned i, num_refs = 0;

   assert(surface && desc);

   memset(desc, 0, sizeof(*desc));
   desc->base.profile = PIPE_VIDEO_PROFILE_MPEG1;
   desc->picture_structure = surface->picture_structure;
   for (i = 0; i < 2; ++i) {
      if (surface->ref[i]) {
         XvMCSurfacePrivate *ref = surface->ref[i]->privData;

         if (ref)
            desc->ref[num_refs++] = ref->video_buffer;
      }
   }
}

static void
RecursiveEndFrame(XvMCSurfacePrivate *surface)
{
   XvMCContextPrivate *context_priv;
   unsigned i;

   assert(surface);

   context_priv = surface->context->privData;

   for ( i = 0; i < 2; ++i ) {
      if (surface->ref[i]) {
         XvMCSurface *ref = surface->ref[i];

         assert(ref);

         surface->ref[i] = NULL;
         RecursiveEndFrame(ref->privData);
         surface->ref[i] = ref;
      }
   }

   if (surface->picture_structure) {
      struct pipe_mpeg12_picture_desc desc;
      GetPictureDescription(surface, &desc);
      surface->picture_structure = 0;

      for (i = 0; i < 2; ++i)
         surface->ref[i] = NULL;

      context_priv->decoder->end_frame(context_priv->decoder, surface->video_buffer, &desc.base);
   }
}

PUBLIC
Status XvMCCreateSurface(Display *dpy, XvMCContext *context, XvMCSurface *surface)
{
   XvMCContextPrivate *context_priv;
   struct pipe_context *pipe;
   XvMCSurfacePrivate *surface_priv;
   struct pipe_video_buffer tmpl;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Creating surface %p.\n", surface);

   assert(dpy);

   if (!context)
      return XvMCBadContext;
   if (!surface)
      return XvMCBadSurface;

   context_priv = context->privData;
   pipe = context_priv->pipe;

   surface_priv = CALLOC(1, sizeof(XvMCSurfacePrivate));
   if (!surface_priv)
      return BadAlloc;

   memset(&tmpl, 0, sizeof(tmpl));
   tmpl.buffer_format = pipe->screen->get_video_param
   (
      pipe->screen,
      PIPE_VIDEO_PROFILE_MPEG2_MAIN,
      PIPE_VIDEO_CAP_PREFERED_FORMAT
   );
   tmpl.chroma_format = context_priv->decoder->chroma_format;
   tmpl.width = context_priv->decoder->width;
   tmpl.height = context_priv->decoder->height;
   tmpl.interlaced = pipe->screen->get_video_param
   (
      pipe->screen,
      PIPE_VIDEO_PROFILE_MPEG2_MAIN,
      PIPE_VIDEO_CAP_PREFERS_INTERLACED
   );

   surface_priv->video_buffer = pipe->create_video_buffer(pipe, &tmpl);
   surface_priv->context = context;

   surface->surface_id = XAllocID(dpy);
   surface->context_id = context->context_id;
   surface->surface_type_id = context->surface_type_id;
   surface->width = context->width;
   surface->height = context->height;
   surface->privData = surface_priv;

   SyncHandle();

   XVMC_MSG(XVMC_TRACE, "[XvMC] Surface %p created.\n", surface);

   return Success;
}

PUBLIC
Status XvMCRenderSurface(Display *dpy, XvMCContext *context, unsigned int picture_structure,
                         XvMCSurface *target_surface, XvMCSurface *past_surface, XvMCSurface *future_surface,
                         unsigned int flags, unsigned int num_macroblocks, unsigned int first_macroblock,
                         XvMCMacroBlockArray *macroblocks, XvMCBlockArray *blocks
)
{
   struct pipe_mpeg12_macroblock mb[num_macroblocks];
   struct pipe_video_decoder *decoder;
   struct pipe_mpeg12_picture_desc desc;

   XvMCContextPrivate *context_priv;
   XvMCSurfacePrivate *target_surface_priv;
   XvMCSurfacePrivate *past_surface_priv;
   XvMCSurfacePrivate *future_surface_priv;
   XvMCMacroBlock *xvmc_mb;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Rendering to surface %p, with past %p and future %p\n",
            target_surface, past_surface, future_surface);

   assert(dpy);

   if (!context || !context->privData)
      return XvMCBadContext;
   if (!target_surface || !target_surface->privData)
      return XvMCBadSurface;

   if (picture_structure != XVMC_TOP_FIELD &&
       picture_structure != XVMC_BOTTOM_FIELD &&
       picture_structure != XVMC_FRAME_PICTURE)
      return BadValue;
   /* Bkwd pred equivalent to fwd (past && !future) */
   if (future_surface && !past_surface)
      return BadMatch;

   assert(context->context_id == target_surface->context_id);
   assert(!past_surface || context->context_id == past_surface->context_id);
   assert(!future_surface || context->context_id == future_surface->context_id);

   assert(macroblocks);
   assert(blocks);

   assert(macroblocks->context_id == context->context_id);
   assert(blocks->context_id == context->context_id);

   assert(flags == 0 || flags == XVMC_SECOND_FIELD);

   context_priv = context->privData;
   decoder = context_priv->decoder;

   target_surface_priv = target_surface->privData;
   past_surface_priv = past_surface ? past_surface->privData : NULL;
   future_surface_priv = future_surface ? future_surface->privData : NULL;

   assert(target_surface_priv->context == context);
   assert(!past_surface || past_surface_priv->context == context);
   assert(!future_surface || future_surface_priv->context == context);

   // call end frame on all referenced frames
   if (past_surface)
      RecursiveEndFrame(past_surface->privData);

   if (future_surface)
      RecursiveEndFrame(future_surface->privData);

   xvmc_mb = macroblocks->macro_blocks + first_macroblock;

   /* If the surface we're rendering hasn't changed the ref frames shouldn't change. */
   if (target_surface_priv->picture_structure > 0 && (
       target_surface_priv->picture_structure != picture_structure ||
       target_surface_priv->ref[0] != past_surface ||
       target_surface_priv->ref[1] != future_surface ||
       (xvmc_mb->x == 0 && xvmc_mb->y == 0))) {

      // If they change anyway we must assume that the current frame is ended
      RecursiveEndFrame(target_surface_priv);
   }

   target_surface_priv->ref[0] = past_surface;
   target_surface_priv->ref[1] = future_surface;

   if (target_surface_priv->picture_structure)
      GetPictureDescription(target_surface_priv, &desc);
   else {
      target_surface_priv->picture_structure = picture_structure;
      GetPictureDescription(target_surface_priv, &desc);
      decoder->begin_frame(decoder, target_surface_priv->video_buffer, &desc.base);
   }

   MacroBlocksToPipe(context_priv, target_surface_priv, picture_structure,
                     xvmc_mb, blocks, mb, num_macroblocks);

   context_priv->decoder->decode_macroblock(context_priv->decoder,
                                            target_surface_priv->video_buffer,
                                            &desc.base,
                                            &mb[0].base, num_macroblocks);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Submitted surface %p for rendering.\n", target_surface);

   return Success;
}

PUBLIC
Status XvMCFlushSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   // don't call flush here, because this is usually
   // called once for every slice instead of every frame

   XVMC_MSG(XVMC_TRACE, "[XvMC] Flushing surface %p\n", surface);

   return Success;
}

PUBLIC
Status XvMCSyncSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Syncing surface %p\n", surface);

   return Success;
}

PUBLIC
Status XvMCPutSurface(Display *dpy, XvMCSurface *surface, Drawable drawable,
                      short srcx, short srcy, unsigned short srcw, unsigned short srch,
                      short destx, short desty, unsigned short destw, unsigned short desth,
                      int flags)
{
   static int dump_window = -1;

   struct pipe_context *pipe;
   struct vl_compositor *compositor;
   struct vl_compositor_state *cstate;

   XvMCSurfacePrivate *surface_priv;
   XvMCContextPrivate *context_priv;
   XvMCSubpicturePrivate *subpicture_priv;
   XvMCContext *context;
   struct u_rect src_rect = {srcx, srcx + srcw, srcy, srcy + srch};
   struct u_rect dst_rect = {destx, destx + destw, desty, desty + desth};

   struct pipe_resource *tex;
   struct pipe_surface surf_templ, *surf;
   struct u_rect *dirty_area;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Displaying surface %p.\n", surface);

   assert(dpy);

   if (!surface || !surface->privData)
      return XvMCBadSurface;

   surface_priv = surface->privData;
   context = surface_priv->context;
   context_priv = context->privData;

   assert(flags == XVMC_TOP_FIELD || flags == XVMC_BOTTOM_FIELD || flags == XVMC_FRAME_PICTURE);
   assert(srcx + srcw - 1 < surface->width);
   assert(srcy + srch - 1 < surface->height);

   subpicture_priv = surface_priv->subpicture ? surface_priv->subpicture->privData : NULL;
   pipe = context_priv->pipe;
   compositor = &context_priv->compositor;
   cstate = &context_priv->cstate;

   tex = vl_screen_texture_from_drawable(context_priv->vscreen, drawable);
   dirty_area = vl_screen_get_dirty_area(context_priv->vscreen);

   memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = tex->format;
   surf_templ.usage = PIPE_BIND_RENDER_TARGET;
   surf = pipe->create_surface(pipe, tex, &surf_templ);

   if (!surf)
      return BadDrawable;

   /*
    * Some apps (mplayer) hit these asserts because they call
    * this function after the window has been resized by the WM
    * but before they've handled the corresponding XEvent and
    * know about the new dimensions. The output should be clipped
    * until the app updates destw and desth.
    */
   /*
   assert(destx + destw - 1 < drawable_surface->width);
   assert(desty + desth - 1 < drawable_surface->height);
    */

   RecursiveEndFrame(surface_priv);

   context_priv->decoder->flush(context_priv->decoder);

   vl_compositor_clear_layers(cstate);
   vl_compositor_set_buffer_layer(cstate, compositor, 0, surface_priv->video_buffer,
                                  &src_rect, NULL, VL_COMPOSITOR_WEAVE);

   if (subpicture_priv) {
      XVMC_MSG(XVMC_TRACE, "[XvMC] Surface %p has subpicture %p.\n", surface, surface_priv->subpicture);

      assert(subpicture_priv->surface == surface);

      if (subpicture_priv->palette)
         vl_compositor_set_palette_layer(cstate, compositor, 1, subpicture_priv->sampler, subpicture_priv->palette,
                                         &subpicture_priv->src_rect, &subpicture_priv->dst_rect, true);
      else
         vl_compositor_set_rgba_layer(cstate, compositor, 1, subpicture_priv->sampler,
                                      &subpicture_priv->src_rect, &subpicture_priv->dst_rect, NULL);

      surface_priv->subpicture = NULL;
      subpicture_priv->surface = NULL;
   }

   // Workaround for r600g, there seems to be a bug in the fence refcounting code
   pipe->screen->fence_reference(pipe->screen, &surface_priv->fence, NULL);

   vl_compositor_set_layer_dst_area(cstate, 0, &dst_rect);
   vl_compositor_set_layer_dst_area(cstate, 1, &dst_rect);
   vl_compositor_render(cstate, compositor, surf, dirty_area);

   pipe->flush(pipe, &surface_priv->fence);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Submitted surface %p for display. Pushing to front buffer.\n", surface);

   pipe->screen->flush_frontbuffer
   (
      pipe->screen, tex, 0, 0,
      vl_screen_get_private(context_priv->vscreen)
   );

   if(dump_window == -1) {
      dump_window = debug_get_num_option("XVMC_DUMP", 0);
   }

   if(dump_window) {
      static unsigned int framenum = 0;
      char cmd[256];

      sprintf(cmd, "xwd -id %d -out xvmc_frame_%08d.xwd", (int)drawable, ++framenum);
      if (system(cmd) != 0)
         XVMC_MSG(XVMC_ERR, "[XvMC] Dumping surface %p failed.\n", surface);
   }

   XVMC_MSG(XVMC_TRACE, "[XvMC] Pushed surface %p to front buffer.\n", surface);

   return Success;
}

PUBLIC
Status XvMCGetSurfaceStatus(Display *dpy, XvMCSurface *surface, int *status)
{
   struct pipe_context *pipe;
   XvMCSurfacePrivate *surface_priv;
   XvMCContextPrivate *context_priv;

   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   assert(status);

   surface_priv = surface->privData;
   context_priv = surface_priv->context->privData;
   pipe = context_priv->pipe;

   *status = 0;

   if (surface_priv->fence)
      if (!pipe->screen->fence_signalled(pipe->screen, surface_priv->fence))
         *status |= XVMC_RENDERING;

   return Success;
}

PUBLIC
Status XvMCDestroySurface(Display *dpy, XvMCSurface *surface)
{
   XvMCSurfacePrivate *surface_priv;
   XvMCContextPrivate *context_priv;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Destroying surface %p.\n", surface);

   assert(dpy);

   if (!surface || !surface->privData)
      return XvMCBadSurface;

   surface_priv = surface->privData;
   context_priv = surface_priv->context->privData;

   if (surface_priv->picture_structure) {
      struct pipe_mpeg12_picture_desc desc;
      GetPictureDescription(surface_priv, &desc);
      context_priv->decoder->end_frame(context_priv->decoder, surface_priv->video_buffer, &desc.base);
   }
   surface_priv->video_buffer->destroy(surface_priv->video_buffer);
   FREE(surface_priv);
   surface->privData = NULL;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Surface %p destroyed.\n", surface);

   return Success;
}

PUBLIC
Status XvMCHideSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface || !surface->privData)
      return XvMCBadSurface;

   /* No op, only for overlaid rendering */

   return Success;
}
