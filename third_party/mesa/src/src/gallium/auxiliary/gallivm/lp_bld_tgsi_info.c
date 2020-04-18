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


#include "util/u_memory.h"
#include "util/u_math.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_strings.h"
#include "lp_bld_debug.h"
#include "lp_bld_tgsi.h"


/**
 * Analysis context.
 *
 * This is where we keep store the value of each channel of the IMM/TEMP/OUT
 * register values, as we walk the shader.
 */
struct analysis_context
{
   struct lp_tgsi_info *info;

   unsigned num_imms;
   float imm[128][4];

   struct lp_tgsi_channel_info temp[32][4];
};


/**
 * Describe the specified channel of the src register.
 */
static void
analyse_src(struct analysis_context *ctx,
            struct lp_tgsi_channel_info *chan_info,
            const struct tgsi_src_register *src,
            unsigned chan)
{
   chan_info->file = TGSI_FILE_NULL;
   if (!src->Indirect && !src->Absolute && !src->Negate) {
      unsigned swizzle = tgsi_util_get_src_register_swizzle(src, chan);
      if (src->File == TGSI_FILE_TEMPORARY) {
         if (src->Index < Elements(ctx->temp)) {
            *chan_info = ctx->temp[src->Index][swizzle];
         }
      } else {
         chan_info->file = src->File;
         if (src->File == TGSI_FILE_IMMEDIATE) {
            assert(src->Index < Elements(ctx->imm));
            if (src->Index < Elements(ctx->imm)) {
               chan_info->u.value = ctx->imm[src->Index][swizzle];
            }
         } else {
            chan_info->u.index = src->Index;
            chan_info->swizzle = swizzle;
         }
      }
   }
}


/**
 * Whether this register channel refers to a specific immediate value.
 */
static boolean
is_immediate(const struct lp_tgsi_channel_info *chan_info, float value)
{
   return chan_info->file == TGSI_FILE_IMMEDIATE &&
          chan_info->u.value == value;
}


static void
analyse_tex(struct analysis_context *ctx,
            const struct tgsi_full_instruction *inst,
            enum lp_build_tex_modifier modifier)
{
   struct lp_tgsi_info *info = ctx->info;
   unsigned chan;

   if (info->num_texs < Elements(info->tex)) {
      struct lp_tgsi_texture_info *tex_info = &info->tex[info->num_texs];
      boolean indirect = FALSE;
      unsigned readmask = 0;

      tex_info->target = inst->Texture.Texture;
      switch (inst->Texture.Texture) {
      case TGSI_TEXTURE_1D:
         readmask = TGSI_WRITEMASK_X;
         break;
      case TGSI_TEXTURE_1D_ARRAY:
      case TGSI_TEXTURE_2D:
      case TGSI_TEXTURE_RECT:
         readmask = TGSI_WRITEMASK_XY;
         break;
      case TGSI_TEXTURE_SHADOW1D:
      case TGSI_TEXTURE_SHADOW1D_ARRAY:
      case TGSI_TEXTURE_SHADOW2D:
      case TGSI_TEXTURE_SHADOWRECT:
      case TGSI_TEXTURE_2D_ARRAY:
      case TGSI_TEXTURE_3D:
      case TGSI_TEXTURE_CUBE:
         readmask = TGSI_WRITEMASK_XYZ;
         break;
      case TGSI_TEXTURE_SHADOW2D_ARRAY:
         readmask = TGSI_WRITEMASK_XYZW;
         break;
      default:
         assert(0);
         return;
      }

      if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV) {
         /* We don't track explicit derivatives, although we could */
         indirect = TRUE;
         tex_info->unit = inst->Src[3].Register.Index;
      }  else {
         if (modifier == LP_BLD_TEX_MODIFIER_PROJECTED ||
             modifier == LP_BLD_TEX_MODIFIER_LOD_BIAS ||
             modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_LOD) {
            readmask |= TGSI_WRITEMASK_W;
         }
         tex_info->unit = inst->Src[1].Register.Index;
      }

      for (chan = 0; chan < 4; ++chan) {
         struct lp_tgsi_channel_info *chan_info = &tex_info->coord[chan];
         if (readmask & (1 << chan)) {
            analyse_src(ctx, chan_info, &inst->Src[0].Register, chan);
            if (chan_info->file != TGSI_FILE_INPUT) {
               indirect = TRUE;
            }
         } else {
            memset(chan_info, 0, sizeof *chan_info);
         }
      }

      if (indirect) {
         info->indirect_textures = TRUE;
      }

      ++info->num_texs;
   } else {
      info->indirect_textures = TRUE;
   }
}


