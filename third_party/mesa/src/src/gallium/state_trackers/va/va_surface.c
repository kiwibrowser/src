/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
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

#include <va/va.h>
#include <va/va_backend.h>
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "va_private.h"

static enum pipe_video_chroma_format
VaRTFormatToPipe(unsigned int va_type)
{
   switch (va_type) {
   case VA_RT_FORMAT_YUV420:
      return PIPE_VIDEO_CHROMA_FORMAT_420;
   case VA_RT_FORMAT_YUV422:
      return PIPE_VIDEO_CHROMA_FORMAT_422;
   case VA_RT_FORMAT_YUV444:
      return PIPE_VIDEO_CHROMA_FORMAT_444;
   default:
      assert(0);
   }

   return -1;
}

VAStatus
vlVaCreateSurfaces(VADriverContextP ctx, int width, int height, int format,
                   int num_surfaces, VASurfaceID *surfaces)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   /* We only support one format */
   if (VA_RT_FORMAT_YUV420 != format)
      return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
		
   if (!(width && height))
      return VA_STATUS_ERROR_INVALID_IMAGE_FORMAT;
		
   if (!vlCreateHTAB())
      return VA_STATUS_ERROR_UNKNOWN; 
		
   vlVaSurfacePriv *va_surface = (vlVaSurfacePriv *)CALLOC(num_surfaces,sizeof(vlVaSurfacePriv));
   if (!va_surface)
      return VA_STATUS_ERROR_ALLOCATION_FAILED;
		
   int n = 0;
   for (n = 0; n < num_surfaces; n++) {
      va_surface[n].width = width;
      va_surface[n].height = height;
      va_surface[n].format = VaRTFormatToPipe(format);
      va_surface[n].ctx = ctx;
      surfaces[n] = vlAddDataHTAB((void *)(va_surface + n));
   }

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaDestroySurfaces(VADriverContextP ctx, VASurfaceID *surface_list, int num_surfaces)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaSyncSurface(VADriverContextP ctx, VASurfaceID render_target)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaQuerySurfaceStatus(VADriverContextP ctx, VASurfaceID render_target, VASurfaceStatus *status)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaPutSurface(VADriverContextP ctx, VASurfaceID surface, void* draw, short srcx, short srcy,
               unsigned short srcw, unsigned short srch, short destx, short desty,
               unsigned short destw, unsigned short desth, VARectangle *cliprects,
               unsigned int number_cliprects,  unsigned int flags)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaLockSurface(VADriverContextP ctx, VASurfaceID surface, unsigned int *fourcc,
                unsigned int *luma_stride, unsigned int *chroma_u_stride, unsigned int *chroma_v_stride,
                unsigned int *luma_offset, unsigned int *chroma_u_offset, unsigned int *chroma_v_offset,
                unsigned int *buffer_name, void **buffer)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaUnlockSurface(VADriverContextP ctx, VASurfaceID surface)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}
