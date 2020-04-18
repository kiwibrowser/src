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

#include "packsingle.h"
#include "indirect.h"
#include "glapi.h"
#include "glthread.h"
#include <GL/glxproto.h>

void
__indirect_glGetSeparableFilter(GLenum target, GLenum format, GLenum type,
                                GLvoid * row, GLvoid * column, GLvoid * span)
{
   __GLX_SINGLE_DECLARE_VARIABLES();
   const __GLXattribute *state;
   xGLXGetSeparableFilterReply reply;
   GLubyte *rowBuf, *colBuf;

   if (!dpy)
      return;
   __GLX_SINGLE_LOAD_VARIABLES();
   state = gc->client_state_private;

   /* Send request */
   __GLX_SINGLE_BEGIN(X_GLsop_GetSeparableFilter, __GLX_PAD(13));
   __GLX_SINGLE_PUT_LONG(0, target);
   __GLX_SINGLE_PUT_LONG(4, format);
   __GLX_SINGLE_PUT_LONG(8, type);
   __GLX_SINGLE_PUT_CHAR(12, state->storePack.swapEndian);
   __GLX_SINGLE_READ_XREPLY();
   compsize = reply.length << 2;

   if (compsize != 0) {
      GLint width, height;
      GLint widthsize, heightsize;

      width = reply.width;
      height = reply.height;

      widthsize = __glImageSize(width, 1, 1, format, type, 0);
      heightsize = __glImageSize(height, 1, 1, format, type, 0);

      /* Allocate a holding buffer to transform the data from */
      rowBuf = (GLubyte *) Xmalloc(widthsize);
      if (!rowBuf) {
         /* Throw data away */
         _XEatData(dpy, compsize);
         __glXSetError(gc, GL_OUT_OF_MEMORY);
         UnlockDisplay(dpy);
         SyncHandle();
         return;
      }
      else {
         __GLX_SINGLE_GET_CHAR_ARRAY(((char *) rowBuf), widthsize);
         __glEmptyImage(gc, 1, width, 1, 1, format, type, rowBuf, row);
         Xfree((char *) rowBuf);
      }
      colBuf = (GLubyte *) Xmalloc(heightsize);
      if (!colBuf) {
         /* Throw data away */
         _XEatData(dpy, compsize - __GLX_PAD(widthsize));
         __glXSetError(gc, GL_OUT_OF_MEMORY);
         UnlockDisplay(dpy);
         SyncHandle();
         return;
      }
      else {
         __GLX_SINGLE_GET_CHAR_ARRAY(((char *) colBuf), heightsize);
         __glEmptyImage(gc, 1, height, 1, 1, format, type, colBuf, column);
         Xfree((char *) colBuf);
      }
   }
   else {
      /*
       ** don't modify user's buffer.
       */
   }
   __GLX_SINGLE_END();

}


/* it is defined to gl_dispatch_stub_NNN in indirect.h */
void gl_dispatch_stub_GetSeparableFilterEXT (GLenum target, GLenum format,
                                             GLenum type, GLvoid * row,
                                             GLvoid * column, GLvoid * span)
{
   struct glx_context *const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   if (gc->isDirect) {
      const _glapi_proc *const table = (_glapi_proc *) GET_DISPATCH();
      PFNGLGETSEPARABLEFILTEREXTPROC p =
         (PFNGLGETSEPARABLEFILTEREXTPROC) table[359];

      p(target, format, type, row, column, span);
      return;
   }
   else
#endif
   {
      Display *const dpy = gc->currentDpy;
      const GLuint cmdlen = __GLX_PAD(13);

      if (dpy != NULL) {
         const __GLXattribute *const state = gc->client_state_private;
         xGLXGetSeparableFilterReply reply;
         GLubyte const *pc =
            __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply,
                                    X_GLvop_GetSeparableFilterEXT, cmdlen);
         unsigned compsize;


         (void) memcpy((void *) (pc + 0), (void *) (&target), 4);
         (void) memcpy((void *) (pc + 4), (void *) (&format), 4);
         (void) memcpy((void *) (pc + 8), (void *) (&type), 4);
         *(int8_t *) (pc + 12) = state->storePack.swapEndian;

         (void) _XReply(dpy, (xReply *) & reply, 0, False);

         compsize = reply.length << 2;

         if (compsize != 0) {
            const GLint width = reply.width;
            const GLint height = reply.height;
            const GLint widthsize =
               __glImageSize(width, 1, 1, format, type, 0);
            const GLint heightsize =
               __glImageSize(height, 1, 1, format, type, 0);
            GLubyte *const buf =
               (GLubyte *) Xmalloc((widthsize > heightsize) ? widthsize :
                                   heightsize);

            if (buf == NULL) {
               /* Throw data away */
               _XEatData(dpy, compsize);
               __glXSetError(gc, GL_OUT_OF_MEMORY);

               UnlockDisplay(dpy);
               SyncHandle();
               return;
            }
            else {
               int extra;

               extra = 4 - (widthsize & 3);
               _XRead(dpy, (char *) buf, widthsize);
               if (extra < 4) {
                  _XEatData(dpy, extra);
               }

               __glEmptyImage(gc, 1, width, 1, 1, format, type, buf, row);

               extra = 4 - (heightsize & 3);
               _XRead(dpy, (char *) buf, heightsize);
               if (extra < 4) {
                  _XEatData(dpy, extra);
               }

               __glEmptyImage(gc, 1, height, 1, 1, format, type, buf, column);

               Xfree((char *) buf);
            }
         }
      }
   }
}
