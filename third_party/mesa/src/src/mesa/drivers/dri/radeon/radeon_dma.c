/**************************************************************************

Copyright (C) 2004 Nicolai Haehnle.
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include <errno.h>
#include "radeon_common.h"
#include "radeon_fog.h"
#include "main/simple_list.h"

#if defined(USE_X86_ASM)
#define COPY_DWORDS( dst, src, nr )					\
do {									\
	int __tmp;							\
	__asm__ __volatile__( "rep ; movsl"				\
			      : "=%c" (__tmp), "=D" (dst), "=S" (__tmp)	\
			      : "0" (nr),				\
			        "D" ((long)dst),			\
			        "S" ((long)src) );			\
} while (0)
#else
#define COPY_DWORDS( dst, src, nr )		\
do {						\
   int j;					\
   for ( j = 0 ; j < nr ; j++ )			\
      dst[j] = ((int *)src)[j];			\
   dst += nr;					\
} while (0)
#endif

void radeonEmitVec4(uint32_t *out, const GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & RADEON_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 4)
		COPY_DWORDS(out, data, count);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out++;
			data += stride;
		}
}

void radeonEmitVec8(uint32_t *out, const GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & RADEON_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 8)
		COPY_DWORDS(out, data, count * 2);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out += 2;
			data += stride;
		}
}

void radeonEmitVec12(uint32_t *out, const GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & RADEON_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 12) {
		COPY_DWORDS(out, data, count * 3);
    }
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out[2] = *(int *)(data + 8);
			out += 3;
			data += stride;
		}
}

void radeonEmitVec16(uint32_t *out, const GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & RADEON_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 16)
		COPY_DWORDS(out, data, count * 4);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out[2] = *(int *)(data + 8);
			out[3] = *(int *)(data + 12);
			out += 4;
			data += stride;
		}
}

void rcommon_emit_vector(struct gl_context * ctx, struct radeon_aos *aos,
			 const GLvoid * data, int size, int stride, int count)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	uint32_t *out;

	if (stride == 0) {
		radeonAllocDmaRegion(rmesa, &aos->bo, &aos->offset, size * 4, 32);
		count = 1;
		aos->stride = 0;
	} else {
		radeonAllocDmaRegion(rmesa, &aos->bo, &aos->offset, size * count * 4, 32);
		aos->stride = size;
	}

	aos->components = size;
	aos->count = count;

	radeon_bo_map(aos->bo, 1);
	out = (uint32_t*)((char*)aos->bo->ptr + aos->offset);
	switch (size) {
	case 1: radeonEmitVec4(out, data, stride, count); break;
	case 2: radeonEmitVec8(out, data, stride, count); break;
	case 3: radeonEmitVec12(out, data, stride, count); break;
	case 4: radeonEmitVec16(out, data, stride, count); break;
	default:
		assert(0);
		break;
	}
	radeon_bo_unmap(aos->bo);
}

void rcommon_emit_vecfog(struct gl_context *ctx, struct radeon_aos *aos,
			 GLvoid *data, int stride, int count)
{
	int i;
	float *out;
	int size = 1;
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

	if (RADEON_DEBUG & RADEON_VERTS)
		fprintf(stderr, "%s count %d stride %d\n",
			__FUNCTION__, count, stride);

	if (stride == 0) {
		radeonAllocDmaRegion( rmesa, &aos->bo, &aos->offset, size * 4, 32 );
		count = 1;
		aos->stride = 0;
	} else {
		radeonAllocDmaRegion(rmesa, &aos->bo, &aos->offset, size * count * 4, 32);
		aos->stride = size;
	}

	aos->components = size;
	aos->count = count;

	/* Emit the data */
	radeon_bo_map(aos->bo, 1);
	out = (float*)((char*)aos->bo->ptr + aos->offset);
	for (i = 0; i < count; i++) {
		out[0] = radeonComputeFogBlendFactor( ctx, *(GLfloat *)data );
		out++;
		data += stride;
	}
	radeon_bo_unmap(aos->bo);
}

