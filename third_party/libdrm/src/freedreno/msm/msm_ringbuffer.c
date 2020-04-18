/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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
#include <inttypes.h>

#include "freedreno_ringbuffer.h"
#include "msm_priv.h"

/* represents a single cmd buffer in the submit ioctl.  Each cmd buffer has
 * a backing bo, and a reloc table.
 */
struct msm_cmd {
	struct list_head list;

	struct fd_ringbuffer *ring;
	struct fd_bo *ring_bo;

	/* reloc's table: */
	struct drm_msm_gem_submit_reloc *relocs;
	uint32_t nr_relocs, max_relocs;

	uint32_t size;
};

struct msm_ringbuffer {
	struct fd_ringbuffer base;

	/* submit ioctl related tables:
	 * Note that bos and cmds are tracked by the parent ringbuffer, since
	 * that is global to the submit ioctl call.  The reloc's table is tracked
	 * per cmd-buffer.
	 */
	struct {
		/* bo's table: */
		struct drm_msm_gem_submit_bo *bos;
		uint32_t nr_bos, max_bos;

		/* cmd's table: */
		struct drm_msm_gem_submit_cmd *cmds;
		uint32_t nr_cmds, max_cmds;
	} submit;

	/* should have matching entries in submit.bos: */
	/* Note, only in parent ringbuffer */
	struct fd_bo **bos;
	uint32_t nr_bos, max_bos;

	/* should have matching entries in submit.cmds: */
	struct msm_cmd **cmds;
	uint32_t nr_cmds, max_cmds;

	/* List of physical cmdstream buffers (msm_cmd) assocated with this
	 * logical fd_ringbuffer.
	 *
	 * Note that this is different from msm_ringbuffer::cmds (which
	 * shadows msm_ringbuffer::submit::cmds for tracking submit ioctl
	 * related stuff, and *only* is tracked in the parent ringbuffer.
	 * And only has "completed" cmd buffers (ie. we already know the
	 * size) added via get_cmd().
	 */
	struct list_head cmd_list;

	int is_growable;
	unsigned cmd_count;

	unsigned seqno;

	/* maps fd_bo to idx: */
	void *bo_table;
};

static inline struct msm_ringbuffer * to_msm_ringbuffer(struct fd_ringbuffer *x)
{
	return (struct msm_ringbuffer *)x;
}

#define INIT_SIZE 0x1000

static pthread_mutex_t idx_lock = PTHREAD_MUTEX_INITIALIZER;
drm_private extern pthread_mutex_t table_lock;

static void ring_bo_del(struct fd_device *dev, struct fd_bo *bo)
{
	int ret;

	pthread_mutex_lock(&table_lock);
	ret = fd_bo_cache_free(&to_msm_device(dev)->ring_cache, bo);
	pthread_mutex_unlock(&table_lock);

	if (ret == 0)
		return;

	fd_bo_del(bo);
}

static struct fd_bo * ring_bo_new(struct fd_device *dev, uint32_t size)
{
	struct fd_bo *bo;

	bo = fd_bo_cache_alloc(&to_msm_device(dev)->ring_cache, &size, 0);
	if (bo)
		return bo;

	bo = fd_bo_new(dev, size, 0);
	if (!bo)
		return NULL;

	/* keep ringbuffer bo's out of the normal bo cache: */
	bo->bo_reuse = FALSE;

	return bo;
}

static void ring_cmd_del(struct msm_cmd *cmd)
{
	if (cmd->ring_bo)
		ring_bo_del(cmd->ring->pipe->dev, cmd->ring_bo);
	list_del(&cmd->list);
	to_msm_ringbuffer(cmd->ring)->cmd_count--;
	free(cmd->relocs);
	free(cmd);
}

static struct msm_cmd * ring_cmd_new(struct fd_ringbuffer *ring, uint32_t size)
{
	struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
	struct msm_cmd *cmd = calloc(1, sizeof(*cmd));

	if (!cmd)
		return NULL;

	cmd->ring = ring;
	cmd->ring_bo = ring_bo_new(ring->pipe->dev, size);
	if (!cmd->ring_bo)
		goto fail;

