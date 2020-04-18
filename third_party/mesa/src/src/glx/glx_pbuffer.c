/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file glx_pbuffer.c
 * Implementation of pbuffer related functions.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <inttypes.h>
#include "glxclient.h"
#include <X11/extensions/extutil.h>
#include <X11/extensions/Xext.h>
#include <assert.h>
#include <string.h>
#include "glxextensions.h"

#ifdef GLX_USE_APPLEGL
#include <pthread.h>
#include "apple_glx_drawable.h"
#include "glx_error.h"
#endif

#define WARN_ONCE_GLX_1_3(a, b) {		\
		static int warned=1;		\
		if(warned) {			\
			warn_GLX_1_3((a), b );	\
			warned=0;		\
		}				\
	}

/**
 * Emit a warning when clients use GLX 1.3 functions on pre-1.3 systems.
 */
static void
warn_GLX_1_3(Display * dpy, const char *function_name)
{
   struct glx_display *priv = __glXInitialize(dpy);

   if (priv->minorVersion < 3) {
      fprintf(stderr,
              "WARNING: Application calling GLX 1.3 function \"%s\" "
              "when GLX 1.3 is not supported!  This is an application bug!\n",
              function_name);
   }
}

#ifndef GLX_USE_APPLEGL
/**
 * Change a drawable's attribute.
 *
 * This function is used to implement \c glXSelectEvent and
 * \c glXSelectEventSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 */
static void
ChangeDrawableAttribute(Display * dpy, GLXDrawable drawable,
                        const CARD32 * attribs, size_t num_attribs)
{
   struct glx_display *priv = __glXInitialize(dpy);
#ifdef GLX_DIRECT_RENDERING
   __GLXDRIdrawable *pdraw;
#endif
   CARD32 *output;
   CARD8 opcode;
   int i;

   if ((dpy == NULL) || (drawable == 0)) {
      return;
   }

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return;

   LockDisplay(dpy);

   if ((priv->majorVersion > 1) || (priv->minorVersion >= 3)) {
      xGLXChangeDrawableAttributesReq *req;

      GetReqExtra(GLXChangeDrawableAttributes, 8 * num_attribs, req);
      output = (CARD32 *) (req + 1);

      req->reqType = opcode;
      req->glxCode = X_GLXChangeDrawableAttributes;
      req->drawable = drawable;
      req->numAttribs = (CARD32) num_attribs;
   }
   else {
      xGLXVendorPrivateWithReplyReq *vpreq;

      GetReqExtra(GLXVendorPrivateWithReply, 8 + (8 * num_attribs), vpreq);
      output = (CARD32 *) (vpreq + 1);

      vpreq->reqType = opcode;
      vpreq->glxCode = X_GLXVendorPrivateWithReply;
      vpreq->vendorCode = X_GLXvop_ChangeDrawableAttributesSGIX;

      output[0] = (CARD32) drawable;
      output[1] = num_attribs;
      output += 2;
   }

   (void) memcpy(output, attribs, sizeof(CARD32) * 2 * num_attribs);

   UnlockDisplay(dpy);
   SyncHandle();

#ifdef GLX_DIRECT_RENDERING
   pdraw = GetGLXDRIDrawable(dpy, drawable);

   if (!pdraw)
      return;

   for (i = 0; i < num_attribs; i++) {
      switch(attribs[i * 2]) {
      case GLX_EVENT_MASK:
	 /* Keep a local copy for masking out DRI2 proto events as needed */
	 pdraw->eventMask = attribs[i * 2 + 1];
	 break;
      }
   }
#endif

   return;
}


#ifdef GLX_DIRECT_RENDERING
static GLenum
determineTextureTarget(const int *attribs, int numAttribs)
{
   GLenum target = 0;
   int i;

   for (i = 0; i < numAttribs; i++) {
      if (attribs[2 * i] == GLX_TEXTURE_TARGET_EXT) {
         switch (attribs[2 * i + 1]) {
         case GLX_TEXTURE_2D_EXT:
            target = GL_TEXTURE_2D;
            break;
         case GLX_TEXTURE_RECTANGLE_EXT:
            target = GL_TEXTURE_RECTANGLE_ARB;
            break;
         }
      }
   }

   return target;
}

static GLenum
determineTextureFormat(const int *attribs, int numAttribs)
{
   int i;

   for (i = 0; i < numAttribs; i++) {
      if (attribs[2 * i] == GLX_TEXTURE_FORMAT_EXT)
         return attribs[2 * i + 1];
   }

   return 0;
}

