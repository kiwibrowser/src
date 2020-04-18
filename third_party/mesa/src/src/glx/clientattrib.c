/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

#include <assert.h>
#include "glxclient.h"
#include "indirect.h"
#include "indirect_vertex_array.h"

/*****************************************************************************/

#ifndef GLX_USE_APPLEGL
static void
do_enable_disable(GLenum array, GLboolean val)
{
   struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
   unsigned index = 0;

   if (array == GL_TEXTURE_COORD_ARRAY) {
      index = __glXGetActiveTextureUnit(state);
   }

   if (!__glXSetArrayEnable(state, array, index, val)) {
      __glXSetError(gc, GL_INVALID_ENUM);
   }
}

void
__indirect_glEnableClientState(GLenum array)
{
   do_enable_disable(array, GL_TRUE);
}

void
__indirect_glDisableClientState(GLenum array)
{
   do_enable_disable(array, GL_FALSE);
}

/************************************************************************/

void
__indirect_glPushClientAttrib(GLuint mask)
{
   struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
   __GLXattribute **spp = gc->attributes.stackPointer, *sp;

   if (spp < &gc->attributes.stack[__GL_CLIENT_ATTRIB_STACK_DEPTH]) {
      if (!(sp = *spp)) {
         sp = (__GLXattribute *) Xmalloc(sizeof(__GLXattribute));
         *spp = sp;
      }
      sp->mask = mask;
      gc->attributes.stackPointer = spp + 1;
      if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
         sp->storePack = state->storePack;
         sp->storeUnpack = state->storeUnpack;
      }
      if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
         __glXPushArrayState(state);
      }
   }
   else {
      __glXSetError(gc, GL_STACK_OVERFLOW);
      return;
   }
}

void
__indirect_glPopClientAttrib(void)
{
   struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
   __GLXattribute **spp = gc->attributes.stackPointer, *sp;
   GLuint mask;

   if (spp > &gc->attributes.stack[0]) {
      --spp;
      sp = *spp;
      assert(sp != 0);
      mask = sp->mask;
      gc->attributes.stackPointer = spp;

      if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
         state->storePack = sp->storePack;
         state->storeUnpack = sp->storeUnpack;
      }
      if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
         __glXPopArrayState(state);
      }

      sp->mask = 0;
   }
   else {
      __glXSetError(gc, GL_STACK_UNDERFLOW);
      return;
   }
}
#endif

void
__glFreeAttributeState(struct glx_context * gc)
{
   __GLXattribute *sp, **spp;

   for (spp = &gc->attributes.stack[0];
        spp < &gc->attributes.stack[__GL_CLIENT_ATTRIB_STACK_DEPTH]; spp++) {
      sp = *spp;
      if (sp) {
         XFree((char *) sp);
      }
      else {
         break;
      }
   }
}
