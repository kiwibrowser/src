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
#include "pipe/p_video_state.h"
#include "pipe/p_state.h"

#include "util/u_memory.h"

#include "vl/vl_csc.h"
#include "vl/vl_winsys.h"

#include "xvmc_private.h"

static Status Validate(Display *dpy, XvPortID port, int surface_type_id,
                       unsigned int width, unsigned int height, int flags,
                       bool *found_port, int *screen, int *chroma_format,
                       int *mc_type, int *surface_flags,
                       unsigned short *subpic_max_w,
                       unsigned short *subpic_max_h)
{
   bool found_surface = false;
   XvAdaptorInfo *adaptor_info;
   unsigned int num_adaptors;
   int num_types;
   unsigned int max_width = 0, max_height = 0;
   Status ret;

   assert(dpy);
   assert(found_port);
   assert(screen);
   assert(chroma_format);
   assert(mc_type);
   assert(surface_flags);
   assert(subpic_max_w);
   assert(subpic_max_h);

   *found_port = false;

   for (unsigned int i = 0; i < XScreenCount(dpy); ++i) {
      ret = XvQueryAdaptors(dpy, XRootWindow(dpy, i), &num_adaptors, &adaptor_info);
      if (ret != Success)
         return ret;

      for (unsigned int j = 0; j < num_adaptors && !*found_port; ++j) {
         for (unsigned int k = 0; k < adaptor_info[j].num_ports && !*found_port; ++k) {
            XvMCSurfaceInfo *surface_info;

            if (adaptor_info[j].base_id + k != port)
               continue;

            *found_port = true;

            surface_info = XvMCListSurfaceTypes(dpy, adaptor_info[j].base_id, &num_types);
            if (!surface_info) {
               XvFreeAdaptorInfo(adaptor_info);
               return BadAlloc;
            }

            for (unsigned int l = 0; l < num_types && !found_surface; ++l) {
               if (surface_info[l].surface_type_id != surface_type_id)
                  continue;

               found_surface = true;
               max_width = surface_info[l].max_width;
               max_height = surface_info[l].max_height;
               *chroma_format = surface_info[l].chroma_format;
               *mc_type = surface_info[l].mc_type;
               *surface_flags = surface_info[l].flags;
               *subpic_max_w = surface_info[l].subpicture_max_width;
               *subpic_max_h = surface_info[l].subpicture_max_height;
               *screen = i;

               XVMC_MSG(XVMC_TRACE, "[XvMC] Found requested context surface format.\n" \
                                    "[XvMC]   screen=%u, port=%u\n" \
                                    "[XvMC]   id=0x%08X\n" \
                                    "[XvMC]   max width=%u, max height=%u\n" \
                                    "[XvMC]   chroma format=0x%08X\n" \
                                    "[XvMC]   acceleration level=0x%08X\n" \
                                    "[XvMC]   flags=0x%08X\n" \
                                    "[XvMC]   subpicture max width=%u, max height=%u\n",
                                    i, port, surface_type_id, max_width, max_height, *chroma_format,
                                    *mc_type, *surface_flags, *subpic_max_w, *subpic_max_h);
            }

            XFree(surface_info);
         }
      }

      XvFreeAdaptorInfo(adaptor_info);
   }

   if (!*found_port) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Could not find a suitable port.\n");
      return XvBadPort;
   }
   if (!found_surface) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Could not find a suitable surface.\n");
      return BadMatch;
   }
   if (width > max_width || height > max_height) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Requested context dimensions (w=%u,h=%u) too large (max w=%u,h=%u).\n",
               width, height, max_width, max_height);
      return BadValue;
   }
   if (flags != XVMC_DIRECT && flags != 0) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Invalid context flags 0x%08X.\n", flags);
      return BadValue;
   }

   return Success;
}

static enum pipe_video_profile ProfileToPipe(int xvmc_profile)
{
   if (xvmc_profile & XVMC_MPEG_1)
      assert(0);
   if (xvmc_profile & XVMC_MPEG_2)
      return PIPE_VIDEO_PROFILE_MPEG2_MAIN;
   if (xvmc_profile & XVMC_H263)
      assert(0);
   if (xvmc_profile & XVMC_MPEG_4)
      assert(0);

   assert(0);

   XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized profile 0x%08X.\n", xvmc_profile);

   return -1;
}

static enum pipe_video_chroma_format FormatToPipe(int xvmc_format)
{
   switch (xvmc_format) {
      case XVMC_CHROMA_FORMAT_420:
         return PIPE_VIDEO_CHROMA_FORMAT_420;
      case XVMC_CHROMA_FORMAT_422:
         return PIPE_VIDEO_CHROMA_FORMAT_422;
      case XVMC_CHROMA_FORMAT_444:
         return PIPE_VIDEO_CHROMA_FORMAT_444;
      default:
         assert(0);
   }

   XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized format 0x%08X.\n", xvmc_format);

   return -1;
}

