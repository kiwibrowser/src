/**************************************************************************
 *
 * Copyright 2010 VMware.
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


#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "os/os_time.h"
#include "gallivm/lp_bld_arit.h"
#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_logic.h"
#include "gallivm/lp_bld_intr.h"
#include "gallivm/lp_bld_flow.h"
#include "gallivm/lp_bld_type.h"

#include "lp_perf.h"
#include "lp_debug.h"
#include "lp_flush.h"
#include "lp_screen.h"
#include "lp_context.h"
#include "lp_state.h"
#include "lp_state_fs.h"
#include "lp_state_setup.h"



/* currently organized to interpolate full float[4] attributes even
 * when some elements are unused.  Later, can pack vertex data more
 * closely.
 */


struct lp_setup_args
{
   /* Function arguments:
    */
   LLVMValueRef v0;
   LLVMValueRef v1;
   LLVMValueRef v2;
   LLVMValueRef facing;		/* boolean */
   LLVMValueRef a0;
   LLVMValueRef dadx;
   LLVMValueRef dady;

   /* Derived:
    */
   LLVMValueRef x0_center;
   LLVMValueRef y0_center;
   LLVMValueRef dy20_ooa;
   LLVMValueRef dy01_ooa;
   LLVMValueRef dx20_ooa;
   LLVMValueRef dx01_ooa;
};



static LLVMTypeRef
type4f(struct gallivm_state *gallivm)
{
   return LLVMVectorType(LLVMFloatTypeInContext(gallivm->context), 4);
}


/* Equivalent of _mm_setr_ps(a,b,c,d)
 */
static LLVMValueRef
vec4f(struct gallivm_state *gallivm,
      LLVMValueRef a, LLVMValueRef b, LLVMValueRef c, LLVMValueRef d,
      const char *name)
{
   LLVMBuilderRef bld = gallivm->builder;
   LLVMValueRef i0 = lp_build_const_int32(gallivm, 0);
   LLVMValueRef i1 = lp_build_const_int32(gallivm, 1);
   LLVMValueRef i2 = lp_build_const_int32(gallivm, 2);
   LLVMValueRef i3 = lp_build_const_int32(gallivm, 3);

   LLVMValueRef res = LLVMGetUndef(type4f(gallivm));

   res = LLVMBuildInsertElement(bld, res, a, i0, "");
   res = LLVMBuildInsertElement(bld, res, b, i1, "");
   res = LLVMBuildInsertElement(bld, res, c, i2, "");
   res = LLVMBuildInsertElement(bld, res, d, i3, name);

   return res;
}

/* Equivalent of _mm_set1_ps(a)
 */
static LLVMValueRef
vec4f_from_scalar(struct gallivm_state *gallivm,
                  LLVMValueRef a,
                  const char *name)
{
   LLVMBuilderRef bld = gallivm->builder;
   LLVMValueRef res = LLVMGetUndef(type4f(gallivm));
   int i;

   for(i = 0; i < 4; ++i) {
      LLVMValueRef index = lp_build_const_int32(gallivm, i);
      res = LLVMBuildInsertElement(bld, res, a, index, i == 3 ? name : "");
   }

   return res;
}

static void
store_coef(struct gallivm_state *gallivm,
	   struct lp_setup_args *args,
	   unsigned slot,
	   LLVMValueRef a0,
	   LLVMValueRef dadx,
	   LLVMValueRef dady)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef idx = lp_build_const_int32(gallivm, slot);

   LLVMBuildStore(builder,
		  a0, 
		  LLVMBuildGEP(builder, args->a0, &idx, 1, ""));

   LLVMBuildStore(builder,
		  dadx, 
		  LLVMBuildGEP(builder, args->dadx, &idx, 1, ""));

   LLVMBuildStore(builder,
		  dady, 
		  LLVMBuildGEP(builder, args->dady, &idx, 1, ""));
}



static void 
emit_constant_coef4(struct gallivm_state *gallivm,
		     struct lp_setup_args *args,
		     unsigned slot,
		     LLVMValueRef vert)
{
   LLVMValueRef zero      = lp_build_const_float(gallivm, 0.0);
   LLVMValueRef zerovec   = vec4f_from_scalar(gallivm, zero, "zero");
   store_coef(gallivm, args, slot, vert, zerovec, zerovec);
}



/**
 * Setup the fragment input attribute with the front-facing value.
 * \param frontface  is the triangle front facing?
 */
