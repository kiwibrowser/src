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


#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#include "svga_tgsi_emit.h"
#include "svga_context.h"


static boolean emit_vs_postamble( struct svga_shader_emitter *emit );
static boolean emit_ps_postamble( struct svga_shader_emitter *emit );


static unsigned
translate_opcode(
   uint opcode )
{
   switch (opcode) {
   case TGSI_OPCODE_ABS:        return SVGA3DOP_ABS;
   case TGSI_OPCODE_ADD:        return SVGA3DOP_ADD;
   case TGSI_OPCODE_BREAKC:     return SVGA3DOP_BREAKC;
   case TGSI_OPCODE_DP2A:       return SVGA3DOP_DP2ADD;
   case TGSI_OPCODE_DP3:        return SVGA3DOP_DP3;
   case TGSI_OPCODE_DP4:        return SVGA3DOP_DP4;
   case TGSI_OPCODE_FRC:        return SVGA3DOP_FRC;
   case TGSI_OPCODE_MAD:        return SVGA3DOP_MAD;
   case TGSI_OPCODE_MAX:        return SVGA3DOP_MAX;
   case TGSI_OPCODE_MIN:        return SVGA3DOP_MIN;
   case TGSI_OPCODE_MOV:        return SVGA3DOP_MOV;
   case TGSI_OPCODE_MUL:        return SVGA3DOP_MUL;
   case TGSI_OPCODE_NOP:        return SVGA3DOP_NOP;
   case TGSI_OPCODE_NRM4:       return SVGA3DOP_NRM;
   default:
      debug_printf("Unkown opcode %u\n", opcode);
      assert( 0 );
      return SVGA3DOP_LAST_INST;
   }
}


static unsigned translate_file( unsigned file )
{
   switch (file) {
   case TGSI_FILE_TEMPORARY: return SVGA3DREG_TEMP;
   case TGSI_FILE_INPUT:     return SVGA3DREG_INPUT;
   case TGSI_FILE_OUTPUT:    return SVGA3DREG_OUTPUT; /* VS3.0+ only */
   case TGSI_FILE_IMMEDIATE: return SVGA3DREG_CONST;
   case TGSI_FILE_CONSTANT:  return SVGA3DREG_CONST;
   case TGSI_FILE_SAMPLER:   return SVGA3DREG_SAMPLER;
   case TGSI_FILE_ADDRESS:   return SVGA3DREG_ADDR;
   default:
      assert( 0 );
      return SVGA3DREG_TEMP;
   }
}


static SVGA3dShaderDestToken
translate_dst_register( struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn,
                        unsigned idx )
{
   const struct tgsi_full_dst_register *reg = &insn->Dst[idx];
   SVGA3dShaderDestToken dest;

   switch (reg->Register.File) {
   case TGSI_FILE_OUTPUT:
      /* Output registers encode semantic information in their name.
       * Need to lookup a table built at decl time:
       */
      dest = emit->output_map[reg->Register.Index];
      break;

   default:
      {
         unsigned index = reg->Register.Index;
         assert(index < SVGA3D_TEMPREG_MAX);
         index = MIN2(index, SVGA3D_TEMPREG_MAX - 1);
         dest = dst_register(translate_file(reg->Register.File), index);
      }
      break;
   }

   dest.mask = reg->Register.WriteMask;
   assert(dest.mask);

   if (insn->Instruction.Saturate)
      dest.dstMod = SVGA3DDSTMOD_SATURATE;

   return dest;
}


static struct src_register
swizzle( struct src_register src,
         int x,
         int y,
         int z,
         int w )
{
   x = (src.base.swizzle >> (x * 2)) & 0x3;
   y = (src.base.swizzle >> (y * 2)) & 0x3;
   z = (src.base.swizzle >> (z * 2)) & 0x3;
   w = (src.base.swizzle >> (w * 2)) & 0x3;

   src.base.swizzle = TRANSLATE_SWIZZLE(x,y,z,w);

   return src;
}

static struct src_register
scalar( struct src_register src,
        int comp )
{
   return swizzle( src, comp, comp, comp, comp );
}

static INLINE boolean
svga_arl_needs_adjustment( const struct svga_shader_emitter *emit )
{
   int i;

   for (i = 0; i < emit->num_arl_consts; ++i) {
      if (emit->arl_consts[i].arl_num == emit->current_arl)
         return TRUE;
   }
   return FALSE;
}

static INLINE int
svga_arl_adjustment( const struct svga_shader_emitter *emit )
{
   int i;

   for (i = 0; i < emit->num_arl_consts; ++i) {
      if (emit->arl_consts[i].arl_num == emit->current_arl)
         return emit->arl_consts[i].number;
   }
   return 0;
}

static struct src_register
translate_src_register( const struct svga_shader_emitter *emit,
                        const struct tgsi_full_src_register *reg )
{
   struct src_register src;

   switch (reg->Register.File) {
   case TGSI_FILE_INPUT:
      /* Input registers are referred to by their semantic name rather
       * than by index.  Use the mapping build up from the decls:
       */
      src = emit->input_map[reg->Register.Index];
      break;

   case TGSI_FILE_IMMEDIATE:
      /* Immediates are appended after TGSI constants in the D3D
       * constant buffer.
       */
      src = src_register( translate_file( reg->Register.File ),
                          reg->Register.Index + emit->imm_start );
      break;

   default:
      src = src_register( translate_file( reg->Register.File ),
                          reg->Register.Index );

      break;
   }

   /* Indirect addressing.
    */
   if (reg->Register.Indirect) {
      if (emit->unit == PIPE_SHADER_FRAGMENT) {
         /* Pixel shaders have only loop registers for relative
          * addressing into inputs. Ignore the redundant address
          * register, the contents of aL should be in sync with it.
          */
         if (reg->Register.File == TGSI_FILE_INPUT) {
            src.base.relAddr = 1;
            src.indirect = src_token(SVGA3DREG_LOOP, 0);
         }
      }
      else {
         /* Constant buffers only.
          */
         if (reg->Register.File == TGSI_FILE_CONSTANT) {
            /* we shift the offset towards the minimum */
            if (svga_arl_needs_adjustment( emit )) {
               src.base.num -= svga_arl_adjustment( emit );
            }
            src.base.relAddr = 1;

            /* Not really sure what should go in the second token:
             */
            src.indirect = src_token( SVGA3DREG_ADDR,
                                      reg->Indirect.Index );

            src.indirect.swizzle = SWIZZLE_XXXX;
         }
      }
   }

   src = swizzle( src,
                  reg->Register.SwizzleX,
                  reg->Register.SwizzleY,
                  reg->Register.SwizzleZ,
                  reg->Register.SwizzleW );

   /* src.mod isn't a bitfield, unfortunately:
    * See tgsi_util_get_full_src_register_sign_mode for implementation details.
    */
   if (reg->Register.Absolute) {
      if (reg->Register.Negate)
         src.base.srcMod = SVGA3DSRCMOD_ABSNEG;
      else
         src.base.srcMod = SVGA3DSRCMOD_ABS;
   }
   else {
      if (reg->Register.Negate)
         src.base.srcMod = SVGA3DSRCMOD_NEG;
      else
         src.base.srcMod = SVGA3DSRCMOD_NONE;
   }

   return src;
}


/*
 * Get a temporary register.
 * Note: if we exceed the temporary register limit we just use
 * register SVGA3D_TEMPREG_MAX - 1.
 */
static INLINE SVGA3dShaderDestToken
get_temp( struct svga_shader_emitter *emit )
{
   int i = emit->nr_hw_temp + emit->internal_temp_count++;
   assert(i < SVGA3D_TEMPREG_MAX);
   i = MIN2(i, SVGA3D_TEMPREG_MAX - 1);
   return dst_register( SVGA3DREG_TEMP, i );
}

/* Release a single temp.  Currently only effective if it was the last
 * allocated temp, otherwise release will be delayed until the next
 * call to reset_temp_regs().
 */
static INLINE void
release_temp( struct svga_shader_emitter *emit,
              SVGA3dShaderDestToken temp )
{
   if (temp.num == emit->internal_temp_count - 1)
      emit->internal_temp_count--;
}

static void reset_temp_regs( struct svga_shader_emitter *emit )
{
   emit->internal_temp_count = 0;
}


/* Replace the src with the temporary specified in the dst, but copying
 * only the necessary channels, and preserving the original swizzle (which is
 * important given that several opcodes have constraints in the allowed
 * swizzles).
 */
static boolean emit_repl( struct svga_shader_emitter *emit,
                          SVGA3dShaderDestToken dst,
                          struct src_register *src0)
{
   unsigned src0_swizzle;
   unsigned chan;

   assert(SVGA3dShaderGetRegType(dst.value) == SVGA3DREG_TEMP);

   src0_swizzle = src0->base.swizzle;

   dst.mask = 0;
   for (chan = 0; chan < 4; ++chan) {
      unsigned swizzle = (src0_swizzle >> (chan *2)) & 0x3;
      dst.mask |= 1 << swizzle;
   }
   assert(dst.mask);

   src0->base.swizzle = SVGA3DSWIZZLE_NONE;

   if (!emit_op1( emit, inst_token( SVGA3DOP_MOV ), dst, *src0 ))
      return FALSE;

   *src0 = src( dst );
   src0->base.swizzle = src0_swizzle;

   return TRUE;
}


static boolean submit_op0( struct svga_shader_emitter *emit,
                           SVGA3dShaderInstToken inst,
                           SVGA3dShaderDestToken dest )
{
   return (emit_instruction( emit, inst ) &&
           emit_dst( emit, dest ));
}

static boolean submit_op1( struct svga_shader_emitter *emit,
                           SVGA3dShaderInstToken inst,
                           SVGA3dShaderDestToken dest,
                           struct src_register src0 )
{
   return emit_op1( emit, inst, dest, src0 );
}


/* SVGA shaders may not refer to >1 constant register in a single
 * instruction.  This function checks for that usage and inserts a
 * move to temporary if detected.
 *
 * The same applies to input registers -- at most a single input
 * register may be read by any instruction.
 */
static boolean submit_op2( struct svga_shader_emitter *emit,
                           SVGA3dShaderInstToken inst,
                           SVGA3dShaderDestToken dest,
                           struct src_register src0,
                           struct src_register src1 )
{
   SVGA3dShaderDestToken temp;
   SVGA3dShaderRegType type0, type1;
   boolean need_temp = FALSE;

   temp.value = 0;
   type0 = SVGA3dShaderGetRegType( src0.base.value );
   type1 = SVGA3dShaderGetRegType( src1.base.value );

   if (type0 == SVGA3DREG_CONST &&
       type1 == SVGA3DREG_CONST &&
       src0.base.num != src1.base.num)
      need_temp = TRUE;

   if (type0 == SVGA3DREG_INPUT &&
       type1 == SVGA3DREG_INPUT &&
       src0.base.num != src1.base.num)
      need_temp = TRUE;

   if (need_temp) {
      temp = get_temp( emit );

      if (!emit_repl( emit, temp, &src0 ))
         return FALSE;
   }

   if (!emit_op2( emit, inst, dest, src0, src1 ))
      return FALSE;

   if (need_temp)
      release_temp( emit, temp );

   return TRUE;
}


/* SVGA shaders may not refer to >1 constant register in a single
 * instruction.  This function checks for that usage and inserts a
 * move to temporary if detected.
 */
static boolean submit_op3( struct svga_shader_emitter *emit,
                           SVGA3dShaderInstToken inst,
                           SVGA3dShaderDestToken dest,
                           struct src_register src0,
                           struct src_register src1,
                           struct src_register src2 )
{
   SVGA3dShaderDestToken temp0;
   SVGA3dShaderDestToken temp1;
   boolean need_temp0 = FALSE;
   boolean need_temp1 = FALSE;
   SVGA3dShaderRegType type0, type1, type2;

   temp0.value = 0;
   temp1.value = 0;
   type0 = SVGA3dShaderGetRegType( src0.base.value );
   type1 = SVGA3dShaderGetRegType( src1.base.value );
   type2 = SVGA3dShaderGetRegType( src2.base.value );

   if (inst.op != SVGA3DOP_SINCOS) {
      if (type0 == SVGA3DREG_CONST &&
          ((type1 == SVGA3DREG_CONST && src0.base.num != src1.base.num) ||
           (type2 == SVGA3DREG_CONST && src0.base.num != src2.base.num)))
         need_temp0 = TRUE;

      if (type1 == SVGA3DREG_CONST &&
          (type2 == SVGA3DREG_CONST && src1.base.num != src2.base.num))
         need_temp1 = TRUE;
   }

   if (type0 == SVGA3DREG_INPUT &&
       ((type1 == SVGA3DREG_INPUT && src0.base.num != src1.base.num) ||
        (type2 == SVGA3DREG_INPUT && src0.base.num != src2.base.num)))
      need_temp0 = TRUE;

   if (type1 == SVGA3DREG_INPUT &&
       (type2 == SVGA3DREG_INPUT && src1.base.num != src2.base.num))
      need_temp1 = TRUE;

