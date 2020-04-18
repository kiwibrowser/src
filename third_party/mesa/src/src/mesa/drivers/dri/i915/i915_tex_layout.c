/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/** @file i915_tex_layout.c
 * Code to layout images in a mipmap tree for i830M-GM915 and G945 and beyond.
 */

#include "intel_mipmap_tree.h"
#include "intel_tex_layout.h"
#include "main/macros.h"
#include "intel_context.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static GLint initial_offsets[6][2] = {
   [FACE_POS_X] = {0, 0},
   [FACE_POS_Y] = {1, 0},
   [FACE_POS_Z] = {1, 1},
   [FACE_NEG_X] = {0, 2},
   [FACE_NEG_Y] = {1, 2},
   [FACE_NEG_Z] = {1, 3},
};


static GLint step_offsets[6][2] = {
   [FACE_POS_X] = {0, 2},
   [FACE_POS_Y] = {-1, 2},
   [FACE_POS_Z] = {-1, 1},
   [FACE_NEG_X] = {0, 2},
   [FACE_NEG_Y] = {-1, 2},
   [FACE_NEG_Z] = {-1, 1},
};


static GLint bottom_offsets[6] = {
   [FACE_POS_X] = 16 + 0 * 8,
   [FACE_POS_Y] = 16 + 1 * 8,
   [FACE_POS_Z] = 16 + 2 * 8,
   [FACE_NEG_X] = 16 + 3 * 8,
   [FACE_NEG_Y] = 16 + 4 * 8,
   [FACE_NEG_Z] = 16 + 5 * 8,
};


/**
 * Cube texture map layout for i830M-GM915 and
 * non-compressed cube texture map on GM945.
 *
 * Hardware layout looks like:
 *
 * +-------+-------+
 * |       |       |
 * |       |       |
 * |       |       |
 * |  +x   |  +y   |
 * |       |       |
 * |       |       |
 * |       |       |
 * |       |       |
 * +---+---+-------+
 * |   |   |       |
 * | +x| +y|       |
 * |   |   |       |
 * |   |   |       |
 * +-+-+---+  +z   |
 * | | |   |       |
 * +-+-+ +z|       |
 *   | |   |       |
 * +-+-+---+-------+
 * |       |       |
 * |       |       |
 * |       |       |
 * |  -x   |  -y   |
 * |       |       |
 * |       |       |
 * |       |       |
 * |       |       |
 * +---+---+-------+
 * |   |   |       |
 * | -x| -y|       |
 * |   |   |       |
 * |   |   |       |
 * +-+-+---+  -z   |
 * | | |   |       |
 * +-+-+ -z|       |
 *   | |   |       |
 *   +-+---+-------+
 *
 */
static void
i915_miptree_layout_cube(struct intel_mipmap_tree * mt)
{
   const GLuint dim = mt->width0;
   GLuint face;
   GLuint lvlWidth = mt->width0, lvlHeight = mt->height0;
   GLint level;

   assert(lvlWidth == lvlHeight); /* cubemap images are square */

   /* double pitch for cube layouts */
   mt->total_width = dim * 2;
   mt->total_height = dim * 4;

   for (level = mt->first_level; level <= mt->last_level; level++) {
      intel_miptree_set_level_info(mt, level,
				   0, 0,
				   lvlWidth, lvlHeight,
				   6);
      lvlWidth /= 2;
      lvlHeight /= 2;
   }

   for (face = 0; face < 6; face++) {
      GLuint x = initial_offsets[face][0] * dim;
      GLuint y = initial_offsets[face][1] * dim;
      GLuint d = dim;

      for (level = mt->first_level; level <= mt->last_level; level++) {
	 intel_miptree_set_image_offset(mt, level, face, x, y);

	 if (d == 0)
	    printf("cube mipmap %d/%d (%d..%d) is 0x0\n",
		   face, level, mt->first_level, mt->last_level);

	 d >>= 1;
	 x += step_offsets[face][0] * d;
	 y += step_offsets[face][1] * d;
      }
   }
}