static GLboolean
CreateDRIDrawable(Display *dpy, struct glx_config *config,
		  XID drawable, XID glxdrawable,
		  const int *attrib_list, size_t num_attribs)
{
   struct glx_display *const priv = __glXInitialize(dpy);
   __GLXDRIdrawable *pdraw;
   struct glx_screen *psc;

   psc = priv->screens[config->screen];
   if (psc->driScreen == NULL)
      return GL_TRUE;

   pdraw = psc->driScreen->createDrawable(psc, drawable,
					  glxdrawable, config);
   if (pdraw == NULL) {
      fprintf(stderr, "failed to create drawable\n");
      return GL_FALSE;
   }

   if (__glxHashInsert(priv->drawHash, glxdrawable, pdraw)) {
      (*pdraw->destroyDrawable) (pdraw);
      return GL_FALSE;
   }

   pdraw->textureTarget = determineTextureTarget(attrib_list, num_attribs);
   pdraw->textureFormat = determineTextureFormat(attrib_list, num_attribs);

   return GL_TRUE;
}

static void
DestroyDRIDrawable(Display *dpy, GLXDrawable drawable, int destroy_xdrawable)
{
   struct glx_display *const priv = __glXInitialize(dpy);
   __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
   XID xid;

   if (pdraw != NULL) {
      xid = pdraw->xDrawable;
      (*pdraw->destroyDrawable) (pdraw);
      __glxHashDelete(priv->drawHash, drawable);
      if (destroy_xdrawable)
         XFreePixmap(priv->dpy, xid);
   }
}

#else

static GLboolean
CreateDRIDrawable(Display *dpy, const struct glx_config * fbconfig,
		  XID drawable, XID glxdrawable,
		  const int *attrib_list, size_t num_attribs)
{
    return GL_FALSE;
}

static void
DestroyDRIDrawable(Display *dpy, GLXDrawable drawable, int destroy_xdrawable)
{
}

#endif

/**
 * Get a drawable's attribute.
 *
 * This function is used to implement \c glXGetSelectedEvent and
 * \c glXGetSelectedEventSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 *
 * \todo
 * The number of attributes returned is likely to be small, probably less than
 * 10.  Given that, this routine should try to use an array on the stack to
 * capture the reply rather than always calling Xmalloc.
 */
static int
GetDrawableAttribute(Display * dpy, GLXDrawable drawable,
                     int attribute, unsigned int *value)
{
   struct glx_display *priv;
   xGLXGetDrawableAttributesReply reply;
   CARD32 *data;
   CARD8 opcode;
   unsigned int length;
   unsigned int i;
   unsigned int num_attributes;
   GLboolean use_glx_1_3;

   if ((dpy == NULL) || (drawable == 0)) {
      return 0;
   }

   priv = __glXInitialize(dpy);
   use_glx_1_3 = ((priv->majorVersion > 1) || (priv->minorVersion >= 3));

   *value = 0;


   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return 0;

   LockDisplay(dpy);

   if (use_glx_1_3) {
      xGLXGetDrawableAttributesReq *req;

      GetReq(GLXGetDrawableAttributes, req);
      req->reqType = opcode;
      req->glxCode = X_GLXGetDrawableAttributes;
      req->drawable = drawable;
   }
   else {
      xGLXVendorPrivateWithReplyReq *vpreq;

      GetReqExtra(GLXVendorPrivateWithReply, 4, vpreq);
      data = (CARD32 *) (vpreq + 1);
      data[0] = (CARD32) drawable;

      vpreq->reqType = opcode;
      vpreq->glxCode = X_GLXVendorPrivateWithReply;
      vpreq->vendorCode = X_GLXvop_GetDrawableAttributesSGIX;
   }

   _XReply(dpy, (xReply *) & reply, 0, False);

   if (reply.type == X_Error) {
      UnlockDisplay(dpy);
      SyncHandle();
      return 0;
   }

   length = reply.length;
   if (length) {
      num_attributes = (use_glx_1_3) ? reply.numAttribs : length / 2;
      data = (CARD32 *) Xmalloc(length * sizeof(CARD32));
      if (data == NULL) {
         /* Throw data on the floor */
         _XEatData(dpy, length);
      }
      else {
         _XRead(dpy, (char *) data, length * sizeof(CARD32));

         /* Search the set of returned attributes for the attribute requested by
          * the caller.
          */
         for (i = 0; i < num_attributes; i++) {
            if (data[i * 2] == attribute) {
               *value = data[(i * 2) + 1];
               break;
            }
         }

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
         {
            __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);

            if (pdraw != NULL && !pdraw->textureTarget)
               pdraw->textureTarget =
                  determineTextureTarget((const int *) data, num_attributes);
            if (pdraw != NULL && !pdraw->textureFormat)
               pdraw->textureFormat =
                  determineTextureFormat((const int *) data, num_attributes);
         }
#endif

         Xfree(data);
      }
   }

   UnlockDisplay(dpy);
   SyncHandle();

   return 0;
}

