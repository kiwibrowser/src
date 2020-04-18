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


#include "pipe/p_compiler.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_defines.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_scan.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_bitmask.h"

#include "svgadump/svga_shader_dump.h"

#include "svga_context.h"
#include "svga_tgsi.h"
#include "svga_tgsi_emit.h"
#include "svga_debug.h"

#include "svga_hw_reg.h"
#include "svga3d_shaderdefs.h"


/* Sinkhole used only in error conditions.
 */
static char err_buf[128];

#if 0
static void svga_destroy_shader_emitter( struct svga_shader_emitter *emit )
{
   if (emit->buf != err_buf)
      FREE(emit->buf);
}
#endif


static boolean svga_shader_expand( struct svga_shader_emitter *emit )
{
   char *new_buf;
   unsigned newsize = emit->size * 2;

   if(emit->buf != err_buf)
      new_buf = REALLOC(emit->buf, emit->size, newsize);
   else
      new_buf = NULL;

   if (new_buf == NULL) {
      emit->ptr = err_buf;
      emit->buf = err_buf;
      emit->size = sizeof(err_buf);
      return FALSE;
   }

   emit->size = newsize;
   emit->ptr = new_buf + (emit->ptr - emit->buf);
   emit->buf = new_buf;
   return TRUE;
}   

static INLINE boolean reserve(  struct svga_shader_emitter *emit,
                                unsigned nr_dwords )
{
   if (emit->ptr - emit->buf + nr_dwords * sizeof(unsigned) >= emit->size) {
      if (!svga_shader_expand( emit ))
         return FALSE;
   }

   return TRUE;
}

boolean svga_shader_emit_dword( struct svga_shader_emitter *emit,
                                unsigned dword )
{
   if (!reserve(emit, 1))
      return FALSE;

   *(unsigned *)emit->ptr = dword;
   emit->ptr += sizeof dword;
   return TRUE;
}

boolean svga_shader_emit_dwords( struct svga_shader_emitter *emit,
                                 const unsigned *dwords,
                                 unsigned nr )
{
   if (!reserve(emit, nr))
      return FALSE;

   memcpy( emit->ptr, dwords, nr * sizeof *dwords );
   emit->ptr += nr * sizeof *dwords;
   return TRUE;
}

boolean svga_shader_emit_opcode( struct svga_shader_emitter *emit,
                                 unsigned opcode )
{
   SVGA3dShaderInstToken *here;

   if (!reserve(emit, 1))
      return FALSE;

   here = (SVGA3dShaderInstToken *)emit->ptr;
   here->value = opcode;

   if (emit->insn_offset) {
      SVGA3dShaderInstToken *prev = (SVGA3dShaderInstToken *)(emit->buf + 
                                                              emit->insn_offset);
      prev->size = (here - prev) - 1;
   }
   
   emit->insn_offset = emit->ptr - emit->buf;
   emit->ptr += sizeof(unsigned);
   return TRUE;
}


static boolean svga_shader_emit_header( struct svga_shader_emitter *emit )
{
   SVGA3dShaderVersion header;

   memset( &header, 0, sizeof header );

   switch (emit->unit) {
   case PIPE_SHADER_FRAGMENT:
      header.value = SVGA3D_PS_30;
      break;
   case PIPE_SHADER_VERTEX:
      header.value = SVGA3D_VS_30;
      break;
   }
 
   return svga_shader_emit_dword( emit, header.value );
}


/**
 * Use the shader info to generate a bitmask indicating which generic
 * inputs are used by the shader.  A set bit indicates that GENERIC[i]
 * is used.
 */
unsigned
svga_get_generic_inputs_mask(const struct tgsi_shader_info *info)
{
   unsigned i, mask = 0x0;

   for (i = 0; i < info->num_inputs; i++) {
      if (info->input_semantic_name[i] == TGSI_SEMANTIC_GENERIC) {
         unsigned j = info->input_semantic_index[i];
         assert(j < sizeof(mask) * 8);
         mask |= 1 << j;
      }
   }

   return mask;
}


/**
 * Given a mask of used generic variables (as returned by the above functions)
 * fill in a table which maps those indexes to small integers.
 * This table is used by the remap_generic_index() function in
 * svga_tgsi_decl_sm30.c
 * Example: if generics_mask = binary(1010) it means that GENERIC[1] and
 * GENERIC[3] are used.  The remap_table will contain:
 *   table[1] = 0;
 *   table[3] = 1;
 * The remaining table entries will be filled in with the next unused
 * generic index (in this example, 2).
 */
void
svga_remap_generics(unsigned generics_mask,
                    int8_t remap_table[MAX_GENERIC_VARYING])
{
   /* Note texcoord[0] is reserved so start at 1 */
   unsigned count = 1, i;

   for (i = 0; i < MAX_GENERIC_VARYING; i++) {
      remap_table[i] = -1;
   }

   /* for each bit set in generic_mask */
   while (generics_mask) {
      unsigned index = ffs(generics_mask) - 1;
      remap_table[index] = count++;
      generics_mask &= ~(1 << index);
   }
}


/**
 * Use the generic remap table to map a TGSI generic varying variable
 * index to a small integer.  If the remapping table doesn't have a
 * valid value for the given index (the table entry is -1) it means
 * the fragment shader doesn't use that VS output.  Just allocate
 * the next free value in that case.  Alternately, we could cull
 * VS instructions that write to register, or replace the register
 * with a dummy temp register.
 * XXX TODO: we should do one of the later as it would save precious
 * texcoord registers.
 */
