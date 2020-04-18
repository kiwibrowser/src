/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * @file
 * Position and shader input interpolation.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "tgsi/tgsi_scan.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_arit.h"
#include "gallivm/lp_bld_swizzle.h"
#include "gallivm/lp_bld_flow.h"
#include "lp_bld_interp.h"


/*
 * The shader JIT function operates on blocks of quads.
 * Each block has 2x2 quads and each quad has 2x2 pixels.
 *
 * We iterate over the quads in order 0, 1, 2, 3:
 *
 * #################
 * #   |   #   |   #
 * #---0---#---1---#
 * #   |   #   |   #
 * #################
 * #   |   #   |   #
 * #---2---#---3---#
 * #   |   #   |   #
 * #################
 *
 * If we iterate over multiple quads at once, quads 01 and 23 are processed
 * together.
 *
 * Within each quad, we have four pixels which are represented in SOA
 * order:
 *
 * #########
 * # 0 | 1 #
 * #---+---#
 * # 2 | 3 #
 * #########
 *
 * So the green channel (for example) of the four pixels is stored in
 * a single vector register: {g0, g1, g2, g3}.
 * The order stays the same even with multiple quads:
 * 0 1 4 5
 * 2 3 6 7
 * is stored as g0..g7
 */


/**
 * Do one perspective divide per quad.
 *
 * For perspective interpolation, the final attribute value is given
 *
 *  a' = a/w = a * oow
 *
 * where
 *
 *  a = a0 + dadx*x + dady*y
 *  w = w0 + dwdx*x + dwdy*y
 *  oow = 1/w = 1/(w0 + dwdx*x + dwdy*y)
 *
 * Instead of computing the division per pixel, with this macro we compute the
 * division on the upper left pixel of each quad, and use a linear
 * approximation in the remaining pixels, given by:
 *
 *  da'dx = (dadx - dwdx*a)*oow
 *  da'dy = (dady - dwdy*a)*oow
 *
 * Ironically, this actually makes things slower -- probably because the
 * divide hardware unit is rarely used, whereas the multiply unit is typically
 * already saturated.
 */
#define PERSPECTIVE_DIVIDE_PER_QUAD 0


static const unsigned char quad_offset_x[16] = {0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3};
static const unsigned char quad_offset_y[16] = {0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3};


static void
attrib_name(LLVMValueRef val, unsigned attrib, unsigned chan, const char *suffix)
{
   if(attrib == 0)
      lp_build_name(val, "pos.%c%s", "xyzw"[chan], suffix);
   else
      lp_build_name(val, "input%u.%c%s", attrib - 1, "xyzw"[chan], suffix);
}

static void
calc_offsets(struct lp_build_context *coeff_bld,
             unsigned quad_start_index,
             LLVMValueRef *pixoffx,
             LLVMValueRef *pixoffy)
{
   unsigned i;
   unsigned num_pix = coeff_bld->type.length;
   struct gallivm_state *gallivm = coeff_bld->gallivm;
   LLVMBuilderRef builder = coeff_bld->gallivm->builder;
   LLVMValueRef nr, pixxf, pixyf;

   *pixoffx = coeff_bld->undef;
   *pixoffy = coeff_bld->undef;

   for (i = 0; i < num_pix; i++) {
      nr = lp_build_const_int32(gallivm, i);
      pixxf = lp_build_const_float(gallivm, quad_offset_x[i % num_pix] +
                                   (quad_start_index & 1) * 2);
      pixyf = lp_build_const_float(gallivm, quad_offset_y[i % num_pix] +
                                   (quad_start_index & 2));
      *pixoffx = LLVMBuildInsertElement(builder, *pixoffx, pixxf, nr, "");
      *pixoffy = LLVMBuildInsertElement(builder, *pixoffy, pixyf, nr, "");
   }
}


/* Much easier, and significantly less instructions in the per-stamp
 * part (less than half) but overall more instructions so a loss if
 * most quads are active. Might be a win though with larger vectors.
 * No ability to do per-quad divide (doable but not implemented)
 * Could be made to work with passed in pixel offsets (i.e. active quad merging).
 */
