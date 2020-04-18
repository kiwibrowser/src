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
#include "tgsi/tgsi_parse.h"
#include "util/u_memory.h"

#include "svga_tgsi_emit.h"


/**
 * Translate TGSI semantic info into SVGA3d semantic info.
 * This is called for VS outputs and PS inputs only.
 */
static boolean
translate_vs_ps_semantic(struct svga_shader_emitter *emit,
                         struct tgsi_declaration_semantic semantic,
                         unsigned *usage,
                         unsigned *idx)
{
   switch (semantic.Name) {
   case TGSI_SEMANTIC_POSITION:  
      *idx = semantic.Index;
      *usage = SVGA3D_DECLUSAGE_POSITION;
      break;
   case TGSI_SEMANTIC_COLOR:     
      *idx = semantic.Index;
      *usage = SVGA3D_DECLUSAGE_COLOR;
      break;
   case TGSI_SEMANTIC_BCOLOR:
      *idx = semantic.Index + 2; /* sharing with COLOR */
      *usage = SVGA3D_DECLUSAGE_COLOR;
      break;
   case TGSI_SEMANTIC_FOG:       
      *idx = 0;
      assert(semantic.Index == 0);
      *usage = SVGA3D_DECLUSAGE_TEXCOORD;
      break;
   case TGSI_SEMANTIC_PSIZE:     
      *idx = semantic.Index;
      *usage = SVGA3D_DECLUSAGE_PSIZE;
      break;
   case TGSI_SEMANTIC_GENERIC:   
      *idx = svga_remap_generic_index(emit->key.generic_remap_table,
                                      semantic.Index);
      *usage = SVGA3D_DECLUSAGE_TEXCOORD;
      break;
   case TGSI_SEMANTIC_NORMAL:    
      *idx = semantic.Index;
      *usage = SVGA3D_DECLUSAGE_NORMAL;
      break;
   case TGSI_SEMANTIC_CLIPDIST:
   case TGSI_SEMANTIC_CLIPVERTEX:
      /* XXX at this time we don't support clip distance or clip vertices */
      debug_warn_once("unsupported clip distance/vertex attribute\n");
      *usage = SVGA3D_DECLUSAGE_TEXCOORD;
      *idx = 0;
      return TRUE;
   default:
      assert(0);
      *usage = SVGA3D_DECLUSAGE_TEXCOORD;
      *idx = 0;
      return FALSE;
   }

   return TRUE;
}


/**
 * Emit a PS input (or VS depth/fog output) register declaration.
 * For example, if usage = SVGA3D_DECLUSAGE_TEXCOORD, reg.num = 1, and
 * index = 3, we'll emit "dcl_texcoord3 v1".
 */
