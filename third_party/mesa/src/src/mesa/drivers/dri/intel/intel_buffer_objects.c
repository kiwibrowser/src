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


#include "main/imports.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/bufferobj.h"

#include "intel_blit.h"
#include "intel_buffer_objects.h"
#include "intel_batchbuffer.h"
#include "intel_context.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"

static GLboolean
intel_bufferobj_unmap(struct gl_context * ctx, struct gl_buffer_object *obj);

/** Allocates a new drm_intel_bo to store the data for the buffer object. */
static void
intel_bufferobj_alloc_buffer(struct intel_context *intel,
			     struct intel_buffer_object *intel_obj)
{
   intel_obj->buffer = drm_intel_bo_alloc(intel->bufmgr, "bufferobj",
					  intel_obj->Base.Size, 64);
}

static void
release_buffer(struct intel_buffer_object *intel_obj)
{
   drm_intel_bo_unreference(intel_obj->buffer);
   intel_obj->buffer = NULL;
   intel_obj->offset = 0;
   intel_obj->source = 0;
}

/**
 * There is some duplication between mesa's bufferobjects and our
 * bufmgr buffers.  Both have an integer handle and a hashtable to
 * lookup an opaque structure.  It would be nice if the handles and
 * internal structure where somehow shared.
 */
static struct gl_buffer_object *
intel_bufferobj_alloc(struct gl_context * ctx, GLuint name, GLenum target)
{
   struct intel_buffer_object *obj = CALLOC_STRUCT(intel_buffer_object);

   _mesa_initialize_buffer_object(ctx, &obj->Base, name, target);

   obj->buffer = NULL;

   return &obj->Base;
}

/**
 * Deallocate/free a vertex/pixel buffer object.
 * Called via glDeleteBuffersARB().
 */
static void
intel_bufferobj_free(struct gl_context * ctx, struct gl_buffer_object *obj)
{
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);

   /* Buffer objects are automatically unmapped when deleting according
    * to the spec, but Mesa doesn't do UnmapBuffer for us at context destroy
    * (though it does if you call glDeleteBuffers)
    */
   if (obj->Pointer)
      intel_bufferobj_unmap(ctx, obj);

   free(intel_obj->sys_buffer);

   drm_intel_bo_unreference(intel_obj->buffer);
   free(intel_obj);
}



/**
 * Allocate space for and store data in a buffer object.  Any data that was
 * previously stored in the buffer object is lost.  If data is NULL,
 * memory will be allocated, but no copy will occur.
 * Called via ctx->Driver.BufferData().
 * \return true for success, false if out of memory
 */
static GLboolean
intel_bufferobj_data(struct gl_context * ctx,
                     GLenum target,
                     GLsizeiptrARB size,
                     const GLvoid * data,
                     GLenum usage, struct gl_buffer_object *obj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   /* Part of the ABI, but this function doesn't use it.
    */
#ifndef I915
   (void) target;
#endif

   intel_obj->Base.Size = size;
   intel_obj->Base.Usage = usage;

   assert(!obj->Pointer); /* Mesa should have unmapped it */

   if (intel_obj->buffer != NULL)
      release_buffer(intel_obj);

   free(intel_obj->sys_buffer);
   intel_obj->sys_buffer = NULL;

   if (size != 0) {
#ifdef I915
      /* On pre-965, stick VBOs in system memory, as we're always doing
       * swtnl with their contents anyway.
       */
      if (target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER) {
	 intel_obj->sys_buffer = malloc(size);
	 if (intel_obj->sys_buffer != NULL) {
	    if (data != NULL)
	       memcpy(intel_obj->sys_buffer, data, size);
	    return true;
	 }
      }
#endif
      intel_bufferobj_alloc_buffer(intel, intel_obj);
      if (!intel_obj->buffer)
         return false;

      if (data != NULL)
	 drm_intel_bo_subdata(intel_obj->buffer, 0, size, data);
   }

   return true;
}


/**
 * Replace data in a subrange of buffer object.  If the data range
 * specified by size + offset extends beyond the end of the buffer or
 * if data is NULL, no copy is performed.
 * Called via glBufferSubDataARB().
 */
