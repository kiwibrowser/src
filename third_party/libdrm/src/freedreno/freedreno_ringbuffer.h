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

#ifndef FREEDRENO_RINGBUFFER_H_
#define FREEDRENO_RINGBUFFER_H_

#include <freedreno_drmif.h>

/* the ringbuffer object is not opaque so that OUT_RING() type stuff
 * can be inlined.  Note that users should not make assumptions about
 * the size of this struct.. more stuff will be added when we eventually
 * have a kernel driver that can deal w/ reloc's..
 */

struct fd_ringbuffer_funcs;
struct fd_ringmarker;

struct fd_ringbuffer {
	int size;
	uint32_t *cur, *end, *start, *last_start;
	struct fd_pipe *pipe;
	const struct fd_ringbuffer_funcs *funcs;
	uint32_t last_timestamp;
	struct fd_ringbuffer *parent;
};

struct fd_ringbuffer * fd_ringbuffer_new(struct fd_pipe *pipe,
		uint32_t size);
void fd_ringbuffer_del(struct fd_ringbuffer *ring);
void fd_ringbuffer_set_parent(struct fd_ringbuffer *ring,
		struct fd_ringbuffer *parent);
void fd_ringbuffer_reset(struct fd_ringbuffer *ring);
int fd_ringbuffer_flush(struct fd_ringbuffer *ring);
/* in_fence_fd: -1 for no in-fence, else fence fd
 * out_fence_fd: NULL for no output-fence requested, else ptr to return out-fence
 */
int fd_ringbuffer_flush2(struct fd_ringbuffer *ring, int in_fence_fd,
		int *out_fence_fd);
void fd_ringbuffer_grow(struct fd_ringbuffer *ring, uint32_t ndwords);
uint32_t fd_ringbuffer_timestamp(struct fd_ringbuffer *ring);

static inline void fd_ringbuffer_emit(struct fd_ringbuffer *ring,
		uint32_t data)
{
	(*ring->cur++) = data;
}

struct fd_reloc {
	struct fd_bo *bo;
#define FD_RELOC_READ             0x0001
#define FD_RELOC_WRITE            0x0002
	uint32_t flags;
	uint32_t offset;
	uint32_t or;
	int32_t  shift;
	uint32_t orhi;      /* used for a5xx+ */
};

/* NOTE: relocs are 2 dwords on a5xx+ */

void fd_ringbuffer_reloc2(struct fd_ringbuffer *ring, const struct fd_reloc *reloc);
will_be_deprecated void fd_ringbuffer_reloc(struct fd_ringbuffer *ring, const struct fd_reloc *reloc);
will_be_deprecated void fd_ringbuffer_emit_reloc_ring(struct fd_ringbuffer *ring,
		struct fd_ringmarker *target, struct fd_ringmarker *end);
uint32_t fd_ringbuffer_cmd_count(struct fd_ringbuffer *ring);
uint32_t fd_ringbuffer_emit_reloc_ring_full(struct fd_ringbuffer *ring,
		struct fd_ringbuffer *target, uint32_t cmd_idx);

will_be_deprecated struct fd_ringmarker * fd_ringmarker_new(struct fd_ringbuffer *ring);
will_be_deprecated void fd_ringmarker_del(struct fd_ringmarker *marker);
will_be_deprecated void fd_ringmarker_mark(struct fd_ringmarker *marker);
will_be_deprecated uint32_t fd_ringmarker_dwords(struct fd_ringmarker *start,
		struct fd_ringmarker *end);
will_be_deprecated int fd_ringmarker_flush(struct fd_ringmarker *marker);

#endif /* FREEDRENO_RINGBUFFER_H_ */