static void
coeffs_init_simple(struct lp_build_interp_soa_context *bld,
                   LLVMValueRef a0_ptr,
                   LLVMValueRef dadx_ptr,
                   LLVMValueRef dady_ptr)
{
   struct lp_build_context *coeff_bld = &bld->coeff_bld;
   struct lp_build_context *setup_bld = &bld->setup_bld;
   struct gallivm_state *gallivm = coeff_bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   unsigned attrib;

   for (attrib = 0; attrib < bld->num_attribs; ++attrib) {
      /*
       * always fetch all 4 values for performance/simplicity
       * Note: we do that here because it seems to generate better
       * code. It generates a lot of moves initially but less
       * moves later. As far as I can tell this looks like a
       * llvm issue, instead of simply reloading the values from
       * the passed in pointers it if it runs out of registers
       * it spills/reloads them. Maybe some optimization passes
       * would help.
       * Might want to investigate this again later.
       */
      const unsigned interp = bld->interp[attrib];
      LLVMValueRef index = lp_build_const_int32(gallivm,
                                attrib * TGSI_NUM_CHANNELS);
      LLVMValueRef ptr;
      LLVMValueRef dadxaos = setup_bld->zero;
      LLVMValueRef dadyaos = setup_bld->zero;
      LLVMValueRef a0aos = setup_bld->zero;

      switch (interp) {
      case LP_INTERP_PERSPECTIVE:
         /* fall-through */

      case LP_INTERP_LINEAR:
         ptr = LLVMBuildGEP(builder, dadx_ptr, &index, 1, "");
         ptr = LLVMBuildBitCast(builder, ptr,
               LLVMPointerType(setup_bld->vec_type, 0), "");
         dadxaos = LLVMBuildLoad(builder, ptr, "");

         ptr = LLVMBuildGEP(builder, dady_ptr, &index, 1, "");
         ptr = LLVMBuildBitCast(builder, ptr,
               LLVMPointerType(setup_bld->vec_type, 0), "");
         dadyaos = LLVMBuildLoad(builder, ptr, "");

         attrib_name(dadxaos, attrib, 0, ".dadxaos");
         attrib_name(dadyaos, attrib, 0, ".dadyaos");
         /* fall-through */

      case LP_INTERP_CONSTANT:
      case LP_INTERP_FACING:
         ptr = LLVMBuildGEP(builder, a0_ptr, &index, 1, "");
         ptr = LLVMBuildBitCast(builder, ptr,
               LLVMPointerType(setup_bld->vec_type, 0), "");
         a0aos = LLVMBuildLoad(builder, ptr, "");
         attrib_name(a0aos, attrib, 0, ".a0aos");
         break;

      case LP_INTERP_POSITION:
         /* Nothing to do as the position coeffs are already setup in slot 0 */
         continue;

      default:
         assert(0);
         break;
      }
      bld->a0aos[attrib] = a0aos;
      bld->dadxaos[attrib] = dadxaos;
      bld->dadyaos[attrib] = dadyaos;
   }
}

/**
 * Interpolate the shader input attribute values.
 * This is called for each (group of) quad(s).
 */
