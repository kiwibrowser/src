/**************************************************************************
 * 
 * Copyright 2011 The Chromium OS authors.
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
 * IN NO EVENT SHALL GOOGLE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_fpc.h"

#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_string.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_dump.h"

static boolean same_src_dst_reg(struct i915_full_src_register* s1, struct i915_full_dst_register* d1)
{
   return (s1->Register.File == d1->Register.File &&
           s1->Register.Indirect == d1->Register.Indirect &&
           s1->Register.Dimension == d1->Register.Dimension &&
           s1->Register.Index == d1->Register.Index);
}

static boolean same_dst_reg(struct i915_full_dst_register* d1, struct i915_full_dst_register* d2)
{
   return (d1->Register.File == d2->Register.File &&
           d1->Register.Indirect == d2->Register.Indirect &&
           d1->Register.Dimension == d2->Register.Dimension &&
           d1->Register.Index == d2->Register.Index);
}

static boolean same_src_reg(struct i915_full_src_register* d1, struct i915_full_src_register* d2)
{
   return (d1->Register.File == d2->Register.File &&
           d1->Register.Indirect == d2->Register.Indirect &&
           d1->Register.Dimension == d2->Register.Dimension &&
           d1->Register.Index == d2->Register.Index &&
           d1->Register.Absolute == d2->Register.Absolute &&
           d1->Register.Negate == d2->Register.Negate);
}

static boolean has_destination(unsigned opcode)
{
   return (opcode != TGSI_OPCODE_NOP &&
           opcode != TGSI_OPCODE_KIL &&
           opcode != TGSI_OPCODE_KILP &&
           opcode != TGSI_OPCODE_END &&
           opcode != TGSI_OPCODE_RET);
}

static boolean is_unswizzled(struct i915_full_src_register* r,
                             unsigned write_mask)
{
   if ( write_mask & TGSI_WRITEMASK_X && r->Register.SwizzleX != TGSI_SWIZZLE_X)
      return FALSE;
   if ( write_mask & TGSI_WRITEMASK_Y && r->Register.SwizzleY != TGSI_SWIZZLE_Y)
      return FALSE;
   if ( write_mask & TGSI_WRITEMASK_Z && r->Register.SwizzleZ != TGSI_SWIZZLE_Z)
      return FALSE;
   if ( write_mask & TGSI_WRITEMASK_W && r->Register.SwizzleW != TGSI_SWIZZLE_W)
      return FALSE;
   return TRUE;
}

static boolean op_commutes(unsigned opcode)
{
   switch(opcode)
   {
      case TGSI_OPCODE_ADD:
      case TGSI_OPCODE_MUL:
      case TGSI_OPCODE_DP2:
      case TGSI_OPCODE_DP3:
      case TGSI_OPCODE_DP4:
         return TRUE;
   }
   return FALSE;
}

static unsigned op_neutral_element(unsigned opcode)
{
   switch(opcode)
   {
      case TGSI_OPCODE_ADD:
         return TGSI_SWIZZLE_ZERO;
      case TGSI_OPCODE_MUL:
      case TGSI_OPCODE_DP2:
      case TGSI_OPCODE_DP3:
      case TGSI_OPCODE_DP4:
         return TGSI_SWIZZLE_ONE;
   }

   debug_printf("Unknown opcode %d\n",opcode);
   return TGSI_SWIZZLE_ZERO;
}

/*
 * Sets the swizzle to the neutral element for the operation for the bits
 * of writemask which are set, swizzle to identity otherwise.
 */
static void set_neutral_element_swizzle(struct i915_full_src_register* r,
                                        unsigned write_mask,
                                        unsigned neutral)
{
   if ( write_mask & TGSI_WRITEMASK_X )
      r->Register.SwizzleX = neutral;
   else
      r->Register.SwizzleX = TGSI_SWIZZLE_X;

   if ( write_mask & TGSI_WRITEMASK_Y )
      r->Register.SwizzleY = neutral;
   else
      r->Register.SwizzleY = TGSI_SWIZZLE_Y;

   if ( write_mask & TGSI_WRITEMASK_Z )
      r->Register.SwizzleZ = neutral;
   else
      r->Register.SwizzleZ = TGSI_SWIZZLE_Z;

   if ( write_mask & TGSI_WRITEMASK_W )
      r->Register.SwizzleW = neutral;
   else
      r->Register.SwizzleW = TGSI_SWIZZLE_W;
}