   if (need_temp0) {
      temp0 = get_temp( emit );

      if (!emit_repl( emit, temp0, &src0 ))
         return FALSE;
   }

   if (need_temp1) {
      temp1 = get_temp( emit );

      if (!emit_repl( emit, temp1, &src1 ))
         return FALSE;
   }

   if (!emit_op3( emit, inst, dest, src0, src1, src2 ))
      return FALSE;

   if (need_temp1)
      release_temp( emit, temp1 );
   if (need_temp0)
      release_temp( emit, temp0 );
   return TRUE;
}




/* SVGA shaders may not refer to >1 constant register in a single
 * instruction.  This function checks for that usage and inserts a
 * move to temporary if detected.
 */
static boolean submit_op4( struct svga_shader_emitter *emit,
                           SVGA3dShaderInstToken inst,
                           SVGA3dShaderDestToken dest,
                           struct src_register src0,
                           struct src_register src1,
                           struct src_register src2,
                           struct src_register src3)
{
   SVGA3dShaderDestToken temp0;
   SVGA3dShaderDestToken temp3;
   boolean need_temp0 = FALSE;
   boolean need_temp3 = FALSE;
   SVGA3dShaderRegType type0, type1, type2, type3;

   temp0.value = 0;
   temp3.value = 0;
   type0 = SVGA3dShaderGetRegType( src0.base.value );
   type1 = SVGA3dShaderGetRegType( src1.base.value );
   type2 = SVGA3dShaderGetRegType( src2.base.value );
   type3 = SVGA3dShaderGetRegType( src2.base.value );

   /* Make life a little easier - this is only used by the TXD
    * instruction which is guaranteed not to have a constant/input reg
    * in one slot at least:
    */
   assert(type1 == SVGA3DREG_SAMPLER);

   if (type0 == SVGA3DREG_CONST &&
       ((type3 == SVGA3DREG_CONST && src0.base.num != src3.base.num) ||
        (type2 == SVGA3DREG_CONST && src0.base.num != src2.base.num)))
      need_temp0 = TRUE;

   if (type3 == SVGA3DREG_CONST &&
       (type2 == SVGA3DREG_CONST && src3.base.num != src2.base.num))
      need_temp3 = TRUE;

   if (type0 == SVGA3DREG_INPUT &&
       ((type3 == SVGA3DREG_INPUT && src0.base.num != src3.base.num) ||
        (type2 == SVGA3DREG_INPUT && src0.base.num != src2.base.num)))
      need_temp0 = TRUE;

   if (type3 == SVGA3DREG_INPUT &&
       (type2 == SVGA3DREG_INPUT && src3.base.num != src2.base.num))
      need_temp3 = TRUE;

   if (need_temp0) {
      temp0 = get_temp( emit );

      if (!emit_repl( emit, temp0, &src0 ))
         return FALSE;
   }

   if (need_temp3) {
      temp3 = get_temp( emit );

      if (!emit_repl( emit, temp3, &src3 ))
         return FALSE;
   }

   if (!emit_op4( emit, inst, dest, src0, src1, src2, src3 ))
      return FALSE;

   if (need_temp3)
      release_temp( emit, temp3 );
   if (need_temp0)
      release_temp( emit, temp0 );
   return TRUE;
}


static boolean alias_src_dst( struct src_register src,
                              SVGA3dShaderDestToken dst )
{
   if (src.base.num != dst.num)
      return FALSE;

   if (SVGA3dShaderGetRegType(dst.value) !=
       SVGA3dShaderGetRegType(src.base.value))
      return FALSE;

   return TRUE;
}


static boolean submit_lrp(struct svga_shader_emitter *emit,
                          SVGA3dShaderDestToken dst,
                          struct src_register src0,
                          struct src_register src1,
                          struct src_register src2)
{
   SVGA3dShaderDestToken tmp;
   boolean need_dst_tmp = FALSE;

   /* The dst reg must be a temporary, and not be the same as src0 or src2 */
   if (SVGA3dShaderGetRegType(dst.value) != SVGA3DREG_TEMP ||
       alias_src_dst(src0, dst) ||
       alias_src_dst(src2, dst))
      need_dst_tmp = TRUE;

   if (need_dst_tmp) {
      tmp = get_temp( emit );
      tmp.mask = dst.mask;
   }
   else {
      tmp = dst;
   }

   if (!submit_op3(emit, inst_token( SVGA3DOP_LRP ), tmp, src0, src1, src2))
      return FALSE;

   if (need_dst_tmp) {
      if (!submit_op1(emit, inst_token( SVGA3DOP_MOV ), dst, src( tmp )))
         return FALSE;
   }

   return TRUE;
}


static boolean emit_def_const( struct svga_shader_emitter *emit,
                               SVGA3dShaderConstType type,
                               unsigned idx,
                               float a,
                               float b,
                               float c,
                               float d )
{
   SVGA3DOpDefArgs def;
   SVGA3dShaderInstToken opcode;

   switch (type) {
   case SVGA3D_CONST_TYPE_FLOAT:
      opcode = inst_token( SVGA3DOP_DEF );
      def.dst = dst_register( SVGA3DREG_CONST, idx );
      def.constValues[0] = a;
      def.constValues[1] = b;
      def.constValues[2] = c;
      def.constValues[3] = d;
      break;
   case SVGA3D_CONST_TYPE_INT:
      opcode = inst_token( SVGA3DOP_DEFI );
      def.dst = dst_register( SVGA3DREG_CONSTINT, idx );
      def.constIValues[0] = (int)a;
      def.constIValues[1] = (int)b;
      def.constIValues[2] = (int)c;
      def.constIValues[3] = (int)d;
      break;
   default:
      assert(0);
      opcode = inst_token( SVGA3DOP_NOP );
      break;
   }

   if (!emit_instruction(emit, opcode) ||
       !svga_shader_emit_dwords( emit, def.values, Elements(def.values)))
      return FALSE;

   return TRUE;
}

static INLINE boolean
create_zero_immediate( struct svga_shader_emitter *emit )
{
   unsigned idx = emit->nr_hw_float_const++;

   /* Emit the constant (0, 0.5, -1, 1) and use swizzling to generate
    * other useful vectors.
    */
   if (!emit_def_const( emit, SVGA3D_CONST_TYPE_FLOAT,
                        idx, 0, 0.5, -1, 1 ))
      return FALSE;

   emit->zero_immediate_idx = idx;
   emit->created_zero_immediate = TRUE;

   return TRUE;
}

static INLINE boolean
create_loop_const( struct svga_shader_emitter *emit )
{
   unsigned idx = emit->nr_hw_int_const++;

   if (!emit_def_const( emit, SVGA3D_CONST_TYPE_INT, idx,
                        255, /* iteration count */
                        0, /* initial value */
                        1, /* step size */
                        0 /* not used, must be 0 */))
      return FALSE;

   emit->loop_const_idx = idx;
   emit->created_loop_const = TRUE;

   return TRUE;
}

static INLINE boolean
create_arl_consts( struct svga_shader_emitter *emit )
{
   int i;

   for (i = 0; i < emit->num_arl_consts; i += 4) {
      int j;
      unsigned idx = emit->nr_hw_float_const++;
      float vals[4];
      for (j = 0; j < 4 && (j + i) < emit->num_arl_consts; ++j) {
         vals[j] = emit->arl_consts[i + j].number;
         emit->arl_consts[i + j].idx = idx;
         switch (j) {
         case 0:
            emit->arl_consts[i + 0].swizzle = TGSI_SWIZZLE_X;
            break;
         case 1:
            emit->arl_consts[i + 0].swizzle = TGSI_SWIZZLE_Y;
            break;
         case 2:
            emit->arl_consts[i + 0].swizzle = TGSI_SWIZZLE_Z;
            break;
         case 3:
            emit->arl_consts[i + 0].swizzle = TGSI_SWIZZLE_W;
            break;
         }
      }
      while (j < 4)
         vals[j++] = 0;

      if (!emit_def_const( emit, SVGA3D_CONST_TYPE_FLOAT, idx,
                           vals[0], vals[1],
                           vals[2], vals[3]))
         return FALSE;
   }

   return TRUE;
}

static INLINE struct src_register
get_vface( struct svga_shader_emitter *emit )
{
   assert(emit->emitted_vface);
   return src_register(SVGA3DREG_MISCTYPE, SVGA3DMISCREG_FACE);
}

/* returns {0, 0, 0, 1} immediate */
static INLINE struct src_register
get_zero_immediate( struct svga_shader_emitter *emit )
{
   assert(emit->created_zero_immediate);
   assert(emit->zero_immediate_idx >= 0);
   return swizzle(src_register( SVGA3DREG_CONST,
                                emit->zero_immediate_idx),
                  0, 0, 0, 3);
}

/* returns {1, 1, 1, -1} immediate */
static INLINE struct src_register
get_pos_neg_one_immediate( struct svga_shader_emitter *emit )
{
   assert(emit->created_zero_immediate);
   assert(emit->zero_immediate_idx >= 0);
   return swizzle(src_register( SVGA3DREG_CONST,
                                emit->zero_immediate_idx),
                  3, 3, 3, 2);
}

/* returns {0.5, 0.5, 0.5, 0.5} immediate */
static INLINE struct src_register
get_half_immediate( struct svga_shader_emitter *emit )
{
   assert(emit->created_zero_immediate);
   assert(emit->zero_immediate_idx >= 0);
   return swizzle(src_register(SVGA3DREG_CONST, emit->zero_immediate_idx),
                  1, 1, 1, 1);
}

/* returns the loop const */
static INLINE struct src_register
get_loop_const( struct svga_shader_emitter *emit )
{
   assert(emit->created_loop_const);
   assert(emit->loop_const_idx >= 0);
   return src_register( SVGA3DREG_CONSTINT,
                        emit->loop_const_idx );
}

static INLINE struct src_register
get_fake_arl_const( struct svga_shader_emitter *emit )
{
   struct src_register reg;
   int idx = 0, swizzle = 0, i;

   for (i = 0; i < emit->num_arl_consts; ++ i) {
      if (emit->arl_consts[i].arl_num == emit->current_arl) {
         idx = emit->arl_consts[i].idx;
         swizzle = emit->arl_consts[i].swizzle;
      }
   }

   reg = src_register( SVGA3DREG_CONST, idx );
   return scalar(reg, swizzle);
}

static INLINE struct src_register
get_tex_dimensions( struct svga_shader_emitter *emit, int sampler_num )
{
   int idx;
   struct src_register reg;

   /* the width/height indexes start right after constants */
   idx = emit->key.fkey.tex[sampler_num].width_height_idx +
         emit->info.file_max[TGSI_FILE_CONSTANT] + 1;

   reg = src_register( SVGA3DREG_CONST, idx );
   return reg;
}

static boolean emit_fake_arl(struct svga_shader_emitter *emit,
                             const struct tgsi_full_instruction *insn)
{
   const struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   struct src_register src1 = get_fake_arl_const( emit );
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   SVGA3dShaderDestToken tmp = get_temp( emit );

   if (!submit_op1(emit, inst_token( SVGA3DOP_MOV ), tmp, src0))
      return FALSE;

   if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ), tmp, src( tmp ),
                    src1))
      return FALSE;

   /* replicate the original swizzle */
   src1 = src(tmp);
   src1.base.swizzle = src0.base.swizzle;

   return submit_op1( emit, inst_token( SVGA3DOP_MOVA ),
                      dst, src1 );
}

static boolean emit_if(struct svga_shader_emitter *emit,
                       const struct tgsi_full_instruction *insn)
{
   struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   struct src_register zero = get_zero_immediate( emit );
   SVGA3dShaderInstToken if_token = inst_token( SVGA3DOP_IFC );

   if_token.control = SVGA3DOPCOMPC_NE;
   zero = scalar(zero, TGSI_SWIZZLE_X);

   if (SVGA3dShaderGetRegType(src0.base.value) == SVGA3DREG_CONST) {
      /*
       * Max different constant registers readable per IFC instruction is 1.
       */
      SVGA3dShaderDestToken tmp = get_temp( emit );

      if (!submit_op1(emit, inst_token( SVGA3DOP_MOV ), tmp, src0))
         return FALSE;

      src0 = scalar(src( tmp ), TGSI_SWIZZLE_X);
   }

   emit->dynamic_branching_level++;

   return (emit_instruction( emit, if_token ) &&
           emit_src( emit, src0 ) &&
           emit_src( emit, zero ) );
}

static boolean emit_endif(struct svga_shader_emitter *emit,
                       const struct tgsi_full_instruction *insn)
{
   emit->dynamic_branching_level--;

   return emit_instruction(emit, inst_token(SVGA3DOP_ENDIF));
}

static boolean emit_else(struct svga_shader_emitter *emit,
                         const struct tgsi_full_instruction *insn)
{
   return emit_instruction(emit, inst_token(SVGA3DOP_ELSE));
}

