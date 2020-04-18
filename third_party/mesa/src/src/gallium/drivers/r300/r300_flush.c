/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "draw/draw_context.h"
#include "draw/draw_private.h"

#include "util/u_simple_list.h"
#include "util/u_upload_mgr.h"

#include "os/os_time.h"

#include "r300_context.h"
#include "r300_cs.h"
#include "r300_emit.h"


static void r300_flush_and_cleanup(struct r300_context *r300, unsigned flags)
{
    struct r300_atom *atom;

    r300_emit_hyperz_end(r300);
    r300_emit_query_end(r300);
    if (r300->screen->caps.is_r500)
        r500_emit_index_bias(r300, 0);

    r300->flush_counter++;
    r300->rws->cs_flush(r300->cs, flags);
    r300->dirty_hw = 0;

    /* New kitchen sink, baby. */
    foreach_atom(r300, atom) {
        if (atom->state || atom->allow_null_state) {
            r300_mark_atom_dirty(r300, atom);
        }
    }
    r300->vertex_arrays_dirty = TRUE;

    /* Unmark HWTCL state for SWTCL. */
    if (!r300->screen->caps.has_tcl) {
        r300->vs_state.dirty = FALSE;
        r300->vs_constants.dirty = FALSE;
        r300->clip_state.dirty = FALSE;
    }
}

void r300_flush(struct pipe_context *pipe,
                unsigned flags,
                struct pipe_fence_handle **fence)
{
    struct r300_context *r300 = r300_context(pipe);
    struct pb_buffer **rfence = (struct pb_buffer**)fence;

    if (r300->draw && !r300->draw_vbo_locked)
	r300_draw_flush_vbuf(r300);

    if (r300->screen->info.drm_minor >= 12) {
        flags |= RADEON_FLUSH_KEEP_TILING_FLAGS;
    }

    if (rfence) {
        /* Create a fence, which is a dummy BO. */
        *rfence = r300->rws->buffer_create(r300->rws, 1, 1,
                                           PIPE_BIND_CUSTOM,
                                           RADEON_DOMAIN_GTT);
        /* Add the fence as a dummy relocation. */
        r300->rws->cs_add_reloc(r300->cs,
                                r300->rws->buffer_get_cs_handle(*rfence),
                                RADEON_USAGE_READWRITE, RADEON_DOMAIN_GTT);
    }

    if (r300->dirty_hw) {
        r300_flush_and_cleanup(r300, flags);
    } else {
        if (rfence) {
            /* We have to create a fence object, but the command stream is empty
             * and we cannot emit an empty CS. Let's write to some reg. */
            CS_LOCALS(r300);
            OUT_CS_REG(RB3D_COLOR_CHANNEL_MASK, 0);
            r300->rws->cs_flush(r300->cs, flags);
        } else {
            /* Even if hw is not dirty, we should at least reset the CS in case
             * the space checking failed for the first draw operation. */
            r300->rws->cs_flush(r300->cs, flags);
        }
    }

    /* Update Hyper-Z status. */
    if (r300->num_z_clears) {
        r300->hyperz_time_of_last_flush = os_time_get();
    } else if (!r300->hyperz_time_of_last_flush > 2000000) {
        /* 2 seconds without a Z clear pretty much means a dead context
         * for HyperZ. */

        r300->hiz_in_use = FALSE;

        /* Decompress Z buffer. */
        if (r300->zmask_in_use) {
            if (r300->locked_zbuffer) {
                r300_decompress_zmask_locked(r300);
            } else {
                r300_decompress_zmask(r300);
            }

            r300_flush_and_cleanup(r300, flags);
        }

        /* Release HyperZ. */
        r300->rws->cs_request_feature(r300->cs, RADEON_FID_R300_HYPERZ_ACCESS,
                                      FALSE);
    }
    r300->num_z_clears = 0;
}

static void r300_flush_wrapped(struct pipe_context *pipe,
                               struct pipe_fence_handle **fence)
{
    r300_flush(pipe, 0, fence);
}

void r300_init_flush_functions(struct r300_context* r300)
{
    r300->context.flush = r300_flush_wrapped;
}
