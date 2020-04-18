/**************************************************************************
 * 
 * Copyright 2005 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef INTEL_BUFFEROBJ_H
#define INTEL_BUFFEROBJ_H

#include "main/mtypes.h"

struct intel_context;
struct gl_buffer_object;


/**
 * Intel vertex/pixel buffer object, derived from Mesa's gl_buffer_object.
 */
struct intel_buffer_object
{
   struct gl_buffer_object Base;
   drm_intel_bo *buffer;     /* the low-level buffer manager's buffer handle */
   GLuint offset;            /* any offset into that buffer */

   /** System memory buffer data, if not using a BO to store the data. */
   void *sys_buffer;

   drm_intel_bo *range_map_bo;
   void *range_map_buffer;
   unsigned int range_map_offset;
   GLsizei range_map_size;

   bool source;
};


/* Get the bm buffer associated with a GL bufferobject:
 */
drm_intel_bo *intel_bufferobj_buffer(struct intel_context *intel,
				     struct intel_buffer_object *obj,
				     GLuint flag);
drm_intel_bo *intel_bufferobj_source(struct intel_context *intel,
				     struct intel_buffer_object *obj,
				     GLuint align,
				     GLuint *offset);

void intel_upload_data(struct intel_context *intel,
		       const void *ptr, GLuint size, GLuint align,
		       drm_intel_bo **return_bo,
		       GLuint *return_offset);

void *intel_upload_map(struct intel_context *intel,
		       GLuint size, GLuint align);
void intel_upload_unmap(struct intel_context *intel,
			const void *ptr, GLuint size, GLuint align,
			drm_intel_bo **return_bo,
			GLuint *return_offset);

void intel_upload_finish(struct intel_context *intel);

/* Hook the bufferobject implementation into mesa:
 */
void intelInitBufferObjectFuncs(struct dd_function_table *functions);

static inline struct intel_buffer_object *
intel_buffer_object(struct gl_buffer_object *obj)
{
   return (struct intel_buffer_object *) obj;
}

#endif
