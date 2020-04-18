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

#include "svga_resource_texture.h"
#include "svga_context.h"
#include "svga_debug.h"
#include "svga_cmd.h"
#include "svga_surface.h"

#include "util/u_surface.h"

#define FILE_DEBUG_FLAG DEBUG_BLIT


/* XXX still have doubts about this... */
static void svga_surface_copy(struct pipe_context *pipe,
                              struct pipe_resource* dst_tex,
                              unsigned dst_level,
                              unsigned dstx, unsigned dsty, unsigned dstz,
                              struct pipe_resource* src_tex,
                              unsigned src_level,
                              const struct pipe_box *src_box)
 {
   struct svga_context *svga = svga_context(pipe);
   struct svga_texture *stex, *dtex;
/*   struct pipe_screen *screen = pipe->screen;
   SVGA3dCopyBox *box;
   enum pipe_error ret;
   struct pipe_surface *srcsurf, *dstsurf;*/
   unsigned dst_face, dst_z, src_face, src_z;

   /* Emit buffered drawing commands, and any back copies.
    */
   svga_surfaces_flush( svga );

   /* Fallback for buffers. */
   if (dst_tex->target == PIPE_BUFFER && src_tex->target == PIPE_BUFFER) {
      util_resource_copy_region(pipe, dst_tex, dst_level, dstx, dsty, dstz,
                                src_tex, src_level, src_box);
      return;
   }

   stex = svga_texture(src_tex);
   dtex = svga_texture(dst_tex);

#if 0
   srcsurf = screen->get_tex_surface(screen, src_tex,
                                     src_level, src_box->z, src_box->z,
                                     PIPE_BIND_SAMPLER_VIEW);

   dstsurf = screen->get_tex_surface(screen, dst_tex,
                                     dst_level, dst_box->z, dst_box->z,
                                     PIPE_BIND_RENDER_TARGET);

   SVGA_DBG(DEBUG_DMA, "blit to sid %p (%d,%d), from sid %p (%d,%d) sz %dx%d\n",
            svga_surface(dstsurf)->handle,
            dstx, dsty,
            svga_surface(srcsurf)->handle,
            src_box->x, src_box->y,
            width, height);

   ret = SVGA3D_BeginSurfaceCopy(svga->swc,
                                 srcsurf,
                                 dstsurf,
                                 &box,
                                 1);
   if(ret != PIPE_OK) {

      svga_context_flush(svga, NULL);

      ret = SVGA3D_BeginSurfaceCopy(svga->swc,
                                    srcsurf,
                                    dstsurf,
                                    &box,
                                    1);
      assert(ret == PIPE_OK);
   }

   box->x = dstx;
   box->y = dsty;
   box->z = 0;
   box->w = width;
   box->h = height;
   box->d = 1;
   box->srcx = src_box->x;
   box->srcy = src_box->y;
   box->srcz = 0;

   SVGA_FIFOCommitAll(svga->swc);

   svga_surface(dstsurf)->dirty = TRUE;
   svga_propagate_surface(pipe, dstsurf);

   pipe_surface_reference(&srcsurf, NULL);
   pipe_surface_reference(&dstsurf, NULL);

#else
   if (src_tex->target == PIPE_TEXTURE_CUBE) {
      src_face = src_box->z;
      src_z = 0;
      assert(src_box->depth == 1);
   }
   else {
      src_face = 0;
      src_z = src_box->z;
   }
   /* different src/dst type???*/
   if (dst_tex->target == PIPE_TEXTURE_CUBE) {
      dst_face = dstz;
      dst_z = 0;
      assert(src_box->depth == 1);
   }
   else {
      dst_face = 0;
      dst_z = dstz;
   }
   svga_texture_copy_handle(svga,
                            stex->handle,
                            src_box->x, src_box->y, src_z,
                            src_level, src_face,
                            dtex->handle,
                            dstx, dsty, dst_z,
                            dst_level, dst_face,
                            src_box->width, src_box->height, src_box->depth);

#endif

}


void
svga_init_blit_functions(struct svga_context *svga)
{
   svga->pipe.resource_copy_region = svga_surface_copy;
}
