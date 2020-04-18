/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

/* Code to layout images in a mipmap tree for i965.
 */

#include "intel_mipmap_tree.h"
#include "intel_tex_layout.h"
#include "intel_context.h"
#include "main/macros.h"

#define FILE_DEBUG_FLAG DEBUG_MIPTREE

static void
brw_miptree_layout_texture_array(struct intel_context *intel,
				 struct intel_mipmap_tree *mt)
{
   GLuint level;
   GLuint qpitch = 0;
   int h0, h1, q;

   h0 = ALIGN(mt->height0, mt->align_h);
   h1 = ALIGN(minify(mt->height0), mt->align_h);
   if (mt->array_spacing_lod0)
      qpitch = h0;
   else
      qpitch = (h0 + h1 + (intel->gen >= 7 ? 12 : 11) * mt->align_h);
   if (mt->compressed)
      qpitch /= 4;

   i945_miptree_layout_2d(mt);

   for (level = mt->first_level; level <= mt->last_level; level++) {
      for (q = 0; q < mt->depth0; q++) {
	 intel_miptree_set_image_offset(mt, level, q, 0, q * qpitch);
      }
   }
   mt->total_height = qpitch * mt->depth0;
}

void
brw_miptree_layout(struct intel_context *intel, struct intel_mipmap_tree *mt)
{
   switch (mt->target) {
   case GL_TEXTURE_CUBE_MAP:
      if (intel->gen >= 5) {
	 /* On Ironlake, cube maps are finally represented as just a series of
	  * MIPLAYOUT_BELOW 2D textures (like 2D texture arrays), separated by a
	  * pitch of qpitch rows, where qpitch is defined by the equation given
	  * in Volume 1 of the BSpec.
	  */
	 brw_miptree_layout_texture_array(intel, mt);
	 break;
      }
      assert(mt->depth0 == 6);
      /* FALLTHROUGH */

   case GL_TEXTURE_3D: {
      GLuint width  = mt->width0;
      GLuint height = mt->height0;
      GLuint depth = mt->depth0;
      GLuint pack_x_pitch, pack_x_nr;
      GLuint pack_y_pitch;
      GLuint level;

      mt->total_height = 0;

      if (mt->compressed) {
          mt->total_width = ALIGN(width, mt->align_w);
          pack_y_pitch = (height + 3) / 4;
      } else {
	 mt->total_width = mt->width0;
	 pack_y_pitch = ALIGN(mt->height0, mt->align_h);
      }

      pack_x_pitch = width;
      pack_x_nr = 1;

      for (level = mt->first_level ; level <= mt->last_level ; level++) {
	 GLint x = 0;
	 GLint y = 0;
	 GLint q, j;

	 intel_miptree_set_level_info(mt, level,
				      0, mt->total_height,
				      width, height, depth);

	 for (q = 0; q < depth; /* empty */) {
	    for (j = 0; j < pack_x_nr && q < depth; j++, q++) {
	       intel_miptree_set_image_offset(mt, level, q, x, y);
	       x += pack_x_pitch;
	    }
            if (x > mt->total_width)
               mt->total_width = x;

	    x = 0;
	    y += pack_y_pitch;
	 }


	 mt->total_height += y;
	 width  = minify(width);
	 height = minify(height);
	 if (mt->target == GL_TEXTURE_3D)
	    depth = minify(depth);

	 if (mt->compressed) {
	    pack_y_pitch = (height + 3) / 4;

	    if (pack_x_pitch > ALIGN(width, mt->align_w)) {
	       pack_x_pitch = ALIGN(width, mt->align_w);
	       pack_x_nr <<= 1;
	    }
	 } else {
            pack_x_nr <<= 1;
	    if (pack_x_pitch > 4) {
	       pack_x_pitch >>= 1;
	    }

	    if (pack_y_pitch > 2) {
	       pack_y_pitch >>= 1;
	       pack_y_pitch = ALIGN(pack_y_pitch, mt->align_h);
	    }
	 }

      }
      /* The 965's sampler lays cachelines out according to how accesses
       * in the texture surfaces run, so they may be "vertical" through
       * memory.  As a result, the docs say in Surface Padding Requirements:
       * Sampling Engine Surfaces that two extra rows of padding are required.
       */
      if (mt->target == GL_TEXTURE_CUBE_MAP)
	 mt->total_height += 2;
      break;
   }

   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_1D_ARRAY:
      brw_miptree_layout_texture_array(intel, mt);
      break;

   default:
      switch (mt->msaa_layout) {
      case INTEL_MSAA_LAYOUT_UMS:
      case INTEL_MSAA_LAYOUT_CMS:
         brw_miptree_layout_texture_array(intel, mt);
         break;
      case INTEL_MSAA_LAYOUT_NONE:
      case INTEL_MSAA_LAYOUT_IMS:
         i945_miptree_layout_2d(mt);
         break;
      }
      break;
   }
   DBG("%s: %dx%dx%d\n", __FUNCTION__,
       mt->total_width, mt->total_height, mt->cpp);
}

