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

#include "pipe/p_screen.h"
#include "pipe/p_screen.h"
#include "pipe/p_video_decoder.h"

#include "util/u_debug.h"
#include "util/u_memory.h"
#include "vl/vl_winsys.h"

#include "va_private.h"

PUBLIC VAStatus
__vaDriverInit_0_31(VADriverContextP ctx)
{
   vlVaDriverContextPriv *driver_context = NULL;
	
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;
		
		
   /* Create private driver context */
   driver_context = CALLOC(1,sizeof(vlVaDriverContextPriv));
   if (!driver_context)
      return VA_STATUS_ERROR_ALLOCATION_FAILED;
		
   driver_context->vscreen = vl_screen_create(ctx->native_dpy, ctx->x11_screen);
   if (!driver_context->vscreen) {
      FREE(driver_context);
      return VA_STATUS_ERROR_ALLOCATION_FAILED;
   }
		
   ctx->str_vendor = "mesa gallium vaapi";
   ctx->vtable = vlVaGetVtable();
   ctx->max_attributes = 1;
   ctx->max_display_attributes = 1;
   ctx->max_entrypoints = VA_MAX_ENTRYPOINTS;
   ctx->max_image_formats = VA_MAX_IMAGE_FORMATS_SUPPORTED;
   ctx->max_profiles = 1;
   ctx->max_subpic_formats = VA_MAX_SUBPIC_FORMATS_SUPPORTED;
   ctx->version_major = 3;
   ctx->version_minor = 1;
   ctx->pDriverData = (void *)driver_context;

   VA_INFO("vl_screen_pointer %p\n",ctx->native_dpy);

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaCreateContext(VADriverContextP ctx, VAConfigID config_id, int picture_width,
                  int picture_height, int flag, VASurfaceID *render_targets,
                  int num_render_targets, VAContextID *conext)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaDestroyContext(VADriverContextP ctx, VAContextID context)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaTerminate(VADriverContextP ctx)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}