int
svga_remap_generic_index(int8_t remap_table[MAX_GENERIC_VARYING],
                         int generic_index)
{
   assert(generic_index < MAX_GENERIC_VARYING);

   if (generic_index >= MAX_GENERIC_VARYING) {
      /* just don't return a random/garbage value */
      generic_index = MAX_GENERIC_VARYING - 1;
   }

   if (remap_table[generic_index] == -1) {
      /* This is a VS output that has no matching PS input.  Find a
       * free index.
       */
      int i, max = 0;
      for (i = 0; i < MAX_GENERIC_VARYING; i++) {
         max = MAX2(max, remap_table[i]);
      }
      remap_table[generic_index] = max + 1;
   }

   return remap_table[generic_index];
}


/* Parse TGSI shader and translate to SVGA/DX9 serialized
 * representation.  
 *
 * In this function SVGA shader is emitted to an in-memory buffer that
 * can be dynamically grown.  Once we've finished and know how large
 * it is, it will be copied to a hardware buffer for upload.
 */
static struct svga_shader_result *
svga_tgsi_translate( const struct svga_shader *shader,
                     struct svga_compile_key key,
                     unsigned unit )
{
   struct svga_shader_result *result = NULL;
   struct svga_shader_emitter emit;

   memset(&emit, 0, sizeof(emit));

   emit.size = 1024;
   emit.buf = MALLOC(emit.size);
   if (emit.buf == NULL) {
      goto fail;
   }

   emit.ptr = emit.buf;
   emit.unit = unit;
   emit.key = key;

   tgsi_scan_shader( shader->tokens, &emit.info);

   emit.imm_start = emit.info.file_max[TGSI_FILE_CONSTANT] + 1;
   
   if (unit == PIPE_SHADER_FRAGMENT)
      emit.imm_start += key.fkey.num_unnormalized_coords;

   if (unit == PIPE_SHADER_VERTEX) {
      emit.imm_start += key.vkey.need_prescale ? 2 : 0;
   }

   emit.nr_hw_float_const = (emit.imm_start + emit.info.file_max[TGSI_FILE_IMMEDIATE] + 1);

   emit.nr_hw_temp = emit.info.file_max[TGSI_FILE_TEMPORARY] + 1;
   
   if (emit.nr_hw_temp >= SVGA3D_TEMPREG_MAX) {
      debug_printf("svga: too many temporary registers (%u)\n", emit.nr_hw_temp);
      goto fail;
   }

   emit.in_main_func = TRUE;

   if (!svga_shader_emit_header( &emit )) {
      debug_printf("svga: emit header failed\n");
      goto fail;
   }

   if (!svga_shader_emit_instructions( &emit, shader->tokens )) {
      debug_printf("svga: emit instructions failed\n");
      goto fail;
   }

   result = CALLOC_STRUCT(svga_shader_result);
   if (result == NULL)
      goto fail;

   result->shader = shader;
   result->tokens = (const unsigned *)emit.buf;
   result->nr_tokens = (emit.ptr - emit.buf) / sizeof(unsigned);
   memcpy(&result->key, &key, sizeof key);
   result->id = UTIL_BITMASK_INVALID_INDEX;

   if (SVGA_DEBUG & DEBUG_TGSI) 
   {
      debug_printf( "#####################################\n" );
      debug_printf( "Shader %u below\n", shader->id );
      tgsi_dump( shader->tokens, 0 );
      if (SVGA_DEBUG & DEBUG_TGSI) {
         debug_printf( "Shader %u compiled below\n", shader->id );
         svga_shader_dump( result->tokens,
                           result->nr_tokens ,
                           FALSE );
      }
      debug_printf( "#####################################\n" );
   }

   return result;

fail:
   FREE(result);
   FREE(emit.buf);
   return NULL;
}




struct svga_shader_result *
svga_translate_fragment_program( const struct svga_fragment_shader *fs,
                                 const struct svga_fs_compile_key *fkey )
{
   struct svga_compile_key key;

   memset(&key, 0, sizeof(key));

   memcpy(&key.fkey, fkey, sizeof *fkey);

   memcpy(key.generic_remap_table, fs->generic_remap_table,
          sizeof(fs->generic_remap_table));

   return svga_tgsi_translate( &fs->base, 
                               key,
                               PIPE_SHADER_FRAGMENT );
}

struct svga_shader_result *
svga_translate_vertex_program( const struct svga_vertex_shader *vs,
                               const struct svga_vs_compile_key *vkey )
{
   struct svga_compile_key key;

   memset(&key, 0, sizeof(key));

   memcpy(&key.vkey, vkey, sizeof *vkey);

   /* Note: we could alternately store the remap table in the vkey but
    * that would make it larger.  We just regenerate it here instead.
    */
   svga_remap_generics(vkey->fs_generic_inputs, key.generic_remap_table);

   return svga_tgsi_translate( &vs->base, 
                               key,
                               PIPE_SHADER_VERTEX );
}


void svga_destroy_shader_result( struct svga_shader_result *result )
{
   FREE((unsigned *)result->tokens);
   FREE(result);
}