static void
protocolDestroyDrawable(Display *dpy, GLXDrawable drawable, CARD32 glxCode)
{
   xGLXDestroyPbufferReq *req;
   CARD8 opcode;

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return;

   LockDisplay(dpy);

   GetReq(GLXDestroyPbuffer, req);
   req->reqType = opcode;
   req->glxCode = glxCode;
   req->pbuffer = (GLXPbuffer) drawable;

   UnlockDisplay(dpy);
   SyncHandle();
}

/**
 * Create a non-pbuffer GLX drawable.
 */
static GLXDrawable
CreateDrawable(Display *dpy, struct glx_config *config,
               Drawable drawable, const int *attrib_list, CARD8 glxCode)
{
   xGLXCreateWindowReq *req;
   struct glx_drawable *glxDraw;
   CARD32 *data;
   unsigned int i;
   CARD8 opcode;
   GLXDrawable xid;

   i = 0;
   if (attrib_list) {
      while (attrib_list[i * 2] != None)
         i++;
   }

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return None;

   glxDraw = Xmalloc(sizeof(*glxDraw));
   if (!glxDraw)
      return None;

   LockDisplay(dpy);
   GetReqExtra(GLXCreateWindow, 8 * i, req);
   data = (CARD32 *) (req + 1);

   req->reqType = opcode;
   req->glxCode = glxCode;
   req->screen = config->screen;
   req->fbconfig = config->fbconfigID;
   req->window = drawable;
   req->glxwindow = xid = XAllocID(dpy);
   req->numAttribs = i;

   if (attrib_list)
      memcpy(data, attrib_list, 8 * i);

   UnlockDisplay(dpy);
   SyncHandle();

   if (InitGLXDrawable(dpy, glxDraw, drawable, xid)) {
      free(glxDraw);
      return None;
   }

   if (!CreateDRIDrawable(dpy, config, drawable, xid, attrib_list, i)) {
      if (glxCode == X_GLXCreatePixmap)
         glxCode = X_GLXDestroyPixmap;
      else
         glxCode = X_GLXDestroyWindow;
      protocolDestroyDrawable(dpy, xid, glxCode);
      xid = None;
   }

   return xid;
}


/**
 * Destroy a non-pbuffer GLX drawable.
 */
static void
DestroyDrawable(Display * dpy, GLXDrawable drawable, CARD32 glxCode)
{
   if ((dpy == NULL) || (drawable == 0)) {
      return;
   }

   protocolDestroyDrawable(dpy, drawable, glxCode);

   DestroyGLXDrawable(dpy, drawable);
   DestroyDRIDrawable(dpy, drawable, GL_FALSE);

   return;
}


/**
 * Create a pbuffer.
 *
 * This function is used to implement \c glXCreatePbuffer and
 * \c glXCreateGLXPbufferSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 */
