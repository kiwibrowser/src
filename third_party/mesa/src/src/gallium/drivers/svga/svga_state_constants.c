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

#include "util/u_inlines.h"
#include "pipe/p_defines.h"

#include "svga_screen.h"
#include "svga_context.h"
#include "svga_state.h"
#include "svga_cmd.h"
#include "svga_tgsi.h"
#include "svga_debug.h"

#include "svga_hw_reg.h"


/*
 * Don't try to send more than 4kb of successive constants.
 */
#define MAX_CONST_REG_COUNT 256  /**< number of float[4] constants */



/**
 * Convert from PIPE_SHADER_* to SVGA3D_SHADERTYPE_*
 */
static int
svga_shader_type(unsigned shader)
{
   assert(PIPE_SHADER_VERTEX + 1 == SVGA3D_SHADERTYPE_VS);
   assert(PIPE_SHADER_FRAGMENT + 1 == SVGA3D_SHADERTYPE_PS);
   assert(shader <= PIPE_SHADER_FRAGMENT);
   return shader + 1;
}


/**
 * Check and emit one shader constant register.
 * \param shader  PIPE_SHADER_FRAGMENT or PIPE_SHADER_VERTEX
 * \param i  which float[4] constant to change
 * \param value  the new float[4] value
 */
static enum pipe_error
emit_const(struct svga_context *svga, unsigned shader, unsigned i,
           const float *value)
{
   enum pipe_error ret = PIPE_OK;

   assert(shader < PIPE_SHADER_TYPES);
   assert(i < SVGA3D_CONSTREG_MAX);

   if (memcmp(svga->state.hw_draw.cb[shader][i], value,
              4 * sizeof(float)) != 0) {
      if (SVGA_DEBUG & DEBUG_CONSTS)
         debug_printf("%s %s %u: %f %f %f %f\n",
                      __FUNCTION__,
                      shader == PIPE_SHADER_VERTEX ? "VERT" : "FRAG",
                      i,
                      value[0],
                      value[1],
                      value[2],
                      value[3]);

      ret = SVGA3D_SetShaderConst( svga->swc,
                                   i,
                                   svga_shader_type(shader),
                                   SVGA3D_CONST_TYPE_FLOAT,
                                   value );
      if (ret != PIPE_OK)
         return ret;

      memcpy(svga->state.hw_draw.cb[shader][i], value, 4 * sizeof(float));
   }

   return ret;
}


/*
 * Check and emit a range of shader constant registers, trying to coalesce
 * successive shader constant updates in a single command in order to save
 * space on the command buffer.  This is a HWv8 feature.
 */
static enum pipe_error
emit_const_range(struct svga_context *svga,
                 unsigned shader,
                 unsigned offset,
                 unsigned count,
                 const float (*values)[4])
{
   unsigned i, j;
   enum pipe_error ret;

#ifdef DEBUG
   if (offset + count > SVGA3D_CONSTREG_MAX) {
      debug_printf("svga: too many constants (offset + count = %u)\n",
                   offset + count);
   }
#endif

   if (offset > SVGA3D_CONSTREG_MAX) {
      /* This isn't OK, but if we propagate an error all the way up we'll
       * just get into more trouble.
       * XXX note that offset is always zero at this time so this is moot.
       */
      return PIPE_OK;
   }

   if (offset + count > SVGA3D_CONSTREG_MAX) {
      /* Just drop the extra constants for now.
       * Ideally we should not have allowed the app to create a shader
       * that exceeds our constant buffer size but there's no way to
       * express that in gallium at this time.
       */
      count = SVGA3D_CONSTREG_MAX - offset;
   }

   i = 0;
   while (i < count) {
      if (memcmp(svga->state.hw_draw.cb[shader][offset + i],
                 values[i],
                 4 * sizeof(float)) != 0) {
         /* Found one dirty constant
          */
         if (SVGA_DEBUG & DEBUG_CONSTS)
            debug_printf("%s %s %d: %f %f %f %f\n",
                         __FUNCTION__,
                         shader == PIPE_SHADER_VERTEX ? "VERT" : "FRAG",
                         offset + i,
                         values[i][0],
                         values[i][1],
                         values[i][2],
                         values[i][3]);

         /* Look for more consecutive dirty constants.
          */
         j = i + 1;
         while (j < count &&
                j < i + MAX_CONST_REG_COUNT &&
                memcmp(svga->state.hw_draw.cb[shader][offset + j],
                       values[j],
                       4 * sizeof(float)) != 0) {

            if (SVGA_DEBUG & DEBUG_CONSTS)
               debug_printf("%s %s %d: %f %f %f %f\n",
                            __FUNCTION__,
                            shader == PIPE_SHADER_VERTEX ? "VERT" : "FRAG",
                            offset + j,
                            values[j][0],
                            values[j][1],
                            values[j][2],
                            values[j][3]);

            ++j;
         }

         assert(j >= i + 1);

         /* Send them all together.
          */
         ret = SVGA3D_SetShaderConsts(svga->swc,
                                      offset + i, j - i,
                                      svga_shader_type(shader),
                                      SVGA3D_CONST_TYPE_FLOAT,
                                      values + i);
         if (ret != PIPE_OK) {
            return ret;
         }

         /*
          * Local copy of the hardware state.
          */
         memcpy(svga->state.hw_draw.cb[shader][offset + i],
                values[i],
                (j - i) * 4 * sizeof(float));

         i = j + 1;
      } else {
         ++i;
      }
   }

   return PIPE_OK;
}