static boolean
emit_decl(struct svga_shader_emitter *emit,
          SVGA3dShaderDestToken reg,
          unsigned usage, 
          unsigned index)
{
   SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;

   /* check values against bitfield sizes */
   assert(index < 16);
   assert(usage <= SVGA3D_DECLUSAGE_MAX);

   opcode = inst_token( SVGA3DOP_DCL );
   dcl.values[0] = 0;
   dcl.values[1] = 0;

   dcl.dst = reg;
   dcl.usage = usage;
   dcl.index = index;
   dcl.values[0] |= 1<<31;

   return (emit_instruction(emit, opcode) &&
           svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
}


/**
 * Emit declaration for PS front/back-face input register.
 */
static boolean
emit_vface_decl(struct svga_shader_emitter *emit)
{
   if (!emit->emitted_vface) {
      SVGA3dShaderDestToken reg =
         dst_register(SVGA3DREG_MISCTYPE, SVGA3DMISCREG_FACE);

      if (!emit_decl( emit, reg, 0, 0 ))
         return FALSE;

      emit->emitted_vface = TRUE;
   }
   return TRUE;
}


/**
 * Emit PS input register to pass depth/fog coordinates.
 * Note that this always goes into texcoord[0].
 */
static boolean
ps30_input_emit_depth_fog( struct svga_shader_emitter *emit,
                           struct src_register *out )
{
   struct src_register reg;

   if (emit->emitted_depth_fog) {
      *out = emit->ps_depth_fog;
      return TRUE;
   }

   if (emit->ps30_input_count >= SVGA3D_INPUTREG_MAX)
      return FALSE;

   reg = src_register( SVGA3DREG_INPUT,
                       emit->ps30_input_count++ );

   *out = emit->ps_depth_fog = reg;

   emit->emitted_depth_fog = TRUE;

   return emit_decl( emit, dst( reg ), SVGA3D_DECLUSAGE_TEXCOORD, 0 );
}


/**
 * Process a PS input declaration.
 * We'll emit a declaration like "dcl_texcoord1 v2"
 */
static boolean
ps30_input(struct svga_shader_emitter *emit,
           struct tgsi_declaration_semantic semantic,
           unsigned idx)
{
   unsigned usage, index;
   SVGA3dShaderDestToken reg;

   if (semantic.Name == TGSI_SEMANTIC_POSITION) {

      emit->ps_true_pos = src_register( SVGA3DREG_MISCTYPE,
                                        SVGA3DMISCREG_POSITION );
      emit->ps_true_pos.base.swizzle = TRANSLATE_SWIZZLE( TGSI_SWIZZLE_X,
                                                          TGSI_SWIZZLE_Y,
                                                          TGSI_SWIZZLE_Y,
                                                          TGSI_SWIZZLE_Y );
      reg = writemask( dst(emit->ps_true_pos),
                       TGSI_WRITEMASK_XY );
      emit->ps_reads_pos = TRUE;

      if (emit->info.reads_z) {
         emit->ps_temp_pos = dst_register( SVGA3DREG_TEMP,
                                           emit->nr_hw_temp );

         emit->input_map[idx] = src_register( SVGA3DREG_TEMP,
                                              emit->nr_hw_temp );
         emit->nr_hw_temp++;

         if (!ps30_input_emit_depth_fog( emit, &emit->ps_depth_pos ))
            return FALSE;

         emit->ps_depth_pos.base.swizzle = TRANSLATE_SWIZZLE( TGSI_SWIZZLE_Z,
                                                              TGSI_SWIZZLE_Z,
                                                              TGSI_SWIZZLE_Z,
                                                              TGSI_SWIZZLE_W );
      }
      else {
         emit->input_map[idx] = emit->ps_true_pos;
      }

      return emit_decl( emit, reg, 0, 0 );
   }
   else if (emit->key.fkey.light_twoside &&
            (semantic.Name == TGSI_SEMANTIC_COLOR)) {

      if (!translate_vs_ps_semantic( emit, semantic, &usage, &index ))
         return FALSE;

      emit->internal_color_idx[emit->internal_color_count] = idx;
      emit->input_map[idx] = src_register( SVGA3DREG_INPUT, emit->ps30_input_count );
      emit->ps30_input_count++;
      emit->internal_color_count++;

      reg = dst( emit->input_map[idx] );

      if (!emit_decl( emit, reg, usage, index ))
         return FALSE;

      semantic.Name = TGSI_SEMANTIC_BCOLOR;
      if (!translate_vs_ps_semantic( emit, semantic, &usage, &index ))
         return FALSE;

      if (emit->ps30_input_count >= SVGA3D_INPUTREG_MAX)
         return FALSE;

      reg = dst_register( SVGA3DREG_INPUT, emit->ps30_input_count++ );

      if (!emit_decl( emit, reg, usage, index ))
         return FALSE;

      if (!emit_vface_decl( emit ))
         return FALSE;

      return TRUE;
   }
   else if (semantic.Name == TGSI_SEMANTIC_FACE) {
      if (!emit_vface_decl( emit ))
         return FALSE;
      emit->emit_frontface = TRUE;
      emit->internal_frontface_idx = idx;
      return TRUE;
   }
   else if (semantic.Name == TGSI_SEMANTIC_FOG) {

      assert(semantic.Index == 0);

      if (!ps30_input_emit_depth_fog( emit, &emit->input_map[idx] ))
         return FALSE;

      emit->input_map[idx].base.swizzle = TRANSLATE_SWIZZLE( TGSI_SWIZZLE_X,
                                                             TGSI_SWIZZLE_X,
                                                             TGSI_SWIZZLE_X,
                                                             TGSI_SWIZZLE_X );

      return TRUE;
   }
   else {

      if (!translate_vs_ps_semantic( emit, semantic, &usage, &index ))
         return FALSE;

      if (emit->ps30_input_count >= SVGA3D_INPUTREG_MAX)
         return FALSE;

      emit->input_map[idx] = src_register( SVGA3DREG_INPUT, emit->ps30_input_count++ );
      reg = dst( emit->input_map[idx] );

      if (!emit_decl( emit, reg, usage, index ))
         return FALSE;

      if (semantic.Name == TGSI_SEMANTIC_GENERIC &&
          emit->key.fkey.sprite_origin_lower_left &&
          index >= 1 &&
          emit->key.fkey.tex[index - 1].sprite_texgen) {
         /* This is a sprite texture coord with lower-left origin.
          * We need to invert the texture T coordinate since the SVGA3D
          * device only supports an upper-left origin.
          */
         unsigned unit = index - 1;

         emit->inverted_texcoords |= (1 << unit);

         /* save original texcoord reg */
         emit->ps_true_texcoord[unit] = emit->input_map[idx];

         /* this temp register will be the results of the MAD instruction */
         emit->ps_inverted_texcoord[unit] =
            src_register(SVGA3DREG_TEMP, emit->nr_hw_temp);
         emit->nr_hw_temp++;

         emit->ps_inverted_texcoord_input[unit] = idx;

         /* replace input_map entry with the temp register */
         emit->input_map[idx] = emit->ps_inverted_texcoord[unit];
      }

      return TRUE;
   }

}


/**
 * Process a PS output declaration.
 * Note that we don't actually emit a SVGA3DOpDcl for PS outputs.
 */
static boolean
ps30_output(struct svga_shader_emitter *emit,
            struct tgsi_declaration_semantic semantic,
            unsigned idx)
{
   switch (semantic.Name) {
   case TGSI_SEMANTIC_COLOR:
      if (emit->unit == PIPE_SHADER_FRAGMENT &&
          emit->key.fkey.white_fragments) {

         emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                               emit->nr_hw_temp++ );
         emit->temp_col[idx] = emit->output_map[idx];
         emit->true_col[idx] = dst_register( SVGA3DREG_COLOROUT, 
                                              semantic.Index );
      }
      else {
         emit->output_map[idx] = dst_register( SVGA3DREG_COLOROUT, 
                                               semantic.Index );
      }
      break;
   case TGSI_SEMANTIC_POSITION:
      emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                            emit->nr_hw_temp++ );
      emit->temp_pos = emit->output_map[idx];
      emit->true_pos = dst_register( SVGA3DREG_DEPTHOUT, 
                                     semantic.Index );
      break;
   default:
      assert(0);
      /* A wild stab in the dark. */
      emit->output_map[idx] = dst_register( SVGA3DREG_COLOROUT, 0 );
      break;
   }

   return TRUE;
}


