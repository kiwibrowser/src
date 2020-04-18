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

#ifndef _INTEL_TEX_OBJ_H
#define _INTEL_TEX_OBJ_H

#include "swrast/s_context.h"


struct intel_texture_object
{
   struct gl_texture_object base;

   /* This is a mirror of base._MaxLevel, updated at validate time,
    * except that we don't bother with the non-base levels for
    * non-mipmapped textures.
    */
   unsigned int _MaxLevel;

   /* Offset for firstLevel image:
    */
   GLuint textureOffset;

   /* On validation any active images held in main memory or in other
    * regions will be copied to this region and the old storage freed.
    */
   struct intel_mipmap_tree *mt;
};


/**
 * intel_texture_image is a subclass of swrast_texture_image because we
 * sometimes fall back to using the swrast module for software rendering.
 */
struct intel_texture_image
{
   struct swrast_texture_image base;

   /* If intelImage->mt != NULL, image data is stored here.
    * Else if intelImage->base.Buffer != NULL, image is stored there.
    * Else there is no image data.
    */
   struct intel_mipmap_tree *mt;
};

static INLINE struct intel_texture_object *
intel_texture_object(struct gl_texture_object *obj)
{
   return (struct intel_texture_object *) obj;
}

static INLINE struct intel_texture_image *
intel_texture_image(struct gl_texture_image *img)
{
   return (struct intel_texture_image *) img;
}

#endif /* _INTEL_TEX_OBJ_H */