static void
attribs_update_simple(struct lp_build_interp_soa_context *bld,
                      struct gallivm_state *gallivm,
                      int quad_start_index,
                      LLVMValueRef loop_iter,
                      int start,
                      int end)
{
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *coeff_bld = &bld->coeff_bld;
   struct lp_build_context *setup_bld = &bld->setup_bld;
   LLVMValueRef oow = NULL;
   unsigned attrib;
   LLVMValueRef pixoffx;
   LLVMValueRef pixoffy;

   /* could do this with code-generated passed in pixel offsets too */
   if (bld->dynamic_offsets) {
      LLVMValueRef ptr;

      assert(loop_iter);
      ptr = LLVMBuildGEP(builder, bld->xoffset_store, &loop_iter, 1, "");
      pixoffx = LLVMBuildLoad(builder, ptr, "");
      ptr = LLVMBuildGEP(builder, bld->yoffset_store, &loop_iter, 1, "");
      pixoffy = LLVMBuildLoad(builder, ptr, "");
   }
   else {
      calc_offsets(coeff_bld, quad_start_index, &pixoffx, &pixoffy);
   }

   pixoffx = LLVMBuildFAdd(builder, pixoffx,
                           lp_build_broadcast_scalar(coeff_bld, bld->x), "");
   pixoffy = LLVMBuildFAdd(builder, pixoffy,
                           lp_build_broadcast_scalar(coeff_bld, bld->y), "");

   for (attrib = start; attrib < end; attrib++) {
      const unsigned mask = bld->mask[attrib];
      const unsigned interp = bld->interp[attrib];
      unsigned chan;

      for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
         if (mask & (1 << chan)) {
            LLVMValueRef index;
            LLVMValueRef dadx = coeff_bld->zero;
            LLVMValueRef dady = coeff_bld->zero;
            LLVMValueRef a = coeff_bld->zero;

            index = lp_build_const_int32(gallivm, chan);
            switch (interp) {
            case LP_INTERP_PERSPECTIVE:
               /* fall-through */

            case LP_INTERP_LINEAR:
               if (attrib == 0 && chan == 0) {
                  dadx = coeff_bld->one;
               }
               else if (attrib == 0 && chan == 1) {
                  dady = coeff_bld->one;
               }
               else {
                  dadx = lp_build_extract_broadcast(gallivm, setup_bld->type,
                                                    coeff_bld->type, bld->dadxaos[attrib],
                                                    index);
                  dady = lp_build_extract_broadcast(gallivm, setup_bld->type,
                                                    coeff_bld->type, bld->dadyaos[attrib],
                                                    index);
                  a = lp_build_extract_broadcast(gallivm, setup_bld->type,
                                                 coeff_bld->type, bld->a0aos[attrib],
                                                 index);
               }
               /*
                * a = a0 + (x * dadx + y * dady)
                */
               dadx = LLVMBuildFMul(builder, dadx, pixoffx, "");
               dady = LLVMBuildFMul(builder, dady, pixoffy, "");
               a = LLVMBuildFAdd(builder, a, dadx, "");
               a = LLVMBuildFAdd(builder, a, dady, "");

               if (interp == LP_INTERP_PERSPECTIVE) {
                  if (oow == NULL) {
                     LLVMValueRef w = bld->attribs[0][3];
                     assert(attrib != 0);
                     assert(bld->mask[0] & TGSI_WRITEMASK_W);
                     oow = lp_build_rcp(coeff_bld, w);
                  }
                  a = lp_build_mul(coeff_bld, a, oow);
               }
               break;

            case LP_INTERP_CONSTANT:
            case LP_INTERP_FACING:
               a = lp_build_extract_broadcast(gallivm, setup_bld->type,
                                              coeff_bld->type, bld->a0aos[attrib],
                                              index);
               break;

            case LP_INTERP_POSITION:
               assert(attrib > 0);
               a = bld->attribs[0][chan];
               break;

            default:
               assert(0);
               break;
            }

            if ((attrib == 0) && (chan == 2)){
               /* FIXME: Depth values can exceed 1.0, due to the fact that
                * setup interpolation coefficients refer to (0,0) which causes
                * precision loss. So we must clamp to 1.0 here to avoid artifacts
                */
               a = lp_build_min(coeff_bld, a, coeff_bld->one);
            }
            bld->attribs[attrib][chan] = a;
         }
      }
   }
}

/**
 * Initialize the bld->a, dadq fields.  This involves fetching
 * those values from the arrays which are passed into the JIT function.
 */