/* Translate the following TGSI FLR instruction.
 *    FLR  DST, SRC
 * To the following SVGA3D instruction sequence.
 *    FRC  TMP, SRC
 *    SUB  DST, SRC, TMP
 */
static boolean emit_floor(struct svga_shader_emitter *emit,
                          const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   SVGA3dShaderDestToken temp = get_temp( emit );

   /* FRC  TMP, SRC */
   if (!submit_op1( emit, inst_token( SVGA3DOP_FRC ), temp, src0 ))
      return FALSE;

   /* SUB  DST, SRC, TMP */
   if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ), dst, src0,
                    negate( src( temp ) ) ))
      return FALSE;

   return TRUE;
}


/* Translate the following TGSI CEIL instruction.
 *    CEIL  DST, SRC
 * To the following SVGA3D instruction sequence.
 *    FRC  TMP, -SRC
 *    ADD  DST, SRC, TMP
 */
static boolean emit_ceil(struct svga_shader_emitter *emit,
                         const struct tgsi_full_instruction *insn)
{
   SVGA3dShaderDestToken dst = translate_dst_register(emit, insn, 0);
   const struct src_register src0 = translate_src_register(emit, &insn->Src[0]);
   SVGA3dShaderDestToken temp = get_temp(emit);

   /* FRC  TMP, -SRC */
   if (!submit_op1(emit, inst_token(SVGA3DOP_FRC), temp, negate(src0)))
      return FALSE;

   /* ADD DST, SRC, TMP */
   if (!submit_op2(emit, inst_token(SVGA3DOP_ADD), dst, src0, src(temp)))
      return FALSE;

   return TRUE;
}


/* Translate the following TGSI DIV instruction.
 *    DIV  DST.xy, SRC0, SRC1
 * To the following SVGA3D instruction sequence.
 *    RCP  TMP.x, SRC1.xxxx
 *    RCP  TMP.y, SRC1.yyyy
 *    MUL  DST.xy, SRC0, TMP
 */
static boolean emit_div(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   const struct src_register src1 = translate_src_register(
      emit, &insn->Src[1] );
   SVGA3dShaderDestToken temp = get_temp( emit );
   int i;

   /* For each enabled element, perform a RCP instruction.  Note that
    * RCP is scalar in SVGA3D:
    */
   for (i = 0; i < 4; i++) {
      unsigned channel = 1 << i;
      if (dst.mask & channel) {
         /* RCP  TMP.?, SRC1.???? */
         if (!submit_op1( emit, inst_token( SVGA3DOP_RCP ),
                          writemask(temp, channel),
                          scalar(src1, i) ))
            return FALSE;
      }
   }

   /* Vector mul:
    * MUL  DST, SRC0, TMP
    */
   if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ), dst, src0,
                    src( temp ) ))
      return FALSE;

   return TRUE;
}

/* Translate the following TGSI DP2 instruction.
 *    DP2  DST, SRC1, SRC2
 * To the following SVGA3D instruction sequence.
 *    MUL  TMP, SRC1, SRC2
 *    ADD  DST, TMP.xxxx, TMP.yyyy
 */
static boolean emit_dp2(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   const struct src_register src1 = translate_src_register(
      emit, &insn->Src[1] );
   SVGA3dShaderDestToken temp = get_temp( emit );
   struct src_register temp_src0, temp_src1;

   /* MUL  TMP, SRC1, SRC2 */
   if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ), temp, src0, src1 ))
      return FALSE;

   temp_src0 = scalar(src( temp ), TGSI_SWIZZLE_X);
   temp_src1 = scalar(src( temp ), TGSI_SWIZZLE_Y);

   /* ADD  DST, TMP.xxxx, TMP.yyyy */
   if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ), dst,
                    temp_src0, temp_src1 ))
      return FALSE;

   return TRUE;
}


/* Translate the following TGSI DPH instruction.
 *    DPH  DST, SRC1, SRC2
 * To the following SVGA3D instruction sequence.
 *    DP3  TMP, SRC1, SRC2
 *    ADD  DST, TMP, SRC2.wwww
 */
static boolean emit_dph(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   struct src_register src1 = translate_src_register(
      emit, &insn->Src[1] );
   SVGA3dShaderDestToken temp = get_temp( emit );

   /* DP3  TMP, SRC1, SRC2 */
   if (!submit_op2( emit, inst_token( SVGA3DOP_DP3 ), temp, src0, src1 ))
      return FALSE;

   src1 = scalar(src1, TGSI_SWIZZLE_W);

   /* ADD  DST, TMP, SRC2.wwww */
   if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ), dst,
                    src( temp ), src1 ))
      return FALSE;

   return TRUE;
}

/* Translate the following TGSI DST instruction.
 *    NRM  DST, SRC
 * To the following SVGA3D instruction sequence.
 *    DP3  TMP, SRC, SRC
 *    RSQ  TMP, TMP
 *    MUL  DST, SRC, TMP
 */
static boolean emit_nrm(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   SVGA3dShaderDestToken temp = get_temp( emit );

   /* DP3  TMP, SRC, SRC */
   if (!submit_op2( emit, inst_token( SVGA3DOP_DP3 ), temp, src0, src0 ))
      return FALSE;

   /* RSQ  TMP, TMP */
   if (!submit_op1( emit, inst_token( SVGA3DOP_RSQ ), temp, src( temp )))
      return FALSE;

   /* MUL  DST, SRC, TMP */
   if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ), dst,
                    src0, src( temp )))
      return FALSE;

   return TRUE;

}

static boolean do_emit_sincos(struct svga_shader_emitter *emit,
                              SVGA3dShaderDestToken dst,
                              struct src_register src0)
{
   src0 = scalar(src0, TGSI_SWIZZLE_X);
   return submit_op1(emit, inst_token(SVGA3DOP_SINCOS), dst, src0);
}

static boolean emit_sincos(struct svga_shader_emitter *emit,
                           const struct tgsi_full_instruction *insn)
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   SVGA3dShaderDestToken temp = get_temp( emit );

   /* SCS TMP SRC */
   if (!do_emit_sincos(emit, writemask(temp, TGSI_WRITEMASK_XY), src0 ))
      return FALSE;

   /* MOV DST TMP */
   if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), dst, src( temp ) ))
      return FALSE;

   return TRUE;
}

/*
 * SCS TMP SRC
 * MOV DST TMP.yyyy
 */
static boolean emit_sin(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   SVGA3dShaderDestToken temp = get_temp( emit );

   /* SCS TMP SRC */
   if (!do_emit_sincos(emit, writemask(temp, TGSI_WRITEMASK_Y), src0))
      return FALSE;

   src0 = scalar(src( temp ), TGSI_SWIZZLE_Y);

   /* MOV DST TMP.yyyy */
   if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), dst, src0 ))
      return FALSE;

   return TRUE;
}

/*
 * SCS TMP SRC
 * MOV DST TMP.xxxx
 */
static boolean emit_cos(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   SVGA3dShaderDestToken temp = get_temp( emit );

   /* SCS TMP SRC */
   if (!do_emit_sincos( emit, writemask(temp, TGSI_WRITEMASK_X), src0 ))
      return FALSE;

   src0 = scalar(src( temp ), TGSI_SWIZZLE_X);

   /* MOV DST TMP.xxxx */
   if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), dst, src0 ))
      return FALSE;

   return TRUE;
}

static boolean emit_ssg(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   SVGA3dShaderDestToken temp0 = get_temp( emit );
   SVGA3dShaderDestToken temp1 = get_temp( emit );
   struct src_register zero, one;

   if (emit->unit == PIPE_SHADER_VERTEX) {
      /* SGN  DST, SRC0, TMP0, TMP1 */
      return submit_op3( emit, inst_token( SVGA3DOP_SGN ), dst, src0,
                         src( temp0 ), src( temp1 ) );
   }

   zero = get_zero_immediate( emit );
   one = scalar( zero, TGSI_SWIZZLE_W );
   zero = scalar( zero, TGSI_SWIZZLE_X );

   /* CMP  TMP0, SRC0, one, zero */
   if (!submit_op3( emit, inst_token( SVGA3DOP_CMP ),
                    writemask( temp0, dst.mask ), src0, one, zero ))
      return FALSE;

   /* CMP  TMP1, negate(SRC0), negate(one), zero */
   if (!submit_op3( emit, inst_token( SVGA3DOP_CMP ),
                    writemask( temp1, dst.mask ), negate( src0 ), negate( one ),
                    zero ))
      return FALSE;

   /* ADD  DST, TMP0, TMP1 */
   return submit_op2( emit, inst_token( SVGA3DOP_ADD ), dst, src( temp0 ),
                      src( temp1 ) );
}

/*
 * ADD DST SRC0, negate(SRC0)
 */
static boolean emit_sub(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn)
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   struct src_register src1 = translate_src_register(
      emit, &insn->Src[1] );

   src1 = negate(src1);

   if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ), dst,
                    src0, src1 ))
      return FALSE;

   return TRUE;
}


static boolean emit_kil(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn )
{
   const struct tgsi_full_src_register *reg = &insn->Src[0];
   struct src_register src0, srcIn;
   /* is the W component tested in another position? */
   const boolean w_tested = (reg->Register.SwizzleW == reg->Register.SwizzleX ||
                             reg->Register.SwizzleW == reg->Register.SwizzleY ||
                             reg->Register.SwizzleW == reg->Register.SwizzleZ);
   const boolean special = (reg->Register.Absolute ||
                            reg->Register.Negate ||
                            reg->Register.Indirect ||
                            reg->Register.SwizzleX != 0 ||
                            reg->Register.SwizzleY != 1 ||
                            reg->Register.SwizzleZ != 2 ||
                            reg->Register.File != TGSI_FILE_TEMPORARY);
   SVGA3dShaderDestToken temp;

   src0 = srcIn = translate_src_register( emit, reg );

   if (special || !w_tested) {
      /* need a temp reg */
      temp = get_temp( emit );
   }

   if (special) {
      /* move the source into a temp register */
      submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                  writemask( temp, TGSI_WRITEMASK_XYZ ),
                  src0 );

      src0 = src( temp );
   }

   /* do the texkill (on the xyz components) */
   if (!submit_op0( emit, inst_token( SVGA3DOP_TEXKILL ), dst(src0) ))
      return FALSE;

   if (!w_tested) {
      /* need to emit a second texkill to test the W component */
      /* put src.wwww into temp register */
      if (!submit_op1(emit,
                      inst_token( SVGA3DOP_MOV ),
                      writemask( temp, TGSI_WRITEMASK_XYZ ),
                      scalar(srcIn, TGSI_SWIZZLE_W)))
         return FALSE;

      /* second texkill */
      if (!submit_op0( emit, inst_token( SVGA3DOP_TEXKILL ), temp ))
         return FALSE;
   }

   return TRUE;
}


/* mesa state tracker always emits kilp as an unconditional
 * kil */
static boolean emit_kilp(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderInstToken inst;
   SVGA3dShaderDestToken temp;
   struct src_register one = scalar( get_zero_immediate( emit ),
                                     TGSI_SWIZZLE_W );

   inst = inst_token( SVGA3DOP_TEXKILL );

   /* texkill doesn't allow negation on the operand so lets move
    * negation of {1} to a temp register */
   temp = get_temp( emit );
   if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), temp,
                    negate( one ) ))
      return FALSE;

   return submit_op0( emit, inst, temp );
}


/**
 * Test if r1 and r2 are the same register.
 */
static boolean
same_register(struct src_register r1, struct src_register r2)
{
   return (r1.base.num == r2.base.num &&
           r1.base.type_upper == r2.base.type_upper &&
           r1.base.type_lower == r2.base.type_lower);
}



/* Implement conditionals by initializing destination reg to 'fail',
 * then set predicate reg with UFOP_SETP, then move 'pass' to dest
 * based on predicate reg.
 *
 * SETP src0, cmp, src1  -- do this first to avoid aliasing problems.
 * MOV dst, fail
 * MOV dst, pass, p0
 */
static boolean
emit_conditional(struct svga_shader_emitter *emit,
                 unsigned compare_func,
                 SVGA3dShaderDestToken dst,
                 struct src_register src0,
                 struct src_register src1,
                 struct src_register pass,
                 struct src_register fail)
{
   SVGA3dShaderDestToken pred_reg = dst_register( SVGA3DREG_PREDICATE, 0 );
   SVGA3dShaderInstToken setp_token, mov_token;
   setp_token = inst_token( SVGA3DOP_SETP );