static void 
emit_facing_coef(struct gallivm_state *gallivm,
		  struct lp_setup_args *args,
		  unsigned slot )
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef float_type = LLVMFloatTypeInContext(gallivm->context);
   LLVMValueRef a0_0 = args->facing;
   LLVMValueRef a0_0f = LLVMBuildSIToFP(builder, a0_0, float_type, "");
   LLVMValueRef zero = lp_build_const_float(gallivm, 0.0);
   LLVMValueRef a0 = vec4f(gallivm, a0_0f, zero, zero, zero, "facing");
   LLVMValueRef zerovec = vec4f_from_scalar(gallivm, zero, "zero");

   store_coef(gallivm, args, slot, a0, zerovec, zerovec);
}


static LLVMValueRef
vert_attrib(struct gallivm_state *gallivm,
	    LLVMValueRef vert,
	    int attr,
	    int elem,
	    const char *name)
{
   LLVMBuilderRef b = gallivm->builder;
   LLVMValueRef idx[2];
   idx[0] = lp_build_const_int32(gallivm, attr);
   idx[1] = lp_build_const_int32(gallivm, elem);
   return LLVMBuildLoad(b, LLVMBuildGEP(b, vert, idx, 2, ""), name);
}


static void
lp_twoside(struct gallivm_state *gallivm,
           struct lp_setup_args *args,
           const struct lp_setup_variant_key *key,
           int bcolor_slot,
           LLVMValueRef attribv[3])
{
   LLVMBuilderRef b = gallivm->builder;
   LLVMValueRef a0_back, a1_back, a2_back;
   LLVMValueRef idx2 = lp_build_const_int32(gallivm, bcolor_slot);

   LLVMValueRef facing = args->facing;
   LLVMValueRef front_facing = LLVMBuildICmp(b, LLVMIntEQ, facing, lp_build_const_int32(gallivm, 0), ""); /** need i1 for if condition */
   
   a0_back = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v0, &idx2, 1, ""), "v0a_back");
   a1_back = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v1, &idx2, 1, ""), "v1a_back");
   a2_back = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v2, &idx2, 1, ""), "v2a_back");

   /* Possibly swap the front and back attrib values,
    *
    * Prefer select to if so we don't have to worry about phis or
    * allocas.
    */
   attribv[0] = LLVMBuildSelect(b, front_facing, a0_back, attribv[0], "");
   attribv[1] = LLVMBuildSelect(b, front_facing, a1_back, attribv[1], "");
   attribv[2] = LLVMBuildSelect(b, front_facing, a2_back, attribv[2], "");

}

