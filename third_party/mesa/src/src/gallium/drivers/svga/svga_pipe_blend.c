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
svga_translate_blend_factor(unsigned factor)
{
   switch (factor) {
   case PIPE_BLENDFACTOR_ZERO:            return SVGA3D_BLENDOP_ZERO;
   case PIPE_BLENDFACTOR_SRC_ALPHA:       return SVGA3D_BLENDOP_SRCALPHA;
   case PIPE_BLENDFACTOR_ONE:             return SVGA3D_BLENDOP_ONE;
   case PIPE_BLENDFACTOR_SRC_COLOR:       return SVGA3D_BLENDOP_SRCCOLOR;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:   return SVGA3D_BLENDOP_INVSRCCOLOR;
   case PIPE_BLENDFACTOR_DST_COLOR:       return SVGA3D_BLENDOP_DESTCOLOR;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:   return SVGA3D_BLENDOP_INVDESTCOLOR;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:   return SVGA3D_BLENDOP_INVSRCALPHA;
   case PIPE_BLENDFACTOR_DST_ALPHA:       return SVGA3D_BLENDOP_DESTALPHA;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:   return SVGA3D_BLENDOP_INVDESTALPHA;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: return SVGA3D_BLENDOP_SRCALPHASAT;
   case PIPE_BLENDFACTOR_CONST_COLOR:     return SVGA3D_BLENDOP_BLENDFACTOR;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR: return SVGA3D_BLENDOP_INVBLENDFACTOR;
   case PIPE_BLENDFACTOR_CONST_ALPHA:     return SVGA3D_BLENDOP_BLENDFACTOR; /* ? */
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA: return SVGA3D_BLENDOP_INVBLENDFACTOR; /* ? */
   default:
      assert(0);
      return SVGA3D_BLENDOP_ZERO;
   }
}

static INLINE unsigned
svga_translate_blend_func(unsigned mode)
{
   switch (mode) {
   case PIPE_BLEND_ADD:              return SVGA3D_BLENDEQ_ADD;
   case PIPE_BLEND_SUBTRACT:         return SVGA3D_BLENDEQ_SUBTRACT;
   case PIPE_BLEND_REVERSE_SUBTRACT: return SVGA3D_BLENDEQ_REVSUBTRACT;
   case PIPE_BLEND_MIN:              return SVGA3D_BLENDEQ_MINIMUM;
   case PIPE_BLEND_MAX:              return SVGA3D_BLENDEQ_MAXIMUM;
   default:
      assert(0);
      return SVGA3D_BLENDEQ_ADD;
   }
}