   switch (compare_func) {
   case PIPE_FUNC_NEVER:
      return submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                         dst, fail );
      break;
   case PIPE_FUNC_LESS:
      setp_token.control = SVGA3DOPCOMP_LT;
      break;
   case PIPE_FUNC_EQUAL:
      setp_token.control = SVGA3DOPCOMP_EQ;
      break;
   case PIPE_FUNC_LEQUAL:
      setp_token.control = SVGA3DOPCOMP_LE;
      break;
   case PIPE_FUNC_GREATER:
      setp_token.control = SVGA3DOPCOMP_GT;
      break;
   case PIPE_FUNC_NOTEQUAL:
      setp_token.control = SVGA3DOPCOMPC_NE;
      break;
   case PIPE_FUNC_GEQUAL:
      setp_token.control = SVGA3DOPCOMP_GE;
      break;
   case PIPE_FUNC_ALWAYS:
      return submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                         dst, pass );
      break;
   }

   if (same_register(src(dst), pass)) {
      /* We'll get bad results if the dst and pass registers are the same
       * so use a temp register containing pass.
       */
      SVGA3dShaderDestToken temp = get_temp(emit);
      if (!submit_op1(emit, inst_token(SVGA3DOP_MOV), temp, pass))
         return FALSE;
      pass = src(temp);
   }

   /* SETP src0, COMPOP, src1 */
   if (!submit_op2( emit, setp_token, pred_reg,
                    src0, src1 ))
      return FALSE;

   mov_token = inst_token( SVGA3DOP_MOV );

   /* MOV dst, fail */
   if (!submit_op1( emit, mov_token, dst,
                    fail ))
      return FALSE;

   /* MOV dst, pass (predicated)
    *
    * Note that the predicate reg (and possible modifiers) is passed
    * as the first source argument.
    */
   mov_token.predicated = 1;
   if (!submit_op2( emit, mov_token, dst,
                    src( pred_reg ), pass ))
      return FALSE;

   return TRUE;
}


static boolean
emit_select(struct svga_shader_emitter *emit,
            unsigned compare_func,
            SVGA3dShaderDestToken dst,
            struct src_register src0,
            struct src_register src1 )
{
   /* There are some SVGA instructions which implement some selects
    * directly, but they are only available in the vertex shader.
    */
   if (emit->unit == PIPE_SHADER_VERTEX) {
      switch (compare_func) {
      case PIPE_FUNC_GEQUAL:
         return submit_op2( emit, inst_token( SVGA3DOP_SGE ), dst, src0, src1 );
      case PIPE_FUNC_LEQUAL:
         return submit_op2( emit, inst_token( SVGA3DOP_SGE ), dst, src1, src0 );
      case PIPE_FUNC_GREATER:
         return submit_op2( emit, inst_token( SVGA3DOP_SLT ), dst, src1, src0 );
      case PIPE_FUNC_LESS:
         return submit_op2( emit, inst_token( SVGA3DOP_SLT ), dst, src0, src1 );
      default:
         break;
      }
   }


   /* Otherwise, need to use the setp approach:
    */
   {
      struct src_register one, zero;
      /* zero immediate is 0,0,0,1 */
      zero = get_zero_immediate( emit );
      one  = scalar( zero, TGSI_SWIZZLE_W );
      zero = scalar( zero, TGSI_SWIZZLE_X );

      return emit_conditional(
         emit,
         compare_func,
         dst,
         src0,
         src1,
         one, zero);
   }
}


static boolean emit_select_op(struct svga_shader_emitter *emit,
                              unsigned compare,
                              const struct tgsi_full_instruction *insn)
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   struct src_register src1 = translate_src_register(
      emit, &insn->Src[1] );

   return emit_select( emit, compare, dst, src0, src1 );
}


/**
 * Translate TGSI CMP instruction.
 */
static boolean
emit_cmp(struct svga_shader_emitter *emit,
         const struct tgsi_full_instruction *insn)
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 =
      translate_src_register(emit, &insn->Src[0] );
   const struct src_register src1 =
      translate_src_register(emit, &insn->Src[1] );
   const struct src_register src2 =
      translate_src_register(emit, &insn->Src[2] );

   if (emit->unit == PIPE_SHADER_VERTEX) {
      struct src_register zero =
         scalar(get_zero_immediate(emit), TGSI_SWIZZLE_X);
      /* We used to simulate CMP with SLT+LRP.  But that didn't work when
       * src1 or src2 was Inf/NaN.  In particular, GLSL sqrt(0) failed
       * because it involves a CMP to handle the 0 case.
       * Use a conditional expression instead.
       */
      return emit_conditional(emit, PIPE_FUNC_LESS, dst,
                              src0, zero, src1, src2);
   }
   else {
      assert(emit->unit == PIPE_SHADER_FRAGMENT);

      /* CMP  DST, SRC0, SRC2, SRC1 */
      return submit_op3( emit, inst_token( SVGA3DOP_CMP ), dst,
                         src0, src2, src1);
   }
}


/* Translate texture instructions to SVGA3D representation.
 */
static boolean emit_tex2(struct svga_shader_emitter *emit,
                         const struct tgsi_full_instruction *insn,
                         SVGA3dShaderDestToken dst )
{
   SVGA3dShaderInstToken inst;
   struct src_register texcoord;
   struct src_register sampler;
   SVGA3dShaderDestToken tmp;

   inst.value = 0;

   switch (insn->Instruction.Opcode) {
   case TGSI_OPCODE_TEX:
      inst.op = SVGA3DOP_TEX;
      break;
   case TGSI_OPCODE_TXP:
      inst.op = SVGA3DOP_TEX;
      inst.control = SVGA3DOPCONT_PROJECT;
      break;
   case TGSI_OPCODE_TXB:
      inst.op = SVGA3DOP_TEX;
      inst.control = SVGA3DOPCONT_BIAS;
      break;
   case TGSI_OPCODE_TXL:
      inst.op = SVGA3DOP_TEXLDL;
      break;
   default:
      assert(0);
      return FALSE;
   }

   texcoord = translate_src_register( emit, &insn->Src[0] );
   sampler = translate_src_register( emit, &insn->Src[1] );

   if (emit->key.fkey.tex[sampler.base.num].unnormalized ||
       emit->dynamic_branching_level > 0)
      tmp = get_temp( emit );

   /* Can't do mipmapping inside dynamic branch constructs.  Force LOD
    * zero in that case.
    */
   if (emit->dynamic_branching_level > 0 &&
       inst.op == SVGA3DOP_TEX &&
       SVGA3dShaderGetRegType(texcoord.base.value) == SVGA3DREG_TEMP) {
      struct src_register zero = get_zero_immediate( emit );

      /* MOV  tmp, texcoord */
      if (!submit_op1( emit,
                       inst_token( SVGA3DOP_MOV ),
                       tmp,
                       texcoord ))
         return FALSE;

      /* MOV  tmp.w, zero */
      if (!submit_op1( emit,
                       inst_token( SVGA3DOP_MOV ),
                       writemask( tmp, TGSI_WRITEMASK_W ),
                       scalar( zero, TGSI_SWIZZLE_X )))
         return FALSE;

      texcoord = src( tmp );
      inst.op = SVGA3DOP_TEXLDL;
   }

   /* Explicit normalization of texcoords:
    */
   if (emit->key.fkey.tex[sampler.base.num].unnormalized) {
      struct src_register wh = get_tex_dimensions( emit, sampler.base.num );

      /* MUL  tmp, SRC0, WH */
      if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ),
                       tmp, texcoord, wh ))
         return FALSE;

      texcoord = src( tmp );
   }

   return submit_op2( emit, inst, dst, texcoord, sampler );
}




/* Translate texture instructions to SVGA3D representation.
 */
static boolean emit_tex4(struct svga_shader_emitter *emit,
                         const struct tgsi_full_instruction *insn,
                         SVGA3dShaderDestToken dst )
{
   SVGA3dShaderInstToken inst;
   struct src_register texcoord;
   struct src_register ddx;
   struct src_register ddy;
   struct src_register sampler;

   texcoord = translate_src_register( emit, &insn->Src[0] );
   ddx      = translate_src_register( emit, &insn->Src[1] );
   ddy      = translate_src_register( emit, &insn->Src[2] );
   sampler  = translate_src_register( emit, &insn->Src[3] );

   inst.value = 0;

   switch (insn->Instruction.Opcode) {
   case TGSI_OPCODE_TXD:
      inst.op = SVGA3DOP_TEXLDD; /* 4 args! */
      break;
   default:
      assert(0);
      return FALSE;
   }

   return submit_op4( emit, inst, dst, texcoord, sampler, ddx, ddy );
}


/**
 * Emit texture swizzle code.
 */
static boolean emit_tex_swizzle( struct svga_shader_emitter *emit,
                                 SVGA3dShaderDestToken dst,
                                 struct src_register src,
                                 unsigned swizzle_x,
                                 unsigned swizzle_y,
                                 unsigned swizzle_z,
                                 unsigned swizzle_w)
{
   const unsigned swizzleIn[4] = {swizzle_x, swizzle_y, swizzle_z, swizzle_w};
   unsigned srcSwizzle[4];
   unsigned srcWritemask = 0x0, zeroWritemask = 0x0, oneWritemask = 0x0;
   int i;

   /* build writemasks and srcSwizzle terms */
   for (i = 0; i < 4; i++) {
      if (swizzleIn[i] == PIPE_SWIZZLE_ZERO) {
         srcSwizzle[i] = TGSI_SWIZZLE_X + i;
         zeroWritemask |= (1 << i);
      }
      else if (swizzleIn[i] == PIPE_SWIZZLE_ONE) {
         srcSwizzle[i] = TGSI_SWIZZLE_X + i;
         oneWritemask |= (1 << i);
      }
      else {
         srcSwizzle[i] = swizzleIn[i];
         srcWritemask |= (1 << i);
      }
   }

   /* write x/y/z/w comps */
   if (dst.mask & srcWritemask) {
      if (!submit_op1(emit,
                      inst_token(SVGA3DOP_MOV),
                      writemask(dst, srcWritemask),
                      swizzle(src,
                              srcSwizzle[0],
                              srcSwizzle[1],
                              srcSwizzle[2],
                              srcSwizzle[3])))
         return FALSE;
   }

   /* write 0 comps */
   if (dst.mask & zeroWritemask) {
      if (!submit_op1(emit,
                      inst_token(SVGA3DOP_MOV),
                      writemask(dst, zeroWritemask),
                      scalar(get_zero_immediate(emit), TGSI_SWIZZLE_X)))
         return FALSE;
   }

   /* write 1 comps */
   if (dst.mask & oneWritemask) {
      if (!submit_op1(emit,
                      inst_token(SVGA3DOP_MOV),
                      writemask(dst, oneWritemask),
                      scalar(get_zero_immediate(emit), TGSI_SWIZZLE_W)))
         return FALSE;
   }

   return TRUE;
}


static boolean emit_tex(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderDestToken dst =
      translate_dst_register( emit, insn, 0 );
   struct src_register src0 =
      translate_src_register( emit, &insn->Src[0] );
   struct src_register src1 =
      translate_src_register( emit, &insn->Src[1] );

   SVGA3dShaderDestToken tex_result;
   const unsigned unit = src1.base.num;

   /* check for shadow samplers */
   boolean compare = (emit->key.fkey.tex[unit].compare_mode ==
                      PIPE_TEX_COMPARE_R_TO_TEXTURE);

   /* texture swizzle */
   boolean swizzle = (emit->key.fkey.tex[unit].swizzle_r != PIPE_SWIZZLE_RED ||
                      emit->key.fkey.tex[unit].swizzle_g != PIPE_SWIZZLE_GREEN ||
                      emit->key.fkey.tex[unit].swizzle_b != PIPE_SWIZZLE_BLUE ||
                      emit->key.fkey.tex[unit].swizzle_a != PIPE_SWIZZLE_ALPHA);

   boolean saturate = insn->Instruction.Saturate != TGSI_SAT_NONE;

   /* If doing compare processing or tex swizzle or saturation, we need to put
    * the fetched color into a temporary so it can be used as a source later on.
    */
   if (compare || swizzle || saturate) {
      tex_result = get_temp( emit );
   }
   else {
      tex_result = dst;
   }

   switch(insn->Instruction.Opcode) {
   case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXP:
   case TGSI_OPCODE_TXL:
      if (!emit_tex2( emit, insn, tex_result ))
         return FALSE;
      break;
   case TGSI_OPCODE_TXD:
      if (!emit_tex4( emit, insn, tex_result ))
         return FALSE;
      break;
   default:
      assert(0);
   }

   if (compare) {
      SVGA3dShaderDestToken dst2;

      if (swizzle || saturate)
         dst2 = tex_result;
      else
         dst2 = dst;

      if (dst.mask & TGSI_WRITEMASK_XYZ) {
         SVGA3dShaderDestToken src0_zdivw = get_temp( emit );
         /* When sampling a depth texture, the result of the comparison is in
          * the Y component.
          */
         struct src_register tex_src_x = scalar(src(tex_result), TGSI_SWIZZLE_Y);
         struct src_register r_coord;

         if (insn->Instruction.Opcode == TGSI_OPCODE_TXP) {
            /* Divide texcoord R by Q */
            if (!submit_op1( emit, inst_token( SVGA3DOP_RCP ),
                             writemask(src0_zdivw, TGSI_WRITEMASK_X),
                             scalar(src0, TGSI_SWIZZLE_W) ))
               return FALSE;

            if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ),
                             writemask(src0_zdivw, TGSI_WRITEMASK_X),
                             scalar(src0, TGSI_SWIZZLE_Z),
                             scalar(src(src0_zdivw), TGSI_SWIZZLE_X) ))
               return FALSE;

            r_coord = scalar(src(src0_zdivw), TGSI_SWIZZLE_X);
         }
         else {
            r_coord = scalar(src0, TGSI_SWIZZLE_Z);
         }

         /* Compare texture sample value against R component of texcoord */
         if (!emit_select(emit,
                          emit->key.fkey.tex[unit].compare_func,
                          writemask( dst2, TGSI_WRITEMASK_XYZ ),
                          r_coord,
                          tex_src_x))
            return FALSE;
      }

      if (dst.mask & TGSI_WRITEMASK_W) {
         struct src_register one =
            scalar( get_zero_immediate( emit ), TGSI_SWIZZLE_W );

        if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                         writemask( dst2, TGSI_WRITEMASK_W ),
                         one ))
           return FALSE;
      }
   }

   if (saturate && !swizzle) {
      /* MOV_SAT real_dst, dst */
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), dst, src(tex_result) ))
         return FALSE;
   }
   else if (swizzle) {
      /* swizzle from tex_result to dst (handles saturation too, if any) */
      emit_tex_swizzle(emit,
                       dst, src(tex_result),
                       emit->key.fkey.tex[unit].swizzle_r,
                       emit->key.fkey.tex[unit].swizzle_g,
                       emit->key.fkey.tex[unit].swizzle_b,
                       emit->key.fkey.tex[unit].swizzle_a);
   }

   return TRUE;
}