static void
i915_miptree_layout_3d(struct intel_mipmap_tree * mt)
{
   GLuint width = mt->width0;
   GLuint height = mt->height0;
   GLuint depth = mt->depth0;
   GLuint stack_height = 0;
   GLint level;

   /* Calculate the size of a single slice. */
   mt->total_width = mt->width0;

   /* XXX: hardware expects/requires 9 levels at minimum. */
   for (level = mt->first_level; level <= MAX2(8, mt->last_level); level++) {
      intel_miptree_set_level_info(mt, level, 0, mt->total_height,
				   width, height, depth);

      stack_height += MAX2(2, height);

      width = minify(width);
      height = minify(height);
      depth = minify(depth);
   }

   /* Fixup depth image_offsets: */
   depth = mt->depth0;
   for (level = mt->first_level; level <= mt->last_level; level++) {
      GLuint i;
      for (i = 0; i < depth; i++) {
	 intel_miptree_set_image_offset(mt, level, i,
					0, i * stack_height);
      }

      depth = minify(depth);
   }

   /* Multiply slice size by texture depth for total size.  It's
    * remarkable how wasteful of memory the i915 texture layouts
    * are.  They are largely fixed in the i945.
    */
   mt->total_height = stack_height * mt->depth0;
}

static void
i915_miptree_layout_2d(struct intel_mipmap_tree * mt)
{
   GLuint width = mt->width0;
   GLuint height = mt->height0;
   GLuint img_height;
   GLint level;

   mt->total_width = mt->width0;
   mt->total_height = 0;

   for (level = mt->first_level; level <= mt->last_level; level++) {
      intel_miptree_set_level_info(mt, level,
				   0, mt->total_height,
				   width, height, 1);

      if (mt->compressed)
	 img_height = ALIGN(height, 4) / 4;
      else
	 img_height = ALIGN(height, 2);

      mt->total_height += img_height;

      width = minify(width);
      height = minify(height);
   }
}

void
i915_miptree_layout(struct intel_mipmap_tree * mt)
{
   switch (mt->target) {
   case GL_TEXTURE_CUBE_MAP:
      i915_miptree_layout_cube(mt);
      break;
   case GL_TEXTURE_3D:
      i915_miptree_layout_3d(mt);
      break;
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE_ARB:
      i915_miptree_layout_2d(mt);
      break;
   default:
      _mesa_problem(NULL, "Unexpected tex target in i915_miptree_layout()");
      break;
   }

   DBG("%s: %dx%dx%d\n", __FUNCTION__,
       mt->total_width, mt->total_height, mt->cpp);
}


/**
 * Compressed cube texture map layout for GM945 and later.
 *
 * The hardware layout looks like the 830-915 layout, except for the small
 * sizes.  A zoomed in view of the layout for 945 is:
 *
 * +-------+-------+
 * |  8x8  |  8x8  |
 * |       |       |
 * |       |       |
 * |  +x   |  +y   |
 * |       |       |
 * |       |       |
 * |       |       |
 * |       |       |
 * +---+---+-------+
 * |4x4|   |  8x8  |
 * | +x|   |       |
 * |   |   |       |
 * |   |   |       |
 * +---+   |  +z   |
 * |4x4|   |       |
 * | +y|   |       |
 * |   |   |       |
 * +---+   +-------+
 *
 * ...
 *
 * +-------+-------+
 * |  8x8  |  8x8  |
 * |       |       |
 * |       |       |
 * |  -x   |  -y   |
 * |       |       |
 * |       |       |
 * |       |       |
 * |       |       |
 * +---+---+-------+
 * |4x4|   |  8x8  |
 * | -x|   |       |
 * |   |   |       |
 * |   |   |       |
 * +---+   |  -z   |
 * |4x4|   |       |
 * | -y|   |       |
 * |   |   |       |
 * +---+   +---+---+---+---+---+---+---+---+---+
 * |4x4|   |4x4|   |2x2|   |2x2|   |2x2|   |2x2|
 * | +z|   | -z|   | +x|   | +y|   | +z|   | -x| ...
 * |   |   |   |   |   |   |   |   |   |   |   |
 * +---+   +---+   +---+   +---+   +---+   +---+
 *
 * The bottom row continues with the remaining 2x2 then the 1x1 mip contents
 * in order, with each of them aligned to a 8x8 block boundary.  Thus, for
 * 32x32 cube maps and smaller, the bottom row layout is going to dictate the
 * pitch of the tree.  For a tree with 4x4 images, the pitch is at least
 * 14 * 8 = 112 texels, for 2x2 it is at least 12 * 8 texels, and for 1x1
 * it is 6 * 8 texels.
 */

