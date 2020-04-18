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

#ifndef INTELTEX_INC
#define INTELTEX_INC

#include "main/mtypes.h"
#include "main/formats.h"
#include "intel_context.h"

struct intel_renderbuffer;

void intelInitTextureFuncs(struct dd_function_table *functions);

void intelInitTextureImageFuncs(struct dd_function_table *functions);

void intelInitTextureSubImageFuncs(struct dd_function_table *functions);

void intelInitTextureCopyImageFuncs(struct dd_function_table *functions);

GLenum intel_mesa_format_to_rb_datatype(gl_format format);

void intelSetTexBuffer(__DRIcontext *pDRICtx,
		       GLint target, __DRIdrawable *pDraw);
void intelSetTexBuffer2(__DRIcontext *pDRICtx,
			GLint target, GLint format, __DRIdrawable *pDraw);

struct intel_mipmap_tree *
intel_miptree_create_for_teximage(struct intel_context *intel,
				  struct intel_texture_object *intelObj,
				  struct intel_texture_image *intelImage,
				  bool expect_accelerated_upload);

GLuint intel_finalize_mipmap_tree(struct intel_context *intel, GLuint unit);

void intel_tex_map_level_images(struct intel_context *intel,
				struct intel_texture_object *intelObj,
				int level,
				GLbitfield mode);

void intel_tex_unmap_level_images(struct intel_context *intel,
				  struct intel_texture_object *intelObj,
				  int level);

void intel_tex_map_images(struct intel_context *intel,
                          struct intel_texture_object *intelObj,
                          GLbitfield mode);

void intel_tex_unmap_images(struct intel_context *intel,
                            struct intel_texture_object *intelObj);
bool
intel_tex_image_s8z24_create_renderbuffers(struct intel_context *intel,
					   struct intel_texture_image *image);

int intel_compressed_num_bytes(GLuint mesaFormat);

bool intel_copy_texsubimage(struct intel_context *intel,
                            struct intel_texture_image *intelImage,
                            GLint dstx, GLint dsty,
                            struct intel_renderbuffer *irb,
                            GLint x, GLint y,
                            GLsizei width, GLsizei height);

#endif