void radeon_init_dma(radeonContextPtr rmesa)
{
	make_empty_list(&rmesa->dma.free);
	make_empty_list(&rmesa->dma.wait);
	make_empty_list(&rmesa->dma.reserved);
	rmesa->dma.minimum_size = MAX_DMA_BUF_SZ;
}

void radeonRefillCurrentDmaRegion(radeonContextPtr rmesa, int size)
{
	struct radeon_dma_bo *dma_bo = NULL;
	/* we set minimum sizes to at least requested size
	   aligned to next 16 bytes. */
	if (size > rmesa->dma.minimum_size)
		rmesa->dma.minimum_size = (size + 15) & (~15);

	radeon_print(RADEON_DMA, RADEON_NORMAL, "%s size %d minimum_size %Zi\n",
			__FUNCTION__, size, rmesa->dma.minimum_size);

	if (is_empty_list(&rmesa->dma.free)
	      || last_elem(&rmesa->dma.free)->bo->size < size) {
		dma_bo = CALLOC_STRUCT(radeon_dma_bo);
		assert(dma_bo);

again_alloc:
		dma_bo->bo = radeon_bo_open(rmesa->radeonScreen->bom,
					    0, rmesa->dma.minimum_size, 4,
					    RADEON_GEM_DOMAIN_GTT, 0);

		if (!dma_bo->bo) {
			rcommonFlushCmdBuf(rmesa, __FUNCTION__);
			goto again_alloc;
		}
		insert_at_head(&rmesa->dma.reserved, dma_bo);
	} else {
		/* We push and pop buffers from end of list so we can keep
		   counter on unused buffers for later freeing them from
		   begin of list */
		dma_bo = last_elem(&rmesa->dma.free);
		remove_from_list(dma_bo);
		insert_at_head(&rmesa->dma.reserved, dma_bo);
	}

	rmesa->dma.current_used = 0;
	rmesa->dma.current_vertexptr = 0;

	if (radeon_cs_space_check_with_bo(rmesa->cmdbuf.cs,
					  first_elem(&rmesa->dma.reserved)->bo,
					  RADEON_GEM_DOMAIN_GTT, 0))
		fprintf(stderr,"failure to revalidate BOs - badness\n");

	if (is_empty_list(&rmesa->dma.reserved)) {
        /* Cmd buff have been flushed in radeon_revalidate_bos */
		goto again_alloc;
	}
	radeon_bo_map(first_elem(&rmesa->dma.reserved)->bo, 1);
}

/* Allocates a region from rmesa->dma.current.  If there isn't enough
 * space in current, grab a new buffer (and discard what was left of current)
 */
void radeonAllocDmaRegion(radeonContextPtr rmesa,
			  struct radeon_bo **pbo, int *poffset,
			  int bytes, int alignment)
{
	if (RADEON_DEBUG & RADEON_IOCTL)
		fprintf(stderr, "%s %d\n", __FUNCTION__, bytes);

	if (rmesa->dma.flush)
		rmesa->dma.flush(rmesa->glCtx);

	assert(rmesa->dma.current_used == rmesa->dma.current_vertexptr);

	alignment--;
	rmesa->dma.current_used = (rmesa->dma.current_used + alignment) & ~alignment;

	if (is_empty_list(&rmesa->dma.reserved)
		|| rmesa->dma.current_used + bytes > first_elem(&rmesa->dma.reserved)->bo->size)
		radeonRefillCurrentDmaRegion(rmesa, bytes);

	*poffset = rmesa->dma.current_used;
	*pbo = first_elem(&rmesa->dma.reserved)->bo;
	radeon_bo_ref(*pbo);

	/* Always align to at least 16 bytes */
	rmesa->dma.current_used = (rmesa->dma.current_used + bytes + 15) & ~15;
	rmesa->dma.current_vertexptr = rmesa->dma.current_used;

	assert(rmesa->dma.current_used <= first_elem(&rmesa->dma.reserved)->bo->size);
}

