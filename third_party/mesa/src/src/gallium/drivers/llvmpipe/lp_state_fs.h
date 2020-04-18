/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#ifndef LP_STATE_FS_H_
#define LP_STATE_FS_H_


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h" /* for tgsi_shader_info */
#include "gallivm/lp_bld_sample.h" /* for struct lp_sampler_static_state */
#include "gallivm/lp_bld_tgsi.h" /* for lp_tgsi_info */
#include "lp_bld_interp.h" /* for struct lp_shader_input */


struct tgsi_token;
struct lp_fragment_shader;


/** Indexes into jit_function[] array */
#define RAST_WHOLE 0
#define RAST_EDGE_TEST 1


struct lp_fragment_shader_variant_key
{
   struct pipe_depth_state depth;
   struct pipe_stencil_state stencil[2];
   struct pipe_blend_state blend;

   struct {
      unsigned enabled:1;
      unsigned func:3;
   } alpha;

   unsigned nr_cbufs:8;
   unsigned nr_samplers:8;	/* actually derivable from just the shader */
   unsigned flatshade:1;
   unsigned occlusion_count:1;

   enum pipe_format zsbuf_format;
   enum pipe_format cbuf_format[PIPE_MAX_COLOR_BUFS];

   struct lp_sampler_static_state sampler[PIPE_MAX_SAMPLERS];
};


/** doubly-linked list item */
struct lp_fs_variant_list_item
{
   struct lp_fragment_shader_variant *base;
   struct lp_fs_variant_list_item *next, *prev;
};


struct lp_fragment_shader_variant
{
   struct lp_fragment_shader_variant_key key;

   boolean opaque;

   struct gallivm_state *gallivm;

   LLVMTypeRef jit_context_ptr_type;
   LLVMTypeRef jit_thread_data_ptr_type;
   LLVMTypeRef jit_linear_context_ptr_type;

   LLVMValueRef function[2];

   lp_jit_frag_func jit_function[2];

   /* Total number of LLVM instructions generated */
   unsigned nr_instrs;

   struct lp_fs_variant_list_item list_item_global, list_item_local;
   struct lp_fragment_shader *shader;

   /* For debugging/profiling purposes */
   unsigned no;
};


/** Subclass of pipe_shader_state */
struct lp_fragment_shader
{
   struct pipe_shader_state base;

   struct lp_tgsi_info info;

   struct lp_fs_variant_list_item variants;

   struct draw_fragment_shader *draw_data;

   /* For debugging/profiling purposes */
   unsigned variant_key_size;
   unsigned no;
   unsigned variants_created;
   unsigned variants_cached;

   /** Fragment shader input interpolation info */
   struct lp_shader_input inputs[PIPE_MAX_SHADER_INPUTS];
};


void
lp_debug_fs_variant(const struct lp_fragment_shader_variant *variant);

void
llvmpipe_remove_shader_variant(struct llvmpipe_context *lp,
                               struct lp_fragment_shader_variant *variant);


#endif /* LP_STATE_FS_H_ */