	list_addtail(&cmd->list, &msm_ring->cmd_list);
	msm_ring->cmd_count++;

	return cmd;

fail:
	ring_cmd_del(cmd);
	return NULL;
}

static void *grow(void *ptr, uint32_t nr, uint32_t *max, uint32_t sz)
{
	if ((nr + 1) > *max) {
		if ((*max * 2) < (nr + 1))
			*max = nr + 5;
		else
			*max = *max * 2;
		ptr = realloc(ptr, *max * sz);
	}
	return ptr;
}

#define APPEND(x, name) ({ \
	(x)->name = grow((x)->name, (x)->nr_ ## name, &(x)->max_ ## name, sizeof((x)->name[0])); \
	(x)->nr_ ## name ++; \
})

static struct msm_cmd *current_cmd(struct fd_ringbuffer *ring)
{
	struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
	assert(!LIST_IS_EMPTY(&msm_ring->cmd_list));
	return LIST_LAST_ENTRY(&msm_ring->cmd_list, struct msm_cmd, list);
}

static uint32_t append_bo(struct fd_ringbuffer *ring, struct fd_bo *bo)
{
	struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
	uint32_t idx;

	idx = APPEND(&msm_ring->submit, bos);
	idx = APPEND(msm_ring, bos);

	msm_ring->submit.bos[idx].flags = 0;
	msm_ring->submit.bos[idx].handle = bo->handle;
	msm_ring->submit.bos[idx].presumed = to_msm_bo(bo)->presumed;

	msm_ring->bos[idx] = fd_bo_ref(bo);

	return idx;
}

/* add (if needed) bo, return idx: */
static uint32_t bo2idx(struct fd_ringbuffer *ring, struct fd_bo *bo, uint32_t flags)
{
	struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
	struct msm_bo *msm_bo = to_msm_bo(bo);
	uint32_t idx;
	pthread_mutex_lock(&idx_lock);
	if (msm_bo->current_ring_seqno == msm_ring->seqno) {
		idx = msm_bo->idx;
	} else {
		void *val;

		if (!msm_ring->bo_table)
			msm_ring->bo_table = drmHashCreate();

		if (!drmHashLookup(msm_ring->bo_table, bo->handle, &val)) {
			/* found */
			idx = (uint32_t)(uintptr_t)val;
		} else {
			idx = append_bo(ring, bo);
			val = (void *)(uintptr_t)idx;
			drmHashInsert(msm_ring->bo_table, bo->handle, val);
		}
		msm_bo->current_ring_seqno = msm_ring->seqno;
		msm_bo->idx = idx;
	}
	pthread_mutex_unlock(&idx_lock);
	if (flags & FD_RELOC_READ)
		msm_ring->submit.bos[idx].flags |= MSM_SUBMIT_BO_READ;
	if (flags & FD_RELOC_WRITE)
		msm_ring->submit.bos[idx].flags |= MSM_SUBMIT_BO_WRITE;
	return idx;
}

static int check_cmd_bo(struct fd_ringbuffer *ring,
		struct drm_msm_gem_submit_cmd *cmd, struct fd_bo *bo)
{
	struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
	return msm_ring->submit.bos[cmd->submit_idx].handle == bo->handle;
}

/* Ensure that submit has corresponding entry in cmds table for the
 * target cmdstream buffer:
 */
static void get_cmd(struct fd_ringbuffer *ring, struct msm_cmd *target_cmd,
		uint32_t submit_offset, uint32_t size, uint32_t type)
{
	struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
	struct drm_msm_gem_submit_cmd *cmd;
	uint32_t i;

	/* figure out if we already have a cmd buf: */
	for (i = 0; i < msm_ring->submit.nr_cmds; i++) {
		cmd = &msm_ring->submit.cmds[i];
		if ((cmd->submit_offset == submit_offset) &&
				(cmd->size == size) &&
				(cmd->type == type) &&
				check_cmd_bo(ring, cmd, target_cmd->ring_bo))
			return;
	}

	/* create cmd buf if not: */
	i = APPEND(&msm_ring->submit, cmds);
	APPEND(msm_ring, cmds);
	msm_ring->cmds[i] = target_cmd;
	cmd = &msm_ring->submit.cmds[i];
	cmd->type = type;
	cmd->submit_idx = bo2idx(ring, target_cmd->ring_bo, FD_RELOC_READ);
	cmd->submit_offset = submit_offset;
	cmd->size = size;
	cmd->pad = 0;

	target_cmd->size = size;
}