/**
 * Emit all the constants in a constant buffer for a shader stage.
 */
static enum pipe_error
emit_consts(struct svga_context *svga, unsigned shader)
{
   struct svga_screen *ss = svga_screen(svga->pipe.screen);
   struct pipe_transfer *transfer = NULL;
   unsigned count;
   const float (*data)[4] = NULL;
   unsigned i;
   enum pipe_error ret = PIPE_OK;
   const unsigned offset = 0;

   assert(shader < PIPE_SHADER_TYPES);

   if (svga->curr.cb[shader] == NULL)
      goto done;

   count = svga->curr.cb[shader]->width0 / (4 * sizeof(float));

   data = (const float (*)[4])pipe_buffer_map(&svga->pipe,
                                              svga->curr.cb[shader],
                                              PIPE_TRANSFER_READ,
					      &transfer);
   if (data == NULL) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto done;
   }

   if (ss->hw_version >= SVGA3D_HWVERSION_WS8_B1) {
      ret = emit_const_range( svga, shader, offset, count, data );
      if (ret != PIPE_OK) {
         goto done;
      }
   } else {
      for (i = 0; i < count; i++) {
         ret = emit_const( svga, shader, offset + i, data[i] );
         if (ret != PIPE_OK) {
            goto done;
         }
      }
   }

done:
   if (data)
      pipe_buffer_unmap(&svga->pipe, transfer);

   return ret;
}


static enum pipe_error
emit_fs_consts(struct svga_context *svga, unsigned dirty)
{
   const struct svga_shader_result *result = svga->state.hw_draw.fs;
   enum pipe_error ret = PIPE_OK;

   ret = emit_consts( svga, PIPE_SHADER_FRAGMENT );
   if (ret != PIPE_OK)
      return ret;

   /* The internally generated fragment shader for xor blending
    * doesn't have a 'result' struct.  It should be fixed to avoid
    * this special case, but work around it with a NULL check:
    */
   if (result) {
      const struct svga_fs_compile_key *key = &result->key.fkey;
      if (key->num_unnormalized_coords) {
         const unsigned offset =
            result->shader->info.file_max[TGSI_FILE_CONSTANT] + 1;
         unsigned i;

         for (i = 0; i < key->num_textures; i++) {
            if (key->tex[i].unnormalized) {
               struct pipe_resource *tex = svga->curr.sampler_views[i]->texture;
               float data[4];

               data[0] = 1.0f / (float) tex->width0;
               data[1] = 1.0f / (float) tex->height0;
               data[2] = 1.0f;
               data[3] = 1.0f;

               ret = emit_const(svga,
                                PIPE_SHADER_FRAGMENT,
                                key->tex[i].width_height_idx + offset,
                                data);
               if (ret != PIPE_OK) {
                  return ret;
               }
            }
         }
      }
   }

   return PIPE_OK;
}


struct svga_tracked_state svga_hw_fs_constants =
{
   "hw fs params",
   (SVGA_NEW_FS_CONST_BUFFER |
    SVGA_NEW_FS_RESULT |
    SVGA_NEW_TEXTURE_BINDING),
   emit_fs_consts
};



static enum pipe_error
emit_vs_consts(struct svga_context *svga, unsigned dirty)
{
   const struct svga_shader_result *result = svga->state.hw_draw.vs;
   const struct svga_vs_compile_key *key;
   enum pipe_error ret = PIPE_OK;
   unsigned offset;

   /* SVGA_NEW_VS_RESULT
    */
   if (result == NULL)
      return PIPE_OK;

   key = &result->key.vkey;

   /* SVGA_NEW_VS_CONST_BUFFER
    */
   ret = emit_consts( svga, PIPE_SHADER_VERTEX );
   if (ret != PIPE_OK)
      return ret;

   /* offset = number of constants in the VS const buffer */
   offset = result->shader->info.file_max[TGSI_FILE_CONSTANT] + 1;

   /* SVGA_NEW_VS_PRESCALE
    * Put the viewport pre-scale/translate values into the const buffer.
    */
   if (key->need_prescale) {
      ret = emit_const( svga, PIPE_SHADER_VERTEX, offset++,
                        svga->state.hw_clear.prescale.scale );
      if (ret != PIPE_OK)
         return ret;

      ret = emit_const( svga, PIPE_SHADER_VERTEX, offset++,
                        svga->state.hw_clear.prescale.translate );
      if (ret != PIPE_OK)
         return ret;
   }

   return PIPE_OK;
}


struct svga_tracked_state svga_hw_vs_constants =
{
   "hw vs params",
   (SVGA_NEW_PRESCALE |
    SVGA_NEW_VS_CONST_BUFFER |
    SVGA_NEW_VS_RESULT),
   emit_vs_consts
};
