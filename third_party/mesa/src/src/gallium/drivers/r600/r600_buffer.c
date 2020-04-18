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
#include "r600_pipe.h"
#include "util/u_upload_mgr.h"
#include "util/u_memory.h"

static void r600_buffer_destroy(struct pipe_screen *screen,
				struct pipe_resource *buf)
{
	struct r600_resource *rbuffer = r600_resource(buf);

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
	struct r600_transfer *transfer = util_slab_alloc(&rctx->pool_transfers);

	assert(box->x + box->width <= resource->width0);

	transfer->transfer.resource = resource;
	transfer->transfer.level = level;
	transfer->transfer.usage = usage;
	transfer->transfer.box = *box;
	transfer->transfer.stride = 0;
	transfer->transfer.layer_stride = 0;
	transfer->transfer.data = NULL;
	transfer->staging = NULL;
	transfer->offset = 0;

	/* Note strides are zero, this is ok for buffers, but not for
	 * textures 2d & higher at least.
	 */
	return &transfer->transfer;
}

static void r600_set_constants_dirty_if_bound(struct r600_context *rctx,
					      struct r600_constbuf_state *state,
					      struct r600_resource *rbuffer)
{
	bool found = false;
	uint32_t mask = state->enabled_mask;

	while (mask) {
		unsigned i = u_bit_scan(&mask);
		if (state->cb[i].buffer == &rbuffer->b.b) {
			found = true;
			state->dirty_mask |= 1 << i;
		}
	}
	if (found) {
		r600_constant_buffers_dirty(rctx, state);
	}
}

static void *r600_buffer_transfer_map(struct pipe_context *pipe,
				      struct pipe_transfer *transfer)
{
	struct r600_resource *rbuffer = r600_resource(transfer->resource);
	struct r600_context *rctx = (struct r600_context*)pipe;
	uint8_t *data;

	if (transfer->usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE &&
	    !(transfer->usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
		assert(transfer->usage & PIPE_TRANSFER_WRITE);

		/* Check if mapping this buffer would cause waiting for the GPU. */
		if (rctx->ws->cs_is_buffer_referenced(rctx->cs, rbuffer->cs_buf, RADEON_USAGE_READWRITE) ||
		    rctx->ws->buffer_is_busy(rbuffer->buf, RADEON_USAGE_READWRITE)) {
			unsigned i, mask;

			/* Discard the buffer. */
			pb_reference(&rbuffer->buf, NULL);

			/* Create a new one in the same pipe_resource. */
			/* XXX We probably want a different alignment for buffers and textures. */
			r600_init_resource(rctx->screen, rbuffer, rbuffer->b.b.width0, 4096,
					   rbuffer->b.b.bind, rbuffer->b.b.usage);

			/* We changed the buffer, now we need to bind it where the old one was bound. */
			/* Vertex buffers. */
			mask = rctx->vertex_buffer_state.enabled_mask;
			while (mask) {
				i = u_bit_scan(&mask);
				if (rctx->vertex_buffer_state.vb[i].buffer == &rbuffer->b.b) {
					rctx->vertex_buffer_state.dirty_mask |= 1 << i;
					r600_vertex_buffers_dirty(rctx);
				}
			}
			/* Streamout buffers. */
			for (i = 0; i < rctx->num_so_targets; i++) {
				if (rctx->so_targets[i]->b.buffer == &rbuffer->b.b) {
					r600_context_streamout_end(rctx);
					rctx->streamout_start = TRUE;
					rctx->streamout_append_bitmask = ~0;
				}
			}
			/* Constant buffers. */
			r600_set_constants_dirty_if_bound(rctx, &rctx->vs_constbuf_state, rbuffer);
			r600_set_constants_dirty_if_bound(rctx, &rctx->ps_constbuf_state, rbuffer);
		}
	}
#if 0 /* this is broken (see Bug 53130) */
	else if ((transfer->usage & PIPE_TRANSFER_DISCARD_RANGE) &&
		 !(transfer->usage & PIPE_TRANSFER_UNSYNCHRONIZED) &&
		 rctx->screen->has_streamout &&
		 /* The buffer range must be aligned to 4. */
		 transfer->box.x % 4 == 0 && transfer->box.width % 4 == 0) {
		assert(transfer->usage & PIPE_TRANSFER_WRITE);

		/* Check if mapping this buffer would cause waiting for the GPU. */
		if (rctx->ws->cs_is_buffer_referenced(rctx->cs, rbuffer->cs_buf, RADEON_USAGE_READWRITE) ||
		    rctx->ws->buffer_is_busy(rbuffer->buf, RADEON_USAGE_READWRITE)) {
			/* Do a wait-free write-only transfer using a temporary buffer. */
			struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;

			rtransfer->staging = (struct r600_resource*)
				pipe_buffer_create(pipe->screen, PIPE_BIND_VERTEX_BUFFER,
						   PIPE_USAGE_STAGING, transfer->box.width);
			return rctx->ws->buffer_map(rtransfer->staging->cs_buf, rctx->cs, PIPE_TRANSFER_WRITE);
		}
	}
#endif

	data = rctx->ws->buffer_map(rbuffer->cs_buf, rctx->cs, transfer->usage);
	if (!data)
		return NULL;

	return (uint8_t*)data + transfer->box.x;
}

static void r600_buffer_transfer_unmap(struct pipe_context *pipe,
					struct pipe_transfer *transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;

	if (rtransfer->staging) {
		struct pipe_box box;
		u_box_1d(0, transfer->box.width, &box);

		/* Copy the staging buffer into the original one. */
		r600_copy_buffer(pipe, transfer->resource, transfer->box.x,
				 &rtransfer->staging->b.b, &box);
		pipe_resource_reference((struct pipe_resource**)&rtransfer->staging, NULL);
	}
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
	NULL,					/* transfer_flush_region */
	r600_buffer_transfer_unmap,		/* transfer_unmap */
	NULL					/* transfer_inline_write */
};

bool r600_init_resource(struct r600_screen *rscreen,
			struct r600_resource *res,
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

struct pipe_resource *r600_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ,
					 unsigned alignment)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	struct r600_resource *rbuffer;

	rbuffer = MALLOC_STRUCT(r600_resource);

	rbuffer->b.b = *templ;
	pipe_reference_init(&rbuffer->b.b.reference, 1);
	rbuffer->b.b.screen = screen;
	rbuffer->b.vtbl = &r600_buffer_vtbl;

	if (!r600_init_resource(rscreen, rbuffer, templ->width0, alignment, templ->bind, templ->usage)) {
		FREE(rbuffer);
		return NULL;
	}
	return &rbuffer->b.b;
}
