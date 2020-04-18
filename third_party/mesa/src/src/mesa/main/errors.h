/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file errors.h
 * Mesa debugging and error handling functions.
 *
 * This file provides functions to record errors, warnings, and miscellaneous
 * debug information.
 */


#ifndef ERRORS_H
#define ERRORS_H


#include "compiler.h"
#include "glheader.h"


#ifdef __cplusplus
extern "C" {
#endif

struct _glapi_table;
struct gl_context;

extern void
_mesa_init_errors_dispatch(struct _glapi_table *disp);

extern void
_mesa_init_errors( struct gl_context *ctx );

extern void
_mesa_free_errors_data( struct gl_context *ctx );

extern void
_mesa_warning( struct gl_context *gc, const char *fmtString, ... ) PRINTFLIKE(2, 3);

extern void
_mesa_problem( const struct gl_context *ctx, const char *fmtString, ... ) PRINTFLIKE(2, 3);

extern void
_mesa_error( struct gl_context *ctx, GLenum error, const char *fmtString, ... ) PRINTFLIKE(3, 4);

extern void
_mesa_debug( const struct gl_context *ctx, const char *fmtString, ... ) PRINTFLIKE(2, 3);

extern void
_mesa_shader_debug( struct gl_context *ctx, GLenum type, GLuint id, const char *msg, int len );

#ifdef __cplusplus
}
#endif


#endif /* ERRORS_H */
