/*
 * (C) Copyright Apple Inc. 2008
 * (C) Copyright IBM Corporation 2004, 2005
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <GL/gl.h>
#include "glxclient.h"
#include <GL/glxproto.h>

CARD32
__glXReadReply(Display * dpy, size_t size, void *dest,
               GLboolean reply_is_always_array)
{
   xGLXSingleReply reply;

   (void) _XReply(dpy, (xReply *) & reply, 0, False);
   if (size != 0) {
      if ((reply.length > 0) || reply_is_always_array) {
         const GLint bytes = (reply_is_always_array)
            ? (4 * reply.length) : (reply.size * size);
         const GLint extra = 4 - (bytes & 3);

         _XRead(dpy, dest, bytes);
         if (extra < 4) {
            _XEatData(dpy, extra);
         }
      }
      else {
         (void) memcpy(dest, &(reply.pad3), size);
      }
   }

   return reply.retval;
}

void
__glXReadPixelReply(Display * dpy, struct glx_context * gc, unsigned max_dim,
                    GLint width, GLint height, GLint depth, GLenum format,
                    GLenum type, void *dest, GLboolean dimensions_in_reply)
{
   xGLXSingleReply reply;
   GLint size;

   (void) _XReply(dpy, (xReply *) & reply, 0, False);

   if (dimensions_in_reply) {
      width = reply.pad3;
      height = reply.pad4;
      depth = reply.pad5;

      if ((height == 0) || (max_dim < 2)) {
         height = 1;
      }
      if ((depth == 0) || (max_dim < 3)) {
         depth = 1;
      }
   }

   size = reply.length * 4;
   if (size != 0) {
      void *buf = Xmalloc(size);

      if (buf == NULL) {
         _XEatData(dpy, size);
         __glXSetError(gc, GL_OUT_OF_MEMORY);
      }
      else {
         const GLint extra = 4 - (size & 3);

         _XRead(dpy, buf, size);
         if (extra < 4) {
            _XEatData(dpy, extra);
         }

         __glEmptyImage(gc, 3, width, height, depth, format, type, buf, dest);
         Xfree(buf);
      }
   }
}

#if 0
GLubyte *
__glXSetupSingleRequest(struct glx_context * gc, GLint sop, GLint cmdlen)
{
   xGLXSingleReq *req;
   Display *const dpy = gc->currentDpy;

   (void) __glXFlushRenderBuffer(gc, gc->pc);
   LockDisplay(dpy);
   GetReqExtra(GLXSingle, cmdlen, req);
   req->reqType = gc->majorOpcode;
   req->contextTag = gc->currentContextTag;
   req->glxCode = sop;
   return (GLubyte *) (req) + sz_xGLXSingleReq;
}
#endif

GLubyte *
__glXSetupVendorRequest(struct glx_context * gc, GLint code, GLint vop,
                        GLint cmdlen)
{
   xGLXVendorPrivateReq *req;
   Display *const dpy = gc->currentDpy;

   (void) __glXFlushRenderBuffer(gc, gc->pc);
   LockDisplay(dpy);
   GetReqExtra(GLXVendorPrivate, cmdlen, req);
   req->reqType = gc->majorOpcode;
   req->glxCode = code;
   req->vendorCode = vop;
   req->contextTag = gc->currentContextTag;
   return (GLubyte *) (req) + sz_xGLXVendorPrivateReq;
}