static void *
svga_create_blend_state(struct pipe_context *pipe,
                        const struct pipe_blend_state *templ)
{
   struct svga_blend_state *blend = CALLOC_STRUCT( svga_blend_state );
   unsigned i;

 
   /* Fill in the per-rendertarget blend state.  We currently only
    * have one rendertarget.
    */
   for (i = 0; i < 1; i++) {
      /* No way to set this in SVGA3D, and no way to correctly implement it on
       * top of D3D9 API.  Instead we try to simulate with various blend modes.
       */
      if (templ->logicop_enable) {
         switch (templ->logicop_func) {
         case PIPE_LOGICOP_XOR:
         case PIPE_LOGICOP_INVERT:
            blend->need_white_fragments = TRUE;
            blend->rt[i].blend_enable = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_ONE;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_ONE;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_SUBTRACT;
            break;
         case PIPE_LOGICOP_CLEAR:
            blend->rt[i].blend_enable = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_ZERO;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_ZERO;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_MINIMUM;
            break;
         case PIPE_LOGICOP_COPY:
            blend->rt[i].blend_enable = FALSE;
            break;
         case PIPE_LOGICOP_COPY_INVERTED:
            blend->rt[i].blend_enable   = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_INVSRCCOLOR;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_ZERO;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_ADD;
            break;
         case PIPE_LOGICOP_NOOP:
            blend->rt[i].blend_enable   = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_ZERO;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_DESTCOLOR;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_ADD;
            break;
         case PIPE_LOGICOP_SET:
            blend->rt[i].blend_enable = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_ONE;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_ONE;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_MAXIMUM;
            break;
         case PIPE_LOGICOP_AND:
            /* Approximate with minimum - works for the 0 & anything case: */
            blend->rt[i].blend_enable = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_SRCCOLOR;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_DESTCOLOR;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_MINIMUM;
            break;
         case PIPE_LOGICOP_AND_REVERSE:
            blend->rt[i].blend_enable = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_SRCCOLOR;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_INVDESTCOLOR;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_MINIMUM;
            break;
         case PIPE_LOGICOP_AND_INVERTED:
            blend->rt[i].blend_enable = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_INVSRCCOLOR;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_DESTCOLOR;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_MINIMUM;
            break;
         case PIPE_LOGICOP_OR:
            /* Approximate with maximum - works for the 1 | anything case: */
            blend->rt[i].blend_enable = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_SRCCOLOR;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_DESTCOLOR;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_MAXIMUM;
            break;
         case PIPE_LOGICOP_OR_REVERSE:
            blend->rt[i].blend_enable = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_SRCCOLOR;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_INVDESTCOLOR;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_MAXIMUM;
            break;
         case PIPE_LOGICOP_OR_INVERTED:
            blend->rt[i].blend_enable = TRUE;
            blend->rt[i].srcblend       = SVGA3D_BLENDOP_INVSRCCOLOR;
            blend->rt[i].dstblend       = SVGA3D_BLENDOP_DESTCOLOR;
            blend->rt[i].blendeq        = SVGA3D_BLENDEQ_MAXIMUM;
            break;
         case PIPE_LOGICOP_NAND:
         case PIPE_LOGICOP_NOR:
         case PIPE_LOGICOP_EQUIV:
            /* Fill these in with plausible values */
            blend->rt[i].blend_enable = FALSE;
            break;
         default:
            assert(0);
            break;
         }
      }
      else {
         blend->rt[i].blend_enable   = templ->rt[0].blend_enable;

         if (templ->rt[0].blend_enable) {
            blend->rt[i].srcblend       = svga_translate_blend_factor(templ->rt[0].rgb_src_factor);
            blend->rt[i].dstblend       = svga_translate_blend_factor(templ->rt[0].rgb_dst_factor);
            blend->rt[i].blendeq        = svga_translate_blend_func(templ->rt[0].rgb_func);
            blend->rt[i].srcblend_alpha = svga_translate_blend_factor(templ->rt[0].alpha_src_factor);
            blend->rt[i].dstblend_alpha = svga_translate_blend_factor(templ->rt[0].alpha_dst_factor);
            blend->rt[i].blendeq_alpha  = svga_translate_blend_func(templ->rt[0].alpha_func);

            if (blend->rt[i].srcblend_alpha != blend->rt[i].srcblend ||
                blend->rt[i].dstblend_alpha != blend->rt[i].dstblend ||
                blend->rt[i].blendeq_alpha  != blend->rt[i].blendeq)
            {
               blend->rt[i].separate_alpha_blend_enable = TRUE;
            }
         }
      }

      blend->rt[i].writemask = templ->rt[0].colormask;
   }

   return blend;
}

static void svga_bind_blend_state(struct pipe_context *pipe,
                                  void *blend)
{
   struct svga_context *svga = svga_context(pipe);

   svga->curr.blend = (struct svga_blend_state*)blend;
   svga->dirty |= SVGA_NEW_BLEND;
}


static void svga_delete_blend_state(struct pipe_context *pipe, void *blend)
{
   FREE(blend);
}

static void svga_set_blend_color( struct pipe_context *pipe,
                                  const struct pipe_blend_color *blend_color )
{
   struct svga_context *svga = svga_context(pipe);

   svga->curr.blend_color = *blend_color;

   svga->dirty |= SVGA_NEW_BLEND_COLOR;
}


void svga_init_blend_functions( struct svga_context *svga )
{
   svga->pipe.create_blend_state = svga_create_blend_state;
   svga->pipe.bind_blend_state = svga_bind_blend_state;
   svga->pipe.delete_blend_state = svga_delete_blend_state;

   svga->pipe.set_blend_color = svga_set_blend_color;
}



