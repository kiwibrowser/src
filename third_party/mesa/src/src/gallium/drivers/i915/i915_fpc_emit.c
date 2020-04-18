/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_fpc.h"
#include "util/u_math.h"

uint
i915_get_temp(struct i915_fp_compile *p)
{
   int bit = ffs(~p->temp_flag);
   if (!bit) {
      i915_program_error(p, "i915_get_temp: out of temporaries");
      return 0;
   }

   p->temp_flag |= 1 << (bit - 1);
   return bit - 1;
}


static void
i915_release_temp(struct i915_fp_compile *p, int reg)
{
   p->temp_flag &= ~(1 << reg);
}


/**
 * Get unpreserved temporary, a temp whose value is not preserved between
 * PS program phases.
 */
uint
i915_get_utemp(struct i915_fp_compile * p)
{
   int bit = ffs(~p->utemp_flag);
   if (!bit) {
      i915_program_error(p, "i915_get_utemp: out of temporaries");
      return 0;
   }

   p->utemp_flag |= 1 << (bit - 1);
   return UREG(REG_TYPE_U, (bit - 1));
}

void
i915_release_utemps(struct i915_fp_compile *p)
{
   p->utemp_flag = ~0x7;
}


uint
i915_emit_decl(struct i915_fp_compile *p,
               uint type, uint nr, uint d0_flags)
{
   uint reg = UREG(type, nr);

   if (type == REG_TYPE_T) {
      if (p->decl_t & (1 << nr))
         return reg;

      p->decl_t |= (1 << nr);
   }
   else if (type == REG_TYPE_S) {
      if (p->decl_s & (1 << nr))
         return reg;

      p->decl_s |= (1 << nr);
   }
   else
      return reg;

   if (p->decl< p->declarations + I915_PROGRAM_SIZE) {
      *(p->decl++) = (D0_DCL | D0_DEST(reg) | d0_flags);
      *(p->decl++) = D1_MBZ;
      *(p->decl++) = D2_MBZ;
   }
   else
      i915_program_error(p, "Out of declarations");

   p->nr_decl_insn++;
   return reg;
}

uint
i915_emit_arith(struct i915_fp_compile * p,
                uint op,
                uint dest,
                uint mask,
                uint saturate, uint src0, uint src1, uint src2)
{
   uint c[3];
   uint nr_const = 0;

   assert(GET_UREG_TYPE(dest) != REG_TYPE_CONST);
   dest = UREG(GET_UREG_TYPE(dest), GET_UREG_NR(dest));
   assert(dest);

   if (GET_UREG_TYPE(src0) == REG_TYPE_CONST)
      c[nr_const++] = 0;
   if (GET_UREG_TYPE(src1) == REG_TYPE_CONST)
      c[nr_const++] = 1;
   if (GET_UREG_TYPE(src2) == REG_TYPE_CONST)
      c[nr_const++] = 2;

   /* Recursively call this function to MOV additional const values
    * into temporary registers.  Use utemp registers for this -
    * currently shouldn't be possible to run out, but keep an eye on
    * this.
    */
   if (nr_const > 1) {
      uint s[3], first, i, old_utemp_flag;

      s[0] = src0;
      s[1] = src1;
      s[2] = src2;
      old_utemp_flag = p->utemp_flag;

      first = GET_UREG_NR(s[c[0]]);
      for (i = 1; i < nr_const; i++) {
         if (GET_UREG_NR(s[c[i]]) != first) {
            uint tmp = i915_get_utemp(p);

            i915_emit_arith(p, A0_MOV, tmp, A0_DEST_CHANNEL_ALL, 0,
                            s[c[i]], 0, 0);
            s[c[i]] = tmp;
         }
      }

      src0 = s[0];
      src1 = s[1];
      src2 = s[2];
      p->utemp_flag = old_utemp_flag;   /* restore */
   }

   if (p->csr< p->program + I915_PROGRAM_SIZE) {
      *(p->csr++) = (op | A0_DEST(dest) | mask | saturate | A0_SRC0(src0));
      *(p->csr++) = (A1_SRC0(src0) | A1_SRC1(src1));
      *(p->csr++) = (A2_SRC1(src1) | A2_SRC2(src2));
   }
   else
      i915_program_error(p, "Out of instructions");

   if (GET_UREG_TYPE(dest) == REG_TYPE_R)
      p->register_phases[GET_UREG_NR(dest)] = p->nr_tex_indirect;

   p->nr_alu_insn++;
   return dest;
}