static void
lp_do_offset_tri(struct gallivm_state *gallivm,
                 struct lp_setup_args *args,
                 const struct lp_setup_variant_key *key,
                 LLVMValueRef inv_det,
                 LLVMValueRef dxyz01,
                 LLVMValueRef dxyz20,
                 LLVMValueRef attribv[3])
{
   LLVMBuilderRef b = gallivm->builder;
   struct lp_build_context bld;
   LLVMValueRef zoffset, mult;
   LLVMValueRef z0_new, z1_new, z2_new;
   LLVMValueRef dzdxdzdy, dzdx, dzdy, dzxyz20, dyzzx01, dyzzx01_dzxyz20, dzx01_dyz20;
   LLVMValueRef z0z1, z0z1z2;
   LLVMValueRef max, max_value, res12;
   LLVMValueRef shuffles[4];
   LLVMTypeRef shuf_type = LLVMInt32TypeInContext(gallivm->context);
   LLVMValueRef onei = lp_build_const_int32(gallivm, 1);
   LLVMValueRef zeroi = lp_build_const_int32(gallivm, 0);
   LLVMValueRef twoi = lp_build_const_int32(gallivm, 2);
   LLVMValueRef threei  = lp_build_const_int32(gallivm, 3);

   /* (res12) = cross(e,f).xy */
   shuffles[0] = twoi;
   shuffles[1] = zeroi;
   shuffles[2] = onei;
   shuffles[3] = twoi;
   dzxyz20 = LLVMBuildShuffleVector(b, dxyz20, dxyz20, LLVMConstVector(shuffles, 4), "");

   shuffles[0] = onei;
   shuffles[1] = twoi;
   shuffles[2] = twoi;
   shuffles[3] = zeroi;
   dyzzx01 = LLVMBuildShuffleVector(b, dxyz01, dxyz01, LLVMConstVector(shuffles, 4), "");

   dyzzx01_dzxyz20 = LLVMBuildFMul(b, dzxyz20, dyzzx01, "dyzzx01_dzxyz20");

   shuffles[0] = twoi;
   shuffles[1] = threei;
   shuffles[2] = LLVMGetUndef(shuf_type);
   shuffles[3] = LLVMGetUndef(shuf_type);
   dzx01_dyz20 = LLVMBuildShuffleVector(b, dyzzx01_dzxyz20, dyzzx01_dzxyz20,
                                        LLVMConstVector(shuffles, 4), "");

   res12 = LLVMBuildFSub(b, dyzzx01_dzxyz20, dzx01_dyz20, "res12");

   /* dzdx = fabsf(res1 * inv_det), dydx = fabsf(res2 * inv_det)*/
   lp_build_context_init(&bld, gallivm, lp_type_float_vec(32, 128));
   dzdxdzdy = LLVMBuildFMul(b, res12, inv_det, "dzdxdzdy");
   dzdxdzdy = lp_build_abs(&bld, dzdxdzdy);

   dzdx = LLVMBuildExtractElement(b, dzdxdzdy, zeroi, "");
   dzdy = LLVMBuildExtractElement(b, dzdxdzdy, onei, "");

   /* zoffset = offset->units + MAX2(dzdx, dzdy) * offset->scale */
   max = LLVMBuildFCmp(b, LLVMRealUGT, dzdx, dzdy, "");
   max_value = LLVMBuildSelect(b, max, dzdx, dzdy, "max"); 

   mult = LLVMBuildFMul(b, max_value, lp_build_const_float(gallivm, key->scale), "");
   zoffset = LLVMBuildFAdd(b, lp_build_const_float(gallivm, key->units), mult, "zoffset");

   /* yuck */
   shuffles[0] = twoi;
   shuffles[1] = lp_build_const_int32(gallivm, 6);
   shuffles[2] = LLVMGetUndef(shuf_type);
   shuffles[3] = LLVMGetUndef(shuf_type);
   z0z1 = LLVMBuildShuffleVector(b, attribv[0], attribv[1], LLVMConstVector(shuffles, 4), "");
   shuffles[0] = zeroi;
   shuffles[1] = onei;
   shuffles[2] = lp_build_const_int32(gallivm, 6);
   shuffles[3] = LLVMGetUndef(shuf_type);
   z0z1z2 = LLVMBuildShuffleVector(b, z0z1, attribv[2], LLVMConstVector(shuffles, 4), "");
   zoffset = vec4f_from_scalar(gallivm, zoffset, "");

   /* clamp and do offset */
   z0z1z2 = lp_build_clamp(&bld, LLVMBuildFAdd(b, z0z1z2, zoffset, ""), bld.zero, bld.one);

   /* insert into args->a0.z, a1.z, a2.z:
    */
   z0_new = LLVMBuildExtractElement(b, z0z1z2, zeroi, "");
   z1_new = LLVMBuildExtractElement(b, z0z1z2, onei, "");
   z2_new = LLVMBuildExtractElement(b, z0z1z2, twoi, "");
   attribv[0] = LLVMBuildInsertElement(b, attribv[0], z0_new, twoi, "");
   attribv[1] = LLVMBuildInsertElement(b, attribv[1], z1_new, twoi, "");
   attribv[2] = LLVMBuildInsertElement(b, attribv[2], z2_new, twoi, "");
}

static void
load_attribute(struct gallivm_state *gallivm,
               struct lp_setup_args *args,
               const struct lp_setup_variant_key *key,
               unsigned vert_attr,
               LLVMValueRef attribv[3])
{
   LLVMBuilderRef b = gallivm->builder;
   LLVMValueRef idx = lp_build_const_int32(gallivm, vert_attr);

   /* Load the vertex data
    */
   attribv[0] = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v0, &idx, 1, ""), "v0a");
   attribv[1] = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v1, &idx, 1, ""), "v1a");
   attribv[2] = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v2, &idx, 1, ""), "v2a");


   /* Potentially modify it according to twoside, etc:
    */
   if (key->twoside) {
      if (vert_attr == key->color_slot && key->bcolor_slot >= 0)
         lp_twoside(gallivm, args, key, key->bcolor_slot, attribv);
      else if (vert_attr == key->spec_slot && key->bspec_slot >= 0)
         lp_twoside(gallivm, args, key, key->bspec_slot, attribv);
   }
}

