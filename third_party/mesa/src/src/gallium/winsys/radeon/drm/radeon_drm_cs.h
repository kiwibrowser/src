/*
 * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */

#ifndef RADEON_DRM_CS_H
#define RADEON_DRM_CS_H

#include "radeon_drm_bo.h"
#include <radeon_drm.h>

struct radeon_cs_context {
    uint32_t                    buf[RADEON_MAX_CMDBUF_DWORDS];

    int fd;
    struct drm_radeon_cs        cs;
    struct drm_radeon_cs_chunk  chunks[3];
    uint64_t                    chunk_array[3];
    uint32_t                    flags[2];

    /* Relocs. */
    unsigned                    nrelocs;
    unsigned                    crelocs;
    unsigned                    validated_crelocs;
    struct radeon_bo            **relocs_bo;
    struct drm_radeon_cs_reloc  *relocs;

    /* 0 = BO not added, 1 = BO added */
    char                        is_handle_added[256];
    unsigned                    reloc_indices_hashlist[256];

    unsigned                    used_vram;
    unsigned                    used_gart;
};

struct radeon_drm_cs {
    struct radeon_winsys_cs base;

    /* We flip between these two CS. While one is being consumed
     * by the kernel in another thread, the other one is being filled
     * by the pipe driver. */
    struct radeon_cs_context csc1;
    struct radeon_cs_context csc2;
    /* The currently-used CS. */
    struct radeon_cs_context *csc;
    /* The CS being currently-owned by the other thread. */
    struct radeon_cs_context *cst;

    /* The winsys. */
    struct radeon_drm_winsys *ws;

    /* Flush CS. */
    void (*flush_cs)(void *ctx, unsigned flags);
    void *flush_data;

    pipe_thread thread;
    int flush_started, kill_thread;
    pipe_semaphore flush_queued, flush_completed;
};

int radeon_get_reloc(struct radeon_cs_context *csc, struct radeon_bo *bo);

static INLINE struct radeon_drm_cs *
radeon_drm_cs(struct radeon_winsys_cs *base)
{
    return (struct radeon_drm_cs*)base;
}

static INLINE boolean
radeon_bo_is_referenced_by_cs(struct radeon_drm_cs *cs,
                              struct radeon_bo *bo)
{
    int num_refs = bo->num_cs_references;
    return num_refs == bo->rws->num_cs ||
           (num_refs && radeon_get_reloc(cs->csc, bo) != -1);
}

static INLINE boolean
radeon_bo_is_referenced_by_cs_for_write(struct radeon_drm_cs *cs,
                                        struct radeon_bo *bo)
{
    int index;

    if (!bo->num_cs_references)
        return FALSE;

    index = radeon_get_reloc(cs->csc, bo);
    if (index == -1)
        return FALSE;

    return cs->csc->relocs[index].write_domain != 0;
}

static INLINE boolean
radeon_bo_is_referenced_by_any_cs(struct radeon_bo *bo)
{
    return bo->num_cs_references != 0;
}

void radeon_drm_cs_sync_flush(struct radeon_drm_cs *cs);
void radeon_drm_cs_init_functions(struct radeon_drm_winsys *ws);

#endif