static void copy_src_reg(struct i915_src_register* o, const struct tgsi_src_register* i)
{
   o->File      = i->File;
   o->Indirect  = i->Indirect;
   o->Dimension = i->Dimension;
   o->Index     = i->Index;
   o->SwizzleX  = i->SwizzleX;
   o->SwizzleY  = i->SwizzleY;
   o->SwizzleZ  = i->SwizzleZ;
   o->SwizzleW  = i->SwizzleW;
   o->Absolute  = i->Absolute;
   o->Negate    = i->Negate;
}

static void copy_dst_reg(struct i915_dst_register* o, const struct tgsi_dst_register* i)
{
   o->File      = i->File;
   o->WriteMask = i->WriteMask;
   o->Indirect  = i->Indirect;
   o->Dimension = i->Dimension;
   o->Index     = i->Index;
}

static void copy_instruction(struct i915_full_instruction* o, const struct tgsi_full_instruction* i)
{
   memcpy(&o->Instruction, &i->Instruction, sizeof(o->Instruction));
   memcpy(&o->Texture, &i->Texture, sizeof(o->Texture));

   copy_dst_reg(&o->Dst[0].Register, &i->Dst[0].Register);

   copy_src_reg(&o->Src[0].Register, &i->Src[0].Register);
   copy_src_reg(&o->Src[1].Register, &i->Src[1].Register);
   copy_src_reg(&o->Src[2].Register, &i->Src[2].Register);
}

static void copy_token(union i915_full_token* o, union tgsi_full_token* i)
{
   if (i->Token.Type != TGSI_TOKEN_TYPE_INSTRUCTION)
      memcpy(o, i, sizeof(*o));
   else
      copy_instruction(&o->FullInstruction, &i->FullInstruction);

}

/*
 * Optimize away things like:
 *    MUL OUT[0].xyz, TEMP[1], TEMP[2]
 *    MOV OUT[0].w, TEMP[2]
 * into:
 *    MUL OUT[0].xyzw, TEMP[1].xyz1, TEMP[2]
 * This is useful for optimizing texenv.
 */
static void i915_fpc_optimize_mov_after_alu(union i915_full_token* current, union i915_full_token* next)
{
   if ( current->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION  &&
        next->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION  &&
        op_commutes(current->FullInstruction.Instruction.Opcode) &&
        current->FullInstruction.Instruction.Saturate == next->FullInstruction.Instruction.Saturate &&
        next->FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
        same_dst_reg(&next->FullInstruction.Dst[0], &current->FullInstruction.Dst[0]) &&
        same_src_reg(&next->FullInstruction.Src[0], &current->FullInstruction.Src[1]) &&
        !same_src_dst_reg(&next->FullInstruction.Src[0], &current->FullInstruction.Dst[0]) &&
        is_unswizzled(&current->FullInstruction.Src[0], current->FullInstruction.Dst[0].Register.WriteMask) &&
        is_unswizzled(&current->FullInstruction.Src[1], current->FullInstruction.Dst[0].Register.WriteMask) &&
        is_unswizzled(&next->FullInstruction.Src[0], next->FullInstruction.Dst[0].Register.WriteMask) )
   {
      next->FullInstruction.Instruction.Opcode = TGSI_OPCODE_NOP;

      set_neutral_element_swizzle(&current->FullInstruction.Src[1], 0, 0);
      set_neutral_element_swizzle(&current->FullInstruction.Src[0],
                                  next->FullInstruction.Dst[0].Register.WriteMask,
                                  op_neutral_element(current->FullInstruction.Instruction.Opcode));

      current->FullInstruction.Dst[0].Register.WriteMask = current->FullInstruction.Dst[0].Register.WriteMask |
                                                           next->FullInstruction.Dst[0].Register.WriteMask;
      return;
   }

   if ( current->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION  &&
        next->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION  &&
        op_commutes(current->FullInstruction.Instruction.Opcode) &&
        current->FullInstruction.Instruction.Saturate == next->FullInstruction.Instruction.Saturate &&
        next->FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
        same_dst_reg(&next->FullInstruction.Dst[0], &current->FullInstruction.Dst[0]) &&
        same_src_reg(&next->FullInstruction.Src[0], &current->FullInstruction.Src[0]) &&
        !same_src_dst_reg(&next->FullInstruction.Src[0], &current->FullInstruction.Dst[0]) &&
        is_unswizzled(&current->FullInstruction.Src[0], current->FullInstruction.Dst[0].Register.WriteMask) &&
        is_unswizzled(&current->FullInstruction.Src[1], current->FullInstruction.Dst[0].Register.WriteMask) &&
        is_unswizzled(&next->FullInstruction.Src[0], next->FullInstruction.Dst[0].Register.WriteMask) )
   {
      next->FullInstruction.Instruction.Opcode = TGSI_OPCODE_NOP;

      set_neutral_element_swizzle(&current->FullInstruction.Src[0], 0, 0);
      set_neutral_element_swizzle(&current->FullInstruction.Src[1],
                                  next->FullInstruction.Dst[0].Register.WriteMask,
                                  op_neutral_element(current->FullInstruction.Instruction.Opcode));

      current->FullInstruction.Dst[0].Register.WriteMask = current->FullInstruction.Dst[0].Register.WriteMask |
                                                           next->FullInstruction.Dst[0].Register.WriteMask;
      return;
   }
}