static void
coeffs_init(struct lp_build_interp_soa_context *bld,
            LLVMValueRef a0_ptr,
            LLVMValueRef dadx_ptr,
            LLVMValueRef dady_ptr)
{
   struct lp_build_context *coeff_bld = &bld->coeff_bld;
   struct lp_build_context *setup_bld = &bld->setup_bld;
   struct gallivm_state *gallivm = coeff_bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef pixoffx, pixoffy;
   unsigned attrib;
   unsigned chan;
   unsigned i;

   pixoffx = coeff_bld->undef;
   pixoffy = coeff_bld->undef;
   for (i = 0; i < coeff_bld->type.length; i++) {
      LLVMValueRef nr = lp_build_const_int32(gallivm, i);
      LLVMValueRef pixxf = lp_build_const_float(gallivm, quad_offset_x[i]);
      LLVMValueRef pixyf = lp_build_const_float(gallivm, quad_offset_y[i]);
      pixoffx = LLVMBuildInsertElement(builder, pixoffx, pixxf, nr, "");
      pixoffy = LLVMBuildInsertElement(builder, pixoffy, pixyf, nr, "");
   }


   for (attrib = 0; attrib < bld->num_attribs; ++attrib) {
      const unsigned mask = bld->mask[attrib];
      const unsigned interp = bld->interp[attrib];
      LLVMValueRef index = lp_build_const_int32(gallivm,
                                attrib * TGSI_NUM_CHANNELS);
      LLVMValueRef ptr;
      LLVMValueRef dadxaos = setup_bld->zero;
      LLVMValueRef dadyaos = setup_bld->zero;
      LLVMValueRef a0aos = setup_bld->zero;

      /* always fetch all 4 values for performance/simplicity */
      switch (interp) {
      case LP_INTERP_PERSPECTIVE:
         /* fall-through */

      case LP_INTERP_LINEAR:
         ptr = LLVMBuildGEP(builder, dadx_ptr, &index, 1, "");
         ptr = LLVMBuildBitCast(builder, ptr,
               LLVMPointerType(setup_bld->vec_type, 0), "");
         dadxaos = LLVMBuildLoad(builder, ptr, "");

         ptr = LLVMBuildGEP(builder, dady_ptr, &index, 1, "");
         ptr = LLVMBuildBitCast(builder, ptr,
               LLVMPointerType(setup_bld->vec_type, 0), "");
         dadyaos = LLVMBuildLoad(builder, ptr, "");

         attrib_name(dadxaos, attrib, 0, ".dadxaos");
         attrib_name(dadyaos, attrib, 0, ".dadyaos");
         /* fall-through */

      case LP_INTERP_CONSTANT:
      case LP_INTERP_FACING:
         ptr = LLVMBuildGEP(builder, a0_ptr, &index, 1, "");
         ptr = LLVMBuildBitCast(builder, ptr,
               LLVMPointerType(setup_bld->vec_type, 0), "");
         a0aos = LLVMBuildLoad(builder, ptr, "");
         attrib_name(a0aos, attrib, 0, ".a0aos");
         break;

      case LP_INTERP_POSITION:
         /* Nothing to do as the position coeffs are already setup in slot 0 */
         continue;

      default:
         assert(0);
         break;
      }

      /*
       * a = a0 + (x * dadx + y * dady)
       * a0aos is the attrib value at top left corner of stamp
       */
      if (interp != LP_INTERP_CONSTANT &&
          interp != LP_INTERP_FACING) {
         LLVMValueRef axaos, ayaos;
         axaos = LLVMBuildFMul(builder, lp_build_broadcast_scalar(setup_bld, bld->x),
                               dadxaos, "");
         ayaos = LLVMBuildFMul(builder, lp_build_broadcast_scalar(setup_bld, bld->y),
                               dadyaos, "");
         a0aos = LLVMBuildFAdd(builder, a0aos, ayaos, "");
         a0aos = LLVMBuildFAdd(builder, a0aos, axaos, "");
      }

      /*
       * dadq = {0, dadx, dady, dadx + dady}
       * for two quads (side by side) this is:
       * {0, dadx, dady, dadx+dady, 2*dadx, 2*dadx+dady, 3*dadx+dady}
       */
      for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
         /* this generates a CRAPLOAD of shuffles... */
         if (mask & (1 << chan)) {
            LLVMValueRef dadx, dady;
            LLVMValueRef dadq, dadq2;
            LLVMValueRef a;
            LLVMValueRef chan_index = lp_build_const_int32(gallivm, chan);

            if (attrib == 0 && chan == 0) {
               a = lp_build_broadcast_scalar(coeff_bld, bld->x);
               dadx = coeff_bld->one;
               dady = coeff_bld->zero;
            }
            else if (attrib == 0 && chan == 1) {
               a = lp_build_broadcast_scalar(coeff_bld, bld->y);
               dady = coeff_bld->one;
               dadx = coeff_bld->zero;
            }
            else {
               dadx = lp_build_extract_broadcast(gallivm, setup_bld->type,
                                              coeff_bld->type, dadxaos, chan_index);
               dady = lp_build_extract_broadcast(gallivm, setup_bld->type,
                                              coeff_bld->type, dadyaos, chan_index);

               /*
                * a = {a, a, a, a}
                */
               a = lp_build_extract_broadcast(gallivm, setup_bld->type,
                                              coeff_bld->type, a0aos, chan_index);
            }

            dadx = LLVMBuildFMul(builder, dadx, pixoffx, "");
            dady = LLVMBuildFMul(builder, dady, pixoffy, "");
            dadq = LLVMBuildFAdd(builder, dadx, dady, "");

            /*
             * Compute the attrib values on the upper-left corner of each
             * group of quads.
             * Note that if we process 2 quads at once this doesn't
             * really exactly to what we want.
             * We need to access elem 0 and 2 respectively later if we process
             * 2 quads at once.
             */

            if (interp != LP_INTERP_CONSTANT &&
                interp != LP_INTERP_FACING) {
               dadq2 = LLVMBuildFAdd(builder, dadq, dadq, "");
               a = LLVMBuildFAdd(builder, a, dadq2, "");
	    }

#if PERSPECTIVE_DIVIDE_PER_QUAD
            /*
             * a *= 1 / w
             */

            /*
             * XXX since we're only going to access elements 0,2 out of 8
             * if we have 8-wide vectors we should do the division only 4-wide.
             * a is really a 2-elements in a 4-wide vector disguised as 8-wide
             * in this case.
             */
            if (interp == LP_INTERP_PERSPECTIVE) {
               LLVMValueRef w = bld->a[0][3];
               assert(attrib != 0);
               assert(bld->mask[0] & TGSI_WRITEMASK_W);
               if (!bld->oow) {
                  bld->oow = lp_build_rcp(coeff_bld, w);
                  lp_build_name(bld->oow, "oow");
               }
               a = lp_build_mul(coeff_bld, a, bld->oow);
            }
#endif

            attrib_name(a, attrib, chan, ".a");
            attrib_name(dadq, attrib, chan, ".dadq");

            if (bld->dynamic_offsets) {
               bld->a[attrib][chan] = lp_build_alloca(gallivm,
                                                      LLVMTypeOf(a), "");
               LLVMBuildStore(builder, a, bld->a[attrib][chan]);
            }
            else {
               bld->a[attrib][chan] = a;
            }
            bld->dadq[attrib][chan] = dadq;
         }
      }
   }
}