static void * msm_ringbuffer_hostptr(struct fd_ringbuffer *ring)
{
	return fd_bo_map(current_cmd(ring)->ring_bo);
}

static uint32_t find_next_reloc_idx(struct msm_cmd *msm_cmd,
		uint32_t start, uint32_t offset)
{
	uint32_t i;

	/* a binary search would be more clever.. */
	for (i = start; i < msm_cmd->nr_relocs; i++) {
		struct drm_msm_gem_submit_reloc *reloc = &msm_cmd->relocs[i];
		if (reloc->submit_offset >= offset)
			return i;
	}

	return i;
}

static void delete_cmds(struct msm_ringbuffer *msm_ring)
{
	struct msm_cmd *cmd, *tmp;

	LIST_FOR_EACH_ENTRY_SAFE(cmd, tmp, &msm_ring->cmd_list, list) {
		ring_cmd_del(cmd);
	}
}

static void flush_reset(struct fd_ringbuffer *ring)
{
	struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
	unsigned i;

	for (i = 0; i < msm_ring->nr_bos; i++) {
		struct msm_bo *msm_bo = to_msm_bo(msm_ring->bos[i]);
		msm_bo->current_ring_seqno = 0;
		fd_bo_del(&msm_bo->base);
	}

	/* for each of the cmd buffers, clear their reloc's: */
	for (i = 0; i < msm_ring->submit.nr_cmds; i++) {
		struct msm_cmd *target_cmd = msm_ring->cmds[i];
		target_cmd->nr_relocs = 0;
	}

	msm_ring->submit.nr_cmds = 0;
	msm_ring->submit.nr_bos = 0;
	msm_ring->nr_cmds = 0;
	msm_ring->nr_bos = 0;

	if (msm_ring->bo_table) {
		drmHashDestroy(msm_ring->bo_table);
		msm_ring->bo_table = NULL;
	}

	if (msm_ring->is_growable) {
		delete_cmds(msm_ring);
	} else {
		/* in old mode, just reset the # of relocs: */
		current_cmd(ring)->nr_relocs = 0;
	}
}

static void finalize_current_cmd(struct fd_ringbuffer *ring, uint32_t *last_start)
{
	uint32_t submit_offset, size, type;
	struct fd_ringbuffer *parent;

	if (ring->parent) {
		parent = ring->parent;
		type = MSM_SUBMIT_CMD_IB_TARGET_BUF;
	} else {
		parent = ring;
		type = MSM_SUBMIT_CMD_BUF;
	}

	submit_offset = offset_bytes(last_start, ring->start);
	size = offset_bytes(ring->cur, last_start);

	get_cmd(parent, current_cmd(ring), submit_offset, size, type);
}

static void dump_submit(struct msm_ringbuffer *msm_ring)
{
	uint32_t i, j;

	for (i = 0; i < msm_ring->submit.nr_bos; i++) {
		struct drm_msm_gem_submit_bo *bo = &msm_ring->submit.bos[i];
		ERROR_MSG("  bos[%d]: handle=%u, flags=%x", i, bo->handle, bo->flags);
	}
	for (i = 0; i < msm_ring->submit.nr_cmds; i++) {
		struct drm_msm_gem_submit_cmd *cmd = &msm_ring->submit.cmds[i];
		struct drm_msm_gem_submit_reloc *relocs = U642VOID(cmd->relocs);
		ERROR_MSG("  cmd[%d]: type=%u, submit_idx=%u, submit_offset=%u, size=%u",
				i, cmd->type, cmd->submit_idx, cmd->submit_offset, cmd->size);
		for (j = 0; j < cmd->nr_relocs; j++) {
			struct drm_msm_gem_submit_reloc *r = &relocs[j];
			ERROR_MSG("    reloc[%d]: submit_offset=%u, or=%08x, shift=%d, reloc_idx=%u"
					", reloc_offset=%"PRIu64, j, r->submit_offset, r->or, r->shift,
					r->reloc_idx, r->reloc_offset);
		}
	}
}

