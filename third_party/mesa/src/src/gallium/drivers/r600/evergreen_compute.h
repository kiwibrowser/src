/*
 * Copyright 2011 Adam Rak <adam.rak@streamnovation.com>
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
 *      Adam Rak <adam.rak@streamnovation.com>
 */

#ifndef EVERGREEN_COMPUTE_H
#define EVERGREEN_COMPUTE_H
#include "r600.h"
#include "r600_pipe.h"

struct r600_atom;
struct evergreen_compute_resource;

void *evergreen_create_compute_state(struct pipe_context *ctx, const const struct pipe_compute_state *cso);
void evergreen_delete_compute_state(struct pipe_context *ctx, void *state);
void evergreen_compute_upload_input(struct pipe_context *context, const uint *block_layout, const uint *grid_layout, const void *input);
void evergreen_init_atom_start_compute_cs(struct r600_context *rctx);
void evergreen_init_compute_state_functions(struct r600_context *rctx);
void evergreen_emit_cs_shader(struct r600_context *rctx, struct r600_atom * atom);

struct pipe_resource *r600_compute_global_buffer_create(struct pipe_screen *screen, const struct pipe_resource *templ);
void r600_compute_global_buffer_destroy(struct pipe_screen *screen, struct pipe_resource *res);
void* r600_compute_global_transfer_map(struct pipe_context *ctx, struct pipe_transfer* transfer);
void r600_compute_global_transfer_unmap(struct pipe_context *ctx, struct pipe_transfer* transfer);
struct pipe_transfer * r600_compute_global_get_transfer(struct pipe_context *, struct pipe_resource *, unsigned level,
                                                        unsigned usage, const struct pipe_box *);
void r600_compute_global_transfer_destroy(struct pipe_context *, struct pipe_transfer *);
void r600_compute_global_transfer_flush_region( struct pipe_context *, struct pipe_transfer *, const struct pipe_box *);
void r600_compute_global_transfer_inline_write( struct pipe_context *, struct pipe_resource *, unsigned level,
                                                unsigned usage, const struct pipe_box *, const void *data, unsigned stride, unsigned layer_stride);


static inline void COMPUTE_DBG(const char *fmt, ...)
{
   static bool check_debug = false, debug = false;

   if (!check_debug) {
		debug = debug_get_bool_option("R600_COMPUTE_DEBUG", FALSE);
   }

   if (debug) {
      va_list ap;
      va_start(ap, fmt);
      _debug_vprintf(fmt, ap);
      va_end(ap);
   }
}

#endif
