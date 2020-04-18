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

#include "util/u_debug.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "draw/draw_context.h"

#include "svga_context.h"
#include "svga_screen.h"
#include "svga_state.h"
#include "svga_draw.h"
#include "svga_cmd.h"
#include "svga_hw_reg.h"

/* This is just enough to decide whether we need to use the draw
 * module (swtnl) or not.
 */
static const struct svga_tracked_state *need_swtnl_state[] =
{
   &svga_update_need_swvfetch,
   &svga_update_need_pipeline,
   &svga_update_need_swtnl,
   NULL
};


/* Atoms to update hardware state prior to emitting a clear or draw
 * packet.
 */
static const struct svga_tracked_state *hw_clear_state[] =
{
   &svga_hw_scissor,
   &svga_hw_viewport,
   &svga_hw_framebuffer,
   NULL
};


/* Atoms to update hardware state prior to emitting a draw packet.
 */
static const struct svga_tracked_state *hw_draw_state[] =
{
   &svga_hw_fs,
   &svga_hw_vs,
   &svga_hw_rss,
   &svga_hw_tss,
   &svga_hw_tss_binding,
   &svga_hw_clip_planes,
   &svga_hw_vdecl,
   &svga_hw_fs_constants,
   &svga_hw_vs_constants,
   NULL
};


static const struct svga_tracked_state *swtnl_draw_state[] =
{
   &svga_update_swtnl_draw,
   &svga_update_swtnl_vdecl,
   NULL
};

/* Flattens the graph of state dependencies.  Could swap the positions
 * of hw_clear_state and need_swtnl_state without breaking anything.
 */
static const struct svga_tracked_state **state_levels[] = 
{
   need_swtnl_state,
   hw_clear_state,
   hw_draw_state,
   swtnl_draw_state
};



static unsigned check_state( unsigned a,
                             unsigned b )
{
   return (a & b);
}

static void accumulate_state( unsigned *a,
			      unsigned b )
{
   *a |= b;
}


static void xor_states( unsigned *result,
                        unsigned a,
                        unsigned b )
{
   *result = a ^ b;
}



static enum pipe_error
update_state(struct svga_context *svga,
             const struct svga_tracked_state *atoms[],
             unsigned *state)
{
   boolean debug = TRUE;
   enum pipe_error ret = PIPE_OK;
   unsigned i;

   ret = svga_hwtnl_flush( svga->hwtnl );
   if (ret != PIPE_OK)
      return ret;

   if (debug) {
      /* Debug version which enforces various sanity checks on the
       * state flags which are generated and checked to help ensure
       * state atoms are ordered correctly in the list.
       */
      unsigned examined, prev;      

      examined = 0;
      prev = *state;

      for (i = 0; atoms[i] != NULL; i++) {	 
	 unsigned generated;

	 assert(atoms[i]->dirty); 
	 assert(atoms[i]->update);

	 if (check_state(*state, atoms[i]->dirty)) {
	    if (0)
               debug_printf("update: %s\n", atoms[i]->name);
	    ret = atoms[i]->update( svga, *state );
            if (ret != PIPE_OK)
               return ret;
	 }

	 /* generated = (prev ^ state)
	  * if (examined & generated)
	  *     fail;
	  */
	 xor_states(&generated, prev, *state);
	 if (check_state(examined, generated)) {
	    debug_printf("state atom %s generated state already examined\n", 
                         atoms[i]->name);
	    assert(0);
	 }
			 
	 prev = *state;
	 accumulate_state(&examined, atoms[i]->dirty);
      }
   }
   else {
      for (i = 0; atoms[i] != NULL; i++) {	 
	 if (check_state(*state, atoms[i]->dirty)) {
	    ret = atoms[i]->update( svga, *state );
            if (ret != PIPE_OK)
               return ret;
         }
      }
   }

   return PIPE_OK;
}



enum pipe_error
svga_update_state(struct svga_context *svga, unsigned max_level)
{
   struct svga_screen *screen = svga_screen(svga->pipe.screen);
   enum pipe_error ret = PIPE_OK;
   int i;

   /* Check for updates to bound textures.  This can't be done in an
    * atom as there is no flag which could provoke this test, and we
    * cannot create one.
    */
   if (svga->state.texture_timestamp != screen->texture_timestamp) {
      svga->state.texture_timestamp = screen->texture_timestamp;
      svga->dirty |= SVGA_NEW_TEXTURE;
   }

   for (i = 0; i <= max_level; i++) {
      svga->dirty |= svga->state.dirty[i];

      if (svga->dirty) {
         ret = update_state( svga, 
                             state_levels[i], 
                             &svga->dirty );
         if (ret != PIPE_OK)
            return ret;

         svga->state.dirty[i] = 0;
      }
   }
   
   for (; i < SVGA_STATE_MAX; i++) 
      svga->state.dirty[i] |= svga->dirty;

   svga->dirty = 0;
   return PIPE_OK;
}




void svga_update_state_retry( struct svga_context *svga,
                              unsigned max_level )
{
   enum pipe_error ret;

   ret = svga_update_state( svga, max_level );

   if (ret == PIPE_ERROR_OUT_OF_MEMORY) {
      svga_context_flush(svga, NULL);
      ret = svga_update_state( svga, max_level );
   }

   assert( ret == PIPE_OK );
}



#define EMIT_RS(_rs, _count, _name, _value)     \
do {                                            \
   _rs[_count].state = _name;                   \
   _rs[_count].uintValue = _value;              \
   _count++;                                    \
} while (0)


/* Setup any hardware state which will be constant through the life of
 * a context.
 */
enum pipe_error svga_emit_initial_state( struct svga_context *svga )
{
   SVGA3dRenderState *rs;
   unsigned count = 0;
   const unsigned COUNT = 2;
   enum pipe_error ret;

   ret = SVGA3D_BeginSetRenderState( svga->swc, &rs, COUNT );
   if (ret != PIPE_OK)
      return ret;

   /* Always use D3D style coordinate space as this is the only one
    * which is implemented on all backends.
    */
   EMIT_RS(rs, count, SVGA3D_RS_COORDINATETYPE, SVGA3D_COORDINATE_LEFTHANDED );
   EMIT_RS(rs, count, SVGA3D_RS_FRONTWINDING, SVGA3D_FRONTWINDING_CW );
   
   assert( COUNT == count );
   SVGA_FIFOCommitAll( svga->swc );

   return PIPE_OK;
}