/**
 * Declare a VS input register.
 * We still make up the input semantics the same as in 2.0
 */
static boolean
vs30_input(struct svga_shader_emitter *emit,
           struct tgsi_declaration_semantic semantic,
           unsigned idx)
{
   SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;
   unsigned usage, index;

   opcode = inst_token( SVGA3DOP_DCL );
   dcl.values[0] = 0;
   dcl.values[1] = 0;

   emit->input_map[idx] = src_register( SVGA3DREG_INPUT, idx );
   dcl.dst = dst_register( SVGA3DREG_INPUT, idx );

   assert(dcl.dst.reserved0);

   svga_generate_vdecl_semantics( idx, &usage, &index );

   dcl.usage = usage;
   dcl.index = index;
   dcl.values[0] |= 1<<31;

   return (emit_instruction(emit, opcode) &&
           svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
}


/**
 * Declare VS output for holding depth/fog.
 */
static boolean
vs30_output_emit_depth_fog(struct svga_shader_emitter *emit,
                           SVGA3dShaderDestToken *out)
{
   SVGA3dShaderDestToken reg;

   if (emit->emitted_depth_fog) {
      *out = emit->vs_depth_fog;
      return TRUE;
   }

   reg = dst_register( SVGA3DREG_OUTPUT, emit->vs30_output_count++ );

   *out = emit->vs_depth_fog = reg;

   emit->emitted_depth_fog = TRUE;

   return emit_decl( emit, reg, SVGA3D_DECLUSAGE_TEXCOORD, 0 );
}


/**
 * Declare a VS output.
 * VS3.0 outputs have proper declarations and semantic info for
 * matching against PS inputs.
 */
static boolean
vs30_output(struct svga_shader_emitter *emit,
            struct tgsi_declaration_semantic semantic,
            unsigned idx)
{
   SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;
   unsigned usage, index;

   opcode = inst_token( SVGA3DOP_DCL );
   dcl.values[0] = 0;
   dcl.values[1] = 0;

   if (!translate_vs_ps_semantic( emit, semantic, &usage, &index ))
      return FALSE;

   if (emit->vs30_output_count >= SVGA3D_OUTPUTREG_MAX)
      return FALSE;

   dcl.dst = dst_register( SVGA3DREG_OUTPUT, emit->vs30_output_count++ );
   dcl.usage = usage;
   dcl.index = index;
   dcl.values[0] |= 1<<31;

   if (semantic.Name == TGSI_SEMANTIC_POSITION) {
      assert(idx == 0);
      emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                            emit->nr_hw_temp++ );
      emit->temp_pos = emit->output_map[idx];
      emit->true_pos = dcl.dst;

      /* Grab an extra output for the depth output */
      if (!vs30_output_emit_depth_fog( emit, &emit->depth_pos ))
         return FALSE;

   }
   else if (semantic.Name == TGSI_SEMANTIC_PSIZE) {
      emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                            emit->nr_hw_temp++ );
      emit->temp_psiz = emit->output_map[idx];

      /* This has the effect of not declaring psiz (below) and not 
       * emitting the final MOV to true_psiz in the postamble.
       */
      if (!emit->key.vkey.allow_psiz)
         return TRUE;

      emit->true_psiz = dcl.dst;
   }
   else if (semantic.Name == TGSI_SEMANTIC_FOG) {
      /*
       * Fog is shared with depth.
       * So we need to decrement out_count since emit_depth_fog will increment it.
       */
      emit->vs30_output_count--;

      if (!vs30_output_emit_depth_fog( emit, &emit->output_map[idx] ))
         return FALSE;

      return TRUE;
   }
   else {
      emit->output_map[idx] = dcl.dst;
   }

   return (emit_instruction(emit, opcode) &&
           svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
}