static boolean emit_bgnloop2( struct svga_shader_emitter *emit,
                              const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderInstToken inst = inst_token( SVGA3DOP_LOOP );
   struct src_register loop_reg = src_register( SVGA3DREG_LOOP, 0 );
   struct src_register const_int = get_loop_const( emit );

   emit->dynamic_branching_level++;

   return (emit_instruction( emit, inst ) &&
           emit_src( emit, loop_reg ) &&
           emit_src( emit, const_int ) );
}

static boolean emit_endloop2( struct svga_shader_emitter *emit,
                              const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderInstToken inst = inst_token( SVGA3DOP_ENDLOOP );

   emit->dynamic_branching_level--;

   return emit_instruction( emit, inst );
}

static boolean emit_brk( struct svga_shader_emitter *emit,
                         const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderInstToken inst = inst_token( SVGA3DOP_BREAK );
   return emit_instruction( emit, inst );
}

static boolean emit_scalar_op1( struct svga_shader_emitter *emit,
                                unsigned opcode,
                                const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderInstToken inst;
   SVGA3dShaderDestToken dst;
   struct src_register src;

   inst = inst_token( opcode );
   dst = translate_dst_register( emit, insn, 0 );
   src = translate_src_register( emit, &insn->Src[0] );
   src = scalar( src, TGSI_SWIZZLE_X );

   return submit_op1( emit, inst, dst, src );
}


static boolean emit_simple_instruction(struct svga_shader_emitter *emit,
                                       unsigned opcode,
                                       const struct tgsi_full_instruction *insn )
{
   const struct tgsi_full_src_register *src = insn->Src;
   SVGA3dShaderInstToken inst;
   SVGA3dShaderDestToken dst;

   inst = inst_token( opcode );
   dst = translate_dst_register( emit, insn, 0 );

   switch (insn->Instruction.NumSrcRegs) {
   case 0:
      return submit_op0( emit, inst, dst );
   case 1:
      return submit_op1( emit, inst, dst,
                         translate_src_register( emit, &src[0] ));
   case 2:
      return submit_op2( emit, inst, dst,
                         translate_src_register( emit, &src[0] ),
                         translate_src_register( emit, &src[1] ) );
   case 3:
      return submit_op3( emit, inst, dst,
                         translate_src_register( emit, &src[0] ),
                         translate_src_register( emit, &src[1] ),
                         translate_src_register( emit, &src[2] ) );
   default:
      assert(0);
      return FALSE;
   }
}


static boolean emit_deriv(struct svga_shader_emitter *emit,
                          const struct tgsi_full_instruction *insn )
{
   if (emit->dynamic_branching_level > 0 &&
       insn->Src[0].Register.File == TGSI_FILE_TEMPORARY)
   {
      struct src_register zero = get_zero_immediate( emit );
      SVGA3dShaderDestToken dst =
         translate_dst_register( emit, insn, 0 );

      /* Deriv opcodes not valid inside dynamic branching, workaround
       * by zeroing out the destination.
       */
      if (!submit_op1(emit,
                      inst_token( SVGA3DOP_MOV ),
                      dst,
                      scalar(zero, TGSI_SWIZZLE_X)))
         return FALSE;

      return TRUE;
   }
   else {
      unsigned opcode;
      const struct tgsi_full_src_register *reg = &insn->Src[0];
      SVGA3dShaderInstToken inst;
      SVGA3dShaderDestToken dst;
      struct src_register src0;

      switch (insn->Instruction.Opcode) {
      case TGSI_OPCODE_DDX:
         opcode = SVGA3DOP_DSX;
         break;
      case TGSI_OPCODE_DDY:
         opcode = SVGA3DOP_DSY;
         break;
      default:
         return FALSE;
      }

      inst = inst_token( opcode );
      dst = translate_dst_register( emit, insn, 0 );
      src0 = translate_src_register( emit, reg );

      /* We cannot use negate or abs on source to dsx/dsy instruction.
       */
      if (reg->Register.Absolute ||
          reg->Register.Negate) {
         SVGA3dShaderDestToken temp = get_temp( emit );

         if (!emit_repl( emit, temp, &src0 ))
            return FALSE;
      }

      return submit_op1( emit, inst, dst, src0 );
   }
}

static boolean emit_arl(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn)
{
   ++emit->current_arl;
   if (emit->unit == PIPE_SHADER_FRAGMENT) {
      /* MOVA not present in pixel shader instruction set.
       * Ignore this instruction altogether since it is
       * only used for loop counters -- and for that
       * we reference aL directly.
       */
      return TRUE;
   }
   if (svga_arl_needs_adjustment( emit )) {
      return emit_fake_arl( emit, insn );
   } else {
      /* no need to adjust, just emit straight arl */
      return emit_simple_instruction(emit, SVGA3DOP_MOVA, insn);
   }
}

static boolean emit_pow(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn)
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   struct src_register src1 = translate_src_register(
      emit, &insn->Src[1] );
   boolean need_tmp = FALSE;

   /* POW can only output to a temporary */
   if (insn->Dst[0].Register.File != TGSI_FILE_TEMPORARY)
      need_tmp = TRUE;

   /* POW src1 must not be the same register as dst */
   if (alias_src_dst( src1, dst ))
      need_tmp = TRUE;

   /* it's a scalar op */
   src0 = scalar( src0, TGSI_SWIZZLE_X );
   src1 = scalar( src1, TGSI_SWIZZLE_X );

   if (need_tmp) {
      SVGA3dShaderDestToken tmp = writemask(get_temp( emit ), TGSI_WRITEMASK_X );

      if (!submit_op2(emit, inst_token( SVGA3DOP_POW ), tmp, src0, src1))
         return FALSE;

      return submit_op1(emit, inst_token( SVGA3DOP_MOV ), dst, scalar(src(tmp), 0) );
   }
   else {
      return submit_op2(emit, inst_token( SVGA3DOP_POW ), dst, src0, src1);
   }
}

static boolean emit_xpd(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn)
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   const struct src_register src1 = translate_src_register(
      emit, &insn->Src[1] );
   boolean need_dst_tmp = FALSE;

   /* XPD can only output to a temporary */
   if (SVGA3dShaderGetRegType(dst.value) != SVGA3DREG_TEMP)
      need_dst_tmp = TRUE;

   /* The dst reg must not be the same as src0 or src1*/
   if (alias_src_dst(src0, dst) ||
       alias_src_dst(src1, dst))
      need_dst_tmp = TRUE;

   if (need_dst_tmp) {
      SVGA3dShaderDestToken tmp = get_temp( emit );

      /* Obey DX9 restrictions on mask:
       */
      tmp.mask = dst.mask & TGSI_WRITEMASK_XYZ;

      if (!submit_op2(emit, inst_token( SVGA3DOP_CRS ), tmp, src0, src1))
         return FALSE;

      if (!submit_op1(emit, inst_token( SVGA3DOP_MOV ), dst, src( tmp )))
         return FALSE;
   }
   else {
      if (!submit_op2(emit, inst_token( SVGA3DOP_CRS ), dst, src0, src1))
         return FALSE;
   }

   /* Need to emit 1.0 to dst.w?
    */
   if (dst.mask & TGSI_WRITEMASK_W) {
      struct src_register zero = get_zero_immediate( emit );

      if (!submit_op1(emit,
                      inst_token( SVGA3DOP_MOV ),
                      writemask(dst, TGSI_WRITEMASK_W),
                      zero))
         return FALSE;
   }

   return TRUE;
}


static boolean emit_lrp(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn)
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 = translate_src_register(
      emit, &insn->Src[0] );
   const struct src_register src1 = translate_src_register(
      emit, &insn->Src[1] );
   const struct src_register src2 = translate_src_register(
      emit, &insn->Src[2] );

   return submit_lrp(emit, dst, src0, src1, src2);
}


static boolean emit_dst_insn(struct svga_shader_emitter *emit,
                             const struct tgsi_full_instruction *insn )
{
   if (emit->unit == PIPE_SHADER_VERTEX) {
      /* SVGA/DX9 has a DST instruction, but only for vertex shaders:
       */
      return emit_simple_instruction(emit, SVGA3DOP_DST, insn);
   }
   else {

      /* result[0] = 1    * 1;
       * result[1] = a[1] * b[1];
       * result[2] = a[2] * 1;
       * result[3] = 1    * b[3];
       */

      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
      SVGA3dShaderDestToken tmp;
      const struct src_register src0 = translate_src_register(
         emit, &insn->Src[0] );
      const struct src_register src1 = translate_src_register(
         emit, &insn->Src[1] );
      struct src_register zero = get_zero_immediate( emit );
      boolean need_tmp = FALSE;

      if (SVGA3dShaderGetRegType(dst.value) != SVGA3DREG_TEMP ||
          alias_src_dst(src0, dst) ||
          alias_src_dst(src1, dst))
         need_tmp = TRUE;

      if (need_tmp) {
         tmp = get_temp( emit );
      }
      else {
         tmp = dst;
      }

      /* tmp.xw = 1.0
       */
      if (tmp.mask & TGSI_WRITEMASK_XW) {
         if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                          writemask(tmp, TGSI_WRITEMASK_XW ),
                          scalar( zero, 3 )))
            return FALSE;
      }

      /* tmp.yz = src0
       */
      if (tmp.mask & TGSI_WRITEMASK_YZ) {
         if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                          writemask(tmp, TGSI_WRITEMASK_YZ ),
                          src0))
            return FALSE;
      }

      /* tmp.yw = tmp * src1
       */
      if (tmp.mask & TGSI_WRITEMASK_YW) {
         if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ),
                          writemask(tmp, TGSI_WRITEMASK_YW ),
                          src(tmp),
                          src1))
            return FALSE;
      }

      /* dst = tmp
       */
      if (need_tmp) {
         if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                          dst,
                          src(tmp)))
            return FALSE;
      }
   }

   return TRUE;
}


static boolean emit_exp(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn)
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 =
      translate_src_register( emit, &insn->Src[0] );
   struct src_register zero = get_zero_immediate( emit );
   SVGA3dShaderDestToken fraction;

   if (dst.mask & TGSI_WRITEMASK_Y)
      fraction = dst;
   else if (dst.mask & TGSI_WRITEMASK_X)
      fraction = get_temp( emit );
   else
      fraction.value = 0;

   /* If y is being written, fill it with src0 - floor(src0).
    */
   if (dst.mask & TGSI_WRITEMASK_XY) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_FRC ),
                       writemask( fraction, TGSI_WRITEMASK_Y ),
                       src0 ))
         return FALSE;
   }

   /* If x is being written, fill it with 2 ^ floor(src0).
    */
   if (dst.mask & TGSI_WRITEMASK_X) {
      if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ),
                       writemask( dst, TGSI_WRITEMASK_X ),
                       src0,
                       scalar( negate( src( fraction ) ), TGSI_SWIZZLE_Y ) ) )
         return FALSE;

      if (!submit_op1( emit, inst_token( SVGA3DOP_EXP ),
                       writemask( dst, TGSI_WRITEMASK_X ),
                       scalar( src( dst ), TGSI_SWIZZLE_X ) ) )
         return FALSE;

      if (!(dst.mask & TGSI_WRITEMASK_Y))
         release_temp( emit, fraction );
   }

   /* If z is being written, fill it with 2 ^ src0 (partial precision).
    */
   if (dst.mask & TGSI_WRITEMASK_Z) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_EXPP ),
                       writemask( dst, TGSI_WRITEMASK_Z ),
                       src0 ) )
         return FALSE;
   }

   /* If w is being written, fill it with one.
    */
   if (dst.mask & TGSI_WRITEMASK_W) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                       writemask(dst, TGSI_WRITEMASK_W),
                       scalar( zero, TGSI_SWIZZLE_W ) ))
         return FALSE;
   }

   return TRUE;
}