void radeonFreeDmaRegions(radeonContextPtr rmesa)
{
	struct radeon_dma_bo *dma_bo;
	struct radeon_dma_bo *temp;
	if (RADEON_DEBUG & RADEON_DMA)
		fprintf(stderr, "%s\n", __FUNCTION__);

	foreach_s(dma_bo, temp, &rmesa->dma.free) {
		remove_from_list(dma_bo);
	        radeon_bo_unref(dma_bo->bo);
		FREE(dma_bo);
	}

	foreach_s(dma_bo, temp, &rmesa->dma.wait) {
		remove_from_list(dma_bo);
	        radeon_bo_unref(dma_bo->bo);
		FREE(dma_bo);
	}

	foreach_s(dma_bo, temp, &rmesa->dma.reserved) {
		remove_from_list(dma_bo);
	        radeon_bo_unref(dma_bo->bo);
		FREE(dma_bo);
	}
}

void radeonReturnDmaRegion(radeonContextPtr rmesa, int return_bytes)
{
	if (is_empty_list(&rmesa->dma.reserved))
		return;

	if (RADEON_DEBUG & RADEON_IOCTL)
		fprintf(stderr, "%s %d\n", __FUNCTION__, return_bytes);
	rmesa->dma.current_used -= return_bytes;
	rmesa->dma.current_vertexptr = rmesa->dma.current_used;
}

static int radeon_bo_is_idle(struct radeon_bo* bo)
{
	uint32_t domain;
	int ret = radeon_bo_is_busy(bo, &domain);
	if (ret == -EINVAL) {
		WARN_ONCE("Your libdrm or kernel doesn't have support for busy query.\n"
			"This may cause small performance drop for you.\n");
	}
	return ret != -EBUSY;
}

void radeonReleaseDmaRegions(radeonContextPtr rmesa)
{
	struct radeon_dma_bo *dma_bo;
	struct radeon_dma_bo *temp;
	const int expire_at = ++rmesa->dma.free.expire_counter + DMA_BO_FREE_TIME;
	const int time = rmesa->dma.free.expire_counter;

	if (RADEON_DEBUG & RADEON_DMA) {
		size_t free = 0,
		       wait = 0,
		       reserved = 0;
		foreach(dma_bo, &rmesa->dma.free)
			++free;

		foreach(dma_bo, &rmesa->dma.wait)
			++wait;

		foreach(dma_bo, &rmesa->dma.reserved)
			++reserved;

		fprintf(stderr, "%s: free %zu, wait %zu, reserved %zu, minimum_size: %zu\n",
		      __FUNCTION__, free, wait, reserved, rmesa->dma.minimum_size);
	}

	/* move waiting bos to free list.
	   wait list provides gpu time to handle data before reuse */
	foreach_s(dma_bo, temp, &rmesa->dma.wait) {
		if (dma_bo->expire_counter == time) {
			WARN_ONCE("Leaking dma buffer object!\n");
			radeon_bo_unref(dma_bo->bo);
			remove_from_list(dma_bo);
			FREE(dma_bo);
			continue;
		}
		/* free objects that are too small to be used because of large request */
		if (dma_bo->bo->size < rmesa->dma.minimum_size) {
		   radeon_bo_unref(dma_bo->bo);
		   remove_from_list(dma_bo);
		   FREE(dma_bo);
		   continue;
		}
		if (!radeon_bo_is_idle(dma_bo->bo)) {
			break;
		}
		remove_from_list(dma_bo);
		dma_bo->expire_counter = expire_at;
		insert_at_tail(&rmesa->dma.free, dma_bo);
	}

	/* move reserved to wait list */
	foreach_s(dma_bo, temp, &rmesa->dma.reserved) {
		radeon_bo_unmap(dma_bo->bo);
		/* free objects that are too small to be used because of large request */
		if (dma_bo->bo->size < rmesa->dma.minimum_size) {
		   radeon_bo_unref(dma_bo->bo);
		   remove_from_list(dma_bo);
		   FREE(dma_bo);
		   continue;
		}
		remove_from_list(dma_bo);
		dma_bo->expire_counter = expire_at;
		insert_at_tail(&rmesa->dma.wait, dma_bo);
	}

	/* free bos that have been unused for some time */
	foreach_s(dma_bo, temp, &rmesa->dma.free) {
		if (dma_bo->expire_counter != time)
			break;
		remove_from_list(dma_bo);
	        radeon_bo_unref(dma_bo->bo);
		FREE(dma_bo);
	}

}