static void
intel_bufferobj_subdata(struct gl_context * ctx,
                        GLintptrARB offset,
                        GLsizeiptrARB size,
                        const GLvoid * data, struct gl_buffer_object *obj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);
   bool busy;

   if (size == 0)
      return;

   assert(intel_obj);

   /* If we have a single copy in system memory, update that */
   if (intel_obj->sys_buffer) {
      if (intel_obj->source)
	 release_buffer(intel_obj);

      if (intel_obj->buffer == NULL) {
	 memcpy((char *)intel_obj->sys_buffer + offset, data, size);
	 return;
      }

      free(intel_obj->sys_buffer);
      intel_obj->sys_buffer = NULL;
   }

   /* Otherwise we need to update the copy in video memory. */
   busy =
      drm_intel_bo_busy(intel_obj->buffer) ||
      drm_intel_bo_references(intel->batch.bo, intel_obj->buffer);

   if (busy) {
      if (size == intel_obj->Base.Size) {
	 /* Replace the current busy bo with fresh data. */
	 drm_intel_bo_unreference(intel_obj->buffer);
	 intel_bufferobj_alloc_buffer(intel, intel_obj);
	 drm_intel_bo_subdata(intel_obj->buffer, 0, size, data);
      } else {
         perf_debug("Using a blit copy to avoid stalling on glBufferSubData() "
                    "to a busy buffer object.\n");
	 drm_intel_bo *temp_bo =
	    drm_intel_bo_alloc(intel->bufmgr, "subdata temp", size, 64);

	 drm_intel_bo_subdata(temp_bo, 0, size, data);

	 intel_emit_linear_blit(intel,
				intel_obj->buffer, offset,
				temp_bo, 0,
				size);

	 drm_intel_bo_unreference(temp_bo);
      }
   } else {
      if (unlikely(INTEL_DEBUG & DEBUG_PERF)) {
         if (drm_intel_bo_busy(intel_obj->buffer)) {
            perf_debug("Stalling on the GPU in glBufferSubData().\n");
         }
      }
      drm_intel_bo_subdata(intel_obj->buffer, offset, size, data);
   }
}


/**
 * Called via glGetBufferSubDataARB().
 */
static void
intel_bufferobj_get_subdata(struct gl_context * ctx,
                            GLintptrARB offset,
                            GLsizeiptrARB size,
                            GLvoid * data, struct gl_buffer_object *obj)
{
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);
   struct intel_context *intel = intel_context(ctx);

   assert(intel_obj);
   if (intel_obj->sys_buffer)
      memcpy(data, (char *)intel_obj->sys_buffer + offset, size);
   else {
      if (drm_intel_bo_references(intel->batch.bo, intel_obj->buffer)) {
	 intel_batchbuffer_flush(intel);
      }
      drm_intel_bo_get_subdata(intel_obj->buffer, offset, size, data);
   }
}



/**
 * Called via glMapBufferRange and glMapBuffer
 *
 * The goal of this extension is to allow apps to accumulate their rendering
 * at the same time as they accumulate their buffer object.  Without it,
 * you'd end up blocking on execution of rendering every time you mapped
 * the buffer to put new data in.
 *
 * We support it in 3 ways: If unsynchronized, then don't bother
 * flushing the batchbuffer before mapping the buffer, which can save blocking
 * in many cases.  If we would still block, and they allow the whole buffer
 * to be invalidated, then just allocate a new buffer to replace the old one.
 * If not, and we'd block, and they allow the subrange of the buffer to be
 * invalidated, then we can make a new little BO, let them write into that,
 * and blit it into the real BO at unmap time.
 */