static boolean emit_lit(struct svga_shader_emitter *emit,
                             const struct tgsi_full_instruction *insn )
{
   if (emit->unit == PIPE_SHADER_VERTEX) {
      /* SVGA/DX9 has a LIT instruction, but only for vertex shaders:
       */
      return emit_simple_instruction(emit, SVGA3DOP_LIT, insn);
   }
   else {
      /* D3D vs. GL semantics can be fairly easily accomodated by
       * variations on this sequence.
       *
       * GL:
       *   tmp.y = src.x
       *   tmp.z = pow(src.y,src.w)
       *   p0 = src0.xxxx > 0
       *   result = zero.wxxw
       *   (p0) result.yz = tmp
       *
       * D3D:
       *   tmp.y = src.x
       *   tmp.z = pow(src.y,src.w)
       *   p0 = src0.xxyy > 0
       *   result = zero.wxxw
       *   (p0) result.yz = tmp
       *
       * Will implement the GL version for now.
       */
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
      SVGA3dShaderDestToken tmp = get_temp( emit );
      const struct src_register src0 = translate_src_register(
         emit, &insn->Src[0] );
      struct src_register zero = get_zero_immediate( emit );

      /* tmp = pow(src.y, src.w)
       */
      if (dst.mask & TGSI_WRITEMASK_Z) {
         if (!submit_op2(emit, inst_token( SVGA3DOP_POW ),
                         tmp,
                         scalar(src0, 1),
                         scalar(src0, 3)))
            return FALSE;
      }

      /* tmp.y = src.x
       */
      if (dst.mask & TGSI_WRITEMASK_Y) {
         if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                          writemask(tmp, TGSI_WRITEMASK_Y ),
                          scalar(src0, 0)))
            return FALSE;
      }

      /* Can't quite do this with emit conditional due to the extra
       * writemask on the predicated mov:
       */
      {
         SVGA3dShaderDestToken pred_reg = dst_register( SVGA3DREG_PREDICATE, 0 );
         SVGA3dShaderInstToken setp_token, mov_token;
         struct src_register predsrc;

         setp_token = inst_token( SVGA3DOP_SETP );
         mov_token = inst_token( SVGA3DOP_MOV );

         setp_token.control = SVGA3DOPCOMP_GT;

         /* D3D vs GL semantics:
          */
         if (0)
            predsrc = swizzle(src0, 0, 0, 1, 1); /* D3D */
         else
            predsrc = swizzle(src0, 0, 0, 0, 0); /* GL */

         /* SETP src0.xxyy, GT, {0}.x */
         if (!submit_op2( emit, setp_token, pred_reg,
                          predsrc,
                          swizzle(zero, 0, 0, 0, 0) ))
            return FALSE;

         /* MOV dst, fail */
         if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), dst,
                          swizzle(zero, 3, 0, 0, 3 )))
             return FALSE;

         /* MOV dst.yz, tmp (predicated)
          *
          * Note that the predicate reg (and possible modifiers) is passed
          * as the first source argument.
          */
         if (dst.mask & TGSI_WRITEMASK_YZ) {
            mov_token.predicated = 1;
            if (!submit_op2( emit, mov_token,
                             writemask(dst, TGSI_WRITEMASK_YZ),
                             src( pred_reg ), src( tmp ) ))
               return FALSE;
         }
      }
   }

   return TRUE;
}


static boolean emit_ex2( struct svga_shader_emitter *emit,
                         const struct tgsi_full_instruction *insn )
{
   SVGA3dShaderInstToken inst;
   SVGA3dShaderDestToken dst;
   struct src_register src0;

   inst = inst_token( SVGA3DOP_EXP );
   dst = translate_dst_register( emit, insn, 0 );
   src0 = translate_src_register( emit, &insn->Src[0] );
   src0 = scalar( src0, TGSI_SWIZZLE_X );

   if (dst.mask != TGSI_WRITEMASK_XYZW) {
      SVGA3dShaderDestToken tmp = get_temp( emit );

      if (!submit_op1( emit, inst, tmp, src0 ))
         return FALSE;

      return submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                         dst,
                         scalar( src( tmp ), TGSI_SWIZZLE_X ) );
   }

   return submit_op1( emit, inst, dst, src0 );
}


static boolean emit_log(struct svga_shader_emitter *emit,
                        const struct tgsi_full_instruction *insn)
{
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 =
      translate_src_register( emit, &insn->Src[0] );
   struct src_register zero = get_zero_immediate( emit );
   SVGA3dShaderDestToken abs_tmp;
   struct src_register abs_src0;
   SVGA3dShaderDestToken log2_abs;

   abs_tmp.value = 0;

   if (dst.mask & TGSI_WRITEMASK_Z)
      log2_abs = dst;
   else if (dst.mask & TGSI_WRITEMASK_XY)
      log2_abs = get_temp( emit );
   else
      log2_abs.value = 0;

   /* If z is being written, fill it with log2( abs( src0 ) ).
    */
   if (dst.mask & TGSI_WRITEMASK_XYZ) {
      if (!src0.base.srcMod || src0.base.srcMod == SVGA3DSRCMOD_ABS)
         abs_src0 = src0;
      else {
         abs_tmp = get_temp( emit );

         if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                          abs_tmp,
                          src0 ) )
            return FALSE;

         abs_src0 = src( abs_tmp );
      }

      abs_src0 = absolute( scalar( abs_src0, TGSI_SWIZZLE_X ) );

      if (!submit_op1( emit, inst_token( SVGA3DOP_LOG ),
                       writemask( log2_abs, TGSI_WRITEMASK_Z ),
                       abs_src0 ) )
         return FALSE;
   }

   if (dst.mask & TGSI_WRITEMASK_XY) {
      SVGA3dShaderDestToken floor_log2;

      if (dst.mask & TGSI_WRITEMASK_X)
         floor_log2 = dst;
      else
         floor_log2 = get_temp( emit );

      /* If x is being written, fill it with floor( log2( abs( src0 ) ) ).
       */
      if (!submit_op1( emit, inst_token( SVGA3DOP_FRC ),
                       writemask( floor_log2, TGSI_WRITEMASK_X ),
                       scalar( src( log2_abs ), TGSI_SWIZZLE_Z ) ) )
         return FALSE;

      if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ),
                       writemask( floor_log2, TGSI_WRITEMASK_X ),
                       scalar( src( log2_abs ), TGSI_SWIZZLE_Z ),
                       negate( src( floor_log2 ) ) ) )
         return FALSE;

      /* If y is being written, fill it with
       * abs ( src0 ) / ( 2 ^ floor( log2( abs( src0 ) ) ) ).
       */
      if (dst.mask & TGSI_WRITEMASK_Y) {
         if (!submit_op1( emit, inst_token( SVGA3DOP_EXP ),
                          writemask( dst, TGSI_WRITEMASK_Y ),
                          negate( scalar( src( floor_log2 ),
                                          TGSI_SWIZZLE_X ) ) ) )
            return FALSE;

         if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ),
                          writemask( dst, TGSI_WRITEMASK_Y ),
                          src( dst ),
                          abs_src0 ) )
            return FALSE;
      }

      if (!(dst.mask & TGSI_WRITEMASK_X))
         release_temp( emit, floor_log2 );

      if (!(dst.mask & TGSI_WRITEMASK_Z))
         release_temp( emit, log2_abs );
   }

   if (dst.mask & TGSI_WRITEMASK_XYZ && src0.base.srcMod &&
       src0.base.srcMod != SVGA3DSRCMOD_ABS)
      release_temp( emit, abs_tmp );

   /* If w is being written, fill it with one.
    */
   if (dst.mask & TGSI_WRITEMASK_W) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                       writemask(dst, TGSI_WRITEMASK_W),
                       scalar( zero, TGSI_SWIZZLE_W ) ))
         return FALSE;
   }

   return TRUE;
}


/**
 * Translate TGSI TRUNC or ROUND instruction.
 * We need to truncate toward zero. Ex: trunc(-1.9) = -1
 * Different approaches are needed for VS versus PS.
 */
static boolean
emit_trunc_round(struct svga_shader_emitter *emit,
                 const struct tgsi_full_instruction *insn,
                 boolean round)
{
   SVGA3dShaderDestToken dst = translate_dst_register(emit, insn, 0);
   const struct src_register src0 =
      translate_src_register(emit, &insn->Src[0] );
   SVGA3dShaderDestToken t1 = get_temp(emit);

   if (round) {
      SVGA3dShaderDestToken t0 = get_temp(emit);
      struct src_register half = get_half_immediate(emit);

      /* t0 = abs(src0) + 0.5 */
      if (!submit_op2(emit, inst_token(SVGA3DOP_ADD), t0,
                      absolute(src0), half))
         return FALSE;

      /* t1 = fract(t0) */
      if (!submit_op1(emit, inst_token(SVGA3DOP_FRC), t1, src(t0)))
         return FALSE;

      /* t1 = t0 - t1 */
      if (!submit_op2(emit, inst_token(SVGA3DOP_ADD), t1, src(t0),
                      negate(src(t1))))
         return FALSE;
   }
   else {
      /* trunc */

      /* t1 = fract(abs(src0)) */
      if (!submit_op1(emit, inst_token(SVGA3DOP_FRC), t1, absolute(src0)))
         return FALSE;

      /* t1 = abs(src0) - t1 */
      if (!submit_op2(emit, inst_token(SVGA3DOP_ADD), t1, absolute(src0),
                      negate(src(t1))))
         return FALSE;
   }

   /*
    * Now we need to multiply t1 by the sign of the original value.
   */
   if (emit->unit == PIPE_SHADER_VERTEX) {
      /* For VS: use SGN instruction */
      /* Need two extra/dummy registers: */
      SVGA3dShaderDestToken t2 = get_temp(emit), t3 = get_temp(emit),
         t4 = get_temp(emit);

      /* t2 = sign(src0) */
      if (!submit_op3(emit, inst_token(SVGA3DOP_SGN), t2, src0,
                      src(t3), src(t4)))
         return FALSE;

      /* dst = t1 * t2 */
      if (!submit_op2(emit, inst_token(SVGA3DOP_MUL), dst, src(t1), src(t2)))
         return FALSE;
   }
   else {
      /* For FS: Use CMP instruction */
      return submit_op3(emit, inst_token( SVGA3DOP_CMP ), dst,
                        src0, src(t1), negate(src(t1)));
   }

   return TRUE;
}


static boolean emit_bgnsub( struct svga_shader_emitter *emit,
                           unsigned position,
                           const struct tgsi_full_instruction *insn )
{
   unsigned i;

   /* Note that we've finished the main function and are now emitting
    * subroutines.  This affects how we terminate the generated
    * shader.
    */
   emit->in_main_func = FALSE;

   for (i = 0; i < emit->nr_labels; i++) {
      if (emit->label[i] == position) {
         return (emit_instruction( emit, inst_token( SVGA3DOP_RET ) ) &&
                 emit_instruction( emit, inst_token( SVGA3DOP_LABEL ) ) &&
                 emit_src( emit, src_register( SVGA3DREG_LABEL, i )));
      }
   }

   assert(0);
   return TRUE;
}

static boolean emit_call( struct svga_shader_emitter *emit,
                           const struct tgsi_full_instruction *insn )
{
   unsigned position = insn->Label.Label;
   unsigned i;

   for (i = 0; i < emit->nr_labels; i++) {
      if (emit->label[i] == position)
         break;
   }

   if (emit->nr_labels == Elements(emit->label))
      return FALSE;

   if (i == emit->nr_labels) {
      emit->label[i] = position;
      emit->nr_labels++;
   }

   return (emit_instruction( emit, inst_token( SVGA3DOP_CALL ) ) &&
           emit_src( emit, src_register( SVGA3DREG_LABEL, i )));
}


static boolean emit_end( struct svga_shader_emitter *emit )
{
   if (emit->unit == PIPE_SHADER_VERTEX) {
      return emit_vs_postamble( emit );
   }
   else {
      return emit_ps_postamble( emit );
   }
}