/* Flush vertices in the current dma region.
 */
void rcommon_flush_last_swtcl_prim( struct gl_context *ctx  )
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	struct radeon_dma *dma = &rmesa->dma;

	if (RADEON_DEBUG & RADEON_IOCTL)
		fprintf(stderr, "%s\n", __FUNCTION__);
	dma->flush = NULL;

	radeon_bo_unmap(rmesa->swtcl.bo);

	if (!is_empty_list(&dma->reserved)) {
	    GLuint current_offset = dma->current_used;

	    assert (dma->current_used +
		    rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
		    dma->current_vertexptr);

	    if (dma->current_used != dma->current_vertexptr) {
		    dma->current_used = dma->current_vertexptr;

		    rmesa->vtbl.swtcl_flush(ctx, current_offset);
	    }
	    rmesa->swtcl.numverts = 0;
	}
	radeon_bo_unref(rmesa->swtcl.bo);
	rmesa->swtcl.bo = NULL;
}
/* Alloc space in the current dma region.
 */
void *
rcommonAllocDmaLowVerts( radeonContextPtr rmesa, int nverts, int vsize )
{
	GLuint bytes = vsize * nverts;
	void *head;
	if (RADEON_DEBUG & RADEON_IOCTL)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if(is_empty_list(&rmesa->dma.reserved)
	      ||rmesa->dma.current_vertexptr + bytes > first_elem(&rmesa->dma.reserved)->bo->size) {
		if (rmesa->dma.flush) {
			rmesa->dma.flush(rmesa->glCtx);
		}

                radeonRefillCurrentDmaRegion(rmesa, bytes);

		return NULL;
	}

        if (!rmesa->dma.flush) {
		/* if cmdbuf flushed DMA restart */
                rmesa->glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
                rmesa->dma.flush = rcommon_flush_last_swtcl_prim;
        }

	ASSERT( vsize == rmesa->swtcl.vertex_size * 4 );
        ASSERT( rmesa->dma.flush == rcommon_flush_last_swtcl_prim );
        ASSERT( rmesa->dma.current_used +
                rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
                rmesa->dma.current_vertexptr );

	if (!rmesa->swtcl.bo) {
		rmesa->swtcl.bo = first_elem(&rmesa->dma.reserved)->bo;
		radeon_bo_ref(rmesa->swtcl.bo);
		radeon_bo_map(rmesa->swtcl.bo, 1);
	}

	head = (rmesa->swtcl.bo->ptr + rmesa->dma.current_vertexptr);
	rmesa->dma.current_vertexptr += bytes;
	rmesa->swtcl.numverts += nverts;
	return head;
}

void radeonReleaseArrays( struct gl_context *ctx, GLuint newinputs )
{
   radeonContextPtr radeon = RADEON_CONTEXT( ctx );
   int i;
	if (RADEON_DEBUG & RADEON_IOCTL)
		fprintf(stderr, "%s\n", __FUNCTION__);

   if (radeon->dma.flush) {
       radeon->dma.flush(radeon->glCtx);
   }
   for (i = 0; i < radeon->tcl.aos_count; i++) {
      if (radeon->tcl.aos[i].bo) {
         radeon_bo_unref(radeon->tcl.aos[i].bo);
         radeon->tcl.aos[i].bo = NULL;

      }
   }
}