/**
 * Increment the shader input attribute values.
 * This is called when we move from one quad to the next.
 */
static void
attribs_update(struct lp_build_interp_soa_context *bld,
               struct gallivm_state *gallivm,
               int quad_start_index,
               LLVMValueRef loop_iter,
               int start,
               int end)
{
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *coeff_bld = &bld->coeff_bld;
   LLVMValueRef shuffle = lp_build_const_int_vec(gallivm, coeff_bld->type, quad_start_index);
   LLVMValueRef oow = NULL;
   unsigned attrib;
   unsigned chan;

   assert(quad_start_index < 4);

   for(attrib = start; attrib < end; ++attrib) {
      const unsigned mask = bld->mask[attrib];
      const unsigned interp = bld->interp[attrib];
      for(chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
         if(mask & (1 << chan)) {
            LLVMValueRef a;
            if (interp == LP_INTERP_CONSTANT ||
                interp == LP_INTERP_FACING) {
               a = bld->a[attrib][chan];
               if (bld->dynamic_offsets) {
                  a = LLVMBuildLoad(builder, a, "");
               }
            }
            else if (interp == LP_INTERP_POSITION) {
               assert(attrib > 0);
               a = bld->attribs[0][chan];
            }
            else {
               LLVMValueRef dadq;

               a = bld->a[attrib][chan];

               /*
                * Broadcast the attribute value for this quad into all elements
                */

               if (bld->dynamic_offsets) {
                  /* stored as vector load as float */
                  LLVMTypeRef ptr_type = LLVMPointerType(LLVMFloatTypeInContext(
                                                            gallivm->context), 0);
                  LLVMValueRef ptr;
                  a = LLVMBuildBitCast(builder, a, ptr_type, "");
                  ptr = LLVMBuildGEP(builder, a, &loop_iter, 1, "");
                  a = LLVMBuildLoad(builder, ptr, "");
                  a = lp_build_broadcast_scalar(&bld->coeff_bld, a);
               }
               else {
                  a = LLVMBuildShuffleVector(builder,
                                             a, coeff_bld->undef, shuffle, "");
               }

               /*
                * Get the derivatives.
                */

               dadq = bld->dadq[attrib][chan];

#if PERSPECTIVE_DIVIDE_PER_QUAD
               if (interp == LP_INTERP_PERSPECTIVE) {
                  LLVMValueRef dwdq = bld->dadq[0][3];

                  if (oow == NULL) {
                     assert(bld->oow);
                     oow = LLVMBuildShuffleVector(coeff_bld->builder,
                                                  bld->oow, coeff_bld->undef,
                                                  shuffle, "");
                  }

                  dadq = lp_build_sub(coeff_bld,
                                      dadq,
                                      lp_build_mul(coeff_bld, a, dwdq));
                  dadq = lp_build_mul(coeff_bld, dadq, oow);
               }
#endif

               /*
                * Add the derivatives
                */

               a = lp_build_add(coeff_bld, a, dadq);

#if !PERSPECTIVE_DIVIDE_PER_QUAD
               if (interp == LP_INTERP_PERSPECTIVE) {
                  if (oow == NULL) {
                     LLVMValueRef w = bld->attribs[0][3];
                     assert(attrib != 0);
                     assert(bld->mask[0] & TGSI_WRITEMASK_W);
                     oow = lp_build_rcp(coeff_bld, w);
                  }
                  a = lp_build_mul(coeff_bld, a, oow);
               }
#endif

               if (attrib == 0 && chan == 2) {
                  /* FIXME: Depth values can exceed 1.0, due to the fact that
                   * setup interpolation coefficients refer to (0,0) which causes
                   * precision loss. So we must clamp to 1.0 here to avoid artifacts
                   */
                  a = lp_build_min(coeff_bld, a, coeff_bld->one);
               }

               attrib_name(a, attrib, chan, "");
            }
            bld->attribs[attrib][chan] = a;
         }
      }
   }
}


