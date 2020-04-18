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

#include "va_private.h"

typedef struct  {
   enum pipe_format pipe_format;
   VAImageFormat    va_format;
   unsigned int     va_flags;
} va_subpicture_formats_supported_t;

static const va_subpicture_formats_supported_t va_subpicture_formats_supported[VA_MAX_SUBPIC_FORMATS_SUPPORTED + 1] = 
{
   { PIPE_FORMAT_B8G8R8A8_UNORM,
      { VA_FOURCC('B','G','R','A'), VA_LSB_FIRST, 32, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 },
      0 },
   { PIPE_FORMAT_R8G8B8A8_UNORM, 
      { VA_FOURCC_RGBA, VA_LSB_FIRST, 32, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 },
      0 }
};

VAStatus
vlVaQuerySubpictureFormats(VADriverContextP ctx, VAImageFormat *format_list,
                           unsigned int *flags, unsigned int *num_formats)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;
		
   if (!(format_list && flags && num_formats))
      return VA_STATUS_ERROR_UNKNOWN;
		
   num_formats[0] = VA_MAX_SUBPIC_FORMATS_SUPPORTED;
		
   int n = 0;
   /* Query supported formats */
   for (n = 0; n < VA_MAX_SUBPIC_FORMATS_SUPPORTED ; n++) {
      const va_subpicture_formats_supported_t * const format_map = &va_subpicture_formats_supported[n];
      flags[n] = format_map->va_flags;
      format_list[n] = format_map->va_format;
   }

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaCreateSubpicture(VADriverContextP ctx, VAImageID image, VASubpictureID *subpicture)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaDestroySubpicture(VADriverContextP ctx, VASubpictureID subpicture)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaSubpictureImage(VADriverContextP ctx, VASubpictureID subpicture, VAImageID image)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaSetSubpictureChromakey(VADriverContextP ctx, VASubpictureID subpicture,
                           unsigned int chromakey_min, unsigned int chromakey_max, unsigned int chromakey_mask)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaSetSubpictureGlobalAlpha(VADriverContextP ctx, VASubpictureID subpicture, float global_alpha)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaAssociateSubpicture(VADriverContextP ctx, VASubpictureID subpicture, VASurfaceID *target_surfaces,
                        int num_surfaces, short src_x, short src_y,
                        unsigned short src_width, unsigned short src_height,
                        short dest_x, short dest_y,
                        unsigned short dest_width,
                        unsigned short dest_height,
                        unsigned int flags)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaDeassociateSubpicture(VADriverContextP ctx, VASubpictureID subpicture,
                          VASurfaceID *target_surfaces, int num_surfaces)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}