static void *
intel_bufferobj_map_range(struct gl_context * ctx,
			  GLintptr offset, GLsizeiptr length,
			  GLbitfield access, struct gl_buffer_object *obj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);

   /* _mesa_MapBufferRange (GL entrypoint) sets these, but the vbo module also
    * internally uses our functions directly.
    */
   obj->Offset = offset;
   obj->Length = length;
   obj->AccessFlags = access;

   if (intel_obj->sys_buffer) {
      const bool read_only =
	 (access & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)) == GL_MAP_READ_BIT;

      if (!read_only && intel_obj->source)
	 release_buffer(intel_obj);

      if (!intel_obj->buffer || intel_obj->source) {
	 obj->Pointer = intel_obj->sys_buffer + offset;
	 return obj->Pointer;
      }

      free(intel_obj->sys_buffer);
      intel_obj->sys_buffer = NULL;
   }

   if (intel_obj->buffer == NULL) {
      obj->Pointer = NULL;
      return NULL;
   }

   /* If the access is synchronized (like a normal buffer mapping), then get
    * things flushed out so the later mapping syncs appropriately through GEM.
    * If the user doesn't care about existing buffer contents and mapping would
    * cause us to block, then throw out the old buffer.
    *
    * If they set INVALIDATE_BUFFER, we can pitch the current contents to
    * achieve the required synchronization.
    */
   if (!(access & GL_MAP_UNSYNCHRONIZED_BIT)) {
      if (drm_intel_bo_references(intel->batch.bo, intel_obj->buffer)) {
	 if (access & GL_MAP_INVALIDATE_BUFFER_BIT) {
	    drm_intel_bo_unreference(intel_obj->buffer);
	    intel_bufferobj_alloc_buffer(intel, intel_obj);
	 } else {
	    intel_flush(ctx);
	 }
      } else if (drm_intel_bo_busy(intel_obj->buffer) &&
		 (access & GL_MAP_INVALIDATE_BUFFER_BIT)) {
	 drm_intel_bo_unreference(intel_obj->buffer);
	 intel_bufferobj_alloc_buffer(intel, intel_obj);
      }
   }

   /* If the user is mapping a range of an active buffer object but
    * doesn't require the current contents of that range, make a new
    * BO, and we'll copy what they put in there out at unmap or
    * FlushRange time.
    */
   if ((access & GL_MAP_INVALIDATE_RANGE_BIT) &&
       drm_intel_bo_busy(intel_obj->buffer)) {
      if (access & GL_MAP_FLUSH_EXPLICIT_BIT) {
	 intel_obj->range_map_buffer = malloc(length);
	 obj->Pointer = intel_obj->range_map_buffer;
      } else {
	 intel_obj->range_map_bo = drm_intel_bo_alloc(intel->bufmgr,
						      "range map",
						      length, 64);
	 if (!(access & GL_MAP_READ_BIT)) {
	    drm_intel_gem_bo_map_gtt(intel_obj->range_map_bo);
	 } else {
	    drm_intel_bo_map(intel_obj->range_map_bo,
			     (access & GL_MAP_WRITE_BIT) != 0);
	 }
	 obj->Pointer = intel_obj->range_map_bo->virtual;
      }
      return obj->Pointer;
   }

   if (access & GL_MAP_UNSYNCHRONIZED_BIT)
      drm_intel_gem_bo_map_unsynchronized(intel_obj->buffer);
   else if (!(access & GL_MAP_READ_BIT)) {
      drm_intel_gem_bo_map_gtt(intel_obj->buffer);
   } else {
      drm_intel_bo_map(intel_obj->buffer, (access & GL_MAP_WRITE_BIT) != 0);
   }

   obj->Pointer = intel_obj->buffer->virtual + offset;
   return obj->Pointer;
}

/* Ideally we'd use a BO to avoid taking up cache space for the temporary
 * data, but FlushMappedBufferRange may be followed by further writes to
 * the pointer, so we would have to re-map after emitting our blit, which
 * would defeat the point.
 */
static void
intel_bufferobj_flush_mapped_range(struct gl_context *ctx,
				   GLintptr offset, GLsizeiptr length,
				   struct gl_buffer_object *obj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);
   drm_intel_bo *temp_bo;

   /* Unless we're in the range map using a temporary system buffer,
    * there's no work to do.
    */
   if (intel_obj->range_map_buffer == NULL)
      return;

   if (length == 0)
      return;

   temp_bo = drm_intel_bo_alloc(intel->bufmgr, "range map flush", length, 64);

   drm_intel_bo_subdata(temp_bo, 0, length, intel_obj->range_map_buffer);

   intel_emit_linear_blit(intel,
			  intel_obj->buffer, obj->Offset + offset,
			  temp_bo, 0,
			  length);

   drm_intel_bo_unreference(temp_bo);
}


/**
 * Called via glUnmapBuffer().
 */
