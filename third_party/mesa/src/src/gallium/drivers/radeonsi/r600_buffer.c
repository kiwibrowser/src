/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Jerome Glisse
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 */
#include <byteswap.h>

#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"

#include "r600.h"
#include "radeonsi_pipe.h"

static void r600_buffer_destroy(struct pipe_screen *screen,
				struct pipe_resource *buf)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	struct si_resource *rbuffer = si_resource(buf);

	pb_reference(&rbuffer->buf, NULL);
	FREE(rbuffer);
}

static struct pipe_transfer *r600_get_transfer(struct pipe_context *ctx,
					       struct pipe_resource *resource,
					       unsigned level,
					       unsigned usage,
					       const struct pipe_box *box)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct pipe_transfer *transfer = util_slab_alloc(&rctx->pool_transfers);

	transfer->resource = resource;
	transfer->level = level;
	transfer->usage = usage;
	transfer->box = *box;
	transfer->stride = 0;
	transfer->layer_stride = 0;
	transfer->data = NULL;

	/* Note strides are zero, this is ok for buffers, but not for
	 * textures 2d & higher at least.
	 */
	return transfer;
}

static void *r600_buffer_transfer_map(struct pipe_context *pipe,
				      struct pipe_transfer *transfer)
{
	struct si_resource *rbuffer = si_resource(transfer->resource);
	struct r600_context *rctx = (struct r600_context*)pipe;
	uint8_t *data;

	data = rctx->ws->buffer_map(rbuffer->cs_buf, rctx->cs, transfer->usage);
	if (!data)
		return NULL;

	return (uint8_t*)data + transfer->box.x;
}

static void r600_buffer_transfer_unmap(struct pipe_context *pipe,
					struct pipe_transfer *transfer)
{
	/* no-op */
}

static void r600_buffer_transfer_flush_region(struct pipe_context *pipe,
						struct pipe_transfer *transfer,
						const struct pipe_box *box)
{
}

static void r600_transfer_destroy(struct pipe_context *ctx,
				  struct pipe_transfer *transfer)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	util_slab_free(&rctx->pool_transfers, transfer);
}

static const struct u_resource_vtbl r600_buffer_vtbl =
{
	u_default_resource_get_handle,		/* get_handle */
	r600_buffer_destroy,			/* resource_destroy */
	r600_get_transfer,			/* get_transfer */
	r600_transfer_destroy,			/* transfer_destroy */
	r600_buffer_transfer_map,		/* transfer_map */
	r600_buffer_transfer_flush_region,	/* transfer_flush_region */
	r600_buffer_transfer_unmap,		/* transfer_unmap */
	NULL	/* transfer_inline_write */
};

bool si_init_resource(struct r600_screen *rscreen,
			struct si_resource *res,
			unsigned size, unsigned alignment,
			unsigned bind, unsigned usage)
{
	uint32_t initial_domain, domains;

	/* Staging resources particpate in transfers and blits only
	 * and are used for uploads and downloads from regular
	 * resources.  We generate them internally for some transfers.
	 */
	if (usage == PIPE_USAGE_STAGING) {
		domains = RADEON_DOMAIN_GTT;
		initial_domain = RADEON_DOMAIN_GTT;
	} else {
		domains = RADEON_DOMAIN_GTT | RADEON_DOMAIN_VRAM;

		switch(usage) {
		case PIPE_USAGE_DYNAMIC:
		case PIPE_USAGE_STREAM:
		case PIPE_USAGE_STAGING:
			initial_domain = RADEON_DOMAIN_GTT;
			break;
		case PIPE_USAGE_DEFAULT:
		case PIPE_USAGE_STATIC:
		case PIPE_USAGE_IMMUTABLE:
		default:
			initial_domain = RADEON_DOMAIN_VRAM;
			break;
		}
	}

	res->buf = rscreen->ws->buffer_create(rscreen->ws, size, alignment, bind, initial_domain);
	if (!res->buf) {
		return false;
	}

	res->cs_buf = rscreen->ws->buffer_get_cs_handle(res->buf);
	res->domains = domains;
	return true;
}

struct pipe_resource *si_buffer_create(struct pipe_screen *screen,
				       const struct pipe_resource *templ)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	struct si_resource *rbuffer;
	/* XXX We probably want a different alignment for buffers and textures. */
	unsigned alignment = 4096;

	rbuffer = MALLOC_STRUCT(si_resource);

	rbuffer->b.b = *templ;
	pipe_reference_init(&rbuffer->b.b.reference, 1);
	rbuffer->b.b.screen = screen;
	rbuffer->b.vtbl = &r600_buffer_vtbl;

	if (!si_init_resource(rscreen, rbuffer, templ->width0, alignment, templ->bind, templ->usage)) {
		FREE(rbuffer);
		return NULL;
	}
	return &rbuffer->b.b;
}

void r600_upload_index_buffer(struct r600_context *rctx,
			      struct pipe_index_buffer *ib, unsigned count)
{
	u_upload_data(rctx->uploader, 0, count * ib->index_size,
		      ib->user_buffer, &ib->offset, &ib->buffer);
}

void r600_upload_const_buffer(struct r600_context *rctx, struct si_resource **rbuffer,
			      const uint8_t *ptr, unsigned size,
			      uint32_t *const_offset)
{
	*rbuffer = NULL;

	if (R600_BIG_ENDIAN) {
		uint32_t *tmpPtr;
		unsigned i;

		if (!(tmpPtr = malloc(size))) {
			R600_ERR("Failed to allocate BE swap buffer.\n");
			return;
		}

		for (i = 0; i < size / 4; ++i) {
			tmpPtr[i] = bswap_32(((uint32_t *)ptr)[i]);
		}

		u_upload_data(rctx->uploader, 0, size, tmpPtr, const_offset,
			      (struct pipe_resource**)rbuffer);

		free(tmpPtr);
	} else {
		u_upload_data(rctx->uploader, 0, size, ptr, const_offset,
			      (struct pipe_resource**)rbuffer);
	}
}