/**
 * Emit a texture load or texkill instruction.
 * \param dest  the dest i915 register
 * \param destmask  the dest register writemask
 * \param sampler  the i915 sampler register
 * \param coord  the i915 source texcoord operand
 * \param opcode  the instruction opcode
 */
uint i915_emit_texld( struct i915_fp_compile *p,
                      uint dest,
                      uint destmask,
                      uint sampler,
                      uint coord,
                      uint opcode,
                      uint num_coord )
{
   const uint k = UREG(GET_UREG_TYPE(coord), GET_UREG_NR(coord));

   int temp = -1;
   uint ignore = 0;

   /* Eliminate the useless texture coordinates. Otherwise we end up generating
    * a swizzle for no reason below. */
   switch(num_coord) {
      case 0:
         /* Ignore x */
         ignore |= (0xf << UREG_CHANNEL_X_SHIFT);
      case 1:
         /* Ignore y */
         ignore |= (0xf << UREG_CHANNEL_Y_SHIFT);
      case 2:
         /* Ignore z */
         ignore |= (0xf << UREG_CHANNEL_Z_SHIFT);
      case 3:
         /* Ignore w */
         ignore |= (0xf << UREG_CHANNEL_W_SHIFT);
   }

   if ( (coord & ~ignore ) != (k & ~ignore) ) {
      /* texcoord is swizzled or negated.  Need to allocate a new temporary
       * register (a utemp / unpreserved temp) won't do.
       */
      uint tempReg;

      temp = i915_get_temp(p);           /* get temp reg index */
      tempReg = UREG(REG_TYPE_R, temp);  /* make i915 register */

      i915_emit_arith( p, A0_MOV,
                       tempReg, A0_DEST_CHANNEL_ALL, /* dest reg, writemask */
                       0,                            /* saturate */
                       coord, 0, 0 );                /* src0, src1, src2 */

      /* new src texcoord is tempReg */
      coord = tempReg;
   }

   /* Don't worry about saturate as we only support  
    */
   if (destmask != A0_DEST_CHANNEL_ALL) {
      /* if not writing to XYZW... */
      uint tmp = i915_get_utemp(p);
      i915_emit_texld( p, tmp, A0_DEST_CHANNEL_ALL, sampler, coord, opcode, num_coord );
      i915_emit_arith( p, A0_MOV, dest, destmask, 0, tmp, 0, 0 );
      /* XXX release utemp here? */
   }
   else {
      assert(GET_UREG_TYPE(dest) != REG_TYPE_CONST);
      assert(dest == UREG(GET_UREG_TYPE(dest), GET_UREG_NR(dest)));

      /* Output register being oC or oD defines a phase boundary */
      if (GET_UREG_TYPE(dest) == REG_TYPE_OC ||
          GET_UREG_TYPE(dest) == REG_TYPE_OD)
         p->nr_tex_indirect++;

      /* Reading from an r# register whose contents depend on output of the
       * current phase defines a phase boundary.
       */
      if (GET_UREG_TYPE(coord) == REG_TYPE_R &&
          p->register_phases[GET_UREG_NR(coord)] == p->nr_tex_indirect)
         p->nr_tex_indirect++;

      if (p->csr< p->program + I915_PROGRAM_SIZE) {
         *(p->csr++) = (opcode |
                        T0_DEST( dest ) |
                        T0_SAMPLER( sampler ));

         *(p->csr++) = T1_ADDRESS_REG( coord );
         *(p->csr++) = T2_MBZ;
      }
      else
         i915_program_error(p, "Out of instructions");

      if (GET_UREG_TYPE(dest) == REG_TYPE_R)
         p->register_phases[GET_UREG_NR(dest)] = p->nr_tex_indirect;

      p->nr_tex_insn++;
   }

   if (temp >= 0)
      i915_release_temp(p, temp);

   return dest;
}


