/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "r300_context.h"
#include "r300_screen.h"
#include "r300_emit.h"

#include <stdio.h>

static struct pipe_query *r300_create_query(struct pipe_context *pipe,
                                            unsigned query_type)
{
    struct r300_context *r300 = r300_context(pipe);
    struct r300_screen *r300screen = r300->screen;
    struct r300_query *q;

    if (query_type != PIPE_QUERY_OCCLUSION_COUNTER &&
        query_type != PIPE_QUERY_OCCLUSION_PREDICATE &&
        query_type != PIPE_QUERY_GPU_FINISHED) {
        return NULL;
    }

    q = CALLOC_STRUCT(r300_query);
    if (!q)
        return NULL;

    q->type = query_type;

    if (query_type == PIPE_QUERY_GPU_FINISHED) {
        return (struct pipe_query*)q;
    }

    if (r300screen->caps.family == CHIP_FAMILY_RV530)
        q->num_pipes = r300screen->info.r300_num_z_pipes;
    else
        q->num_pipes = r300screen->info.r300_num_gb_pipes;

    q->buf = r300->rws->buffer_create(r300->rws, 4096, 4096,
                                      PIPE_BIND_CUSTOM, RADEON_DOMAIN_GTT);
    if (!q->buf) {
        FREE(q);
        return NULL;
    }
    q->cs_buf = r300->rws->buffer_get_cs_handle(q->buf);

    return (struct pipe_query*)q;
}

static void r300_destroy_query(struct pipe_context* pipe,
                               struct pipe_query* query)
{
    struct r300_query* q = r300_query(query);

    pb_reference(&q->buf, NULL);
    FREE(query);
}

void r300_resume_query(struct r300_context *r300,
                       struct r300_query *query)
{
    r300->query_current = query;
    r300_mark_atom_dirty(r300, &r300->query_start);
}

static void r300_begin_query(struct pipe_context* pipe,
                             struct pipe_query* query)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_query* q = r300_query(query);

    if (q->type == PIPE_QUERY_GPU_FINISHED)
        return;

    if (r300->query_current != NULL) {
        fprintf(stderr, "r300: begin_query: "
                "Some other query has already been started.\n");
        assert(0);
        return;
    }

    q->num_results = 0;
    r300_resume_query(r300, q);
}

void r300_stop_query(struct r300_context *r300)
{
    r300_emit_query_end(r300);
    r300->query_current = NULL;
}

static void r300_end_query(struct pipe_context* pipe,
	                   struct pipe_query* query)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_query *q = r300_query(query);

    if (q->type == PIPE_QUERY_GPU_FINISHED) {
        pb_reference(&q->buf, NULL);
        r300_flush(pipe, RADEON_FLUSH_ASYNC,
                   (struct pipe_fence_handle**)&q->buf);
        return;
    }

    if (q != r300->query_current) {
        fprintf(stderr, "r300: end_query: Got invalid query.\n");
        assert(0);
        return;
    }

    r300_stop_query(r300);
}

static boolean r300_get_query_result(struct pipe_context* pipe,
                                     struct pipe_query* query,
                                     boolean wait,
                                     union pipe_query_result *vresult)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_query *q = r300_query(query);
    unsigned i;
    uint32_t temp, *map;

    if (q->type == PIPE_QUERY_GPU_FINISHED) {
        if (wait) {
            r300->rws->buffer_wait(q->buf, RADEON_USAGE_READWRITE);
            vresult->b = TRUE;
        } else {
            vresult->b = !r300->rws->buffer_is_busy(q->buf, RADEON_USAGE_READWRITE);
        }
        return vresult->b;
    }

    map = r300->rws->buffer_map(q->cs_buf, r300->cs,
                                PIPE_TRANSFER_READ |
                                (!wait ? PIPE_TRANSFER_DONTBLOCK : 0));
    if (!map)
        return FALSE;

    /* Sum up the results. */
    temp = 0;
    for (i = 0; i < q->num_results; i++) {
        /* Convert little endian values written by GPU to CPU byte order */
        temp += util_le32_to_cpu(*map);
        map++;
    }

    r300->rws->buffer_unmap(q->cs_buf);

    if (q->type == PIPE_QUERY_OCCLUSION_PREDICATE) {
        vresult->b = temp != 0;
    } else {
        vresult->u64 = temp;
    }
    return TRUE;
}

static void r300_render_condition(struct pipe_context *pipe,
                                  struct pipe_query *query,
                                  uint mode)
{
    struct r300_context *r300 = r300_context(pipe);
    union pipe_query_result result;
    boolean wait;

    r300->skip_rendering = FALSE;

    if (query) {
        wait = mode == PIPE_RENDER_COND_WAIT ||
               mode == PIPE_RENDER_COND_BY_REGION_WAIT;

        if (r300_get_query_result(pipe, query, wait, &result)) {
            if (r300_query(query)->type == PIPE_QUERY_OCCLUSION_PREDICATE) {
                r300->skip_rendering = !result.b;
            } else {
                r300->skip_rendering = !result.u64;
	    }
        }
    }
}

void r300_init_query_functions(struct r300_context* r300)
{
    r300->context.create_query = r300_create_query;
    r300->context.destroy_query = r300_destroy_query;
    r300->context.begin_query = r300_begin_query;
    r300->context.end_query = r300_end_query;
    r300->context.get_query_result = r300_get_query_result;
    r300->context.render_condition = r300_render_condition;
}
