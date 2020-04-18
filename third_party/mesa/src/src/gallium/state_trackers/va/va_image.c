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

#include "pipe/p_format.h"

#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_debug.h"

#include "va_private.h"

typedef struct  {
   enum pipe_format pipe_format;
   VAImageFormat       va_format;
} va_image_formats_supported_t;

static const va_image_formats_supported_t va_image_formats_supported[VA_MAX_IMAGE_FORMATS_SUPPORTED] = 
{
   { PIPE_FORMAT_B8G8R8A8_UNORM,
      { VA_FOURCC('B','G','R','A'), VA_LSB_FIRST, 32, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }},
   { PIPE_FORMAT_R8G8B8A8_UNORM, 
      { VA_FOURCC_RGBA, VA_LSB_FIRST, 32, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 }}
};

VAStatus
vlVaQueryImageFormats(VADriverContextP ctx, VAImageFormat *format_list, int *num_formats)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   if (!(format_list && num_formats))
      return VA_STATUS_ERROR_UNKNOWN;
		
   int n = 0;
	
   num_formats[0] = VA_MAX_IMAGE_FORMATS_SUPPORTED;
	
   /* Query supported formats */
   for (n = 0; n < VA_MAX_IMAGE_FORMATS_SUPPORTED; n++) {
      format_list[n] = va_image_formats_supported[n].va_format;
   }

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaCreateImage(VADriverContextP ctx, VAImageFormat *format, int width, int height, VAImage *image)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   if(!format)
      return VA_STATUS_ERROR_UNKNOWN;
		
   if (!(width && height))
      return VA_STATUS_ERROR_INVALID_IMAGE_FORMAT;
		
   if (!vlCreateHTAB())
      return VA_STATUS_ERROR_UNKNOWN; 
		
   switch (format->fourcc) {
   case VA_FOURCC('B','G','R','A'):
      VA_INFO("Creating BGRA image of size %dx%d\n",width,height);
      break;
   case VA_FOURCC_RGBA:
      VA_INFO("Creating RGBA image of size %dx%d\n",width,height);
      break;
   default:
      VA_ERROR("Couldn't create image of type %0x08\n",format->fourcc);
      return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
   }
		
   VA_INFO("Image %p created successfully\n",format);
	
   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaDeriveImage(VADriverContextP ctx, VASurfaceID surface, VAImage *image)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaDestroyImage(VADriverContextP ctx, VAImageID image)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaSetImagePalette(VADriverContextP ctx, VAImageID image, unsigned char *palette)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaGetImage(VADriverContextP ctx, VASurfaceID surface, int x, int y,
             unsigned int width, unsigned int height, VAImageID image)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaPutImage(VADriverContextP ctx, VASurfaceID surface, VAImageID image,
             int src_x, int src_y, unsigned int src_width, unsigned int src_height,
             int dest_x, int dest_y, unsigned int dest_width, unsigned int dest_height)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}