uint
i915_emit_const1f(struct i915_fp_compile * p, float c0)
{
   struct i915_fragment_shader *ifs = p->shader;
   unsigned reg, idx;

   if (c0 == 0.0)
      return swizzle(UREG(REG_TYPE_R, 0), ZERO, ZERO, ZERO, ZERO);
   if (c0 == 1.0)
      return swizzle(UREG(REG_TYPE_R, 0), ONE, ONE, ONE, ONE);

   for (reg = 0; reg < I915_MAX_CONSTANT; reg++) {
      if (ifs->constant_flags[reg] == I915_CONSTFLAG_USER)
         continue;
      for (idx = 0; idx < 4; idx++) {
         if (!(ifs->constant_flags[reg] & (1 << idx)) ||
             ifs->constants[reg][idx] == c0) {
            ifs->constants[reg][idx] = c0;
            ifs->constant_flags[reg] |= 1 << idx;
            if (reg + 1 > ifs->num_constants)
               ifs->num_constants = reg + 1;
            return swizzle(UREG(REG_TYPE_CONST, reg), idx, ZERO, ZERO, ONE);
         }
      }
   }

   i915_program_error(p, "i915_emit_const1f: out of constants");
   return 0;
}

uint
i915_emit_const2f(struct i915_fp_compile * p, float c0, float c1)
{
   struct i915_fragment_shader *ifs = p->shader;
   unsigned reg, idx;

   if (c0 == 0.0)
      return swizzle(i915_emit_const1f(p, c1), ZERO, X, Z, W);
   if (c0 == 1.0)
      return swizzle(i915_emit_const1f(p, c1), ONE, X, Z, W);

   if (c1 == 0.0)
      return swizzle(i915_emit_const1f(p, c0), X, ZERO, Z, W);
   if (c1 == 1.0)
      return swizzle(i915_emit_const1f(p, c0), X, ONE, Z, W);

   // XXX emit swizzle here for 0, 1, -1 and any combination thereof
   // we can use swizzle + neg for that
   for (reg = 0; reg < I915_MAX_CONSTANT; reg++) {
      if (ifs->constant_flags[reg] == 0xf ||
          ifs->constant_flags[reg] == I915_CONSTFLAG_USER)
         continue;
      for (idx = 0; idx < 3; idx++) {
         if (!(ifs->constant_flags[reg] & (3 << idx))) {
            ifs->constants[reg][idx + 0] = c0;
            ifs->constants[reg][idx + 1] = c1;
            ifs->constant_flags[reg] |= 3 << idx;
            if (reg + 1 > ifs->num_constants)
               ifs->num_constants = reg + 1;
            return swizzle(UREG(REG_TYPE_CONST, reg), idx, idx + 1, ZERO, ONE);
         }
      }
   }

   i915_program_error(p, "i915_emit_const2f: out of constants");
   return 0;
}

uint
i915_emit_const4f(struct i915_fp_compile * p,
                  float c0, float c1, float c2, float c3)
{
   struct i915_fragment_shader *ifs = p->shader;
   unsigned reg;

   // XXX emit swizzle here for 0, 1, -1 and any combination thereof
   // we can use swizzle + neg for that
   for (reg = 0; reg < I915_MAX_CONSTANT; reg++) {
      if (ifs->constant_flags[reg] == 0xf &&
          ifs->constants[reg][0] == c0 &&
          ifs->constants[reg][1] == c1 &&
          ifs->constants[reg][2] == c2 &&
          ifs->constants[reg][3] == c3) {
         return UREG(REG_TYPE_CONST, reg);
      }
      else if (ifs->constant_flags[reg] == 0) {

         ifs->constants[reg][0] = c0;
         ifs->constants[reg][1] = c1;
         ifs->constants[reg][2] = c2;
         ifs->constants[reg][3] = c3;
         ifs->constant_flags[reg] = 0xf;
         if (reg + 1 > ifs->num_constants)
            ifs->num_constants = reg + 1;
         return UREG(REG_TYPE_CONST, reg);
      }
   }

   i915_program_error(p, "i915_emit_const4f: out of constants");
   return 0;
}


uint
i915_emit_const4fv(struct i915_fp_compile * p, const float * c)
{
   return i915_emit_const4f(p, c[0], c[1], c[2], c[3]);
}