static boolean svga_emit_instruction( struct svga_shader_emitter *emit,
                                      unsigned position,
                                      const struct tgsi_full_instruction *insn )
{
   switch (insn->Instruction.Opcode) {

   case TGSI_OPCODE_ARL:
      return emit_arl( emit, insn );

   case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXP:
   case TGSI_OPCODE_TXL:
   case TGSI_OPCODE_TXD:
      return emit_tex( emit, insn );

   case TGSI_OPCODE_DDX:
   case TGSI_OPCODE_DDY:
      return emit_deriv( emit, insn );

   case TGSI_OPCODE_BGNSUB:
      return emit_bgnsub( emit, position, insn );

   case TGSI_OPCODE_ENDSUB:
      return TRUE;

   case TGSI_OPCODE_CAL:
      return emit_call( emit, insn );

   case TGSI_OPCODE_FLR:
      return emit_floor( emit, insn );

   case TGSI_OPCODE_TRUNC:
      return emit_trunc_round( emit, insn, FALSE );

   case TGSI_OPCODE_ROUND:
      return emit_trunc_round( emit, insn, TRUE );

   case TGSI_OPCODE_CEIL:
      return emit_ceil( emit, insn );

   case TGSI_OPCODE_CMP:
      return emit_cmp( emit, insn );

   case TGSI_OPCODE_DIV:
      return emit_div( emit, insn );

   case TGSI_OPCODE_DP2:
      return emit_dp2( emit, insn );

   case TGSI_OPCODE_DPH:
      return emit_dph( emit, insn );

   case TGSI_OPCODE_NRM:
      return emit_nrm( emit, insn );

   case TGSI_OPCODE_COS:
      return emit_cos( emit, insn );

   case TGSI_OPCODE_SIN:
      return emit_sin( emit, insn );

   case TGSI_OPCODE_SCS:
      return emit_sincos( emit, insn );

   case TGSI_OPCODE_END:
      /* TGSI always finishes the main func with an END */
      return emit_end( emit );

   case TGSI_OPCODE_KIL:
      return emit_kil( emit, insn );

      /* Selection opcodes.  The underlying language is fairly
       * non-orthogonal about these.
       */
   case TGSI_OPCODE_SEQ:
      return emit_select_op( emit, PIPE_FUNC_EQUAL, insn );

   case TGSI_OPCODE_SNE:
      return emit_select_op( emit, PIPE_FUNC_NOTEQUAL, insn );

   case TGSI_OPCODE_SGT:
      return emit_select_op( emit, PIPE_FUNC_GREATER, insn );

   case TGSI_OPCODE_SGE:
      return emit_select_op( emit, PIPE_FUNC_GEQUAL, insn );

   case TGSI_OPCODE_SLT:
      return emit_select_op( emit, PIPE_FUNC_LESS, insn );

   case TGSI_OPCODE_SLE:
      return emit_select_op( emit, PIPE_FUNC_LEQUAL, insn );

   case TGSI_OPCODE_SUB:
      return emit_sub( emit, insn );

   case TGSI_OPCODE_POW:
      return emit_pow( emit, insn );

   case TGSI_OPCODE_EX2:
      return emit_ex2( emit, insn );

   case TGSI_OPCODE_EXP:
      return emit_exp( emit, insn );

   case TGSI_OPCODE_LOG:
      return emit_log( emit, insn );

   case TGSI_OPCODE_LG2:
      return emit_scalar_op1( emit, SVGA3DOP_LOG, insn );

   case TGSI_OPCODE_RSQ:
      return emit_scalar_op1( emit, SVGA3DOP_RSQ, insn );

   case TGSI_OPCODE_RCP:
      return emit_scalar_op1( emit, SVGA3DOP_RCP, insn );

   case TGSI_OPCODE_CONT:
   case TGSI_OPCODE_RET:
      /* This is a noop -- we tell mesa that we can't support RET
       * within a function (early return), so this will always be
       * followed by an ENDSUB.
       */
      return TRUE;

      /* These aren't actually used by any of the frontends we care
       * about:
       */
   case TGSI_OPCODE_CLAMP:
   case TGSI_OPCODE_AND:
   case TGSI_OPCODE_OR:
   case TGSI_OPCODE_I2F:
   case TGSI_OPCODE_NOT:
   case TGSI_OPCODE_SHL:
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_XOR:
      return FALSE;

   case TGSI_OPCODE_IF:
      return emit_if( emit, insn );
   case TGSI_OPCODE_ELSE:
      return emit_else( emit, insn );
   case TGSI_OPCODE_ENDIF:
      return emit_endif( emit, insn );

   case TGSI_OPCODE_BGNLOOP:
      return emit_bgnloop2( emit, insn );
   case TGSI_OPCODE_ENDLOOP:
      return emit_endloop2( emit, insn );
   case TGSI_OPCODE_BRK:
      return emit_brk( emit, insn );

   case TGSI_OPCODE_XPD:
      return emit_xpd( emit, insn );

   case TGSI_OPCODE_KILP:
      return emit_kilp( emit, insn );

   case TGSI_OPCODE_DST:
      return emit_dst_insn( emit, insn );

   case TGSI_OPCODE_LIT:
      return emit_lit( emit, insn );

   case TGSI_OPCODE_LRP:
      return emit_lrp( emit, insn );

   case TGSI_OPCODE_SSG:
      return emit_ssg( emit, insn );

   default: {
      unsigned opcode = translate_opcode(insn->Instruction.Opcode);

      if (opcode == SVGA3DOP_LAST_INST)
         return FALSE;

      if (!emit_simple_instruction( emit, opcode, insn ))
         return FALSE;
   }
   }

   return TRUE;
}


static boolean svga_emit_immediate( struct svga_shader_emitter *emit,
                                    struct tgsi_full_immediate *imm)
{
   static const float id[4] = {0,0,0,1};
   float value[4];
   unsigned i;

   assert(1 <= imm->Immediate.NrTokens && imm->Immediate.NrTokens <= 5);
   for (i = 0; i < imm->Immediate.NrTokens - 1; i++)
      value[i] = imm->u[i].Float;

   for ( ; i < 4; i++ )
      value[i] = id[i];

   return emit_def_const( emit, SVGA3D_CONST_TYPE_FLOAT,
                          emit->imm_start + emit->internal_imm_count++,
                          value[0], value[1], value[2], value[3]);
}

static boolean make_immediate( struct svga_shader_emitter *emit,
                               float a,
                               float b,
                               float c,
                               float d,
                               struct src_register *out )
{
   unsigned idx = emit->nr_hw_float_const++;

   if (!emit_def_const( emit, SVGA3D_CONST_TYPE_FLOAT,
                        idx, a, b, c, d ))
      return FALSE;

   *out = src_register( SVGA3DREG_CONST, idx );

   return TRUE;
}

static boolean emit_vs_preamble( struct svga_shader_emitter *emit )
{
   if (!emit->key.vkey.need_prescale) {
      if (!make_immediate( emit, 0, 0, .5, .5,
                           &emit->imm_0055))
         return FALSE;
   }

   return TRUE;
}

static boolean emit_ps_preamble( struct svga_shader_emitter *emit )
{
   if (emit->ps_reads_pos && emit->info.reads_z) {
      /*
       * Assemble the position from various bits of inputs. Depth and W are
       * passed in a texcoord this is due to D3D's vPos not hold Z or W.
       * Also fixup the perspective interpolation.
       *
       * temp_pos.xy = vPos.xy
       * temp_pos.w = rcp(texcoord1.w);
       * temp_pos.z = texcoord1.z * temp_pos.w;
       */
      if (!submit_op1( emit,
                       inst_token(SVGA3DOP_MOV),
                       writemask( emit->ps_temp_pos, TGSI_WRITEMASK_XY ),
                       emit->ps_true_pos ))
         return FALSE;

      if (!submit_op1( emit,
                       inst_token(SVGA3DOP_RCP),
                       writemask( emit->ps_temp_pos, TGSI_WRITEMASK_W ),
                       scalar( emit->ps_depth_pos, TGSI_SWIZZLE_W ) ))
         return FALSE;

      if (!submit_op2( emit,
                       inst_token(SVGA3DOP_MUL),
                       writemask( emit->ps_temp_pos, TGSI_WRITEMASK_Z ),
                       scalar( emit->ps_depth_pos, TGSI_SWIZZLE_Z ),
                       scalar( src(emit->ps_temp_pos), TGSI_SWIZZLE_W ) ))
         return FALSE;
   }

   return TRUE;
}

static boolean emit_ps_postamble( struct svga_shader_emitter *emit )
{
   unsigned i;

   /* PS oDepth is incredibly fragile and it's very hard to catch the
    * types of usage that break it during shader emit.  Easier just to
    * redirect the main program to a temporary and then only touch
    * oDepth with a hand-crafted MOV below.
    */
   if (SVGA3dShaderGetRegType(emit->true_pos.value) != 0) {

      if (!submit_op1( emit,
                       inst_token(SVGA3DOP_MOV),
                       emit->true_pos,
                       scalar(src(emit->temp_pos), TGSI_SWIZZLE_Z) ))
         return FALSE;
   }

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      if (SVGA3dShaderGetRegType(emit->true_col[i].value) != 0) {

         /* Potentially override output colors with white for XOR
          * logicop workaround.
          */
         if (emit->unit == PIPE_SHADER_FRAGMENT &&
             emit->key.fkey.white_fragments) {

            struct src_register one = scalar( get_zero_immediate( emit ),
                                              TGSI_SWIZZLE_W );

            if (!submit_op1( emit,
                             inst_token(SVGA3DOP_MOV),
                             emit->true_col[i],
                             one ))
               return FALSE;
         }
         else {
            if (!submit_op1( emit,
                             inst_token(SVGA3DOP_MOV),
                             emit->true_col[i],
                             src(emit->temp_col[i]) ))
               return FALSE;
         }
      }
   }

   return TRUE;
}

static boolean emit_vs_postamble( struct svga_shader_emitter *emit )
{
   /* PSIZ output is incredibly fragile and it's very hard to catch
    * the types of usage that break it during shader emit.  Easier
    * just to redirect the main program to a temporary and then only
    * touch PSIZ with a hand-crafted MOV below.
    */
   if (SVGA3dShaderGetRegType(emit->true_psiz.value) != 0) {
      if (!submit_op1( emit,
                       inst_token(SVGA3DOP_MOV),
                       emit->true_psiz,
                       scalar(src(emit->temp_psiz), TGSI_SWIZZLE_X) ))
         return FALSE;
   }

   /* Need to perform various manipulations on vertex position to cope
    * with the different GL and D3D clip spaces.
    */
   if (emit->key.vkey.need_prescale) {
      SVGA3dShaderDestToken temp_pos = emit->temp_pos;
      SVGA3dShaderDestToken depth = emit->depth_pos;
      SVGA3dShaderDestToken pos = emit->true_pos;
      unsigned offset = emit->info.file_max[TGSI_FILE_CONSTANT] + 1;
      struct src_register prescale_scale = src_register( SVGA3DREG_CONST,
                                                         offset + 0 );
      struct src_register prescale_trans = src_register( SVGA3DREG_CONST,
                                                         offset + 1 );

      if (!submit_op1( emit,
                       inst_token(SVGA3DOP_MOV),
                       writemask(depth, TGSI_WRITEMASK_W),
                       scalar(src(temp_pos), TGSI_SWIZZLE_W) ))
         return FALSE;

      /* MUL temp_pos.xyz,    temp_pos,      prescale.scale
       * MAD result.position, temp_pos.wwww, prescale.trans, temp_pos
       *   --> Note that prescale.trans.w == 0
       */
      if (!submit_op2( emit,
                       inst_token(SVGA3DOP_MUL),
                       writemask(temp_pos, TGSI_WRITEMASK_XYZ),
                       src(temp_pos),
                       prescale_scale ))
         return FALSE;

      if (!submit_op3( emit,
                       inst_token(SVGA3DOP_MAD),
                       pos,
                       swizzle(src(temp_pos), 3, 3, 3, 3),
                       prescale_trans,
                       src(temp_pos)))
         return FALSE;

      /* Also write to depth value */
      if (!submit_op3( emit,
                       inst_token(SVGA3DOP_MAD),
                       writemask(depth, TGSI_WRITEMASK_Z),
                       swizzle(src(temp_pos), 3, 3, 3, 3),
                       prescale_trans,
                       src(temp_pos) ))
         return FALSE;
   }
   else {
      SVGA3dShaderDestToken temp_pos = emit->temp_pos;
      SVGA3dShaderDestToken depth = emit->depth_pos;
      SVGA3dShaderDestToken pos = emit->true_pos;
      struct src_register imm_0055 = emit->imm_0055;

      /* Adjust GL clipping coordinate space to hardware (D3D-style):
       *
       * DP4 temp_pos.z, {0,0,.5,.5}, temp_pos
       * MOV result.position, temp_pos
       */
      if (!submit_op2( emit,
                       inst_token(SVGA3DOP_DP4),
                       writemask(temp_pos, TGSI_WRITEMASK_Z),
                       imm_0055,
                       src(temp_pos) ))
         return FALSE;

      if (!submit_op1( emit,
                       inst_token(SVGA3DOP_MOV),
                       pos,
                       src(temp_pos) ))
         return FALSE;

      /* Move the manipulated depth into the extra texcoord reg */
      if (!submit_op1( emit,
                       inst_token(SVGA3DOP_MOV),
                       writemask(depth, TGSI_WRITEMASK_ZW),
                       src(temp_pos) ))
         return FALSE;
   }

   return TRUE;
}

