/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include "freedreno_drmif.h"
#include "freedreno_priv.h"
#include "freedreno_ringbuffer.h"

struct fd_ringbuffer *
fd_ringbuffer_new(struct fd_pipe *pipe, uint32_t size)
{
	struct fd_ringbuffer *ring;

	ring = pipe->funcs->ringbuffer_new(pipe, size);
	if (!ring)
		return NULL;

	ring->pipe = pipe;
	ring->start = ring->funcs->hostptr(ring);
	ring->end = &(ring->start[ring->size/4]);

	ring->cur = ring->last_start = ring->start;

	return ring;
}

void fd_ringbuffer_del(struct fd_ringbuffer *ring)
{
	fd_ringbuffer_reset(ring);
	ring->funcs->destroy(ring);
}

/* ringbuffers which are IB targets should set the toplevel rb (ie.
 * the IB source) as it's parent before emitting reloc's, to ensure
 * the bookkeeping works out properly.
 */
void fd_ringbuffer_set_parent(struct fd_ringbuffer *ring,
					 struct fd_ringbuffer *parent)
{
	ring->parent = parent;
}

void fd_ringbuffer_reset(struct fd_ringbuffer *ring)
{
	uint32_t *start = ring->start;
	if (ring->pipe->id == FD_PIPE_2D)
		start = &ring->start[0x140];
	ring->cur = ring->last_start = start;
	if (ring->funcs->reset)
		ring->funcs->reset(ring);
}

int fd_ringbuffer_flush(struct fd_ringbuffer *ring)
{
	return ring->funcs->flush(ring, ring->last_start, -1, NULL);
}

int fd_ringbuffer_flush2(struct fd_ringbuffer *ring, int in_fence_fd,
		int *out_fence_fd)
{
	return ring->funcs->flush(ring, ring->last_start, in_fence_fd, out_fence_fd);
}

void fd_ringbuffer_grow(struct fd_ringbuffer *ring, uint32_t ndwords)
{
	assert(ring->funcs->grow);     /* unsupported on kgsl */

	/* there is an upper bound on IB size, which appears to be 0x100000 */
	if (ring->size < 0x100000)
		ring->size *= 2;

	ring->funcs->grow(ring, ring->size);

	ring->start = ring->funcs->hostptr(ring);
	ring->end = &(ring->start[ring->size/4]);

	ring->cur = ring->last_start = ring->start;
}

uint32_t fd_ringbuffer_timestamp(struct fd_ringbuffer *ring)
{
	return ring->last_timestamp;
}

void fd_ringbuffer_reloc(struct fd_ringbuffer *ring,
				    const struct fd_reloc *reloc)
{
	assert(ring->pipe->gpu_id < 500);
	ring->funcs->emit_reloc(ring, reloc);
}

void fd_ringbuffer_reloc2(struct fd_ringbuffer *ring,
				     const struct fd_reloc *reloc)
{
	ring->funcs->emit_reloc(ring, reloc);
}

void fd_ringbuffer_emit_reloc_ring(struct fd_ringbuffer *ring,
		struct fd_ringmarker *target, struct fd_ringmarker *end)
{
	uint32_t submit_offset, size;

	/* This function is deprecated and not supported on 64b devices: */
	assert(ring->pipe->gpu_id < 500);
	assert(target->ring == end->ring);

	submit_offset = offset_bytes(target->cur, target->ring->start);
	size = offset_bytes(end->cur, target->cur);

	ring->funcs->emit_reloc_ring(ring, target->ring, 0, submit_offset, size);
}

uint32_t fd_ringbuffer_cmd_count(struct fd_ringbuffer *ring)
{
	if (!ring->funcs->cmd_count)
		return 1;
	return ring->funcs->cmd_count(ring);
}

uint32_t
fd_ringbuffer_emit_reloc_ring_full(struct fd_ringbuffer *ring,
		struct fd_ringbuffer *target, uint32_t cmd_idx)
{
	uint32_t size = offset_bytes(target->cur, target->start);
	return ring->funcs->emit_reloc_ring(ring, target, cmd_idx, 0, size);
}

struct fd_ringmarker * fd_ringmarker_new(struct fd_ringbuffer *ring)
{
	struct fd_ringmarker *marker = NULL;

	marker = calloc(1, sizeof(*marker));
	if (!marker) {
		ERROR_MSG("allocation failed");
		return NULL;
	}

	marker->ring = ring;

	marker->cur = marker->ring->cur;

	return marker;
}

void fd_ringmarker_del(struct fd_ringmarker *marker)
{
	free(marker);
}

void fd_ringmarker_mark(struct fd_ringmarker *marker)
{
	marker->cur = marker->ring->cur;
}

uint32_t fd_ringmarker_dwords(struct fd_ringmarker *start,
					 struct fd_ringmarker *end)
{
	return end->cur - start->cur;
}

int fd_ringmarker_flush(struct fd_ringmarker *marker)
{
	struct fd_ringbuffer *ring = marker->ring;
	return ring->funcs->flush(ring, marker->cur, -1, NULL);
}