static GLXDrawable
CreatePbuffer(Display * dpy, struct glx_config *config,
              unsigned int width, unsigned int height,
              const int *attrib_list, GLboolean size_in_attribs)
{
   struct glx_display *priv = __glXInitialize(dpy);
   GLXDrawable id = 0;
   CARD32 *data;
   CARD8 opcode;
   unsigned int i;
   Pixmap pixmap;
   GLboolean glx_1_3 = GL_FALSE;

   i = 0;
   if (attrib_list) {
      while (attrib_list[i * 2])
         i++;
   }

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return None;

   LockDisplay(dpy);
   id = XAllocID(dpy);

   if ((priv->majorVersion > 1) || (priv->minorVersion >= 3)) {
      xGLXCreatePbufferReq *req;
      unsigned int extra = (size_in_attribs) ? 0 : 2;

      glx_1_3 = GL_TRUE;

      GetReqExtra(GLXCreatePbuffer, (8 * (i + extra)), req);
      data = (CARD32 *) (req + 1);

      req->reqType = opcode;
      req->glxCode = X_GLXCreatePbuffer;
      req->screen = config->screen;
      req->fbconfig = config->fbconfigID;
      req->pbuffer = id;
      req->numAttribs = i + extra;

      if (!size_in_attribs) {
         data[(2 * i) + 0] = GLX_PBUFFER_WIDTH;
         data[(2 * i) + 1] = width;
         data[(2 * i) + 2] = GLX_PBUFFER_HEIGHT;
         data[(2 * i) + 3] = height;
         data += 4;
      }
   }
   else {
      xGLXVendorPrivateReq *vpreq;

      GetReqExtra(GLXVendorPrivate, 20 + (8 * i), vpreq);
      data = (CARD32 *) (vpreq + 1);

      vpreq->reqType = opcode;
      vpreq->glxCode = X_GLXVendorPrivate;
      vpreq->vendorCode = X_GLXvop_CreateGLXPbufferSGIX;

      data[0] = config->screen;
      data[1] = config->fbconfigID;
      data[2] = id;
      data[3] = width;
      data[4] = height;
      data += 5;
   }

   (void) memcpy(data, attrib_list, sizeof(CARD32) * 2 * i);

   UnlockDisplay(dpy);
   SyncHandle();

   pixmap = XCreatePixmap(dpy, RootWindow(dpy, config->screen),
			  width, height, config->rgbBits);

   if (!CreateDRIDrawable(dpy, config, pixmap, id, attrib_list, i)) {
      CARD32 o = glx_1_3 ? X_GLXDestroyPbuffer : X_GLXvop_DestroyGLXPbufferSGIX;
      XFreePixmap(dpy, pixmap);
      protocolDestroyDrawable(dpy, id, o);
      id = None;
   }

   return id;
}

/**
 * Destroy a pbuffer.
 *
 * This function is used to implement \c glXDestroyPbuffer and
 * \c glXDestroyGLXPbufferSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 */
static void
DestroyPbuffer(Display * dpy, GLXDrawable drawable)
{
   struct glx_display *priv = __glXInitialize(dpy);
   CARD8 opcode;

   if ((dpy == NULL) || (drawable == 0)) {
      return;
   }

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return;

   LockDisplay(dpy);

   if ((priv->majorVersion > 1) || (priv->minorVersion >= 3)) {
      xGLXDestroyPbufferReq *req;

      GetReq(GLXDestroyPbuffer, req);
      req->reqType = opcode;
      req->glxCode = X_GLXDestroyPbuffer;
      req->pbuffer = (GLXPbuffer) drawable;
   }
   else {
      xGLXVendorPrivateWithReplyReq *vpreq;
      CARD32 *data;

      GetReqExtra(GLXVendorPrivateWithReply, 4, vpreq);
      data = (CARD32 *) (vpreq + 1);

      data[0] = (CARD32) drawable;

      vpreq->reqType = opcode;
      vpreq->glxCode = X_GLXVendorPrivateWithReply;
      vpreq->vendorCode = X_GLXvop_DestroyGLXPbufferSGIX;
   }

   UnlockDisplay(dpy);
   SyncHandle();

   DestroyDRIDrawable(dpy, drawable, GL_TRUE);

   return;
}

/**
 * Create a new pbuffer.
 */
_X_EXPORT GLXPbufferSGIX
glXCreateGLXPbufferSGIX(Display * dpy, GLXFBConfigSGIX config,
                        unsigned int width, unsigned int height,
                        int *attrib_list)
{
   return (GLXPbufferSGIX) CreatePbuffer(dpy, (struct glx_config *) config,
                                         width, height,
                                         attrib_list, GL_FALSE);
}

#endif /* GLX_USE_APPLEGL */

/**
 * Create a new pbuffer.
 */