/**
 * Generate the position vectors.
 *
 * Parameter x0, y0 are the integer values with upper left coordinates.
 */
static void
pos_init(struct lp_build_interp_soa_context *bld,
         LLVMValueRef x0,
         LLVMValueRef y0)
{
   LLVMBuilderRef builder = bld->coeff_bld.gallivm->builder;
   struct lp_build_context *coeff_bld = &bld->coeff_bld;

   bld->x = LLVMBuildSIToFP(builder, x0, coeff_bld->elem_type, "");
   bld->y = LLVMBuildSIToFP(builder, y0, coeff_bld->elem_type, "");
}


/**
 * Initialize fragment shader input attribute info.
 */
void
lp_build_interp_soa_init(struct lp_build_interp_soa_context *bld,
                         struct gallivm_state *gallivm,
                         unsigned num_inputs,
                         const struct lp_shader_input *inputs,
                         LLVMBuilderRef builder,
                         struct lp_type type,
                         boolean dynamic_offsets,
                         LLVMValueRef a0_ptr,
                         LLVMValueRef dadx_ptr,
                         LLVMValueRef dady_ptr,
                         LLVMValueRef x0,
                         LLVMValueRef y0)
{
   struct lp_type coeff_type;
   struct lp_type setup_type;
   unsigned attrib;
   unsigned chan;

   memset(bld, 0, sizeof *bld);

   memset(&coeff_type, 0, sizeof coeff_type);
   coeff_type.floating = TRUE;
   coeff_type.sign = TRUE;
   coeff_type.width = 32;
   coeff_type.length = type.length;

   memset(&setup_type, 0, sizeof setup_type);
   setup_type.floating = TRUE;
   setup_type.sign = TRUE;
   setup_type.width = 32;
   setup_type.length = TGSI_NUM_CHANNELS;


   /* XXX: we don't support interpolating into any other types */
   assert(memcmp(&coeff_type, &type, sizeof coeff_type) == 0);

   lp_build_context_init(&bld->coeff_bld, gallivm, coeff_type);
   lp_build_context_init(&bld->setup_bld, gallivm, setup_type);

   /* For convenience */
   bld->pos = bld->attribs[0];
   bld->inputs = (const LLVMValueRef (*)[TGSI_NUM_CHANNELS]) bld->attribs[1];

   /* Position */
   bld->mask[0] = TGSI_WRITEMASK_XYZW;
   bld->interp[0] = LP_INTERP_LINEAR;

   /* Inputs */
   for (attrib = 0; attrib < num_inputs; ++attrib) {
      bld->mask[1 + attrib] = inputs[attrib].usage_mask;
      bld->interp[1 + attrib] = inputs[attrib].interp;
   }
   bld->num_attribs = 1 + num_inputs;

   /* Ensure all masked out input channels have a valid value */
   for (attrib = 0; attrib < bld->num_attribs; ++attrib) {
      for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
         bld->attribs[attrib][chan] = bld->coeff_bld.undef;
      }
   }

   pos_init(bld, x0, y0);

   if (coeff_type.length > 4) {
      bld->simple_interp = TRUE;
      if (dynamic_offsets) {
         /* XXX this should use a global static table */
         unsigned i;
         unsigned num_loops = 16 / type.length;
         LLVMValueRef pixoffx, pixoffy, index;
         LLVMValueRef ptr;

         bld->dynamic_offsets = TRUE;
         bld->xoffset_store = lp_build_array_alloca(gallivm,
                                                    lp_build_vec_type(gallivm, type),
                                                    lp_build_const_int32(gallivm, num_loops),
                                                    "");
         bld->yoffset_store = lp_build_array_alloca(gallivm,
                                                    lp_build_vec_type(gallivm, type),
                                                    lp_build_const_int32(gallivm, num_loops),
                                                    "");
         for (i = 0; i < num_loops; i++) {
            index = lp_build_const_int32(gallivm, i);
            calc_offsets(&bld->coeff_bld, i*type.length/4, &pixoffx, &pixoffy);
            ptr = LLVMBuildGEP(builder, bld->xoffset_store, &index, 1, "");
            LLVMBuildStore(builder, pixoffx, ptr);
            ptr = LLVMBuildGEP(builder, bld->yoffset_store, &index, 1, "");
            LLVMBuildStore(builder, pixoffy, ptr);
         }
      }
      coeffs_init_simple(bld, a0_ptr, dadx_ptr, dady_ptr);
   }
   else {
      bld->simple_interp = FALSE;
      if (dynamic_offsets) {
         bld->dynamic_offsets = TRUE;
      }
      coeffs_init(bld, a0_ptr, dadx_ptr, dady_ptr);
   }

}