static int msm_ringbuffer_flush(struct fd_ringbuffer *ring, uint32_t *last_start,
		int in_fence_fd, int *out_fence_fd)
{
	struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
	struct drm_msm_gem_submit req = {
			.flags = to_msm_pipe(ring->pipe)->pipe,
	};
	uint32_t i;
	int ret;

	if (in_fence_fd != -1) {
		req.flags |= MSM_SUBMIT_FENCE_FD_IN | MSM_SUBMIT_NO_IMPLICIT;
		req.fence_fd = in_fence_fd;
	}

	if (out_fence_fd) {
		req.flags |= MSM_SUBMIT_FENCE_FD_OUT;
	}

	finalize_current_cmd(ring, last_start);

	/* needs to be after get_cmd() as that could create bos/cmds table: */
	req.bos = VOID2U64(msm_ring->submit.bos),
	req.nr_bos = msm_ring->submit.nr_bos;
	req.cmds = VOID2U64(msm_ring->submit.cmds),
	req.nr_cmds = msm_ring->submit.nr_cmds;

	/* for each of the cmd's fix up their reloc's: */
	for (i = 0; i < msm_ring->submit.nr_cmds; i++) {
		struct drm_msm_gem_submit_cmd *cmd = &msm_ring->submit.cmds[i];
		struct msm_cmd *msm_cmd = msm_ring->cmds[i];
		uint32_t a = find_next_reloc_idx(msm_cmd, 0, cmd->submit_offset);
		uint32_t b = find_next_reloc_idx(msm_cmd, a, cmd->submit_offset + cmd->size);
		cmd->relocs = VOID2U64(&msm_cmd->relocs[a]);
		cmd->nr_relocs = (b > a) ? b - a : 0;
	}

	DEBUG_MSG("nr_cmds=%u, nr_bos=%u", req.nr_cmds, req.nr_bos);

	ret = drmCommandWriteRead(ring->pipe->dev->fd, DRM_MSM_GEM_SUBMIT,
			&req, sizeof(req));
	if (ret) {
		ERROR_MSG("submit failed: %d (%s)", ret, strerror(errno));
		dump_submit(msm_ring);
	} else if (!ret) {
		/* update timestamp on all rings associated with submit: */
		for (i = 0; i < msm_ring->submit.nr_cmds; i++) {
			struct msm_cmd *msm_cmd = msm_ring->cmds[i];
			msm_cmd->ring->last_timestamp = req.fence;
		}

		if (out_fence_fd) {
			*out_fence_fd = req.fence_fd;
		}
	}

	flush_reset(ring);

	return ret;
}

static void msm_ringbuffer_grow(struct fd_ringbuffer *ring, uint32_t size)
{
	assert(to_msm_ringbuffer(ring)->is_growable);
	finalize_current_cmd(ring, ring->last_start);
	ring_cmd_new(ring, size);
}

static void msm_ringbuffer_reset(struct fd_ringbuffer *ring)
{
	flush_reset(ring);
}

static void msm_ringbuffer_emit_reloc(struct fd_ringbuffer *ring,
		const struct fd_reloc *r)
{
	struct fd_ringbuffer *parent = ring->parent ? ring->parent : ring;
	struct msm_bo *msm_bo = to_msm_bo(r->bo);
	struct drm_msm_gem_submit_reloc *reloc;
	struct msm_cmd *cmd = current_cmd(ring);
	uint32_t idx = APPEND(cmd, relocs);
	uint32_t addr;

	reloc = &cmd->relocs[idx];

	reloc->reloc_idx = bo2idx(parent, r->bo, r->flags);
	reloc->reloc_offset = r->offset;
	reloc->or = r->or;
	reloc->shift = r->shift;
	reloc->submit_offset = offset_bytes(ring->cur, ring->start);

	addr = msm_bo->presumed;
	if (reloc->shift < 0)
		addr >>= -reloc->shift;
	else
		addr <<= reloc->shift;
	(*ring->cur++) = addr | r->or;

	if (ring->pipe->gpu_id >= 500) {
		struct drm_msm_gem_submit_reloc *reloc_hi;

		/* NOTE: grab reloc_idx *before* APPEND() since that could
		 * realloc() meaning that 'reloc' ptr is no longer valid:
		 */
		uint32_t reloc_idx = reloc->reloc_idx;

		idx = APPEND(cmd, relocs);

		reloc_hi = &cmd->relocs[idx];

		reloc_hi->reloc_idx = reloc_idx;
		reloc_hi->reloc_offset = r->offset;
		reloc_hi->or = r->orhi;
		reloc_hi->shift = r->shift - 32;
		reloc_hi->submit_offset = offset_bytes(ring->cur, ring->start);

		addr = msm_bo->presumed >> 32;
		if (reloc_hi->shift < 0)
			addr >>= -reloc_hi->shift;
		else
			addr <<= reloc_hi->shift;
		(*ring->cur++) = addr | r->orhi;
	}
}