/**
 * Process an instruction, and update the register values accordingly.
 */
static void
analyse_instruction(struct analysis_context *ctx,
                    struct tgsi_full_instruction *inst)
{
   struct lp_tgsi_info *info = ctx->info;
   struct lp_tgsi_channel_info (*regs)[4];
   unsigned max_regs;
   unsigned i;
   unsigned index;
   unsigned chan;

   for (i = 0; i < inst->Instruction.NumDstRegs; ++i) {
      const struct tgsi_dst_register *dst = &inst->Dst[i].Register;

      /*
       * Get the lp_tgsi_channel_info array corresponding to the destination
       * register file.
       */

      if (dst->File == TGSI_FILE_TEMPORARY) {
         regs = ctx->temp;
         max_regs = Elements(ctx->temp);
      } else if (dst->File == TGSI_FILE_OUTPUT) {
         regs = info->output;
         max_regs = Elements(info->output);
      } else if (dst->File == TGSI_FILE_ADDRESS ||
                 dst->File == TGSI_FILE_PREDICATE) {
         continue;
      } else {
         assert(0);
         continue;
      }

      /*
       * Detect direct TEX instructions
       */

      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_TEX:
         analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_NONE);
         break;
      case TGSI_OPCODE_TXD:
         analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV);
         break;
      case TGSI_OPCODE_TXB:
         analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_LOD_BIAS);
         break;
      case TGSI_OPCODE_TXL:
         analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_LOD);
         break;
      case TGSI_OPCODE_TXP:
         analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_PROJECTED);
         break;
      default:
         break;
      }

      /*
       * Keep track of assignments and writes
       */

      if (dst->Indirect) {
         /*
          * It could be any register index so clear all register indices.
          */

         for (chan = 0; chan < 4; ++chan) {
            if (dst->WriteMask & (1 << chan)) {
               for (index = 0; index < max_regs; ++index) {
                  regs[index][chan].file = TGSI_FILE_NULL;
               }
            }
         }
      } else if (dst->Index < max_regs) {
         /*
          * Update this destination register value.
          */

         struct lp_tgsi_channel_info res[4];

         memset(res, 0, sizeof res);

         if (!inst->Instruction.Predicate &&
             !inst->Instruction.Saturate) {
            for (chan = 0; chan < 4; ++chan) {
               if (dst->WriteMask & (1 << chan)) {
                  if (inst->Instruction.Opcode == TGSI_OPCODE_MOV) {
                     analyse_src(ctx, &res[chan],
                                 &inst->Src[0].Register, chan);
                  } else if (inst->Instruction.Opcode == TGSI_OPCODE_MUL) {
                     /*
                      * Propagate values across 1.0 and 0.0 multiplications.
                      */

                     struct lp_tgsi_channel_info src0;
                     struct lp_tgsi_channel_info src1;

                     analyse_src(ctx, &src0, &inst->Src[0].Register, chan);
                     analyse_src(ctx, &src1, &inst->Src[1].Register, chan);

                     if (is_immediate(&src0, 0.0f)) {
                        res[chan] = src0;
                     } else if (is_immediate(&src1, 0.0f)) {
                        res[chan] = src1;
                     } else if (is_immediate(&src0, 1.0f)) {
                        res[chan] = src1;
                     } else if (is_immediate(&src1, 1.0f)) {
                        res[chan] = src0;
                     }
                  }
               }
            }
         }

         for (chan = 0; chan < 4; ++chan) {
            if (dst->WriteMask & (1 << chan)) {
               regs[dst->Index][chan] = res[chan];
            }
         }
      }
   }

   /*
    * Clear all temporaries information in presence of a control flow opcode.
    */

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_IF:
   case TGSI_OPCODE_IFC:
   case TGSI_OPCODE_ELSE:
   case TGSI_OPCODE_ENDIF:
   case TGSI_OPCODE_BGNLOOP:
   case TGSI_OPCODE_BRK:
   case TGSI_OPCODE_BREAKC:
   case TGSI_OPCODE_CONT:
   case TGSI_OPCODE_ENDLOOP:
   case TGSI_OPCODE_CALLNZ:
   case TGSI_OPCODE_CAL:
   case TGSI_OPCODE_BGNSUB:
   case TGSI_OPCODE_ENDSUB:
   case TGSI_OPCODE_SWITCH:
   case TGSI_OPCODE_CASE:
   case TGSI_OPCODE_DEFAULT:
   case TGSI_OPCODE_ENDSWITCH:
   case TGSI_OPCODE_RET:
   case TGSI_OPCODE_END:
      /* XXX: Are there more cases? */
      memset(&ctx->temp, 0, sizeof ctx->temp);
      memset(&info->output, 0, sizeof info->output);
   default:
      break;
   }
}


