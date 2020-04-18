/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009  VMware, Inc.
 * Copyright (c) 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef GLFORMATS_H
#define GLFORMATS_H


#include <GL/gl.h>


#ifdef __cplusplus
extern "C" {
#endif

extern GLboolean
_mesa_type_is_packed(GLenum type);

extern GLint
_mesa_sizeof_type( GLenum type );

extern GLint
_mesa_sizeof_packed_type( GLenum type );

extern GLint
_mesa_components_in_format( GLenum format );

extern GLint
_mesa_bytes_per_pixel( GLenum format, GLenum type );

extern GLboolean
_mesa_is_type_integer(GLenum type);

extern GLboolean
_mesa_is_type_unsigned(GLenum type);

extern GLboolean
_mesa_is_enum_format_integer(GLenum format);

extern GLboolean
_mesa_is_enum_format_or_type_integer(GLenum format, GLenum type);

extern GLboolean
_mesa_is_color_format(GLenum format);

extern GLboolean
_mesa_is_depth_format(GLenum format);

extern GLboolean
_mesa_is_stencil_format(GLenum format);

extern GLboolean
_mesa_is_ycbcr_format(GLenum format);

extern GLboolean
_mesa_is_depthstencil_format(GLenum format);

extern GLboolean
_mesa_is_depth_or_stencil_format(GLenum format);

extern GLboolean
_mesa_is_dudv_format(GLenum format);

extern GLboolean
_mesa_is_compressed_format(struct gl_context *ctx, GLenum format);

extern GLenum
_mesa_base_format_to_integer_format(GLenum format);

extern GLboolean
_mesa_base_format_has_channel(GLenum base_format, GLenum pname);

extern GLenum
_mesa_generic_compressed_format_to_uncompressed_format(GLenum format);

extern GLenum
_mesa_error_check_format_and_type(const struct gl_context *ctx,
                                  GLenum format, GLenum type);


#ifdef __cplusplus
}
#endif

#endif /* GLFORMATS_H */