PUBLIC
Status XvMCCreateContext(Display *dpy, XvPortID port, int surface_type_id,
                         int width, int height, int flags, XvMCContext *context)
{
   bool found_port;
   int scrn = 0;
   int chroma_format = 0;
   int mc_type = 0;
   int surface_flags = 0;
   unsigned short subpic_max_w = 0;
   unsigned short subpic_max_h = 0;
   Status ret;
   struct vl_screen *vscreen;
   struct pipe_context *pipe;
   XvMCContextPrivate *context_priv;
   vl_csc_matrix csc;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Creating context %p.\n", context);

   assert(dpy);

   if (!context)
      return XvMCBadContext;

   ret = Validate(dpy, port, surface_type_id, width, height, flags,
                  &found_port, &scrn, &chroma_format, &mc_type, &surface_flags,
                  &subpic_max_w, &subpic_max_h);

   /* Success and XvBadPort have the same value */
   if (ret != Success || !found_port)
      return ret;

   /* XXX: Current limits */
   if (chroma_format != XVMC_CHROMA_FORMAT_420) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Cannot decode requested surface type. Unsupported chroma format.\n");
      return BadImplementation;
   }
   if ((mc_type & ~XVMC_IDCT) != (XVMC_MOCOMP | XVMC_MPEG_2)) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Cannot decode requested surface type. Non-MPEG2/Mocomp/iDCT acceleration unsupported.\n");
      return BadImplementation;
   }
   if (surface_flags & XVMC_INTRA_UNSIGNED) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Cannot decode requested surface type. Unsigned intra unsupported.\n");
      return BadImplementation;
   }

   context_priv = CALLOC(1, sizeof(XvMCContextPrivate));
   if (!context_priv)
      return BadAlloc;

   /* TODO: Reuse screen if process creates another context */
   vscreen = vl_screen_create(dpy, scrn);

   if (!vscreen) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Could not create VL screen.\n");
      FREE(context_priv);
      return BadAlloc;
   }

   pipe = vscreen->pscreen->context_create(vscreen->pscreen, vscreen);
   if (!pipe) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Could not create VL context.\n");
      vl_screen_destroy(vscreen);
      FREE(context_priv);
      return BadAlloc;
   }

   context_priv->decoder = pipe->create_video_decoder
   (
      pipe, ProfileToPipe(mc_type),
      (mc_type & XVMC_IDCT) ? PIPE_VIDEO_ENTRYPOINT_IDCT : PIPE_VIDEO_ENTRYPOINT_MC,
      FormatToPipe(chroma_format),
      width, height, 2,
      true
   );

   if (!context_priv->decoder) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Could not create VL decoder.\n");
      pipe->destroy(pipe);
      vl_screen_destroy(vscreen);
      FREE(context_priv);
      return BadAlloc;
   }

   if (!vl_compositor_init(&context_priv->compositor, pipe)) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Could not create VL compositor.\n");
      context_priv->decoder->destroy(context_priv->decoder);
      pipe->destroy(pipe);
      vl_screen_destroy(vscreen);
      FREE(context_priv);
      return BadAlloc;
   }

   if (!vl_compositor_init_state(&context_priv->cstate, pipe)) {
      XVMC_MSG(XVMC_ERR, "[XvMC] Could not create VL compositor state.\n");
      vl_compositor_cleanup(&context_priv->compositor);
      context_priv->decoder->destroy(context_priv->decoder);
      pipe->destroy(pipe);
      vl_screen_destroy(vscreen);
      FREE(context_priv);
      return BadAlloc;
   }


   context_priv->color_standard =
      debug_get_bool_option("G3DVL_NO_CSC", FALSE) ?
      VL_CSC_COLOR_STANDARD_IDENTITY : VL_CSC_COLOR_STANDARD_BT_601;
   context_priv->procamp = vl_default_procamp;

   vl_csc_get_matrix
   (
      context_priv->color_standard,
      &context_priv->procamp, true, &csc
   );
   vl_compositor_set_csc_matrix(&context_priv->cstate, (const vl_csc_matrix *)&csc);

   context_priv->vscreen = vscreen;
   context_priv->pipe = pipe;
   context_priv->subpicture_max_width = subpic_max_w;
   context_priv->subpicture_max_height = subpic_max_h;

   context->context_id = XAllocID(dpy);
   context->surface_type_id = surface_type_id;
   context->width = width;
   context->height = height;
   context->flags = flags;
   context->port = port;
   context->privData = context_priv;

   SyncHandle();

   XVMC_MSG(XVMC_TRACE, "[XvMC] Context %p created.\n", context);

   return Success;
}

PUBLIC
Status XvMCDestroyContext(Display *dpy, XvMCContext *context)
{
   XvMCContextPrivate *context_priv;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Destroying context %p.\n", context);

   assert(dpy);

   if (!context || !context->privData)
      return XvMCBadContext;

   context_priv = context->privData;
   context_priv->decoder->destroy(context_priv->decoder);
   vl_compositor_cleanup_state(&context_priv->cstate);
   vl_compositor_cleanup(&context_priv->compositor);
   context_priv->pipe->destroy(context_priv->pipe);
   vl_screen_destroy(context_priv->vscreen);
   FREE(context_priv);
   context->privData = NULL;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Context %p destroyed.\n", context);

   return Success;
}