static INLINE void
dump_info(const struct tgsi_token *tokens,
          struct lp_tgsi_info *info)
{
   unsigned index;
   unsigned chan;

   tgsi_dump(tokens, 0);

   for (index = 0; index < info->num_texs; ++index) {
      const struct lp_tgsi_texture_info *tex_info = &info->tex[index];
      debug_printf("TEX[%u] =", index);
      for (chan = 0; chan < 4; ++chan) {
         const struct lp_tgsi_channel_info *chan_info =
               &tex_info->coord[chan];
         if (chan_info->file != TGSI_FILE_NULL) {
            debug_printf(" %s[%u].%c",
                         tgsi_file_names[chan_info->file],
                         chan_info->u.index,
                         "xyzw01"[chan_info->swizzle]);
         } else {
            debug_printf(" _");
         }
      }
      debug_printf(", SAMP[%u], %s\n",
                   tex_info->unit,
                   tgsi_texture_names[tex_info->target]);
   }

   for (index = 0; index < PIPE_MAX_SHADER_OUTPUTS; ++index) {
      for (chan = 0; chan < 4; ++chan) {
         const struct lp_tgsi_channel_info *chan_info =
               &info->output[index][chan];
         if (chan_info->file != TGSI_FILE_NULL) {
            debug_printf("OUT[%u].%c = ", index, "xyzw"[chan]);
            if (chan_info->file == TGSI_FILE_IMMEDIATE) {
               debug_printf("%f", chan_info->u.value);
            } else {
               const char *file_name;
               switch (chan_info->file) {
               case TGSI_FILE_CONSTANT:
                  file_name = "CONST";
                  break;
               case TGSI_FILE_INPUT:
                  file_name = "IN";
                  break;
               default:
                  file_name = "???";
                  break;
               }
               debug_printf("%s[%u].%c",
                            file_name,
                            chan_info->u.index,
                            "xyzw01"[chan_info->swizzle]);
            }
            debug_printf("\n");
         }
      }
   }
}


/**
 * Detect any direct relationship between the output color
 */
void
lp_build_tgsi_info(const struct tgsi_token *tokens,
                   struct lp_tgsi_info *info)
{
   struct tgsi_parse_context parse;
   struct analysis_context ctx;
   unsigned index;
   unsigned chan;

   memset(info, 0, sizeof *info);

   tgsi_scan_shader(tokens, &info->base);

   memset(&ctx, 0, sizeof ctx);
   ctx.info = info;

   tgsi_parse_init(&parse, tokens);

   while (!tgsi_parse_end_of_tokens(&parse)) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         {
            struct tgsi_full_instruction *inst =
                  &parse.FullToken.FullInstruction;

            if (inst->Instruction.Opcode == TGSI_OPCODE_END ||
                inst->Instruction.Opcode == TGSI_OPCODE_BGNSUB) {
               /* We reached the end of main function body. */
               goto finished;
            }

            analyse_instruction(&ctx, inst);
         }
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         {
            const unsigned size =
                  parse.FullToken.FullImmediate.Immediate.NrTokens - 1;
            assert(size <= 4);
            if (ctx.num_imms < Elements(ctx.imm)) {
               for (chan = 0; chan < size; ++chan) {
                  float value = parse.FullToken.FullImmediate.u[chan].Float;
                  ctx.imm[ctx.num_imms][chan] = value;

                  if (value < 0.0f || value > 1.0f) {
                     info->unclamped_immediates = TRUE;
                  }
               }
               ++ctx.num_imms;
            }
         }
         break;

      case TGSI_TOKEN_TYPE_PROPERTY:
         break;

      default:
         assert(0);
      }
   }
finished:

   tgsi_parse_free(&parse);


   /*
    * Link the output color values.
    */

   for (index = 0; index < PIPE_MAX_COLOR_BUFS; ++index) {
      const struct lp_tgsi_channel_info null_output[4];
      info->cbuf[index] = null_output;
   }

   for (index = 0; index < info->base.num_outputs; ++index) {
      unsigned semantic_name = info->base.output_semantic_name[index];
      unsigned semantic_index = info->base.output_semantic_index[index];
      if (semantic_name == TGSI_SEMANTIC_COLOR &&
          semantic_index < PIPE_MAX_COLOR_BUFS) {
         info->cbuf[semantic_index] = info->output[index];
      }
   }

   if (gallivm_debug & GALLIVM_DEBUG_TGSI) {
      dump_info(tokens, info);
   }
}
