/*
 * Copyright (C) 2008 Nicolai Haehnle.
 * Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.
 *
 * The Weather Channel (TM) funded Tungsten Graphics to develop the
 * initial release of the Radeon 8500 driver under the XFree86 license.
 * This notice must be preserved.
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

#ifndef RADEON_TEXTURE_H
#define RADEON_TEXTURE_H

#include "main/formats.h"
#include "main/mfeatures.h"

extern gl_format _radeon_texformat_rgba8888;
extern gl_format _radeon_texformat_argb8888;
extern gl_format _radeon_texformat_rgb565;
extern gl_format _radeon_texformat_argb4444;
extern gl_format _radeon_texformat_argb1555;
extern gl_format _radeon_texformat_al88;

extern 
void copy_rows(void* dst, GLuint dststride, const void* src, GLuint srcstride,
	GLuint numrows, GLuint rowsize);
struct gl_texture_image *radeonNewTextureImage(struct gl_context *ctx);
void radeonFreeTextureImageBuffer(struct gl_context *ctx, struct gl_texture_image *timage);

void radeon_teximage_map(radeon_texture_image *image, GLboolean write_enable);
void radeon_teximage_unmap(radeon_texture_image *image);
int radeon_validate_texture_miptree(struct gl_context * ctx,
				    struct gl_sampler_object *samp,
				    struct gl_texture_object *texObj);


void radeon_swrast_map_texture_images(struct gl_context *ctx, struct gl_texture_object *texObj);
void radeon_swrast_unmap_texture_images(struct gl_context *ctx, struct gl_texture_object *texObj);

gl_format radeonChooseTextureFormat_mesa(struct gl_context * ctx,
                                         GLenum target,
                                         GLint internalFormat,
                                         GLenum format,
                                         GLenum type);

gl_format radeonChooseTextureFormat(struct gl_context * ctx,
                                    GLint internalFormat,
                                    GLenum format,
                                    GLenum type, GLboolean fbo);

void radeonCopyTexSubImage(struct gl_context *ctx, GLuint dims,
                           struct gl_texture_image *texImage,
                           GLint xoffset, GLint yoffset, GLint zoffset,
                           struct gl_renderbuffer *rb,
                           GLint x, GLint y,
                           GLsizei width, GLsizei height);

unsigned radeonIsFormatRenderable(gl_format mesa_format);

#if FEATURE_OES_EGL_image
void radeon_image_target_texture_2d(struct gl_context *ctx, GLenum target,
				    struct gl_texture_object *texObj,
				    struct gl_texture_image *texImage,
				    GLeglImageOES image_handle);
#endif

void
radeon_init_common_texture_funcs(radeonContextPtr radeon,
				 struct dd_function_table *functions);

#endif