static void 
emit_coef4( struct gallivm_state *gallivm,
	    struct lp_setup_args *args,
	    unsigned slot,
	    LLVMValueRef a0,
	    LLVMValueRef a1,
	    LLVMValueRef a2)
{
   LLVMBuilderRef b = gallivm->builder;
   LLVMValueRef dy20_ooa = args->dy20_ooa;
   LLVMValueRef dy01_ooa = args->dy01_ooa;
   LLVMValueRef dx20_ooa = args->dx20_ooa;
   LLVMValueRef dx01_ooa = args->dx01_ooa;
   LLVMValueRef x0_center = args->x0_center;
   LLVMValueRef y0_center = args->y0_center;

   LLVMValueRef da01 = LLVMBuildFSub(b, a0, a1, "da01");
   LLVMValueRef da20 = LLVMBuildFSub(b, a2, a0, "da20");

   /* Calculate dadx (vec4f)
    */
   LLVMValueRef da01_dy20_ooa = LLVMBuildFMul(b, da01, dy20_ooa, "da01_dy20_ooa");
   LLVMValueRef da20_dy01_ooa = LLVMBuildFMul(b, da20, dy01_ooa, "da20_dy01_ooa");
   LLVMValueRef dadx          = LLVMBuildFSub(b, da01_dy20_ooa, da20_dy01_ooa, "dadx");

   /* Calculate dady (vec4f)
    */
   LLVMValueRef da01_dx20_ooa = LLVMBuildFMul(b, da01, dx20_ooa, "da01_dx20_ooa");
   LLVMValueRef da20_dx01_ooa = LLVMBuildFMul(b, da20, dx01_ooa, "da20_dx01_ooa");
   LLVMValueRef dady          = LLVMBuildFSub(b, da20_dx01_ooa, da01_dx20_ooa, "dady");

   /* Calculate a0 - the attribute value at the origin
    */
   LLVMValueRef dadx_x0       = LLVMBuildFMul(b, dadx, x0_center, "dadx_x0"); 
   LLVMValueRef dady_y0       = LLVMBuildFMul(b, dady, y0_center, "dady_y0"); 
   LLVMValueRef attr_v0       = LLVMBuildFAdd(b, dadx_x0, dady_y0, "attr_v0"); 
   LLVMValueRef attr_0        = LLVMBuildFSub(b, a0, attr_v0, "attr_0"); 

   store_coef(gallivm, args, slot, attr_0, dadx, dady);
}