static uint32_t msm_ringbuffer_emit_reloc_ring(struct fd_ringbuffer *ring,
		struct fd_ringbuffer *target, uint32_t cmd_idx,
		uint32_t submit_offset, uint32_t size)
{
	struct msm_cmd *cmd = NULL;
	uint32_t idx = 0;

	LIST_FOR_EACH_ENTRY(cmd, &to_msm_ringbuffer(target)->cmd_list, list) {
		if (idx == cmd_idx)
			break;
		idx++;
	}

	assert(cmd && (idx == cmd_idx));

	if (idx < (to_msm_ringbuffer(target)->cmd_count - 1)) {
		/* All but the last cmd buffer is fully "baked" (ie. already has
		 * done get_cmd() to add it to the cmds table).  But in this case,
		 * the size we get is invalid (since it is calculated from the
		 * last cmd buffer):
		 */
		size = cmd->size;
	} else {
		get_cmd(ring, cmd, submit_offset, size, MSM_SUBMIT_CMD_IB_TARGET_BUF);
	}

	msm_ringbuffer_emit_reloc(ring, &(struct fd_reloc){
		.bo = cmd->ring_bo,
		.flags = FD_RELOC_READ,
		.offset = submit_offset,
	});

	return size;
}

static uint32_t msm_ringbuffer_cmd_count(struct fd_ringbuffer *ring)
{
	return to_msm_ringbuffer(ring)->cmd_count;
}

static void msm_ringbuffer_destroy(struct fd_ringbuffer *ring)
{
	struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);

	flush_reset(ring);
	delete_cmds(msm_ring);

	free(msm_ring->submit.cmds);
	free(msm_ring->submit.bos);
	free(msm_ring->bos);
	free(msm_ring->cmds);
	free(msm_ring);
}

static const struct fd_ringbuffer_funcs funcs = {
		.hostptr = msm_ringbuffer_hostptr,
		.flush = msm_ringbuffer_flush,
		.grow = msm_ringbuffer_grow,
		.reset = msm_ringbuffer_reset,
		.emit_reloc = msm_ringbuffer_emit_reloc,
		.emit_reloc_ring = msm_ringbuffer_emit_reloc_ring,
		.cmd_count = msm_ringbuffer_cmd_count,
		.destroy = msm_ringbuffer_destroy,
};

drm_private struct fd_ringbuffer * msm_ringbuffer_new(struct fd_pipe *pipe,
		uint32_t size)
{
	struct msm_ringbuffer *msm_ring;
	struct fd_ringbuffer *ring;

	msm_ring = calloc(1, sizeof(*msm_ring));
	if (!msm_ring) {
		ERROR_MSG("allocation failed");
		return NULL;
	}

	if (size == 0) {
		assert(pipe->dev->version >= FD_VERSION_UNLIMITED_CMDS);
		size = INIT_SIZE;
		msm_ring->is_growable = TRUE;
	}

	list_inithead(&msm_ring->cmd_list);
	msm_ring->seqno = ++to_msm_device(pipe->dev)->ring_cnt;

	ring = &msm_ring->base;
	ring->funcs = &funcs;
	ring->size = size;
	ring->pipe = pipe;   /* needed in ring_cmd_new() */

	ring_cmd_new(ring, size);

	return ring;
}