/**
 * Advance the position and inputs to the given quad within the block.
 */
void
lp_build_interp_soa_update_inputs(struct lp_build_interp_soa_context *bld,
                                  struct gallivm_state *gallivm,
                                  int quad_start_index)
{
   assert(quad_start_index < 4);

   if (bld->simple_interp) {
      attribs_update_simple(bld, gallivm, quad_start_index, NULL, 1, bld->num_attribs);
   }
   else {
      attribs_update(bld, gallivm, quad_start_index, NULL, 1, bld->num_attribs);
   }
}

void
lp_build_interp_soa_update_pos(struct lp_build_interp_soa_context *bld,
                               struct gallivm_state *gallivm,
                               int quad_start_index)
{
   assert(quad_start_index < 4);

   if (bld->simple_interp) {
      attribs_update_simple(bld, gallivm, quad_start_index, NULL, 0, 1);
   }
   else {
      attribs_update(bld, gallivm, quad_start_index, NULL, 0, 1);
   }
}

void
lp_build_interp_soa_update_inputs_dyn(struct lp_build_interp_soa_context *bld,
                                      struct gallivm_state *gallivm,
                                      LLVMValueRef quad_start_index)
{
   if (bld->simple_interp) {
      attribs_update_simple(bld, gallivm, 0, quad_start_index, 1, bld->num_attribs);
   }
   else {
      attribs_update(bld, gallivm, 0, quad_start_index, 1, bld->num_attribs);
   }
}

void
lp_build_interp_soa_update_pos_dyn(struct lp_build_interp_soa_context *bld,
                                   struct gallivm_state *gallivm,
                                   LLVMValueRef quad_start_index)
{
   if (bld->simple_interp) {
      attribs_update_simple(bld, gallivm, 0, quad_start_index, 0, 1);
   }
   else {
      attribs_update(bld, gallivm, 0, quad_start_index, 0, 1);
   }
}

