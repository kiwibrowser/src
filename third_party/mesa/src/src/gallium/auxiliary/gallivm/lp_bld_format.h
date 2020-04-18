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

#ifndef LP_BLD_FORMAT_H
#define LP_BLD_FORMAT_H


/**
 * @file
 * Pixel format helpers.
 */

#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_init.h"

#include "pipe/p_format.h"

struct util_format_description;
struct lp_type;
struct lp_build_context;


/*
 * AoS
 */

LLVMValueRef
lp_build_format_swizzle_aos(const struct util_format_description *desc,
                            struct lp_build_context *bld,
                            LLVMValueRef unswizzled);

LLVMValueRef
lp_build_pack_rgba_aos(struct gallivm_state *gallivm,
                       const struct util_format_description *desc,
                       LLVMValueRef rgba);

LLVMValueRef
lp_build_fetch_rgba_aos(struct gallivm_state *gallivm,
                        const struct util_format_description *format_desc,
                        struct lp_type type,
                        LLVMValueRef base_ptr,
                        LLVMValueRef offset,
                        LLVMValueRef i,
                        LLVMValueRef j);

LLVMValueRef
lp_build_fetch_rgba_aos_array(struct gallivm_state *gallivm,
                        const struct util_format_description *format_desc,
                        struct lp_type type,
                        LLVMValueRef base_ptr,
                        LLVMValueRef offset);


/*
 * SoA
 */

void
lp_build_format_swizzle_soa(const struct util_format_description *format_desc,
                            struct lp_build_context *bld,
                            const LLVMValueRef unswizzled[4],
                            LLVMValueRef swizzled_out[4]);

void
lp_build_unpack_rgba_soa(struct gallivm_state *gallivm,
                         const struct util_format_description *format_desc,
                         struct lp_type type,
                         LLVMValueRef packed,
                         LLVMValueRef rgba_out[4]);

void
lp_build_rgba8_to_f32_soa(struct gallivm_state *gallivm,
                          struct lp_type dst_type,
                          LLVMValueRef packed,
                          LLVMValueRef *rgba);

void
lp_build_fetch_rgba_soa(struct gallivm_state *gallivm,
                        const struct util_format_description *format_desc,
                        struct lp_type type,
                        LLVMValueRef base_ptr,
                        LLVMValueRef offsets,
                        LLVMValueRef i,
                        LLVMValueRef j,
                        LLVMValueRef rgba_out[4]);

/*
 * YUV
 */


LLVMValueRef
lp_build_fetch_subsampled_rgba_aos(struct gallivm_state *gallivm,
                                   const struct util_format_description *format_desc,
                                   unsigned n,
                                   LLVMValueRef base_ptr,
                                   LLVMValueRef offset,
                                   LLVMValueRef i,
                                   LLVMValueRef j);

#endif /* !LP_BLD_FORMAT_H */
