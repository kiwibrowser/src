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
#include "util/u_math.h"
#include "util/u_memory.h"

#include "svga_context.h"
#include "svga_hw_reg.h"


static INLINE unsigned
svga_translate_compare_func(unsigned func)
{
   switch (func) {
   case PIPE_FUNC_NEVER:     return SVGA3D_CMP_NEVER;
   case PIPE_FUNC_LESS:      return SVGA3D_CMP_LESS;
   case PIPE_FUNC_LEQUAL:    return SVGA3D_CMP_LESSEQUAL;
   case PIPE_FUNC_GREATER:   return SVGA3D_CMP_GREATER;
   case PIPE_FUNC_GEQUAL:    return SVGA3D_CMP_GREATEREQUAL;
   case PIPE_FUNC_NOTEQUAL:  return SVGA3D_CMP_NOTEQUAL;
   case PIPE_FUNC_EQUAL:     return SVGA3D_CMP_EQUAL;
   case PIPE_FUNC_ALWAYS:    return SVGA3D_CMP_ALWAYS;
   default:
      assert(0);
      return SVGA3D_CMP_ALWAYS;
   }
}

static INLINE unsigned
svga_translate_stencil_op(unsigned op)
{
   switch (op) {
   case PIPE_STENCIL_OP_KEEP:      return SVGA3D_STENCILOP_KEEP;
   case PIPE_STENCIL_OP_ZERO:      return SVGA3D_STENCILOP_ZERO;
   case PIPE_STENCIL_OP_REPLACE:   return SVGA3D_STENCILOP_REPLACE;
   case PIPE_STENCIL_OP_INCR:      return SVGA3D_STENCILOP_INCRSAT;
   case PIPE_STENCIL_OP_DECR:      return SVGA3D_STENCILOP_DECRSAT;
   case PIPE_STENCIL_OP_INCR_WRAP: return SVGA3D_STENCILOP_INCR;
   case PIPE_STENCIL_OP_DECR_WRAP: return SVGA3D_STENCILOP_DECR;
   case PIPE_STENCIL_OP_INVERT:    return SVGA3D_STENCILOP_INVERT;
   default:
      assert(0);
      return SVGA3D_STENCILOP_KEEP;
   }
}


static void *
svga_create_depth_stencil_state(struct pipe_context *pipe,
				const struct pipe_depth_stencil_alpha_state *templ)
{
   struct svga_depth_stencil_state *ds = CALLOC_STRUCT( svga_depth_stencil_state );

   /* Don't try to figure out CW/CCW correspondence with
    * stencil[0]/[1] at this point.  Presumably this can change as
    * back/front face are modified.
    */
   ds->stencil[0].enabled = templ->stencil[0].enabled;
   if (ds->stencil[0].enabled) {
      ds->stencil[0].func  = svga_translate_compare_func(templ->stencil[0].func);
      ds->stencil[0].fail  = svga_translate_stencil_op(templ->stencil[0].fail_op);
      ds->stencil[0].zfail = svga_translate_stencil_op(templ->stencil[0].zfail_op);
      ds->stencil[0].pass  = svga_translate_stencil_op(templ->stencil[0].zpass_op);
      
      /* SVGA3D has one ref/mask/writemask triple shared between front &
       * back face stencil.  We really need two:
       */
      ds->stencil_mask      = templ->stencil[0].valuemask & 0xff;
      ds->stencil_writemask = templ->stencil[0].writemask & 0xff;
   }


   ds->stencil[1].enabled = templ->stencil[1].enabled;
   if (templ->stencil[1].enabled) {
      ds->stencil[1].func   = svga_translate_compare_func(templ->stencil[1].func);
      ds->stencil[1].fail   = svga_translate_stencil_op(templ->stencil[1].fail_op);
      ds->stencil[1].zfail  = svga_translate_stencil_op(templ->stencil[1].zfail_op);
      ds->stencil[1].pass   = svga_translate_stencil_op(templ->stencil[1].zpass_op);

      ds->stencil_mask      = templ->stencil[1].valuemask & 0xff;
      ds->stencil_writemask = templ->stencil[1].writemask & 0xff;
   }


   ds->zenable = templ->depth.enabled;
   if (ds->zenable) {
      ds->zfunc = svga_translate_compare_func(templ->depth.func);
      ds->zwriteenable = templ->depth.writemask;
   }

   ds->alphatestenable = templ->alpha.enabled;
   if (ds->alphatestenable) {
      ds->alphafunc = svga_translate_compare_func(templ->alpha.func);
      ds->alpharef = templ->alpha.ref_value;
   }

   return ds;
}

static void svga_bind_depth_stencil_state(struct pipe_context *pipe,
                                          void *depth_stencil)
{
   struct svga_context *svga = svga_context(pipe);

   svga->curr.depth = (const struct svga_depth_stencil_state *)depth_stencil;
   svga->dirty |= SVGA_NEW_DEPTH_STENCIL;
}

static void svga_delete_depth_stencil_state(struct pipe_context *pipe,
                                            void *depth_stencil)
{
   FREE(depth_stencil);
}


static void svga_set_stencil_ref( struct pipe_context *pipe,
                                  const struct pipe_stencil_ref *stencil_ref )
{
   struct svga_context *svga = svga_context(pipe);

   svga->curr.stencil_ref = *stencil_ref;

   svga->dirty |= SVGA_NEW_STENCIL_REF;
}

static void
svga_set_sample_mask(struct pipe_context *pipe,
                     unsigned sample_mask)
{
}


void svga_init_depth_stencil_functions( struct svga_context *svga )
{
   svga->pipe.create_depth_stencil_alpha_state = svga_create_depth_stencil_state;
   svga->pipe.bind_depth_stencil_alpha_state = svga_bind_depth_stencil_state;
   svga->pipe.delete_depth_stencil_alpha_state = svga_delete_depth_stencil_state;

   svga->pipe.set_stencil_ref = svga_set_stencil_ref;
   svga->pipe.set_sample_mask = svga_set_sample_mask;
}




