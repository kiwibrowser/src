/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * C - JIT interfaces
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_memory.h"
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_debug.h"
#include "lp_context.h"
#include "lp_jit.h"


static void
lp_jit_create_types(struct lp_fragment_shader_variant *lp)
{
   struct gallivm_state *gallivm = lp->gallivm;
   LLVMContextRef lc = gallivm->context;
   LLVMTypeRef texture_type;

   /* struct lp_jit_texture */
   {
      LLVMTypeRef elem_types[LP_JIT_TEXTURE_NUM_FIELDS];

      elem_types[LP_JIT_TEXTURE_WIDTH]  =
      elem_types[LP_JIT_TEXTURE_HEIGHT] =
      elem_types[LP_JIT_TEXTURE_DEPTH] =
      elem_types[LP_JIT_TEXTURE_FIRST_LEVEL] =
      elem_types[LP_JIT_TEXTURE_LAST_LEVEL] =  LLVMInt32TypeInContext(lc);
      elem_types[LP_JIT_TEXTURE_ROW_STRIDE] =
      elem_types[LP_JIT_TEXTURE_IMG_STRIDE] =
         LLVMArrayType(LLVMInt32TypeInContext(lc), LP_MAX_TEXTURE_LEVELS);
      elem_types[LP_JIT_TEXTURE_DATA] =
         LLVMArrayType(LLVMPointerType(LLVMInt8TypeInContext(lc), 0),
                       LP_MAX_TEXTURE_LEVELS);
      elem_types[LP_JIT_TEXTURE_MIN_LOD] =
      elem_types[LP_JIT_TEXTURE_MAX_LOD] =
      elem_types[LP_JIT_TEXTURE_LOD_BIAS] = LLVMFloatTypeInContext(lc);
      elem_types[LP_JIT_TEXTURE_BORDER_COLOR] = 
         LLVMArrayType(LLVMFloatTypeInContext(lc), 4);

      texture_type = LLVMStructTypeInContext(lc, elem_types,
                                             Elements(elem_types), 0);
#if HAVE_LLVM < 0x0300
      LLVMAddTypeName(gallivm->module, "texture", texture_type);

      LLVMInvalidateStructLayout(gallivm->target, texture_type);
#endif

      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, width,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_WIDTH);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, height,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_HEIGHT);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, depth,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_DEPTH);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, first_level,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_FIRST_LEVEL);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, last_level,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_LAST_LEVEL);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, row_stride,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_ROW_STRIDE);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, img_stride,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_IMG_STRIDE);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, data,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_DATA);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, min_lod,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_MIN_LOD);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, max_lod,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_MAX_LOD);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, lod_bias,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_LOD_BIAS);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, border_color,
                             gallivm->target, texture_type,
                             LP_JIT_TEXTURE_BORDER_COLOR);

      LP_CHECK_STRUCT_SIZE(struct lp_jit_texture,
                           gallivm->target, texture_type);
   }

   /* struct lp_jit_context */
   {
      LLVMTypeRef elem_types[LP_JIT_CTX_COUNT];
      LLVMTypeRef context_type;

      elem_types[LP_JIT_CTX_CONSTANTS] = LLVMPointerType(LLVMFloatTypeInContext(lc), 0);
      elem_types[LP_JIT_CTX_ALPHA_REF] = LLVMFloatTypeInContext(lc);
      elem_types[LP_JIT_CTX_STENCIL_REF_FRONT] =
      elem_types[LP_JIT_CTX_STENCIL_REF_BACK] = LLVMInt32TypeInContext(lc);
      elem_types[LP_JIT_CTX_BLEND_COLOR] = LLVMPointerType(LLVMInt8TypeInContext(lc), 0);
      elem_types[LP_JIT_CTX_TEXTURES] = LLVMArrayType(texture_type,
                                                      PIPE_MAX_SAMPLERS);

      context_type = LLVMStructTypeInContext(lc, elem_types,
                                             Elements(elem_types), 0);

#if HAVE_LLVM < 0x0300
      LLVMInvalidateStructLayout(gallivm->target, context_type);

      LLVMAddTypeName(gallivm->module, "context", context_type);
#endif

      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, constants,
                             gallivm->target, context_type,
                             LP_JIT_CTX_CONSTANTS);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, alpha_ref_value,
                             gallivm->target, context_type,
                             LP_JIT_CTX_ALPHA_REF);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, stencil_ref_front,
                             gallivm->target, context_type,
                             LP_JIT_CTX_STENCIL_REF_FRONT);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, stencil_ref_back,
                             gallivm->target, context_type,
                             LP_JIT_CTX_STENCIL_REF_BACK);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, blend_color,
                             gallivm->target, context_type,
                             LP_JIT_CTX_BLEND_COLOR);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, textures,
                             gallivm->target, context_type,
                             LP_JIT_CTX_TEXTURES);
      LP_CHECK_STRUCT_SIZE(struct lp_jit_context,
                           gallivm->target, context_type);

      lp->jit_context_ptr_type = LLVMPointerType(context_type, 0);
   }

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      LLVMDumpModule(gallivm->module);
   }
}


void
lp_jit_screen_cleanup(struct llvmpipe_screen *screen)
{
   /* nothing */
}


void
lp_jit_screen_init(struct llvmpipe_screen *screen)
{
   lp_build_init();
}


void
lp_jit_init_types(struct lp_fragment_shader_variant *lp)
{
   if (!lp->jit_context_ptr_type)
      lp_jit_create_types(lp);
}