static GLboolean
intel_bufferobj_unmap(struct gl_context * ctx, struct gl_buffer_object *obj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);
   assert(obj->Pointer);
   if (intel_obj->sys_buffer != NULL) {
      /* always keep the mapping around. */
   } else if (intel_obj->range_map_buffer != NULL) {
      /* Since we've emitted some blits to buffers that will (likely) be used
       * in rendering operations in other cache domains in this batch, emit a
       * flush.  Once again, we wish for a domain tracker in libdrm to cover
       * usage inside of a batchbuffer.
       */
      intel_batchbuffer_emit_mi_flush(intel);
      free(intel_obj->range_map_buffer);
      intel_obj->range_map_buffer = NULL;
   } else if (intel_obj->range_map_bo != NULL) {
      drm_intel_bo_unmap(intel_obj->range_map_bo);

      intel_emit_linear_blit(intel,
			     intel_obj->buffer, obj->Offset,
			     intel_obj->range_map_bo, 0,
			     obj->Length);

      /* Since we've emitted some blits to buffers that will (likely) be used
       * in rendering operations in other cache domains in this batch, emit a
       * flush.  Once again, we wish for a domain tracker in libdrm to cover
       * usage inside of a batchbuffer.
       */
      intel_batchbuffer_emit_mi_flush(intel);

      drm_intel_bo_unreference(intel_obj->range_map_bo);
      intel_obj->range_map_bo = NULL;
   } else if (intel_obj->buffer != NULL) {
      drm_intel_bo_unmap(intel_obj->buffer);
   }
   obj->Pointer = NULL;
   obj->Offset = 0;
   obj->Length = 0;

   return true;
}

drm_intel_bo *
intel_bufferobj_buffer(struct intel_context *intel,
                       struct intel_buffer_object *intel_obj,
		       GLuint flag)
{
   if (intel_obj->source)
      release_buffer(intel_obj);

   if (intel_obj->buffer == NULL) {
      intel_bufferobj_alloc_buffer(intel, intel_obj);
      drm_intel_bo_subdata(intel_obj->buffer,
			   0, intel_obj->Base.Size,
			   intel_obj->sys_buffer);

      free(intel_obj->sys_buffer);
      intel_obj->sys_buffer = NULL;
      intel_obj->offset = 0;
   }

   return intel_obj->buffer;
}

#define INTEL_UPLOAD_SIZE (64*1024)

void
intel_upload_finish(struct intel_context *intel)
{
   if (!intel->upload.bo)
	   return;

   if (intel->upload.buffer_len) {
	   drm_intel_bo_subdata(intel->upload.bo,
				intel->upload.buffer_offset,
				intel->upload.buffer_len,
				intel->upload.buffer);
	   intel->upload.buffer_len = 0;
   }

   drm_intel_bo_unreference(intel->upload.bo);
   intel->upload.bo = NULL;
}

static void wrap_buffers(struct intel_context *intel, GLuint size)
{
   intel_upload_finish(intel);

   if (size < INTEL_UPLOAD_SIZE)
      size = INTEL_UPLOAD_SIZE;

   intel->upload.bo = drm_intel_bo_alloc(intel->bufmgr, "upload", size, 0);
   intel->upload.offset = 0;
}

void intel_upload_data(struct intel_context *intel,
		       const void *ptr, GLuint size, GLuint align,
		       drm_intel_bo **return_bo,
		       GLuint *return_offset)
{
   GLuint base, delta;

   base = (intel->upload.offset + align - 1) / align * align;
   if (intel->upload.bo == NULL || base + size > intel->upload.bo->size) {
      wrap_buffers(intel, size);
      base = 0;
   }

   drm_intel_bo_reference(intel->upload.bo);
   *return_bo = intel->upload.bo;
   *return_offset = base;

   delta = base - intel->upload.offset;
   if (intel->upload.buffer_len &&
       intel->upload.buffer_len + delta + size > sizeof(intel->upload.buffer))
   {
      drm_intel_bo_subdata(intel->upload.bo,
			   intel->upload.buffer_offset,
			   intel->upload.buffer_len,
			   intel->upload.buffer);
      intel->upload.buffer_len = 0;
   }

   if (size < sizeof(intel->upload.buffer))
   {
      if (intel->upload.buffer_len == 0)
	 intel->upload.buffer_offset = base;
      else
	 intel->upload.buffer_len += delta;

      memcpy(intel->upload.buffer + intel->upload.buffer_len, ptr, size);
      intel->upload.buffer_len += size;
   }
   else
   {
      drm_intel_bo_subdata(intel->upload.bo, base, size, ptr);
   }

   intel->upload.offset = base + size;
}

