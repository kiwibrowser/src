/*
 * Copyright (C) 2009 Francisco Jerez.
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

#include "nouveau_driver.h"
#include "nouveau_bufferobj.h"
#include "nouveau_context.h"

#include "main/bufferobj.h"

static inline char *
get_bufferobj_map(struct gl_context *ctx, struct gl_buffer_object *obj,
		  unsigned flags)
{
	struct nouveau_bufferobj *nbo = to_nouveau_bufferobj(obj);
	void *map = NULL;

	if (nbo->sys) {
		map = nbo->sys;
	} else if (nbo->bo) {
		nouveau_bo_map(nbo->bo, flags, context_client(ctx));
		map = nbo->bo->map;
	}

	return map;
}

static struct gl_buffer_object *
nouveau_bufferobj_new(struct gl_context *ctx, GLuint buffer, GLenum target)
{
	struct nouveau_bufferobj *nbo;

	nbo = CALLOC_STRUCT(nouveau_bufferobj);
	if (!nbo)
		return NULL;

	_mesa_initialize_buffer_object(ctx, &nbo->base, buffer, target);

	return &nbo->base;
}

static void
nouveau_bufferobj_del(struct gl_context *ctx, struct gl_buffer_object *obj)
{
	struct nouveau_bufferobj *nbo = to_nouveau_bufferobj(obj);

	nouveau_bo_ref(NULL, &nbo->bo);
	FREE(nbo->sys);
	FREE(nbo);
}

static GLboolean
nouveau_bufferobj_data(struct gl_context *ctx, GLenum target, GLsizeiptrARB size,
		       const GLvoid *data, GLenum usage,
		       struct gl_buffer_object *obj)
{
	struct nouveau_bufferobj *nbo = to_nouveau_bufferobj(obj);
	int ret;

	obj->Size = size;
	obj->Usage = usage;

	/* Free previous storage */
	nouveau_bo_ref(NULL, &nbo->bo);
	FREE(nbo->sys);

	if (target == GL_ELEMENT_ARRAY_BUFFER_ARB ||
	    (size < 512 && usage == GL_DYNAMIC_DRAW_ARB) ||
	    context_chipset(ctx) < 0x10) {
		/* Heuristic: keep it in system ram */
		nbo->sys = MALLOC(size);

	} else {
		/* Get a hardware BO */
		ret = nouveau_bo_new(context_dev(ctx),
				     NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
				     size, NULL, &nbo->bo);
		assert(!ret);
	}

	if (data)
		memcpy(get_bufferobj_map(ctx, obj, NOUVEAU_BO_WR), data, size);

	return GL_TRUE;
}

static void
nouveau_bufferobj_subdata(struct gl_context *ctx, GLintptrARB offset,
			  GLsizeiptrARB size, const GLvoid *data,
			  struct gl_buffer_object *obj)
{
	memcpy(get_bufferobj_map(ctx, obj, NOUVEAU_BO_WR) + offset, data, size);
}

static void
nouveau_bufferobj_get_subdata(struct gl_context *ctx, GLintptrARB offset,
			   GLsizeiptrARB size, GLvoid *data,
			   struct gl_buffer_object *obj)
{
	memcpy(data, get_bufferobj_map(ctx, obj, NOUVEAU_BO_RD) + offset, size);
}

static void *
nouveau_bufferobj_map_range(struct gl_context *ctx, GLintptr offset,
			    GLsizeiptr length, GLbitfield access,
			    struct gl_buffer_object *obj)
{
	unsigned flags = 0;
	char *map;

	assert(!obj->Pointer);

	if (!(access & GL_MAP_UNSYNCHRONIZED_BIT)) {
		if (access & GL_MAP_READ_BIT)
			flags |= NOUVEAU_BO_RD;
		if (access & GL_MAP_WRITE_BIT)
			flags |= NOUVEAU_BO_WR;
	}

	map = get_bufferobj_map(ctx, obj, flags);
	if (!map)
		return NULL;

	obj->Pointer = map + offset;
	obj->Offset = offset;
	obj->Length = length;
	obj->AccessFlags = access;

	return obj->Pointer;
}

static GLboolean
nouveau_bufferobj_unmap(struct gl_context *ctx, struct gl_buffer_object *obj)
{
	assert(obj->Pointer);

	obj->Pointer = NULL;
	obj->Offset = 0;
	obj->Length = 0;
	obj->AccessFlags = 0;

	return GL_TRUE;
}

void
nouveau_bufferobj_functions_init(struct dd_function_table *functions)
{
	functions->NewBufferObject = nouveau_bufferobj_new;
	functions->DeleteBuffer	= nouveau_bufferobj_del;
	functions->BufferData = nouveau_bufferobj_data;
	functions->BufferSubData = nouveau_bufferobj_subdata;
	functions->GetBufferSubData = nouveau_bufferobj_get_subdata;
	functions->MapBufferRange = nouveau_bufferobj_map_range;
	functions->UnmapBuffer = nouveau_bufferobj_unmap;
}