_X_EXPORT GLXPbuffer
glXCreatePbuffer(Display * dpy, GLXFBConfig config, const int *attrib_list)
{
   int i, width, height;
#ifdef GLX_USE_APPLEGL
   GLXPbuffer result;
   int errorcode;
#endif

   width = 0;
   height = 0;

   WARN_ONCE_GLX_1_3(dpy, __func__);

#ifdef GLX_USE_APPLEGL
   for (i = 0; attrib_list[i]; ++i) {
      switch (attrib_list[i]) {
      case GLX_PBUFFER_WIDTH:
         width = attrib_list[i + 1];
         ++i;
         break;

      case GLX_PBUFFER_HEIGHT:
         height = attrib_list[i + 1];
         ++i;
         break;

      case GLX_LARGEST_PBUFFER:
         /* This is a hint we should probably handle, but how? */
         ++i;
         break;

      case GLX_PRESERVED_CONTENTS:
         /* The contents are always preserved with AppleSGLX with CGL. */
         ++i;
         break;

      default:
         return None;
      }
   }

   if (apple_glx_pbuffer_create(dpy, config, width, height, &errorcode,
                                &result)) {
      /* 
       * apple_glx_pbuffer_create only sets the errorcode to core X11
       * errors. 
       */
      __glXSendError(dpy, errorcode, 0, X_GLXCreatePbuffer, true);

      return None;
   }

   return result;
#else
   for (i = 0; attrib_list[i * 2]; i++) {
      switch (attrib_list[i * 2]) {
      case GLX_PBUFFER_WIDTH:
         width = attrib_list[i * 2 + 1];
         break;
      case GLX_PBUFFER_HEIGHT:
         height = attrib_list[i * 2 + 1];
         break;
      }
   }

   return (GLXPbuffer) CreatePbuffer(dpy, (struct glx_config *) config,
                                     width, height, attrib_list, GL_TRUE);
#endif
}


/**
 * Destroy an existing pbuffer.
 */
_X_EXPORT void
glXDestroyPbuffer(Display * dpy, GLXPbuffer pbuf)
{
#ifdef GLX_USE_APPLEGL
   if (apple_glx_pbuffer_destroy(dpy, pbuf)) {
      __glXSendError(dpy, GLXBadPbuffer, pbuf, X_GLXDestroyPbuffer, false);
   }
#else
   DestroyPbuffer(dpy, pbuf);
#endif
}


/**
 * Query an attribute of a drawable.
 */
_X_EXPORT void
glXQueryDrawable(Display * dpy, GLXDrawable drawable,
                 int attribute, unsigned int *value)
{
   WARN_ONCE_GLX_1_3(dpy, __func__);
#ifdef GLX_USE_APPLEGL
   Window root;
   int x, y;
   unsigned int width, height, bd, depth;

   if (apple_glx_pixmap_query(drawable, attribute, value))
      return;                   /*done */

   if (apple_glx_pbuffer_query(drawable, attribute, value))
      return;                   /*done */

   /*
    * The OpenGL spec states that we should report GLXBadDrawable if
    * the drawable is invalid, however doing so would require that we
    * use XSetErrorHandler(), which is known to not be thread safe.
    * If we use a round-trip call to validate the drawable, there could
    * be a race, so instead we just opt in favor of letting the
    * XGetGeometry request fail with a GetGeometry request X error 
    * rather than GLXBadDrawable, in what is hoped to be a rare
    * case of an invalid drawable.  In practice most and possibly all
    * X11 apps using GLX shouldn't notice a difference.
    */
   if (XGetGeometry
       (dpy, drawable, &root, &x, &y, &width, &height, &bd, &depth)) {
      switch (attribute) {
      case GLX_WIDTH:
         *value = width;
         break;

      case GLX_HEIGHT:
         *value = height;
         break;
      }
   }
#else
   GetDrawableAttribute(dpy, drawable, attribute, value);
#endif
}


#ifndef GLX_USE_APPLEGL
/**
 * Query an attribute of a pbuffer.
 */
_X_EXPORT int
glXQueryGLXPbufferSGIX(Display * dpy, GLXPbufferSGIX drawable,
                       int attribute, unsigned int *value)
{
   return GetDrawableAttribute(dpy, drawable, attribute, value);
}
#endif

/**
 * Select the event mask for a drawable.
 */
_X_EXPORT void
glXSelectEvent(Display * dpy, GLXDrawable drawable, unsigned long mask)
{
#ifdef GLX_USE_APPLEGL
   XWindowAttributes xwattr;

   if (apple_glx_pbuffer_set_event_mask(drawable, mask))
      return;                   /*done */

   /* 
    * The spec allows a window, but currently there are no valid
    * events for a window, so do nothing.
    */
   if (XGetWindowAttributes(dpy, drawable, &xwattr))
      return;                   /*done */
   /* The drawable seems to be invalid.  Report an error. */

   __glXSendError(dpy, GLXBadDrawable, drawable,
                  X_GLXChangeDrawableAttributes, false);
#else
   CARD32 attribs[2];

   attribs[0] = (CARD32) GLX_EVENT_MASK;
   attribs[1] = (CARD32) mask;

   ChangeDrawableAttribute(dpy, drawable, attribs, 1);
#endif
}


/**
 * Get the selected event mask for a drawable.
 */