/*
 * Optimize away things like:
 *    MOV TEMP[0].xyz TEMP[0].xyzx
 * into:
 *    NOP
 */
static boolean i915_fpc_useless_mov(union tgsi_full_token* tgsi_current)
{
   union i915_full_token current;
   copy_token(&current , tgsi_current);
   if ( current.Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION  &&
        current.FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
        has_destination(current.FullInstruction.Instruction.Opcode) &&
        current.FullInstruction.Instruction.Saturate == TGSI_SAT_NONE &&
        current.FullInstruction.Src[0].Register.Absolute == 0 &&
        current.FullInstruction.Src[0].Register.Negate == 0 &&
        is_unswizzled(&current.FullInstruction.Src[0], current.FullInstruction.Dst[0].Register.WriteMask) &&
        same_src_dst_reg(&current.FullInstruction.Src[0], &current.FullInstruction.Dst[0]) )
   {
      return TRUE;
   }
   return FALSE;
}

/*
 * Optimize away things like:
 *    *** TEMP[0], TEMP[1], TEMP[2]
 *    MOV OUT[0] TEMP[0]
 * into:
 *    *** OUT[0], TEMP[1], TEMP[2]
 */
static void i915_fpc_optimize_useless_mov_after_inst(union i915_full_token* current, union i915_full_token* next)
{
   if ( current->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION  &&
        next->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION  &&
        next->FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
        has_destination(current->FullInstruction.Instruction.Opcode) &&
        next->FullInstruction.Instruction.Saturate == TGSI_SAT_NONE &&
        next->FullInstruction.Src[0].Register.Absolute == 0 &&
        next->FullInstruction.Src[0].Register.Negate == 0 &&
        next->FullInstruction.Dst[0].Register.File == TGSI_FILE_OUTPUT &&
        is_unswizzled(&next->FullInstruction.Src[0], next->FullInstruction.Dst[0].Register.WriteMask) &&
        current->FullInstruction.Dst[0].Register.WriteMask == next->FullInstruction.Dst[0].Register.WriteMask &&
        same_src_dst_reg(&next->FullInstruction.Src[0], &current->FullInstruction.Dst[0]) )
   {
      next->FullInstruction.Instruction.Opcode = TGSI_OPCODE_NOP;

      current->FullInstruction.Dst[0] = next->FullInstruction.Dst[0];
      return;
   }
}

struct i915_token_list* i915_optimize(const struct tgsi_token *tokens)
{
   struct i915_token_list *out_tokens = MALLOC(sizeof(struct i915_token_list));
   struct tgsi_parse_context parse;
   int i = 0;

   out_tokens->NumTokens = 0;

   /* Count the tokens */
   tgsi_parse_init( &parse, tokens );
   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );
      out_tokens->NumTokens++;
   }
   tgsi_parse_free (&parse);

   /* Allocate our tokens */
   out_tokens->Tokens = MALLOC(sizeof(union i915_full_token) * out_tokens->NumTokens);

   tgsi_parse_init( &parse, tokens );
   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

      if (i915_fpc_useless_mov(&parse.FullToken)) {
         out_tokens->NumTokens--;
         continue;
      }

      copy_token(&out_tokens->Tokens[i] , &parse.FullToken);

      if (i > 0) {
         i915_fpc_optimize_useless_mov_after_inst(&out_tokens->Tokens[i-1], &out_tokens->Tokens[i]);
         i915_fpc_optimize_mov_after_alu(&out_tokens->Tokens[i-1], &out_tokens->Tokens[i]);
      }
      i++;
   }
   tgsi_parse_free (&parse);

   return out_tokens;
}

void i915_optimize_free(struct i915_token_list* tokens)
{
   free(tokens->Tokens);
   free(tokens);
}


