/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#ifndef SVGA_STATE_H
#define SVGA_STATE_H


#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"

struct svga_context;


void svga_init_state( struct svga_context *svga );
void svga_destroy_state( struct svga_context *svga );


struct svga_tracked_state {
   const char *name;
   unsigned dirty;
   enum pipe_error (*update)( struct svga_context *svga, unsigned dirty );
};

/* NEED_SWTNL
 */
extern struct svga_tracked_state svga_update_need_swvfetch;
extern struct svga_tracked_state svga_update_need_pipeline;
extern struct svga_tracked_state svga_update_need_swtnl;

/* HW_CLEAR
 */
extern struct svga_tracked_state svga_hw_viewport;
extern struct svga_tracked_state svga_hw_scissor;
extern struct svga_tracked_state svga_hw_framebuffer;

/* HW_DRAW
 */
extern struct svga_tracked_state svga_hw_vs;
extern struct svga_tracked_state svga_hw_fs;
extern struct svga_tracked_state svga_hw_rss;
extern struct svga_tracked_state svga_hw_tss;
extern struct svga_tracked_state svga_hw_tss_binding;
extern struct svga_tracked_state svga_hw_clip_planes;
extern struct svga_tracked_state svga_hw_vdecl;
extern struct svga_tracked_state svga_hw_fs_constants;
extern struct svga_tracked_state svga_hw_vs_constants;

/* SWTNL_DRAW
 */
extern struct svga_tracked_state svga_update_swtnl_draw;
extern struct svga_tracked_state svga_update_swtnl_vdecl;

/* Bring the hardware fully up-to-date so that we can emit draw
 * commands.
 */
#define SVGA_STATE_NEED_SWTNL        0
#define SVGA_STATE_HW_CLEAR          1
#define SVGA_STATE_HW_DRAW           2
#define SVGA_STATE_SWTNL_DRAW        3
#define SVGA_STATE_MAX               4


enum pipe_error svga_update_state( struct svga_context *svga,
                                   unsigned level );

void svga_update_state_retry( struct svga_context *svga,
                              unsigned level );


enum pipe_error svga_emit_initial_state( struct svga_context *svga );

enum pipe_error svga_reemit_framebuffer_bindings( struct svga_context *svga );

enum pipe_error svga_reemit_tss_bindings( struct svga_context *svga );

#endif
