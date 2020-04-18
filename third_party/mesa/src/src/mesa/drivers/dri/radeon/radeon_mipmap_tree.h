/*
 * Copyright (C) 2008 Nicolai Haehnle.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __RADEON_MIPMAP_TREE_H_
#define __RADEON_MIPMAP_TREE_H_

#include "radeon_common.h"

typedef struct _radeon_mipmap_tree radeon_mipmap_tree;
typedef struct _radeon_mipmap_level radeon_mipmap_level;
typedef struct _radeon_mipmap_image radeon_mipmap_image;

struct _radeon_mipmap_image {
	GLuint offset; /** Offset of this image from the start of mipmap tree buffer, in bytes */
};

struct _radeon_mipmap_level {
	GLuint width;
	GLuint height;
	GLuint depth;
	GLuint size; /** Size of each image, in bytes */
	GLuint rowstride; /** in bytes */
	GLuint valid;
	radeon_mipmap_image faces[6];
};

/* store the max possible in the miptree */
#define RADEON_MIPTREE_MAX_TEXTURE_LEVELS 15

/**
 * A mipmap tree contains texture images in the layout that the hardware
 * expects.
 *
 * The meta-data of mipmap trees is immutable, i.e. you cannot change the
 * layout on-the-fly; however, the texture contents (i.e. texels) can be
 * changed.
 */
struct _radeon_mipmap_tree {
	struct radeon_bo *bo;
	GLuint refcount;

	GLuint totalsize; /** total size of the miptree, in bytes */

	GLenum target; /** GL_TEXTURE_xxx */
	GLenum mesaFormat; /** MESA_FORMAT_xxx */
	GLuint faces; /** # of faces: 6 for cubemaps, 1 otherwise */
	GLuint baseLevel; /** gl_texture_object->baseLevel it was created for */
	GLuint numLevels; /** Number of mip levels stored in this mipmap tree */

	GLuint width0; /** Width of baseLevel image */
	GLuint height0; /** Height of baseLevel image */
	GLuint depth0; /** Depth of baseLevel image */

	GLuint tilebits; /** RADEON_TXO_xxx_TILE */

	radeon_mipmap_level levels[RADEON_MIPTREE_MAX_TEXTURE_LEVELS];
};

void radeon_miptree_reference(radeon_mipmap_tree *mt, radeon_mipmap_tree **ptr);
void radeon_miptree_unreference(radeon_mipmap_tree **ptr);

GLboolean radeon_miptree_matches_image(radeon_mipmap_tree *mt,
				       struct gl_texture_image *texImage);
				       
void radeon_try_alloc_miptree(radeonContextPtr rmesa, radeonTexObj *t);
GLuint radeon_miptree_image_offset(radeon_mipmap_tree *mt,
				   GLuint face, GLuint level);
uint32_t get_base_teximage_offset(radeonTexObj *texObj);

unsigned get_texture_image_row_stride(radeonContextPtr rmesa, gl_format format, unsigned width, unsigned tiling, unsigned target);

unsigned get_texture_image_size(
		gl_format format,
		unsigned rowStride,
		unsigned height,
		unsigned depth,
		unsigned tiling);

radeon_mipmap_tree *radeon_miptree_create(radeonContextPtr rmesa,
					  GLenum target, gl_format mesaFormat, GLuint baseLevel, GLuint numLevels,
					  GLuint width0, GLuint height0, GLuint depth0, GLuint tilebits);
#endif /* __RADEON_MIPMAP_TREE_H_ */