static void 
emit_linear_coef( struct gallivm_state *gallivm,
		  struct lp_setup_args *args,
		  unsigned slot,
		  LLVMValueRef attribv[3])
{
   /* nothing to do anymore */
   emit_coef4(gallivm,
              args, slot, 
              attribv[0],
              attribv[1],
              attribv[2]);
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void 
apply_perspective_corr( struct gallivm_state *gallivm,
                        struct lp_setup_args *args,
                        unsigned slot,
                        LLVMValueRef attribv[3])
{
   LLVMBuilderRef b = gallivm->builder;

   /* premultiply by 1/w  (v[0][3] is always 1/w):
    */
   LLVMValueRef v0_oow = vec4f_from_scalar(gallivm, vert_attrib(gallivm, args->v0, 0, 3, ""), "v0_oow");
   LLVMValueRef v1_oow = vec4f_from_scalar(gallivm, vert_attrib(gallivm, args->v1, 0, 3, ""), "v1_oow");
   LLVMValueRef v2_oow = vec4f_from_scalar(gallivm, vert_attrib(gallivm, args->v2, 0, 3, ""), "v2_oow");

   attribv[0] = LLVMBuildFMul(b, attribv[0], v0_oow, "v0_oow_v0a");
   attribv[1] = LLVMBuildFMul(b, attribv[1], v1_oow, "v1_oow_v1a");
   attribv[2] = LLVMBuildFMul(b, attribv[2], v2_oow, "v2_oow_v2a");
}


static void
emit_position_coef( struct gallivm_state *gallivm,
		    struct lp_setup_args *args,
		    int slot,
		    LLVMValueRef attribv[3])
{
   emit_linear_coef(gallivm, args, slot, attribv);
}


/**
 * Applys cylindrical wrapping to vertex attributes if enabled.
 * Input coordinates must be in [0, 1] range, otherwise results are undefined.
 *
 * @param cyl_wrap  TGSI_CYLINDRICAL_WRAP_x flags
 */
static void
emit_apply_cyl_wrap(struct gallivm_state *gallivm,
                    struct lp_setup_args *args,
                    uint cyl_wrap,
		    LLVMValueRef attribv[3])

{
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_type type = lp_float32_vec4_type();
   LLVMTypeRef float_vec_type = lp_build_vec_type(gallivm, type);
   LLVMValueRef pos_half;
   LLVMValueRef neg_half;
   LLVMValueRef cyl_mask;
   LLVMValueRef offset;
   LLVMValueRef delta;
   LLVMValueRef one;

   if (!cyl_wrap)
      return;

   /* Constants */
   pos_half = lp_build_const_vec(gallivm, type, +0.5f);
   neg_half = lp_build_const_vec(gallivm, type, -0.5f);
   cyl_mask = lp_build_const_mask_aos(gallivm, type, cyl_wrap);

   one = lp_build_const_vec(gallivm, type, 1.0f);
   one = LLVMBuildBitCast(builder, one, lp_build_int_vec_type(gallivm, type), "");
   one = LLVMBuildAnd(builder, one, cyl_mask, "");

   /* Edge v0 -> v1 */
   delta = LLVMBuildFSub(builder, attribv[1], attribv[0], "");

   offset     = lp_build_compare(gallivm, type, PIPE_FUNC_GREATER, delta, pos_half);
   offset     = LLVMBuildAnd(builder, offset, one, "");
   offset     = LLVMBuildBitCast(builder, offset, float_vec_type, "");
   attribv[0] = LLVMBuildFAdd(builder, attribv[0], offset, "");

   offset     = lp_build_compare(gallivm, type, PIPE_FUNC_LESS, delta, neg_half);
   offset     = LLVMBuildAnd(builder, offset, one, "");
   offset     = LLVMBuildBitCast(builder, offset, float_vec_type, "");
   attribv[1] = LLVMBuildFAdd(builder, attribv[1], offset, "");

   /* Edge v1 -> v2 */
   delta = LLVMBuildFSub(builder, attribv[2], attribv[1], "");

   offset     = lp_build_compare(gallivm, type, PIPE_FUNC_GREATER, delta, pos_half);
   offset     = LLVMBuildAnd(builder, offset, one, "");
   offset     = LLVMBuildBitCast(builder, offset, float_vec_type, "");
   attribv[1] = LLVMBuildFAdd(builder, attribv[1], offset, "");

   offset     = lp_build_compare(gallivm, type, PIPE_FUNC_LESS, delta, neg_half);
   offset     = LLVMBuildAnd(builder, offset, one, "");
   offset     = LLVMBuildBitCast(builder, offset, float_vec_type, "");
   attribv[2] = LLVMBuildFAdd(builder, attribv[2], offset, "");

   /* Edge v2 -> v0 */
   delta = LLVMBuildFSub(builder, attribv[0], attribv[2], "");

   offset     = lp_build_compare(gallivm, type, PIPE_FUNC_GREATER, delta, pos_half);
   offset     = LLVMBuildAnd(builder, offset, one, "");
   offset     = LLVMBuildBitCast(builder, offset, float_vec_type, "");
   attribv[2] = LLVMBuildFAdd(builder, attribv[2], offset, "");

   offset     = lp_build_compare(gallivm, type, PIPE_FUNC_LESS, delta, neg_half);
   offset     = LLVMBuildAnd(builder, offset, one, "");
   offset     = LLVMBuildBitCast(builder, offset, float_vec_type, "");
   attribv[0] = LLVMBuildFAdd(builder, attribv[0], offset, "");
}


/**
 * Compute the inputs-> dadx, dady, a0 values.
 */
static void 
emit_tri_coef( struct gallivm_state *gallivm,
               const struct lp_setup_variant_key *key,
               struct lp_setup_args *args)
{
   unsigned slot;

   LLVMValueRef attribs[3];

  /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < key->num_inputs; slot++) {
      switch (key->inputs[slot].interp) {
      case LP_INTERP_CONSTANT:
         load_attribute(gallivm, args, key, key->inputs[slot].src_index, attribs);
         if (key->flatshade_first) {
            emit_constant_coef4(gallivm, args, slot+1, attribs[0]);
         }
         else {
            emit_constant_coef4(gallivm, args, slot+1, attribs[2]);
         }
         break;

      case LP_INTERP_LINEAR:
         load_attribute(gallivm, args, key, key->inputs[slot].src_index, attribs);
	 emit_apply_cyl_wrap(gallivm, args, key->inputs[slot].cyl_wrap, attribs);
         emit_linear_coef(gallivm, args, slot+1, attribs);
         break;

      case LP_INTERP_PERSPECTIVE:
         load_attribute(gallivm, args, key, key->inputs[slot].src_index, attribs);
	 emit_apply_cyl_wrap(gallivm, args, key->inputs[slot].cyl_wrap, attribs);
         apply_perspective_corr(gallivm, args, slot+1, attribs);
         emit_linear_coef(gallivm, args, slot+1, attribs);
         break;

      case LP_INTERP_POSITION:
         /*
          * The generated pixel interpolators will pick up the coeffs from
          * slot 0.
          */
         break;

      case LP_INTERP_FACING:
         emit_facing_coef(gallivm, args, slot+1);
         break;

      default:
         assert(0);
      }
   }
}


/* XXX: generic code:
 */