_X_EXPORT void
glXGetSelectedEvent(Display * dpy, GLXDrawable drawable, unsigned long *mask)
{
#ifdef GLX_USE_APPLEGL
   XWindowAttributes xwattr;

   if (apple_glx_pbuffer_get_event_mask(drawable, mask))
      return;                   /*done */

   /* 
    * The spec allows a window, but currently there are no valid
    * events for a window, so do nothing, but set the mask to 0.
    */
   if (XGetWindowAttributes(dpy, drawable, &xwattr)) {
      /* The window is valid, so set the mask to 0. */
      *mask = 0;
      return;                   /*done */
   }
   /* The drawable seems to be invalid.  Report an error. */

   __glXSendError(dpy, GLXBadDrawable, drawable, X_GLXGetDrawableAttributes,
                  true);
#else
   unsigned int value;


   /* The non-sense with value is required because on LP64 platforms
    * sizeof(unsigned int) != sizeof(unsigned long).  On little-endian
    * we could just type-cast the pointer, but why?
    */

   GetDrawableAttribute(dpy, drawable, GLX_EVENT_MASK_SGIX, &value);
   *mask = value;
#endif
}


_X_EXPORT GLXPixmap
glXCreatePixmap(Display * dpy, GLXFBConfig config, Pixmap pixmap,
                const int *attrib_list)
{
   WARN_ONCE_GLX_1_3(dpy, __func__);

#ifdef GLX_USE_APPLEGL
   const struct glx_config *modes = (const struct glx_config *) config;

   if (apple_glx_pixmap_create(dpy, modes->screen, pixmap, modes))
      return None;

   return pixmap;
#else
   return CreateDrawable(dpy, (struct glx_config *) config,
                         (Drawable) pixmap, attrib_list, X_GLXCreatePixmap);
#endif
}


_X_EXPORT GLXWindow
glXCreateWindow(Display * dpy, GLXFBConfig config, Window win,
                const int *attrib_list)
{
   WARN_ONCE_GLX_1_3(dpy, __func__);
#ifdef GLX_USE_APPLEGL
   XWindowAttributes xwattr;
   XVisualInfo *visinfo;

   (void) attrib_list;          /*unused according to GLX 1.4 */

   XGetWindowAttributes(dpy, win, &xwattr);

   visinfo = glXGetVisualFromFBConfig(dpy, config);

   if (NULL == visinfo) {
      __glXSendError(dpy, GLXBadFBConfig, 0, X_GLXCreateWindow, false);
      return None;
   }

   if (visinfo->visualid != XVisualIDFromVisual(xwattr.visual)) {
      __glXSendError(dpy, BadMatch, 0, X_GLXCreateWindow, true);
      return None;
   }

   XFree(visinfo);

   return win;
#else
   return CreateDrawable(dpy, (struct glx_config *) config,
                         (Drawable) win, attrib_list, X_GLXCreateWindow);
#endif
}


_X_EXPORT void
glXDestroyPixmap(Display * dpy, GLXPixmap pixmap)
{
   WARN_ONCE_GLX_1_3(dpy, __func__);
#ifdef GLX_USE_APPLEGL
   if (apple_glx_pixmap_destroy(dpy, pixmap))
      __glXSendError(dpy, GLXBadPixmap, pixmap, X_GLXDestroyPixmap, false);
#else
   DestroyDrawable(dpy, (GLXDrawable) pixmap, X_GLXDestroyPixmap);
#endif
}


_X_EXPORT void
glXDestroyWindow(Display * dpy, GLXWindow win)
{
   WARN_ONCE_GLX_1_3(dpy, __func__);
#ifndef GLX_USE_APPLEGL
   DestroyDrawable(dpy, (GLXDrawable) win, X_GLXDestroyWindow);
#endif
}

#ifndef GLX_USE_APPLEGL
_X_EXPORT
GLX_ALIAS_VOID(glXDestroyGLXPbufferSGIX,
               (Display * dpy, GLXPbufferSGIX pbuf),
               (dpy, pbuf), glXDestroyPbuffer)

_X_EXPORT
GLX_ALIAS_VOID(glXSelectEventSGIX,
               (Display * dpy, GLXDrawable drawable,
                unsigned long mask), (dpy, drawable, mask), glXSelectEvent)

_X_EXPORT
GLX_ALIAS_VOID(glXGetSelectedEventSGIX,
               (Display * dpy, GLXDrawable drawable,
                unsigned long *mask), (dpy, drawable, mask),
               glXGetSelectedEvent)
#endif