void *intel_upload_map(struct intel_context *intel, GLuint size, GLuint align)
{
   GLuint base, delta;
   char *ptr;

   base = (intel->upload.offset + align - 1) / align * align;
   if (intel->upload.bo == NULL || base + size > intel->upload.bo->size) {
      wrap_buffers(intel, size);
      base = 0;
   }

   delta = base - intel->upload.offset;
   if (intel->upload.buffer_len &&
       intel->upload.buffer_len + delta + size > sizeof(intel->upload.buffer))
   {
      drm_intel_bo_subdata(intel->upload.bo,
			   intel->upload.buffer_offset,
			   intel->upload.buffer_len,
			   intel->upload.buffer);
      intel->upload.buffer_len = 0;
   }

   if (size <= sizeof(intel->upload.buffer)) {
      if (intel->upload.buffer_len == 0)
	 intel->upload.buffer_offset = base;
      else
	 intel->upload.buffer_len += delta;

      ptr = intel->upload.buffer + intel->upload.buffer_len;
      intel->upload.buffer_len += size;
   } else
      ptr = malloc(size);

   return ptr;
}

void intel_upload_unmap(struct intel_context *intel,
			const void *ptr, GLuint size, GLuint align,
			drm_intel_bo **return_bo,
			GLuint *return_offset)
{
   GLuint base;

   base = (intel->upload.offset + align - 1) / align * align;
   if (size > sizeof(intel->upload.buffer)) {
      drm_intel_bo_subdata(intel->upload.bo, base, size, ptr);
      free((void*)ptr);
   }

   drm_intel_bo_reference(intel->upload.bo);
   *return_bo = intel->upload.bo;
   *return_offset = base;

   intel->upload.offset = base + size;
}

drm_intel_bo *
intel_bufferobj_source(struct intel_context *intel,
                       struct intel_buffer_object *intel_obj,
		       GLuint align, GLuint *offset)
{
   if (intel_obj->buffer == NULL) {
      intel_upload_data(intel,
			intel_obj->sys_buffer, intel_obj->Base.Size, align,
			&intel_obj->buffer, &intel_obj->offset);
      intel_obj->source = 1;
   }

   *offset = intel_obj->offset;
   return intel_obj->buffer;
}

static void
intel_bufferobj_copy_subdata(struct gl_context *ctx,
			     struct gl_buffer_object *src,
			     struct gl_buffer_object *dst,
			     GLintptr read_offset, GLintptr write_offset,
			     GLsizeiptr size)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_src = intel_buffer_object(src);
   struct intel_buffer_object *intel_dst = intel_buffer_object(dst);
   drm_intel_bo *src_bo, *dst_bo;
   GLuint src_offset;

   if (size == 0)
      return;

   /* If we're in system memory, just map and memcpy. */
   if (intel_src->sys_buffer || intel_dst->sys_buffer) {
      /* The same buffer may be used, but note that regions copied may
       * not overlap.
       */
      if (src == dst) {
	 char *ptr = intel_bufferobj_map_range(ctx, 0, dst->Size,
					       GL_MAP_READ_BIT |
					       GL_MAP_WRITE_BIT,
					       dst);
	 memmove(ptr + write_offset, ptr + read_offset, size);
	 intel_bufferobj_unmap(ctx, dst);
      } else {
	 const char *src_ptr;
	 char *dst_ptr;

	 src_ptr =  intel_bufferobj_map_range(ctx, 0, src->Size,
					      GL_MAP_READ_BIT, src);
	 dst_ptr =  intel_bufferobj_map_range(ctx, 0, dst->Size,
					      GL_MAP_WRITE_BIT, dst);

	 memcpy(dst_ptr + write_offset, src_ptr + read_offset, size);

	 intel_bufferobj_unmap(ctx, src);
	 intel_bufferobj_unmap(ctx, dst);
      }
      return;
   }

   /* Otherwise, we have real BOs, so blit them. */

   dst_bo = intel_bufferobj_buffer(intel, intel_dst, INTEL_WRITE_PART);
   src_bo = intel_bufferobj_source(intel, intel_src, 64, &src_offset);

   intel_emit_linear_blit(intel,
			  dst_bo, write_offset,
			  src_bo, read_offset + src_offset, size);

   /* Since we've emitted some blits to buffers that will (likely) be used
    * in rendering operations in other cache domains in this batch, emit a
    * flush.  Once again, we wish for a domain tracker in libdrm to cover
    * usage inside of a batchbuffer.
    */
   intel_batchbuffer_emit_mi_flush(intel);
}

#if FEATURE_APPLE_object_purgeable
static GLenum
intel_buffer_purgeable(drm_intel_bo *buffer)
{
   int retained = 0;

   if (buffer != NULL)
      retained = drm_intel_bo_madvise (buffer, I915_MADV_DONTNEED);

   return retained ? GL_VOLATILE_APPLE : GL_RELEASED_APPLE;
}