static void
set_noalias(LLVMBuilderRef builder,
	    LLVMValueRef function,
	    const LLVMTypeRef *arg_types,
	    int nr_args)
{
   int i;
   for(i = 0; i < nr_args; ++i)
      if(LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         LLVMAddAttribute(LLVMGetParam(function, i),
			  LLVMNoAliasAttribute);
}

static void
init_args(struct gallivm_state *gallivm,
          const struct lp_setup_variant_key *key,
	  struct lp_setup_args *args)
{
   LLVMBuilderRef b = gallivm->builder;
   LLVMTypeRef shuf_type = LLVMInt32TypeInContext(gallivm->context);
   LLVMValueRef onef = lp_build_const_float(gallivm, 1.0);
   LLVMValueRef onei = lp_build_const_int32(gallivm, 1);
   LLVMValueRef zeroi = lp_build_const_int32(gallivm, 0);
   LLVMValueRef pixel_center, xy0_center, dxy01, dxy20, dyx20;
   LLVMValueRef e, f, ef, ooa;
   LLVMValueRef shuffles[4];
   LLVMValueRef attr_pos[3];
   struct lp_type typef4 = lp_type_float_vec(32, 128);

   /* The internal position input is in slot zero:
    */
   load_attribute(gallivm, args, key, 0, attr_pos);

   pixel_center = lp_build_const_vec(gallivm, typef4,
                                  key->pixel_center_half ? 0.5 : 0.0);

   /*
    * xy are first two elems in v0a/v1a/v2a but just use vec4 arit
    * also offset_tri uses actually xyz in them
    */
   xy0_center = LLVMBuildFSub(b, attr_pos[0], pixel_center, "xy0_center" );

   dxy01 = LLVMBuildFSub(b, attr_pos[0], attr_pos[1], "dxy01");
   dxy20 = LLVMBuildFSub(b, attr_pos[2], attr_pos[0], "dxy20");

   shuffles[0] = onei;
   shuffles[1] = zeroi;
   shuffles[2] = LLVMGetUndef(shuf_type);
   shuffles[3] = LLVMGetUndef(shuf_type);

   dyx20 = LLVMBuildShuffleVector(b, dxy20, dxy20, LLVMConstVector(shuffles, 4), "");

   ef = LLVMBuildFMul(b, dxy01, dyx20, "ef");
   e = LLVMBuildExtractElement(b, ef, zeroi, "");
   f = LLVMBuildExtractElement(b, ef, onei, "");

   ooa  = LLVMBuildFDiv(b, onef, LLVMBuildFSub(b, e, f, ""), "ooa");

   ooa = vec4f_from_scalar(gallivm, ooa, "");

   /* tri offset calc shares a lot of arithmetic, do it here */
   if (key->scale != 0.0f || key->units != 0.0f) {
      lp_do_offset_tri(gallivm, args, key, ooa, dxy01, dxy20, attr_pos);
   }

   dxy20 = LLVMBuildFMul(b, dxy20, ooa, "");
   dxy01 = LLVMBuildFMul(b, dxy01, ooa, "");

   args->dy20_ooa  = lp_build_extract_broadcast(gallivm, typef4, typef4, dxy20, onei);
   args->dy01_ooa  = lp_build_extract_broadcast(gallivm, typef4, typef4, dxy01, onei);

   args->dx20_ooa  = lp_build_extract_broadcast(gallivm, typef4, typef4, dxy20, zeroi);
   args->dx01_ooa  = lp_build_extract_broadcast(gallivm, typef4, typef4, dxy01, zeroi);

   args->x0_center = lp_build_extract_broadcast(gallivm, typef4, typef4, xy0_center, zeroi);
   args->y0_center = lp_build_extract_broadcast(gallivm, typef4, typef4, xy0_center, onei);

   /* might want to merge that with other coef emit in the future */
   emit_position_coef(gallivm, args, 0, attr_pos);
}

/**
 * Generate the runtime callable function for the coefficient calculation.
 *
 */
static struct lp_setup_variant *
generate_setup_variant(struct lp_setup_variant_key *key,
                       struct llvmpipe_context *lp)
{
   struct lp_setup_variant *variant = NULL;
   struct gallivm_state *gallivm;
   struct lp_setup_args args;
   char func_name[256];
   LLVMTypeRef vec4f_type;
   LLVMTypeRef func_type;
   LLVMTypeRef arg_types[7];
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   int64_t t0 = 0, t1;

   if (0)
      goto fail;

   variant = CALLOC_STRUCT(lp_setup_variant);
   if (variant == NULL)
      goto fail;

   variant->gallivm = gallivm = gallivm_create();
   if (!variant->gallivm) {
      goto fail;
   }

   builder = gallivm->builder;

   if (LP_DEBUG & DEBUG_COUNTERS) {
      t0 = os_time_get();
   }

   memcpy(&variant->key, key, key->size);
   variant->list_item_global.base = variant;

   util_snprintf(func_name, sizeof(func_name), "fs%u_setup%u",
		 0,
		 variant->no);

   /* Currently always deal with full 4-wide vertex attributes from
    * the vertices.
    */

   vec4f_type = LLVMVectorType(LLVMFloatTypeInContext(gallivm->context), 4);

   arg_types[0] = LLVMPointerType(vec4f_type, 0);        /* v0 */
   arg_types[1] = LLVMPointerType(vec4f_type, 0);        /* v1 */
   arg_types[2] = LLVMPointerType(vec4f_type, 0);        /* v2 */
   arg_types[3] = LLVMInt32TypeInContext(gallivm->context); /* facing */
   arg_types[4] = LLVMPointerType(vec4f_type, 0);	/* a0, aligned */
   arg_types[5] = LLVMPointerType(vec4f_type, 0);	/* dadx, aligned */
   arg_types[6] = LLVMPointerType(vec4f_type, 0);	/* dady, aligned */

   func_type = LLVMFunctionType(LLVMVoidTypeInContext(gallivm->context),
                                arg_types, Elements(arg_types), 0);

   variant->function = LLVMAddFunction(gallivm->module, func_name, func_type);
   if (!variant->function)
      goto fail;

   LLVMSetFunctionCallConv(variant->function, LLVMCCallConv);

   args.v0       = LLVMGetParam(variant->function, 0);
   args.v1       = LLVMGetParam(variant->function, 1);
   args.v2       = LLVMGetParam(variant->function, 2);
   args.facing   = LLVMGetParam(variant->function, 3);
   args.a0       = LLVMGetParam(variant->function, 4);
   args.dadx     = LLVMGetParam(variant->function, 5);
   args.dady     = LLVMGetParam(variant->function, 6);

   lp_build_name(args.v0, "in_v0");
   lp_build_name(args.v1, "in_v1");
   lp_build_name(args.v2, "in_v2");
   lp_build_name(args.facing, "in_facing");
   lp_build_name(args.a0, "out_a0");
   lp_build_name(args.dadx, "out_dadx");
   lp_build_name(args.dady, "out_dady");

   /*
    * Function body
    */
   block = LLVMAppendBasicBlockInContext(gallivm->context,
                                         variant->function, "entry");
   LLVMPositionBuilderAtEnd(builder, block);

   set_noalias(builder, variant->function, arg_types, Elements(arg_types));
   init_args(gallivm, &variant->key, &args);
   emit_tri_coef(gallivm, &variant->key, &args);

   LLVMBuildRetVoid(builder);

   gallivm_verify_function(gallivm, variant->function);

   gallivm_compile_module(gallivm);

   variant->jit_function = (lp_jit_setup_triangle)
      gallivm_jit_function(gallivm, variant->function);
   if (!variant->jit_function)
      goto fail;

   /*
    * Update timing information:
    */
   if (LP_DEBUG & DEBUG_COUNTERS) {
      t1 = os_time_get();
      LP_COUNT_ADD(llvm_compile_time, t1 - t0);
      LP_COUNT_ADD(nr_llvm_compiles, 1);
   }
   
   return variant;

fail:
   if (variant) {
      if (variant->function) {
         gallivm_free_function(gallivm,
                               variant->function,
                               variant->jit_function);
      }
      if (variant->gallivm) {
         gallivm_destroy(variant->gallivm);
      }
      FREE(variant);
   }
   
   return NULL;
}



static void
lp_make_setup_variant_key(struct llvmpipe_context *lp,
			  struct lp_setup_variant_key *key)
{
   struct lp_fragment_shader *fs = lp->fs;
   unsigned i;

   assert(sizeof key->inputs[0] == sizeof(uint));
   
   key->num_inputs = fs->info.base.num_inputs;
   key->flatshade_first = lp->rasterizer->flatshade_first;
   key->pixel_center_half = lp->rasterizer->gl_rasterization_rules;
   key->twoside = lp->rasterizer->light_twoside;
   key->size = Offset(struct lp_setup_variant_key,
		      inputs[key->num_inputs]);

   key->color_slot  = lp->color_slot [0];
   key->bcolor_slot = lp->bcolor_slot[0];
   key->spec_slot   = lp->color_slot [1];
   key->bspec_slot  = lp->bcolor_slot[1];
   assert(key->color_slot  == lp->color_slot [0]);
   assert(key->bcolor_slot == lp->bcolor_slot[0]);
   assert(key->spec_slot   == lp->color_slot [1]);
   assert(key->bspec_slot  == lp->bcolor_slot[1]);

   key->units = (float) (lp->rasterizer->offset_units * lp->mrd);
   key->scale = lp->rasterizer->offset_scale;
   key->pad = 0;
   memcpy(key->inputs, fs->inputs, key->num_inputs * sizeof key->inputs[0]);
   for (i = 0; i < key->num_inputs; i++) {
      if (key->inputs[i].interp == LP_INTERP_COLOR) {
         if (lp->rasterizer->flatshade)
	    key->inputs[i].interp = LP_INTERP_CONSTANT;
	 else
	    key->inputs[i].interp = LP_INTERP_PERSPECTIVE;
      }
   }

}


static void
remove_setup_variant(struct llvmpipe_context *lp,
		     struct lp_setup_variant *variant)
{
   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      debug_printf("llvmpipe: del setup_variant #%u total %u\n",
		   variant->no, lp->nr_setup_variants);
   }

   if (variant->function) {
      gallivm_free_function(variant->gallivm,
                            variant->function,
                            variant->jit_function);
   }

   if (variant->gallivm) {
      gallivm_destroy(variant->gallivm);
   }

   remove_from_list(&variant->list_item_global);
   lp->nr_setup_variants--;
   FREE(variant);
}