static boolean
ps30_sampler( struct svga_shader_emitter *emit,
              struct tgsi_declaration_semantic semantic,
              unsigned idx )
{
   SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;

   opcode = inst_token( SVGA3DOP_DCL );
   dcl.values[0] = 0;
   dcl.values[1] = 0;

   dcl.dst = dst_register( SVGA3DREG_SAMPLER, idx );
   dcl.type = svga_tgsi_sampler_type( emit, idx );
   dcl.values[0] |= 1<<31;

   return (emit_instruction(emit, opcode) &&
           svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
}


boolean
svga_translate_decl_sm30( struct svga_shader_emitter *emit,
                          const struct tgsi_full_declaration *decl )
{
   unsigned first = decl->Range.First;
   unsigned last = decl->Range.Last;
   unsigned idx;

   for( idx = first; idx <= last; idx++ ) {
      boolean ok;

      switch (decl->Declaration.File) {
      case TGSI_FILE_SAMPLER:
         assert (emit->unit == PIPE_SHADER_FRAGMENT);
         ok = ps30_sampler( emit, decl->Semantic, idx );
         break;

      case TGSI_FILE_INPUT:
         if (emit->unit == PIPE_SHADER_VERTEX)
            ok = vs30_input( emit, decl->Semantic, idx );
         else
            ok = ps30_input( emit, decl->Semantic, idx );
         break;

      case TGSI_FILE_OUTPUT:
         if (emit->unit == PIPE_SHADER_VERTEX)
            ok = vs30_output( emit, decl->Semantic, idx );
         else
            ok = ps30_output( emit, decl->Semantic, idx );
         break;

      default:
         /* don't need to declare other vars */
         ok = TRUE;
      }

      if (!ok)
         return FALSE;
   }

   return TRUE;
}