static GLenum
intel_buffer_object_purgeable(struct gl_context * ctx,
                              struct gl_buffer_object *obj,
                              GLenum option)
{
   struct intel_buffer_object *intel_obj = intel_buffer_object (obj);

   if (intel_obj->buffer != NULL)
      return intel_buffer_purgeable(intel_obj->buffer);

   if (option == GL_RELEASED_APPLE) {
      if (intel_obj->sys_buffer != NULL) {
         free(intel_obj->sys_buffer);
         intel_obj->sys_buffer = NULL;
      }

      return GL_RELEASED_APPLE;
   } else {
      /* XXX Create the buffer and madvise(MADV_DONTNEED)? */
      struct intel_context *intel = intel_context(ctx);
      drm_intel_bo *bo = intel_bufferobj_buffer(intel, intel_obj, INTEL_READ);

      return intel_buffer_purgeable(bo);
   }
}

static GLenum
intel_texture_object_purgeable(struct gl_context * ctx,
                               struct gl_texture_object *obj,
                               GLenum option)
{
   struct intel_texture_object *intel;

   (void) ctx;
   (void) option;

   intel = intel_texture_object(obj);
   if (intel->mt == NULL || intel->mt->region == NULL)
      return GL_RELEASED_APPLE;

   return intel_buffer_purgeable(intel->mt->region->bo);
}

static GLenum
intel_render_object_purgeable(struct gl_context * ctx,
                              struct gl_renderbuffer *obj,
                              GLenum option)
{
   struct intel_renderbuffer *intel;

   (void) ctx;
   (void) option;

   intel = intel_renderbuffer(obj);
   if (intel->mt == NULL)
      return GL_RELEASED_APPLE;

   return intel_buffer_purgeable(intel->mt->region->bo);
}

static GLenum
intel_buffer_unpurgeable(drm_intel_bo *buffer)
{
   int retained;

   retained = 0;
   if (buffer != NULL)
      retained = drm_intel_bo_madvise (buffer, I915_MADV_WILLNEED);

   return retained ? GL_RETAINED_APPLE : GL_UNDEFINED_APPLE;
}

static GLenum
intel_buffer_object_unpurgeable(struct gl_context * ctx,
                                struct gl_buffer_object *obj,
                                GLenum option)
{
   (void) ctx;
   (void) option;

   return intel_buffer_unpurgeable(intel_buffer_object (obj)->buffer);
}

static GLenum
intel_texture_object_unpurgeable(struct gl_context * ctx,
                                 struct gl_texture_object *obj,
                                 GLenum option)
{
   struct intel_texture_object *intel;

   (void) ctx;
   (void) option;

   intel = intel_texture_object(obj);
   if (intel->mt == NULL || intel->mt->region == NULL)
      return GL_UNDEFINED_APPLE;

   return intel_buffer_unpurgeable(intel->mt->region->bo);
}

static GLenum
intel_render_object_unpurgeable(struct gl_context * ctx,
                                struct gl_renderbuffer *obj,
                                GLenum option)
{
   struct intel_renderbuffer *intel;

   (void) ctx;
   (void) option;

   intel = intel_renderbuffer(obj);
   if (intel->mt == NULL)
      return GL_UNDEFINED_APPLE;

   return intel_buffer_unpurgeable(intel->mt->region->bo);
}
#endif

void
intelInitBufferObjectFuncs(struct dd_function_table *functions)
{
   functions->NewBufferObject = intel_bufferobj_alloc;
   functions->DeleteBuffer = intel_bufferobj_free;
   functions->BufferData = intel_bufferobj_data;
   functions->BufferSubData = intel_bufferobj_subdata;
   functions->GetBufferSubData = intel_bufferobj_get_subdata;
   functions->MapBufferRange = intel_bufferobj_map_range;
   functions->FlushMappedBufferRange = intel_bufferobj_flush_mapped_range;
   functions->UnmapBuffer = intel_bufferobj_unmap;
   functions->CopyBufferSubData = intel_bufferobj_copy_subdata;

#if FEATURE_APPLE_object_purgeable
   functions->BufferObjectPurgeable = intel_buffer_object_purgeable;
   functions->TextureObjectPurgeable = intel_texture_object_purgeable;
   functions->RenderObjectPurgeable = intel_render_object_purgeable;

   functions->BufferObjectUnpurgeable = intel_buffer_object_unpurgeable;
   functions->TextureObjectUnpurgeable = intel_texture_object_unpurgeable;
   functions->RenderObjectUnpurgeable = intel_render_object_unpurgeable;
#endif
}