/* When the number of setup variants exceeds a threshold, cull a
 * fraction (currently a quarter) of them.
 */
static void
cull_setup_variants(struct llvmpipe_context *lp)
{
   struct pipe_context *pipe = &lp->pipe;
   int i;

   /*
    * XXX: we need to flush the context until we have some sort of reference
    * counting in fragment shaders as they may still be binned
    * Flushing alone might not be sufficient we need to wait on it too.
    */
   llvmpipe_finish(pipe, __FUNCTION__);

   for (i = 0; i < LP_MAX_SETUP_VARIANTS / 4; i++) {
      struct lp_setup_variant_list_item *item;
      if (is_empty_list(&lp->setup_variants_list)) {
         break;
      }
      item = last_elem(&lp->setup_variants_list);
      assert(item);
      assert(item->base);
      remove_setup_variant(lp, item->base);
   }
}


/**
 * Update fragment/vertex shader linkage state.  This is called just
 * prior to drawing something when some fragment-related state has
 * changed.
 */
void 
llvmpipe_update_setup(struct llvmpipe_context *lp)
{
   struct lp_setup_variant_key *key = &lp->setup_variant.key;
   struct lp_setup_variant *variant = NULL;
   struct lp_setup_variant_list_item *li;

   lp_make_setup_variant_key(lp, key);

   foreach(li, &lp->setup_variants_list) {
      if(li->base->key.size == key->size &&
	 memcmp(&li->base->key, key, key->size) == 0) {
         variant = li->base;
         break;
      }
   }

   if (variant) {
      move_to_head(&lp->setup_variants_list, &variant->list_item_global);
   }
   else {
      if (lp->nr_setup_variants >= LP_MAX_SETUP_VARIANTS) {
	 cull_setup_variants(lp);
      }

      variant = generate_setup_variant(key, lp);
      if (variant) {
         insert_at_head(&lp->setup_variants_list, &variant->list_item_global);
         lp->nr_setup_variants++;
         llvmpipe_variant_count++;
      }
   }

   lp_setup_set_setup_variant(lp->setup,
			      variant);
}

