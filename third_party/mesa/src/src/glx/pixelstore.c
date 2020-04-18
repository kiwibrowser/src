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

#include "glxclient.h"
#include "indirect.h"

#if !defined(__GNUC__)
#  define __builtin_expect(x, y) x
#endif

/**
 * Send glPixelStore command to the server
 * 
 * \param gc     Current GLX context
 * \param sop    Either \c X_GLsop_PixelStoref or \c X_GLsop_PixelStorei
 * \param pname  Selector of which pixel parameter is to be set.
 * \param param  Value that \c pname is set to.
 *
 * \sa __indirect_glPixelStorei,  __indirect_glPixelStoref
 */
static void
send_PixelStore(struct glx_context * gc, unsigned sop, GLenum pname,
                const void *param)
{
   Display *const dpy = gc->currentDpy;
   const GLuint cmdlen = 8;
   if (__builtin_expect(dpy != NULL, 1)) {
      GLubyte const *pc = __glXSetupSingleRequest(gc, sop, cmdlen);
      (void) memcpy((void *) (pc + 0), (void *) (&pname), 4);
      (void) memcpy((void *) (pc + 4), param, 4);
      UnlockDisplay(dpy);
      SyncHandle();
   }
   return;
}

/*
** Specify parameters that control the storage format of pixel arrays.
*/
void
__indirect_glPixelStoref(GLenum pname, GLfloat param)
{
   struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = gc->client_state_private;
   Display *dpy = gc->currentDpy;
   GLuint a;

   if (!dpy)
      return;

   switch (pname) {
   case GL_PACK_ROW_LENGTH:
      a = (GLuint) (param + 0.5);
      if (((GLint) a) < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storePack.rowLength = a;
      break;
   case GL_PACK_IMAGE_HEIGHT:
      a = (GLuint) (param + 0.5);
      if (((GLint) a) < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storePack.imageHeight = a;
      break;
   case GL_PACK_SKIP_ROWS:
      a = (GLuint) (param + 0.5);
      if (((GLint) a) < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storePack.skipRows = a;
      break;
   case GL_PACK_SKIP_PIXELS:
      a = (GLuint) (param + 0.5);
      if (((GLint) a) < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storePack.skipPixels = a;
      break;
   case GL_PACK_SKIP_IMAGES:
      a = (GLuint) (param + 0.5);
      if (((GLint) a) < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storePack.skipImages = a;
      break;
   case GL_PACK_ALIGNMENT:
      a = (GLint) (param + 0.5);
      switch (a) {
      case 1:
      case 2:
      case 4:
      case 8:
         state->storePack.alignment = a;
         break;
      default:
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      break;
   case GL_PACK_SWAP_BYTES:
      state->storePack.swapEndian = (param != 0);
      break;
   case GL_PACK_LSB_FIRST:
      state->storePack.lsbFirst = (param != 0);
      break;

   case GL_UNPACK_ROW_LENGTH:
      a = (GLuint) (param + 0.5);
      if (((GLint) a) < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storeUnpack.rowLength = a;
      break;
   case GL_UNPACK_IMAGE_HEIGHT:
      a = (GLuint) (param + 0.5);
      if (((GLint) a) < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storeUnpack.imageHeight = a;
      break;
   case GL_UNPACK_SKIP_ROWS:
      a = (GLuint) (param + 0.5);
      if (((GLint) a) < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storeUnpack.skipRows = a;
      break;
   case GL_UNPACK_SKIP_PIXELS:
      a = (GLuint) (param + 0.5);
      if (((GLint) a) < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storeUnpack.skipPixels = a;
      break;
   case GL_UNPACK_SKIP_IMAGES:
      a = (GLuint) (param + 0.5);
      if (((GLint) a) < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storeUnpack.skipImages = a;
      break;
   case GL_UNPACK_ALIGNMENT:
      a = (GLint) (param + 0.5);
      switch (a) {
      case 1:
      case 2:
      case 4:
      case 8:
         state->storeUnpack.alignment = a;
         break;
      default:
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      break;
   case GL_UNPACK_SWAP_BYTES:
      state->storeUnpack.swapEndian = (param != 0);
      break;
   case GL_UNPACK_LSB_FIRST:
      state->storeUnpack.lsbFirst = (param != 0);
      break;

      /* Group all of the pixel store modes that need to be sent to the
       * server here.  Care must be used to only send modes to the server that
       * won't affect the size of the data sent to or received from the
       * server.  GL_PACK_INVERT_MESA is safe in this respect, but other,
       * future modes may not be.
       */
   case GL_PACK_INVERT_MESA:
      send_PixelStore(gc, X_GLsop_PixelStoref, pname, &param);
      break;

   default:
      __glXSetError(gc, GL_INVALID_ENUM);
      break;
   }
}

void
__indirect_glPixelStorei(GLenum pname, GLint param)
{
   struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = gc->client_state_private;
   Display *dpy = gc->currentDpy;

   if (!dpy)
      return;

   switch (pname) {
   case GL_PACK_ROW_LENGTH:
      if (param < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storePack.rowLength = param;
      break;
   case GL_PACK_IMAGE_HEIGHT:
      if (param < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storePack.imageHeight = param;
      break;
   case GL_PACK_SKIP_ROWS:
      if (param < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storePack.skipRows = param;
      break;
   case GL_PACK_SKIP_PIXELS:
      if (param < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storePack.skipPixels = param;
      break;
   case GL_PACK_SKIP_IMAGES:
      if (param < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storePack.skipImages = param;
      break;
   case GL_PACK_ALIGNMENT:
      switch (param) {
      case 1:
      case 2:
      case 4:
      case 8:
         state->storePack.alignment = param;
         break;
      default:
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      break;
   case GL_PACK_SWAP_BYTES:
      state->storePack.swapEndian = (param != 0);
      break;
   case GL_PACK_LSB_FIRST:
      state->storePack.lsbFirst = (param != 0);
      break;

   case GL_UNPACK_ROW_LENGTH:
      if (param < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storeUnpack.rowLength = param;
      break;
   case GL_UNPACK_IMAGE_HEIGHT:
      if (param < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storeUnpack.imageHeight = param;
      break;
   case GL_UNPACK_SKIP_ROWS:
      if (param < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storeUnpack.skipRows = param;
      break;
   case GL_UNPACK_SKIP_PIXELS:
      if (param < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storeUnpack.skipPixels = param;
      break;
   case GL_UNPACK_SKIP_IMAGES:
      if (param < 0) {
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      state->storeUnpack.skipImages = param;
      break;
   case GL_UNPACK_ALIGNMENT:
      switch (param) {
      case 1:
      case 2:
      case 4:
      case 8:
         state->storeUnpack.alignment = param;
         break;
      default:
         __glXSetError(gc, GL_INVALID_VALUE);
         return;
      }
      break;
   case GL_UNPACK_SWAP_BYTES:
      state->storeUnpack.swapEndian = (param != 0);
      break;
   case GL_UNPACK_LSB_FIRST:
      state->storeUnpack.lsbFirst = (param != 0);
      break;

      /* Group all of the pixel store modes that need to be sent to the
       * server here.  Care must be used to only send modes to the server that
       * won't affect the size of the data sent to or received from the
       * server.  GL_PACK_INVERT_MESA is safe in this respect, but other,
       * future modes may not be.
       */
   case GL_PACK_INVERT_MESA:
      send_PixelStore(gc, X_GLsop_PixelStorei, pname, &param);
      break;

   default:
      __glXSetError(gc, GL_INVALID_ENUM);
      break;
   }
}