static void
i945_miptree_layout_cube(struct intel_mipmap_tree * mt)
{
   const GLuint dim = mt->width0;
   GLuint face;
   GLuint lvlWidth = mt->width0, lvlHeight = mt->height0;
   GLint level;

   assert(lvlWidth == lvlHeight); /* cubemap images are square */

   /* Depending on the size of the largest images, pitch can be
    * determined either by the old-style packing of cubemap faces,
    * or the final row of 4x4, 2x2 and 1x1 faces below this.
    */
   if (dim > 32)
      mt->total_width = dim * 2;
   else
      mt->total_width = 14 * 8;

   if (dim >= 4)
      mt->total_height = dim * 4 + 4;
   else
      mt->total_height = 4;

   /* Set all the levels to effectively occupy the whole rectangular region. */
   for (level = mt->first_level; level <= mt->last_level; level++) {
      intel_miptree_set_level_info(mt, level,
				   0, 0,
				   lvlWidth, lvlHeight, 6);
      lvlWidth /= 2;
      lvlHeight /= 2;
   }

   for (face = 0; face < 6; face++) {
      GLuint x = initial_offsets[face][0] * dim;
      GLuint y = initial_offsets[face][1] * dim;
      GLuint d = dim;

      if (dim == 4 && face >= 4) {
	 y = mt->total_height - 4;
	 x = (face - 4) * 8;
      } else if (dim < 4 && (face > 0 || mt->first_level > 0)) {
	 y = mt->total_height - 4;
	 x = face * 8;
      }

      for (level = mt->first_level; level <= mt->last_level; level++) {
	 intel_miptree_set_image_offset(mt, level, face, x, y);

	 d >>= 1;

	 switch (d) {
	 case 4:
	    switch (face) {
	    case FACE_POS_X:
	    case FACE_NEG_X:
	       x += step_offsets[face][0] * d;
	       y += step_offsets[face][1] * d;
	       break;
	    case FACE_POS_Y:
	    case FACE_NEG_Y:
	       y += 12;
	       x -= 8;
	       break;
	    case FACE_POS_Z:
	    case FACE_NEG_Z:
	       y = mt->total_height - 4;
	       x = (face - 4) * 8;
	       break;
	    }
	    break;

	 case 2:
	    y = mt->total_height - 4;
	    x = bottom_offsets[face];
	    break;

	 case 1:
	    x += 48;
	    break;

	 default:
	    x += step_offsets[face][0] * d;
	    y += step_offsets[face][1] * d;
	    break;
	 }
      }
   }
}

static void
i945_miptree_layout_3d(struct intel_mipmap_tree * mt)
{
   GLuint width = mt->width0;
   GLuint height = mt->height0;
   GLuint depth = mt->depth0;
   GLuint pack_x_pitch, pack_x_nr;
   GLuint pack_y_pitch;
   GLuint level;

   mt->total_width = mt->width0;
   mt->total_height = 0;

   pack_y_pitch = MAX2(mt->height0, 2);
   pack_x_pitch = mt->total_width;
   pack_x_nr = 1;

   for (level = mt->first_level; level <= mt->last_level; level++) {
      GLint x = 0;
      GLint y = 0;
      GLint q, j;

      intel_miptree_set_level_info(mt, level,
				   0, mt->total_height,
				   width, height, depth);

      for (q = 0; q < depth;) {
	 for (j = 0; j < pack_x_nr && q < depth; j++, q++) {
	    intel_miptree_set_image_offset(mt, level, q, x, y);
	    x += pack_x_pitch;
	 }

	 x = 0;
	 y += pack_y_pitch;
      }

      mt->total_height += y;

      if (pack_x_pitch > 4) {
	 pack_x_pitch >>= 1;
	 pack_x_nr <<= 1;
	 assert(pack_x_pitch * pack_x_nr <= mt->total_width);
      }

      if (pack_y_pitch > 2) {
	 pack_y_pitch >>= 1;
      }

      width = minify(width);
      height = minify(height);
      depth = minify(depth);
   }
}

void
i945_miptree_layout(struct intel_mipmap_tree * mt)
{
   switch (mt->target) {
   case GL_TEXTURE_CUBE_MAP:
      if (mt->compressed)
	 i945_miptree_layout_cube(mt);
      else
	 i915_miptree_layout_cube(mt);
      break;
   case GL_TEXTURE_3D:
      i945_miptree_layout_3d(mt);
      break;
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE_ARB:
      i945_miptree_layout_2d(mt);
      break;
   default:
      _mesa_problem(NULL, "Unexpected tex target in i945_miptree_layout()");
      break;
   }

   DBG("%s: %dx%dx%d\n", __FUNCTION__,
       mt->total_width, mt->total_height, mt->cpp);
}