void
lp_delete_setup_variants(struct llvmpipe_context *lp)
{
   struct lp_setup_variant_list_item *li;
   li = first_elem(&lp->setup_variants_list);
   while(!at_end(&lp->setup_variants_list, li)) {
      struct lp_setup_variant_list_item *next = next_elem(li);
      remove_setup_variant(lp, li->base);
      li = next;
   }
}

void
lp_dump_setup_coef( const struct lp_setup_variant_key *key,
		    const float (*sa0)[4],
		    const float (*sdadx)[4],
		    const float (*sdady)[4])
{
   int i, slot;

   for (i = 0; i < TGSI_NUM_CHANNELS; i++) {
      float a0   = sa0  [0][i];
      float dadx = sdadx[0][i];
      float dady = sdady[0][i];

      debug_printf("POS.%c: a0 = %f, dadx = %f, dady = %f\n",
		   "xyzw"[i],
		   a0, dadx, dady);
   }

   for (slot = 0; slot < key->num_inputs; slot++) {
      unsigned usage_mask = key->inputs[slot].usage_mask;
      for (i = 0; i < TGSI_NUM_CHANNELS; i++) {
	 if (usage_mask & (1 << i)) {
	    float a0   = sa0  [1 + slot][i];
	    float dadx = sdadx[1 + slot][i];
	    float dady = sdady[1 + slot][i];

	    debug_printf("IN[%u].%c: a0 = %f, dadx = %f, dady = %f\n",
			 slot,
			 "xyzw"[i],
			 a0, dadx, dady);
	 }
      }
   }
}