/*
  0: IF VFACE :4
  1:   COLOR = FrontColor;
  2: ELSE
  3:   COLOR = BackColor;
  4: ENDIF
 */
static boolean emit_light_twoside( struct svga_shader_emitter *emit )
{
   struct src_register vface, zero;
   struct src_register front[2];
   struct src_register back[2];
   SVGA3dShaderDestToken color[2];
   int count =  emit->internal_color_count;
   int i;
   SVGA3dShaderInstToken if_token;

   if (count == 0)
      return TRUE;

   vface = get_vface( emit );
   zero = get_zero_immediate( emit );

   /* Can't use get_temp() to allocate the color reg as such
    * temporaries will be reclaimed after each instruction by the call
    * to reset_temp_regs().
    */
   for (i = 0; i < count; i++) {
      color[i] = dst_register( SVGA3DREG_TEMP, emit->nr_hw_temp++ );
      front[i] = emit->input_map[emit->internal_color_idx[i]];

      /* Back is always the next input:
       */
      back[i] = front[i];
      back[i].base.num = front[i].base.num + 1;

      /* Reassign the input_map to the actual front-face color:
       */
      emit->input_map[emit->internal_color_idx[i]] = src(color[i]);
   }

   if_token = inst_token( SVGA3DOP_IFC );

   if (emit->key.fkey.front_ccw)
      if_token.control = SVGA3DOPCOMP_LT;
   else
      if_token.control = SVGA3DOPCOMP_GT;

   zero = scalar(zero, TGSI_SWIZZLE_X);

   if (!(emit_instruction( emit, if_token ) &&
         emit_src( emit, vface ) &&
         emit_src( emit, zero ) ))
      return FALSE;

   for (i = 0; i < count; i++) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), color[i], front[i] ))
         return FALSE;
   }

   if (!(emit_instruction( emit, inst_token( SVGA3DOP_ELSE))))
      return FALSE;

   for (i = 0; i < count; i++) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), color[i], back[i] ))
         return FALSE;
   }

   if (!emit_instruction( emit, inst_token( SVGA3DOP_ENDIF ) ))
      return FALSE;

   return TRUE;
}

/*
  0: SETP_GT TEMP, VFACE, 0
  where TEMP is a fake frontface register
 */
static boolean emit_frontface( struct svga_shader_emitter *emit )
{
   struct src_register vface, zero;
   SVGA3dShaderDestToken temp;
   struct src_register pass, fail;

   vface = get_vface( emit );
   zero = get_zero_immediate( emit );

   /* Can't use get_temp() to allocate the fake frontface reg as such
    * temporaries will be reclaimed after each instruction by the call
    * to reset_temp_regs().
    */
   temp = dst_register( SVGA3DREG_TEMP,
                        emit->nr_hw_temp++ );

   if (emit->key.fkey.front_ccw) {
      pass = scalar( zero, TGSI_SWIZZLE_X );
      fail = scalar( zero, TGSI_SWIZZLE_W );
   } else {
      pass = scalar( zero, TGSI_SWIZZLE_W );
      fail = scalar( zero, TGSI_SWIZZLE_X );
   }

   if (!emit_conditional(emit, PIPE_FUNC_GREATER,
                         temp, vface, scalar( zero, TGSI_SWIZZLE_X ),
                         pass, fail))
      return FALSE;

   /* Reassign the input_map to the actual front-face color:
    */
   emit->input_map[emit->internal_frontface_idx] = src(temp);

   return TRUE;
}


/**
 * Emit code to invert the T component of the incoming texture coordinate.
 * This is used for drawing point sprites when
 * pipe_rasterizer_state::sprite_coord_mode == PIPE_SPRITE_COORD_LOWER_LEFT.
 */
static boolean emit_inverted_texcoords( struct svga_shader_emitter *emit )
{
   struct src_register zero = get_zero_immediate(emit);
   struct src_register pos_neg_one = get_pos_neg_one_immediate( emit );
   unsigned inverted_texcoords = emit->inverted_texcoords;

   while (inverted_texcoords) {
      const unsigned unit = ffs(inverted_texcoords) - 1;

      assert(emit->inverted_texcoords & (1 << unit));

      assert(unit < Elements(emit->ps_true_texcoord));

      assert(unit < Elements(emit->ps_inverted_texcoord_input));

      assert(emit->ps_inverted_texcoord_input[unit]
             < Elements(emit->input_map));

      /* inverted = coord * (1, -1, 1, 1) + (0, 1, 0, 0) */
      if (!submit_op3(emit,
                      inst_token(SVGA3DOP_MAD),
                      dst(emit->ps_inverted_texcoord[unit]),
                      emit->ps_true_texcoord[unit],
                      swizzle(pos_neg_one, 0, 3, 0, 0),  /* (1, -1, 1, 1) */
                      swizzle(zero, 0, 3, 0, 0)))  /* (0, 1, 0, 0) */
         return FALSE;

      /* Reassign the input_map entry to the new texcoord register */
      emit->input_map[emit->ps_inverted_texcoord_input[unit]] =
         emit->ps_inverted_texcoord[unit];

      inverted_texcoords &= ~(1 << unit);
   }

   return TRUE;
}


static INLINE boolean
needs_to_create_zero( struct svga_shader_emitter *emit )
{
   int i;

   if (emit->unit == PIPE_SHADER_FRAGMENT) {
      if (emit->key.fkey.light_twoside)
         return TRUE;

      if (emit->key.fkey.white_fragments)
         return TRUE;

      if (emit->emit_frontface)
         return TRUE;

      if (emit->info.opcode_count[TGSI_OPCODE_DST] >= 1 ||
          emit->info.opcode_count[TGSI_OPCODE_SSG] >= 1 ||
          emit->info.opcode_count[TGSI_OPCODE_LIT] >= 1)
         return TRUE;

      if (emit->inverted_texcoords)
         return TRUE;

      /* look for any PIPE_SWIZZLE_ZERO/ONE terms */
      for (i = 0; i < emit->key.fkey.num_textures; i++) {
         if (emit->key.fkey.tex[i].swizzle_r > PIPE_SWIZZLE_ALPHA ||
             emit->key.fkey.tex[i].swizzle_g > PIPE_SWIZZLE_ALPHA ||
             emit->key.fkey.tex[i].swizzle_b > PIPE_SWIZZLE_ALPHA ||
             emit->key.fkey.tex[i].swizzle_a > PIPE_SWIZZLE_ALPHA)
            return TRUE;
      }

      for (i = 0; i < emit->key.fkey.num_textures; i++) {
         if (emit->key.fkey.tex[i].compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE)
            return TRUE;
      }
   }

   if (emit->unit == PIPE_SHADER_VERTEX) {
      if (emit->info.opcode_count[TGSI_OPCODE_CMP] >= 1)
         return TRUE;
   }

   if (emit->info.opcode_count[TGSI_OPCODE_IF] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_BGNLOOP] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_DDX] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_DDY] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_ROUND] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_SGE] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_SGT] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_SLE] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_SLT] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_SNE] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_SEQ] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_EXP] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_LOG] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_XPD] >= 1 ||
       emit->info.opcode_count[TGSI_OPCODE_KILP] >= 1)
      return TRUE;

   return FALSE;
}

static INLINE boolean
needs_to_create_loop_const( struct svga_shader_emitter *emit )
{
   return (emit->info.opcode_count[TGSI_OPCODE_BGNLOOP] >= 1);
}

static INLINE boolean
needs_to_create_arl_consts( struct svga_shader_emitter *emit )
{
   return (emit->num_arl_consts > 0);
}

static INLINE boolean
pre_parse_add_indirect( struct svga_shader_emitter *emit,
                        int num, int current_arl)
{
   int i;
   assert(num < 0);

   for (i = 0; i < emit->num_arl_consts; ++i) {
      if (emit->arl_consts[i].arl_num == current_arl)
         break;
   }
   /* new entry */
   if (emit->num_arl_consts == i) {
      ++emit->num_arl_consts;
   }
   emit->arl_consts[i].number = (emit->arl_consts[i].number > num) ?
                                num :
                                emit->arl_consts[i].number;
   emit->arl_consts[i].arl_num = current_arl;
   return TRUE;
}

static boolean
pre_parse_instruction( struct svga_shader_emitter *emit,
                       const struct tgsi_full_instruction *insn,
                       int current_arl)
{
   if (insn->Src[0].Register.Indirect &&
       insn->Src[0].Indirect.File == TGSI_FILE_ADDRESS) {
      const struct tgsi_full_src_register *reg = &insn->Src[0];
      if (reg->Register.Index < 0) {
         pre_parse_add_indirect(emit, reg->Register.Index, current_arl);
      }
   }

   if (insn->Src[1].Register.Indirect &&
       insn->Src[1].Indirect.File == TGSI_FILE_ADDRESS) {
      const struct tgsi_full_src_register *reg = &insn->Src[1];
      if (reg->Register.Index < 0) {
         pre_parse_add_indirect(emit, reg->Register.Index, current_arl);
      }
   }

   if (insn->Src[2].Register.Indirect &&
       insn->Src[2].Indirect.File == TGSI_FILE_ADDRESS) {
      const struct tgsi_full_src_register *reg = &insn->Src[2];
      if (reg->Register.Index < 0) {
         pre_parse_add_indirect(emit, reg->Register.Index, current_arl);
      }
   }

   return TRUE;
}

static boolean
pre_parse_tokens( struct svga_shader_emitter *emit,
                  const struct tgsi_token *tokens )
{
   struct tgsi_parse_context parse;
   int current_arl = 0;

   tgsi_parse_init( &parse, tokens );

   while (!tgsi_parse_end_of_tokens( &parse )) {
      tgsi_parse_token( &parse );
      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_IMMEDIATE:
      case TGSI_TOKEN_TYPE_DECLARATION:
         break;
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (parse.FullToken.FullInstruction.Instruction.Opcode ==
             TGSI_OPCODE_ARL) {
            ++current_arl;
         }
         if (!pre_parse_instruction( emit, &parse.FullToken.FullInstruction,
                                     current_arl ))
            return FALSE;
         break;
      default:
         break;
      }

   }
   return TRUE;
}

static boolean svga_shader_emit_helpers( struct svga_shader_emitter *emit )

{
   if (needs_to_create_zero( emit )) {
      create_zero_immediate( emit );
   }
   if (needs_to_create_loop_const( emit )) {
      create_loop_const( emit );
   }
   if (needs_to_create_arl_consts( emit )) {
      create_arl_consts( emit );
   }

   if (emit->unit == PIPE_SHADER_FRAGMENT) {
      if (!emit_ps_preamble( emit ))
         return FALSE;

      if (emit->key.fkey.light_twoside) {
         if (!emit_light_twoside( emit ))
            return FALSE;
      }
      if (emit->emit_frontface) {
         if (!emit_frontface( emit ))
            return FALSE;
      }
      if (emit->inverted_texcoords) {
         if (!emit_inverted_texcoords( emit ))
            return FALSE;
      }
   }

   return TRUE;
}

boolean svga_shader_emit_instructions( struct svga_shader_emitter *emit,
                                       const struct tgsi_token *tokens )
{
   struct tgsi_parse_context parse;
   boolean ret = TRUE;
   boolean helpers_emitted = FALSE;
   unsigned line_nr = 0;

   tgsi_parse_init( &parse, tokens );
   emit->internal_imm_count = 0;

   if (emit->unit == PIPE_SHADER_VERTEX) {
      ret = emit_vs_preamble( emit );
      if (!ret)
         goto done;
   }

   pre_parse_tokens(emit, tokens);

   while (!tgsi_parse_end_of_tokens( &parse )) {
      tgsi_parse_token( &parse );

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_IMMEDIATE:
         ret = svga_emit_immediate( emit, &parse.FullToken.FullImmediate );
         if (!ret)
            goto done;
         break;

      case TGSI_TOKEN_TYPE_DECLARATION:
         ret = svga_translate_decl_sm30( emit, &parse.FullToken.FullDeclaration );
         if (!ret)
            goto done;
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (!helpers_emitted) {
            if (!svga_shader_emit_helpers( emit ))
               goto done;
            helpers_emitted = TRUE;
         }
         ret = svga_emit_instruction( emit,
                                      line_nr++,
                                      &parse.FullToken.FullInstruction );
         if (!ret)
            goto done;
         break;
      default:
         break;
      }

      reset_temp_regs( emit );
   }

   /* Need to terminate the current subroutine.  Note that the
    * hardware doesn't tolerate shaders without sub-routines
    * terminating with RET+END.
    */
   if (!emit->in_main_func) {
      ret = emit_instruction( emit, inst_token( SVGA3DOP_RET ) );
      if (!ret)
         goto done;
   }

   assert(emit->dynamic_branching_level == 0);

   /* Need to terminate the whole shader:
    */
   ret = emit_instruction( emit, inst_token( SVGA3DOP_END ) );
   if (!ret)
      goto done;

done:
   tgsi_parse_free( &parse );
   return ret;
}
