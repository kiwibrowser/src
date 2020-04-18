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
 * Helper arithmetic functions.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#ifndef LP_BLD_ARIT_H
#define LP_BLD_ARIT_H


#include "gallivm/lp_bld.h"


struct lp_type;
struct lp_build_context;


/**
 * Complement, i.e., 1 - a.
 */
LLVMValueRef
lp_build_comp(struct lp_build_context *bld,
              LLVMValueRef a);

LLVMValueRef
lp_build_add(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_horizontal_add(struct lp_build_context *bld,
                        LLVMValueRef a);

LLVMValueRef
lp_build_hadd_partial4(struct lp_build_context *bld,
                       LLVMValueRef vectors[],
                       unsigned num_vecs);

LLVMValueRef
lp_build_sub(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_mul(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_mul_imm(struct lp_build_context *bld,
                 LLVMValueRef a,
                 int b);

LLVMValueRef
lp_build_div(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_lerp(struct lp_build_context *bld,
              LLVMValueRef x,
              LLVMValueRef v0,
              LLVMValueRef v1);

/**
 * Bilinear interpolation.
 *
 * Values indices are in v_{yx}.
 */
LLVMValueRef
lp_build_lerp_2d(struct lp_build_context *bld,
                 LLVMValueRef x,
                 LLVMValueRef y,
                 LLVMValueRef v00,
                 LLVMValueRef v01,
                 LLVMValueRef v10,
                 LLVMValueRef v11);

LLVMValueRef
lp_build_min(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_max(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_clamp(struct lp_build_context *bld,
               LLVMValueRef a,
               LLVMValueRef min,
               LLVMValueRef max);

LLVMValueRef
lp_build_abs(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_negate(struct lp_build_context *bld,
                LLVMValueRef a);

LLVMValueRef
lp_build_sgn(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_set_sign(struct lp_build_context *bld,
                  LLVMValueRef a, LLVMValueRef sign);

LLVMValueRef
lp_build_int_to_float(struct lp_build_context *bld,
                      LLVMValueRef a);

LLVMValueRef
lp_build_round(struct lp_build_context *bld,
               LLVMValueRef a);

LLVMValueRef
lp_build_floor(struct lp_build_context *bld,
               LLVMValueRef a);

LLVMValueRef
lp_build_ceil(struct lp_build_context *bld,
              LLVMValueRef a);

LLVMValueRef
lp_build_trunc(struct lp_build_context *bld,
               LLVMValueRef a);

LLVMValueRef
lp_build_fract(struct lp_build_context *bld,
               LLVMValueRef a);

LLVMValueRef
lp_build_fract_safe(struct lp_build_context *bld,
                    LLVMValueRef a);

LLVMValueRef
lp_build_ifloor(struct lp_build_context *bld,
                LLVMValueRef a);
LLVMValueRef
lp_build_iceil(struct lp_build_context *bld,
               LLVMValueRef a);

LLVMValueRef
lp_build_iround(struct lp_build_context *bld,
                LLVMValueRef a);

LLVMValueRef
lp_build_itrunc(struct lp_build_context *bld,
                LLVMValueRef a);

void
lp_build_ifloor_fract(struct lp_build_context *bld,
                      LLVMValueRef a,
                      LLVMValueRef *out_ipart,
                      LLVMValueRef *out_fpart);

void
lp_build_ifloor_fract_safe(struct lp_build_context *bld,
                           LLVMValueRef a,
                           LLVMValueRef *out_ipart,
                           LLVMValueRef *out_fpart);

LLVMValueRef
lp_build_sqrt(struct lp_build_context *bld,
              LLVMValueRef a);

LLVMValueRef
lp_build_rcp(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_rsqrt(struct lp_build_context *bld,
               LLVMValueRef a);

LLVMValueRef
lp_build_cos(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_sin(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_pow(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_exp(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_log(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_exp2(struct lp_build_context *bld,
              LLVMValueRef a);

LLVMValueRef
lp_build_extract_exponent(struct lp_build_context *bld,
                          LLVMValueRef x,
                          int bias);

LLVMValueRef
lp_build_extract_mantissa(struct lp_build_context *bld,
                          LLVMValueRef x);

LLVMValueRef
lp_build_log2(struct lp_build_context *bld,
              LLVMValueRef a);

LLVMValueRef
lp_build_fast_log2(struct lp_build_context *bld,
                   LLVMValueRef a);

LLVMValueRef
lp_build_ilog2(struct lp_build_context *bld,
               LLVMValueRef x);

void
lp_build_exp2_approx(struct lp_build_context *bld,
                     LLVMValueRef x,
                     LLVMValueRef *p_exp2_int_part,
                     LLVMValueRef *p_frac_part,
                     LLVMValueRef *p_exp2);

void
lp_build_log2_approx(struct lp_build_context *bld,
                     LLVMValueRef x,
                     LLVMValueRef *p_exp,
                     LLVMValueRef *p_floor_log2,
                     LLVMValueRef *p_log2);

LLVMValueRef
lp_build_mod(struct lp_build_context *bld,
             LLVMValueRef x,
             LLVMValueRef y);

#endif /* !LP_BLD_ARIT_H */
