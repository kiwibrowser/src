/*
 * Copyright (C) 2009-2010 Francisco Jerez.
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

#include "main/bufferobj.h"
#include "nouveau_driver.h"
#include "nouveau_array.h"
#include "nouveau_bufferobj.h"
#include "nouveau_context.h"

#define EXTRACT(in_t, out_t) extract_func_##in_t##_to_##out_t

#define EXTRACT_FUNC(in_t, out_t, k)			\
static out_t EXTRACT(in_t, out_t)			\
(struct nouveau_array *a, int i, int j) {		\
	in_t x = ((in_t *)(a->buf + i * a->stride))[j];	\
							\
	return (out_t)x / (k);				\
}

EXTRACT_FUNC(GLchar, unsigned, 1);
EXTRACT_FUNC(GLchar, float, SCHAR_MAX);
EXTRACT_FUNC(GLubyte, unsigned, 1);
EXTRACT_FUNC(GLubyte, float, UCHAR_MAX);
EXTRACT_FUNC(GLshort, unsigned, 1);
EXTRACT_FUNC(GLshort, float, SHRT_MAX);
EXTRACT_FUNC(GLushort, unsigned, 1);
EXTRACT_FUNC(GLushort, float, USHRT_MAX);
EXTRACT_FUNC(GLint, unsigned, 1);
EXTRACT_FUNC(GLint, float, INT_MAX);
EXTRACT_FUNC(GLuint, unsigned, 1);
EXTRACT_FUNC(GLuint, float, UINT_MAX);
EXTRACT_FUNC(GLfloat, unsigned, 1.0 / UINT_MAX);
EXTRACT_FUNC(GLfloat, float, 1);

#undef EXTRACT_FUNC

static void
get_array_extract(struct nouveau_array *a, extract_u_t *extract_u,
		  extract_f_t *extract_f)
{
	switch (a->type) {
	case GL_BYTE:
		*extract_u = EXTRACT(GLchar, unsigned);
		*extract_f = EXTRACT(GLchar, float);
		break;
	case GL_UNSIGNED_BYTE:
		*extract_u = EXTRACT(GLubyte, unsigned);
		*extract_f = EXTRACT(GLubyte, float);
		break;
	case GL_SHORT:
		*extract_u = EXTRACT(GLshort, unsigned);
		*extract_f = EXTRACT(GLshort, float);
		break;
	case GL_UNSIGNED_SHORT:
		*extract_u = EXTRACT(GLushort, unsigned);
		*extract_f = EXTRACT(GLushort, float);
		break;
	case GL_INT:
		*extract_u = EXTRACT(GLint, unsigned);
		*extract_f = EXTRACT(GLint, float);
		break;
	case GL_UNSIGNED_INT:
		*extract_u = EXTRACT(GLuint, unsigned);
		*extract_f = EXTRACT(GLuint, float);
		break;
	case GL_FLOAT:
		*extract_u = EXTRACT(GLfloat, unsigned);
		*extract_f = EXTRACT(GLfloat, float);
		break;
	default:
		assert(0);
	}
}
#undef EXTRACT

void
nouveau_init_array(struct nouveau_array *a, int attr, int stride,
		   int fields, int type, struct gl_buffer_object *obj,
		   const void *ptr, GLboolean map, struct gl_context *ctx)
{
	struct nouveau_client *client = context_client(ctx);

	a->attr = attr;
	a->stride = stride;
	a->fields = fields;
	a->type = type;
	a->buf = NULL;

	if (obj) {
		if (nouveau_bufferobj_hw(obj)) {
			struct nouveau_bufferobj *nbo =
				to_nouveau_bufferobj(obj);

			nouveau_bo_ref(nbo->bo, &a->bo);
			a->offset = (intptr_t)ptr;

			if (map) {
				nouveau_bo_map(a->bo, NOUVEAU_BO_RD, client);
				a->buf = a->bo->map + a->offset;
			}

		} else {
			nouveau_bo_ref(NULL, &a->bo);
			a->offset = 0;

			if (map)
				a->buf = ADD_POINTERS(
					nouveau_bufferobj_sys(obj), ptr);
		}
	}

	if (a->buf)
		get_array_extract(a, &a->extract_u, &a->extract_f);
}

void
nouveau_deinit_array(struct nouveau_array *a)
{
	a->buf = NULL;
	a->fields = 0;
}

void
nouveau_cleanup_array(struct nouveau_array *a)
{
	nouveau_deinit_array(a);
	nouveau_bo_ref(NULL, &a->bo);
}
