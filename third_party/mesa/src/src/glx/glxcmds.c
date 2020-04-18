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

/**
 * \file glxcmds.c
 * Client-side GLX interface.
 */

#include "glxclient.h"
#include "glapi.h"
#include "glxextensions.h"
#include "indirect.h"
#include "glx_error.h"

#ifdef GLX_DIRECT_RENDERING
#ifdef GLX_USE_APPLEGL
#include "apple_glx_context.h"
#include "apple_glx.h"
#else
#include <sys/time.h>
#ifdef XF86VIDMODE
#include <X11/extensions/xf86vmode.h>
#endif
#include "xf86dri.h"
#endif
#else
#endif

#if defined(USE_XCB)
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/glx.h>
#endif

static const char __glXGLXClientVendorName[] = "Mesa Project and SGI";
static const char __glXGLXClientVersion[] = "1.4";

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)

/**
 * Get the __DRIdrawable for the drawable associated with a GLXContext
 *
 * \param dpy       The display associated with \c drawable.
 * \param drawable  GLXDrawable whose __DRIdrawable part is to be retrieved.
 * \param scrn_num  If non-NULL, the drawables screen is stored there
 * \returns  A pointer to the context's __DRIdrawable on success, or NULL if
 *           the drawable is not associated with a direct-rendering context.
 */
_X_HIDDEN __GLXDRIdrawable *
GetGLXDRIDrawable(Display * dpy, GLXDrawable drawable)
{
   struct glx_display *priv = __glXInitialize(dpy);
   __GLXDRIdrawable *pdraw;

   if (priv == NULL)
      return NULL;

   if (__glxHashLookup(priv->drawHash, drawable, (void *) &pdraw) == 0)
      return pdraw;

   return NULL;
}

#endif

_X_HIDDEN struct glx_drawable *
GetGLXDrawable(Display *dpy, GLXDrawable drawable)
{
   struct glx_display *priv = __glXInitialize(dpy);
   struct glx_drawable *glxDraw;

   if (priv == NULL)
      return NULL;

   if (__glxHashLookup(priv->glXDrawHash, drawable, (void *) &glxDraw) == 0)
      return glxDraw;

   return NULL;
}

_X_HIDDEN int
InitGLXDrawable(Display *dpy, struct glx_drawable *glxDraw, XID xDrawable,
		GLXDrawable drawable)
{
   struct glx_display *priv = __glXInitialize(dpy);

   if (!priv)
      return -1;

   glxDraw->xDrawable = xDrawable;
   glxDraw->drawable = drawable;
   glxDraw->lastEventSbc = 0;
   glxDraw->eventSbcWrap = 0;

   return __glxHashInsert(priv->glXDrawHash, drawable, glxDraw);
}

_X_HIDDEN void
DestroyGLXDrawable(Display *dpy, GLXDrawable drawable)
{
   struct glx_display *priv = __glXInitialize(dpy);
   struct glx_drawable *glxDraw;

   if (!priv)
      return;

   glxDraw = GetGLXDrawable(dpy, drawable);
   __glxHashDelete(priv->glXDrawHash, drawable);
   free(glxDraw);
}

/**
 * Get the GLX per-screen data structure associated with a GLX context.
 *
 * \param dpy   Display for which the GLX per-screen information is to be
 *              retrieved.
 * \param scrn  Screen on \c dpy for which the GLX per-screen information is
 *              to be retrieved.
 * \returns A pointer to the GLX per-screen data if \c dpy and \c scrn
 *          specify a valid GLX screen, or NULL otherwise.
 *
 * \todo Should this function validate that \c scrn is within the screen
 *       number range for \c dpy?
 */

_X_HIDDEN struct glx_screen *
GetGLXScreenConfigs(Display * dpy, int scrn)
{
   struct glx_display *const priv = __glXInitialize(dpy);

   return (priv
           && priv->screens !=
           NULL) ? priv->screens[scrn] : NULL;
}


static int
GetGLXPrivScreenConfig(Display * dpy, int scrn, struct glx_display ** ppriv,
                       struct glx_screen ** ppsc)
{
   /* Initialize the extension, if needed .  This has the added value
    * of initializing/allocating the display private
    */

   if (dpy == NULL) {
      return GLX_NO_EXTENSION;
   }

   *ppriv = __glXInitialize(dpy);
   if (*ppriv == NULL) {
      return GLX_NO_EXTENSION;
   }

   /* Check screen number to see if its valid */
   if ((scrn < 0) || (scrn >= ScreenCount(dpy))) {
      return GLX_BAD_SCREEN;
   }

   /* Check to see if the GL is supported on this screen */
   *ppsc = (*ppriv)->screens[scrn];
   if ((*ppsc)->configs == NULL) {
      /* No support for GL on this screen regardless of visual */
      return GLX_BAD_VISUAL;
   }

   return Success;
}


/**
 * Determine if a \c GLXFBConfig supplied by the application is valid.
 *
 * \param dpy     Application supplied \c Display pointer.
 * \param config  Application supplied \c GLXFBConfig.
 *
 * \returns If the \c GLXFBConfig is valid, the a pointer to the matching
 *          \c struct glx_config structure is returned.  Otherwise, \c NULL
 *          is returned.
 */
static struct glx_config *
ValidateGLXFBConfig(Display * dpy, GLXFBConfig fbconfig)
{
   struct glx_display *const priv = __glXInitialize(dpy);
   int num_screens = ScreenCount(dpy);
   unsigned i;
   struct glx_config *config;

   if (priv != NULL) {
      for (i = 0; i < num_screens; i++) {
	 for (config = priv->screens[i]->configs; config != NULL;
	      config = config->next) {
	    if (config == (struct glx_config *) fbconfig) {
	       return config;
	    }
	 }
      }
   }

   return NULL;
}

_X_HIDDEN Bool
glx_context_init(struct glx_context *gc,
		 struct glx_screen *psc, struct glx_config *config)
{
   gc->majorOpcode = __glXSetupForCommand(psc->display->dpy);
   if (!gc->majorOpcode)
      return GL_FALSE;

   gc->screen = psc->scr;
   gc->psc = psc;
   gc->config = config;
   gc->isDirect = GL_TRUE;
   gc->currentContextTag = -1;

   return GL_TRUE;
}


/**
 * Create a new context.
 *
 * \param renderType   For FBConfigs, what is the rendering type?
 */

static GLXContext
CreateContext(Display *dpy, int generic_id, struct glx_config *config,
              GLXContext shareList_user, Bool allowDirect,
	      unsigned code, int renderType, int screen)
{
   struct glx_context *gc;
   struct glx_screen *psc;
   struct glx_context *shareList = (struct glx_context *) shareList_user;
   if (dpy == NULL)
      return NULL;

   psc = GetGLXScreenConfigs(dpy, screen);
   if (psc == NULL)
      return NULL;

   if (generic_id == None)
      return NULL;

   gc = NULL;
#ifdef GLX_USE_APPLEGL
   gc = applegl_create_context(psc, config, shareList, renderType);
#else
   if (allowDirect && psc->vtable->create_context)
      gc = psc->vtable->create_context(psc, config, shareList, renderType);
   if (!gc)
      gc = indirect_create_context(psc, config, shareList, renderType);
#endif
   if (!gc)
      return NULL;

   LockDisplay(dpy);
   switch (code) {
   case X_GLXCreateContext: {
      xGLXCreateContextReq *req;

      /* Send the glXCreateContext request */
      GetReq(GLXCreateContext, req);
      req->reqType = gc->majorOpcode;
      req->glxCode = X_GLXCreateContext;
      req->context = gc->xid = XAllocID(dpy);
      req->visual = generic_id;
      req->screen = screen;
      req->shareList = shareList ? shareList->xid : None;
      req->isDirect = gc->isDirect;
      break;
   }

   case X_GLXCreateNewContext: {
      xGLXCreateNewContextReq *req;

      /* Send the glXCreateNewContext request */
      GetReq(GLXCreateNewContext, req);
      req->reqType = gc->majorOpcode;
      req->glxCode = X_GLXCreateNewContext;
      req->context = gc->xid = XAllocID(dpy);
      req->fbconfig = generic_id;
      req->screen = screen;
      req->renderType = renderType;
      req->shareList = shareList ? shareList->xid : None;
      req->isDirect = gc->isDirect;
      break;
   }

   case X_GLXvop_CreateContextWithConfigSGIX: {
      xGLXVendorPrivateWithReplyReq *vpreq;
      xGLXCreateContextWithConfigSGIXReq *req;

      /* Send the glXCreateNewContext request */
      GetReqExtra(GLXVendorPrivateWithReply,
		  sz_xGLXCreateContextWithConfigSGIXReq -
		  sz_xGLXVendorPrivateWithReplyReq, vpreq);
      req = (xGLXCreateContextWithConfigSGIXReq *) vpreq;
      req->reqType = gc->majorOpcode;
      req->glxCode = X_GLXVendorPrivateWithReply;
      req->vendorCode = X_GLXvop_CreateContextWithConfigSGIX;
      req->context = gc->xid = XAllocID(dpy);
      req->fbconfig = generic_id;
      req->screen = screen;
      req->renderType = renderType;
      req->shareList = shareList ? shareList->xid : None;
      req->isDirect = gc->isDirect;
      break;
   }

   default:
      /* What to do here?  This case is the sign of an internal error.  It
       * should never be reachable.
       */
      break;
   }

   UnlockDisplay(dpy);
   SyncHandle();

   gc->share_xid = shareList ? shareList->xid : None;
   gc->imported = GL_FALSE;
   gc->renderType = renderType;

   return (GLXContext) gc;
}

_X_EXPORT GLXContext
glXCreateContext(Display * dpy, XVisualInfo * vis,
                 GLXContext shareList, Bool allowDirect)
{
   struct glx_config *config = NULL;
   int renderType = 0;

#if defined(GLX_DIRECT_RENDERING) || defined(GLX_USE_APPLEGL)
   struct glx_screen *const psc = GetGLXScreenConfigs(dpy, vis->screen);

   config = glx_config_find_visual(psc->visuals, vis->visualid);
   if (config == NULL) {
      xError error;

      error.errorCode = BadValue;
      error.resourceID = vis->visualid;
      error.sequenceNumber = dpy->request;
      error.type = X_Error;
      error.majorCode = __glXSetupForCommand(dpy);
      error.minorCode = X_GLXCreateContext;
      _XError(dpy, &error);
      return None;
   }

   renderType = config->rgbMode ? GLX_RGBA_TYPE : GLX_COLOR_INDEX_TYPE;
#endif

   return CreateContext(dpy, vis->visualid, config, shareList, allowDirect,
                        X_GLXCreateContext, renderType, vis->screen);
}

static void
glx_send_destroy_context(Display *dpy, XID xid)
{
   CARD8 opcode = __glXSetupForCommand(dpy);
   xGLXDestroyContextReq *req;

   LockDisplay(dpy);
   GetReq(GLXDestroyContext, req);
   req->reqType = opcode;
   req->glxCode = X_GLXDestroyContext;
   req->context = xid;
   UnlockDisplay(dpy);
   SyncHandle();
}

/*
** Destroy the named context
*/

_X_EXPORT void
glXDestroyContext(Display * dpy, GLXContext ctx)
{
   struct glx_context *gc = (struct glx_context *) ctx;

   if (gc == NULL || gc->xid == None)
      return;

   __glXLock();
   if (!gc->imported)
      glx_send_destroy_context(dpy, gc->xid);

   if (gc->currentDpy) {
      /* This context is bound to some thread.  According to the man page,
       * we should not actually delete the context until it's unbound.
       * Note that we set gc->xid = None above.  In MakeContextCurrent()
       * we check for that and delete the context there.
       */
      gc->xid = None;
   } else {
      gc->vtable->destroy(gc);
   }
   __glXUnlock();
}

/*
** Return the major and minor version #s for the GLX extension
*/
_X_EXPORT Bool
glXQueryVersion(Display * dpy, int *major, int *minor)
{
   struct glx_display *priv;

   /* Init the extension.  This fetches the major and minor version. */
   priv = __glXInitialize(dpy);
   if (!priv)
      return GL_FALSE;

   if (major)
      *major = priv->majorVersion;
   if (minor)
      *minor = priv->minorVersion;
   return GL_TRUE;
}

/*
** Query the existance of the GLX extension
*/
_X_EXPORT Bool
glXQueryExtension(Display * dpy, int *errorBase, int *eventBase)
{
   int major_op, erb, evb;
   Bool rv;

   rv = XQueryExtension(dpy, GLX_EXTENSION_NAME, &major_op, &evb, &erb);
   if (rv) {
      if (errorBase)
         *errorBase = erb;
      if (eventBase)
         *eventBase = evb;
   }
   return rv;
}

/*
** Put a barrier in the token stream that forces the GL to finish its
** work before X can proceed.
*/
_X_EXPORT void
glXWaitGL(void)
{
   struct glx_context *gc = __glXGetCurrentContext();

   if (gc && gc->vtable->wait_gl)
      gc->vtable->wait_gl(gc);
}

/*
** Put a barrier in the token stream that forces X to finish its
** work before GL can proceed.
*/
_X_EXPORT void
glXWaitX(void)
{
   struct glx_context *gc = __glXGetCurrentContext();

   if (gc && gc->vtable->wait_x)
      gc->vtable->wait_x(gc);
}

_X_EXPORT void
glXUseXFont(Font font, int first, int count, int listBase)
{
   struct glx_context *gc = __glXGetCurrentContext();

   if (gc && gc->vtable->use_x_font)
      gc->vtable->use_x_font(gc, font, first, count, listBase);
}

/************************************************************************/

/*
** Copy the source context to the destination context using the
** attribute "mask".
*/
_X_EXPORT void
glXCopyContext(Display * dpy, GLXContext source_user,
	       GLXContext dest_user, unsigned long mask)
{
   struct glx_context *source = (struct glx_context *) source_user;
   struct glx_context *dest = (struct glx_context *) dest_user;
#ifdef GLX_USE_APPLEGL
   struct glx_context *gc = __glXGetCurrentContext();
   int errorcode;
   bool x11error;

   if(apple_glx_copy_context(gc->driContext, source->driContext, dest->driContext,
                             mask, &errorcode, &x11error)) {
      __glXSendError(dpy, errorcode, 0, X_GLXCopyContext, x11error);
   }
   
#else
   xGLXCopyContextReq *req;
   struct glx_context *gc = __glXGetCurrentContext();
   GLXContextTag tag;
   CARD8 opcode;

   opcode = __glXSetupForCommand(dpy);
   if (!opcode) {
      return;
   }

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   if (gc->isDirect) {
      /* NOT_DONE: This does not work yet */
   }
#endif

   /*
    ** If the source is the current context, send its tag so that the context
    ** can be flushed before the copy.
    */
   if (source == gc && dpy == gc->currentDpy) {
      tag = gc->currentContextTag;
   }
   else {
      tag = 0;
   }

   /* Send the glXCopyContext request */
   LockDisplay(dpy);
   GetReq(GLXCopyContext, req);
   req->reqType = opcode;
   req->glxCode = X_GLXCopyContext;
   req->source = source ? source->xid : None;
   req->dest = dest ? dest->xid : None;
   req->mask = mask;
   req->contextTag = tag;
   UnlockDisplay(dpy);
   SyncHandle();
#endif /* GLX_USE_APPLEGL */
}


/**
 * Determine if a context uses direct rendering.
 *
 * \param dpy        Display where the context was created.
 * \param contextID  ID of the context to be tested.
 *
 * \returns \c GL_TRUE if the context is direct rendering or not.
 */
static Bool
__glXIsDirect(Display * dpy, GLXContextID contextID)
{
#if !defined(USE_XCB)
   xGLXIsDirectReq *req;
   xGLXIsDirectReply reply;
#endif
   CARD8 opcode;

   opcode = __glXSetupForCommand(dpy);
   if (!opcode) {
      return GL_FALSE;
   }

#ifdef USE_XCB
   xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_generic_error_t *err;
   xcb_glx_is_direct_reply_t *reply = xcb_glx_is_direct_reply(c,
                                                              xcb_glx_is_direct
                                                              (c, contextID),
                                                              &err);

   const Bool is_direct = (reply != NULL && reply->is_direct) ? True : False;

   if (err != NULL) {
      __glXSendErrorForXcb(dpy, err);
      free(err);
   }

   free(reply);

   return is_direct;
#else
   /* Send the glXIsDirect request */
   LockDisplay(dpy);
   GetReq(GLXIsDirect, req);
   req->reqType = opcode;
   req->glxCode = X_GLXIsDirect;
   req->context = contextID;
   _XReply(dpy, (xReply *) & reply, 0, False);
   UnlockDisplay(dpy);
   SyncHandle();

   return reply.isDirect;
#endif /* USE_XCB */
}

/**
 * \todo
 * Shouldn't this function \b always return \c GL_FALSE when
 * \c GLX_DIRECT_RENDERING is not defined?  Do we really need to bother with
 * the GLX protocol here at all?
 */
_X_EXPORT Bool
glXIsDirect(Display * dpy, GLXContext gc_user)
{
   struct glx_context *gc = (struct glx_context *) gc_user;

   if (!gc) {
      return GL_FALSE;
   }
   else if (gc->isDirect) {
      return GL_TRUE;
   }
#ifdef GLX_USE_APPLEGL  /* TODO: indirect on darwin */
      return GL_FALSE;
#else
   return __glXIsDirect(dpy, gc->xid);
#endif
}

_X_EXPORT GLXPixmap
glXCreateGLXPixmap(Display * dpy, XVisualInfo * vis, Pixmap pixmap)
{
#ifdef GLX_USE_APPLEGL
   int screen = vis->screen;
   struct glx_screen *const psc = GetGLXScreenConfigs(dpy, screen);
   const struct glx_config *config;

   config = glx_config_find_visual(psc->visuals, vis->visualid);
   
   if(apple_glx_pixmap_create(dpy, vis->screen, pixmap, config))
      return None;
   
   return pixmap;
#else
   xGLXCreateGLXPixmapReq *req;
   struct glx_drawable *glxDraw;
   GLXPixmap xid;
   CARD8 opcode;

   opcode = __glXSetupForCommand(dpy);
   if (!opcode) {
      return None;
   }

   glxDraw = Xmalloc(sizeof(*glxDraw));
   if (!glxDraw)
      return None;

   /* Send the glXCreateGLXPixmap request */
   LockDisplay(dpy);
   GetReq(GLXCreateGLXPixmap, req);
   req->reqType = opcode;
   req->glxCode = X_GLXCreateGLXPixmap;
   req->screen = vis->screen;
   req->visual = vis->visualid;
   req->pixmap = pixmap;
   req->glxpixmap = xid = XAllocID(dpy);
   UnlockDisplay(dpy);
   SyncHandle();

   if (InitGLXDrawable(dpy, glxDraw, pixmap, req->glxpixmap)) {
      free(glxDraw);
      return None;
   }

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   do {
      /* FIXME: Maybe delay __DRIdrawable creation until the drawable
       * is actually bound to a context... */

      struct glx_display *const priv = __glXInitialize(dpy);
      __GLXDRIdrawable *pdraw;
      struct glx_screen *psc;
      struct glx_config *config;

      psc = priv->screens[vis->screen];
      if (psc->driScreen == NULL)
         return xid;

      config = glx_config_find_visual(psc->visuals, vis->visualid);
      pdraw = psc->driScreen->createDrawable(psc, pixmap, xid, config);
      if (pdraw == NULL) {
         fprintf(stderr, "failed to create pixmap\n");
         xid = None;
         break;
      }

      if (__glxHashInsert(priv->drawHash, xid, pdraw)) {
         (*pdraw->destroyDrawable) (pdraw);
         xid = None;
         break;
      }
   } while (0);

   if (xid == None) {
      xGLXDestroyGLXPixmapReq *dreq;
      LockDisplay(dpy);
      GetReq(GLXDestroyGLXPixmap, dreq);
      dreq->reqType = opcode;
      dreq->glxCode = X_GLXDestroyGLXPixmap;
      dreq->glxpixmap = xid;
      UnlockDisplay(dpy);
      SyncHandle();
   }
#endif

   return xid;
#endif
}

/*
** Destroy the named pixmap
*/
_X_EXPORT void
glXDestroyGLXPixmap(Display * dpy, GLXPixmap glxpixmap)
{
#ifdef GLX_USE_APPLEGL
   if(apple_glx_pixmap_destroy(dpy, glxpixmap))
      __glXSendError(dpy, GLXBadPixmap, glxpixmap, X_GLXDestroyPixmap, false);
#else
   xGLXDestroyGLXPixmapReq *req;
   CARD8 opcode;

   opcode = __glXSetupForCommand(dpy);
   if (!opcode) {
      return;
   }

   /* Send the glXDestroyGLXPixmap request */
   LockDisplay(dpy);
   GetReq(GLXDestroyGLXPixmap, req);
   req->reqType = opcode;
   req->glxCode = X_GLXDestroyGLXPixmap;
   req->glxpixmap = glxpixmap;
   UnlockDisplay(dpy);
   SyncHandle();

   DestroyGLXDrawable(dpy, glxpixmap);

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   {
      struct glx_display *const priv = __glXInitialize(dpy);
      __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, glxpixmap);

      if (pdraw != NULL) {
         (*pdraw->destroyDrawable) (pdraw);
         __glxHashDelete(priv->drawHash, glxpixmap);
      }
   }
#endif
#endif /* GLX_USE_APPLEGL */
}

_X_EXPORT void
glXSwapBuffers(Display * dpy, GLXDrawable drawable)
{
#ifdef GLX_USE_APPLEGL
   struct glx_context * gc = __glXGetCurrentContext();
   if(gc && apple_glx_is_current_drawable(dpy, gc->driContext, drawable)) {
      apple_glx_swap_buffers(gc->driContext);
   } else {
      __glXSendError(dpy, GLXBadCurrentWindow, 0, X_GLXSwapBuffers, false);
   }
#else
   struct glx_context *gc;
   GLXContextTag tag;
   CARD8 opcode;
#ifdef USE_XCB
   xcb_connection_t *c;
#else
   xGLXSwapBuffersReq *req;
#endif

   gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   {
      __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);

      if (pdraw != NULL) {
         if (gc && drawable == gc->currentDrawable) {
            glFlush();
         }

         (*pdraw->psc->driScreen->swapBuffers)(pdraw, 0, 0, 0);
         return;
      }
   }
#endif

   opcode = __glXSetupForCommand(dpy);
   if (!opcode) {
      return;
   }

   /*
    ** The calling thread may or may not have a current context.  If it
    ** does, send the context tag so the server can do a flush.
    */
   if ((gc != NULL) && (dpy == gc->currentDpy) &&
       ((drawable == gc->currentDrawable)
        || (drawable == gc->currentReadable))) {
      tag = gc->currentContextTag;
   }
   else {
      tag = 0;
   }

#ifdef USE_XCB
   c = XGetXCBConnection(dpy);
   xcb_glx_swap_buffers(c, tag, drawable);
   xcb_flush(c);
#else
   /* Send the glXSwapBuffers request */
   LockDisplay(dpy);
   GetReq(GLXSwapBuffers, req);
   req->reqType = opcode;
   req->glxCode = X_GLXSwapBuffers;
   req->drawable = drawable;
   req->contextTag = tag;
   UnlockDisplay(dpy);
   SyncHandle();
   XFlush(dpy);
#endif /* USE_XCB */
#endif /* GLX_USE_APPLEGL */
}


/*
** Return configuration information for the given display, screen and
** visual combination.
*/
_X_EXPORT int
glXGetConfig(Display * dpy, XVisualInfo * vis, int attribute,
             int *value_return)
{
   struct glx_display *priv;
   struct glx_screen *psc;
   struct glx_config *config;
   int status;

   status = GetGLXPrivScreenConfig(dpy, vis->screen, &priv, &psc);
   if (status == Success) {
      config = glx_config_find_visual(psc->visuals, vis->visualid);

      /* Lookup attribute after first finding a match on the visual */
      if (config != NULL) {
	 return glx_config_get(config, attribute, value_return);
      }

      status = GLX_BAD_VISUAL;
   }

   /*
    ** If we can't find the config for this visual, this visual is not
    ** supported by the OpenGL implementation on the server.
    */
   if ((status == GLX_BAD_VISUAL) && (attribute == GLX_USE_GL)) {
      *value_return = GL_FALSE;
      status = Success;
   }

   return status;
}

/************************************************************************/

static void
init_fbconfig_for_chooser(struct glx_config * config,
                          GLboolean fbconfig_style_tags)
{
   memset(config, 0, sizeof(struct glx_config));
   config->visualID = (XID) GLX_DONT_CARE;
   config->visualType = GLX_DONT_CARE;

   /* glXChooseFBConfig specifies different defaults for these two than
    * glXChooseVisual.
    */
   if (fbconfig_style_tags) {
      config->rgbMode = GL_TRUE;
      config->doubleBufferMode = GLX_DONT_CARE;
   }

   config->visualRating = GLX_DONT_CARE;
   config->transparentPixel = GLX_NONE;
   config->transparentRed = GLX_DONT_CARE;
   config->transparentGreen = GLX_DONT_CARE;
   config->transparentBlue = GLX_DONT_CARE;
   config->transparentAlpha = GLX_DONT_CARE;
   config->transparentIndex = GLX_DONT_CARE;

   config->drawableType = GLX_WINDOW_BIT;
   config->renderType =
      (config->rgbMode) ? GLX_RGBA_BIT : GLX_COLOR_INDEX_BIT;
   config->xRenderable = GLX_DONT_CARE;
   config->fbconfigID = (GLXFBConfigID) (GLX_DONT_CARE);

   config->swapMethod = GLX_DONT_CARE;
}

#define MATCH_DONT_CARE( param )        \
  do {                                  \
    if ( ((int) a-> param != (int) GLX_DONT_CARE)   \
         && (a-> param != b-> param) ) {        \
      return False;                             \
    }                                           \
  } while ( 0 )

#define MATCH_MINIMUM( param )                  \
  do {                                          \
    if ( ((int) a-> param != (int) GLX_DONT_CARE)	\
         && (a-> param > b-> param) ) {         \
      return False;                             \
    }                                           \
  } while ( 0 )

#define MATCH_EXACT( param )                    \
  do {                                          \
    if ( a-> param != b-> param) {              \
      return False;                             \
    }                                           \
  } while ( 0 )

/* Test that all bits from a are contained in b */
#define MATCH_MASK(param)			\
  do {						\
    if ((a->param & ~b->param) != 0)		\
      return False;				\
  } while (0);

/**
 * Determine if two GLXFBConfigs are compatible.
 *
 * \param a  Application specified config to test.
 * \param b  Server specified config to test against \c a.
 */
static Bool
fbconfigs_compatible(const struct glx_config * const a,
                     const struct glx_config * const b)
{
   MATCH_DONT_CARE(doubleBufferMode);
   MATCH_DONT_CARE(visualType);
   MATCH_DONT_CARE(visualRating);
   MATCH_DONT_CARE(xRenderable);
   MATCH_DONT_CARE(fbconfigID);
   MATCH_DONT_CARE(swapMethod);

   MATCH_MINIMUM(rgbBits);
   MATCH_MINIMUM(numAuxBuffers);
   MATCH_MINIMUM(redBits);
   MATCH_MINIMUM(greenBits);
   MATCH_MINIMUM(blueBits);
   MATCH_MINIMUM(alphaBits);
   MATCH_MINIMUM(depthBits);
   MATCH_MINIMUM(stencilBits);
   MATCH_MINIMUM(accumRedBits);
   MATCH_MINIMUM(accumGreenBits);
   MATCH_MINIMUM(accumBlueBits);
   MATCH_MINIMUM(accumAlphaBits);
   MATCH_MINIMUM(sampleBuffers);
   MATCH_MINIMUM(maxPbufferWidth);
   MATCH_MINIMUM(maxPbufferHeight);
   MATCH_MINIMUM(maxPbufferPixels);
   MATCH_MINIMUM(samples);

   MATCH_DONT_CARE(stereoMode);
   MATCH_EXACT(level);

   MATCH_MASK(drawableType);
   MATCH_MASK(renderType);

   /* There is a bug in a few of the XFree86 DDX drivers.  They contain
    * visuals with a "transparent type" of 0 when they really mean GLX_NONE.
    * Technically speaking, it is a bug in the DDX driver, but there is
    * enough of an installed base to work around the problem here.  In any
    * case, 0 is not a valid value of the transparent type, so we'll treat 0
    * from the app as GLX_DONT_CARE. We'll consider GLX_NONE from the app and
    * 0 from the server to be a match to maintain backward compatibility with
    * the (broken) drivers.
    */

   if (a->transparentPixel != (int) GLX_DONT_CARE && a->transparentPixel != 0) {
      if (a->transparentPixel == GLX_NONE) {
         if (b->transparentPixel != GLX_NONE && b->transparentPixel != 0)
            return False;
      }
      else {
         MATCH_EXACT(transparentPixel);
      }

      switch (a->transparentPixel) {
      case GLX_TRANSPARENT_RGB:
         MATCH_DONT_CARE(transparentRed);
         MATCH_DONT_CARE(transparentGreen);
         MATCH_DONT_CARE(transparentBlue);
         MATCH_DONT_CARE(transparentAlpha);
         break;

      case GLX_TRANSPARENT_INDEX:
         MATCH_DONT_CARE(transparentIndex);
         break;

      default:
         break;
      }
   }

   return True;
}


/* There's some trickly language in the GLX spec about how this is supposed
 * to work.  Basically, if a given component size is either not specified
 * or the requested size is zero, it is supposed to act like PERFER_SMALLER.
 * Well, that's really hard to do with the code as-is.  This behavior is
 * closer to correct, but still not technically right.
 */
#define PREFER_LARGER_OR_ZERO(comp)             \
  do {                                          \
    if ( ((*a)-> comp) != ((*b)-> comp) ) {     \
      if ( ((*a)-> comp) == 0 ) {               \
        return -1;                              \
      }                                         \
      else if ( ((*b)-> comp) == 0 ) {          \
        return 1;                               \
      }                                         \
      else {                                    \
        return ((*b)-> comp) - ((*a)-> comp) ;  \
      }                                         \
    }                                           \
  } while( 0 )

#define PREFER_LARGER(comp)                     \
  do {                                          \
    if ( ((*a)-> comp) != ((*b)-> comp) ) {     \
      return ((*b)-> comp) - ((*a)-> comp) ;    \
    }                                           \
  } while( 0 )

#define PREFER_SMALLER(comp)                    \
  do {                                          \
    if ( ((*a)-> comp) != ((*b)-> comp) ) {     \
      return ((*a)-> comp) - ((*b)-> comp) ;    \
    }                                           \
  } while( 0 )

/**
 * Compare two GLXFBConfigs.  This function is intended to be used as the
 * compare function passed in to qsort.
 *
 * \returns If \c a is a "better" config, according to the specification of
 *          SGIX_fbconfig, a number less than zero is returned.  If \c b is
 *          better, then a number greater than zero is return.  If both are
 *          equal, zero is returned.
 * \sa qsort, glXChooseVisual, glXChooseFBConfig, glXChooseFBConfigSGIX
 */
static int
fbconfig_compare(struct glx_config **a, struct glx_config **b)
{
   /* The order of these comparisons must NOT change.  It is defined by
    * the GLX 1.3 spec and ARB_multisample.
    */

   PREFER_SMALLER(visualSelectGroup);

   /* The sort order for the visualRating is GLX_NONE, GLX_SLOW, and
    * GLX_NON_CONFORMANT_CONFIG.  It just so happens that this is the
    * numerical sort order of the enums (0x8000, 0x8001, and 0x800D).
    */
   PREFER_SMALLER(visualRating);

   /* This isn't quite right.  It is supposed to compare the sum of the
    * components the user specifically set minimums for.
    */
   PREFER_LARGER_OR_ZERO(redBits);
   PREFER_LARGER_OR_ZERO(greenBits);
   PREFER_LARGER_OR_ZERO(blueBits);
   PREFER_LARGER_OR_ZERO(alphaBits);

   PREFER_SMALLER(rgbBits);

   if (((*a)->doubleBufferMode != (*b)->doubleBufferMode)) {
      /* Prefer single-buffer.
       */
      return (!(*a)->doubleBufferMode) ? -1 : 1;
   }

   PREFER_SMALLER(numAuxBuffers);

   PREFER_LARGER_OR_ZERO(depthBits);
   PREFER_SMALLER(stencilBits);

   /* This isn't quite right.  It is supposed to compare the sum of the
    * components the user specifically set minimums for.
    */
   PREFER_LARGER_OR_ZERO(accumRedBits);
   PREFER_LARGER_OR_ZERO(accumGreenBits);
   PREFER_LARGER_OR_ZERO(accumBlueBits);
   PREFER_LARGER_OR_ZERO(accumAlphaBits);

   PREFER_SMALLER(visualType);

   /* None of the multisample specs say where this comparison should happen,
    * so I put it near the end.
    */
   PREFER_SMALLER(sampleBuffers);
   PREFER_SMALLER(samples);

   /* None of the pbuffer or fbconfig specs say that this comparison needs
    * to happen at all, but it seems like it should.
    */
   PREFER_LARGER(maxPbufferWidth);
   PREFER_LARGER(maxPbufferHeight);
   PREFER_LARGER(maxPbufferPixels);

   return 0;
}


/**
 * Selects and sorts a subset of the supplied configs based on the attributes.
 * This function forms to basis of \c glXChooseVisual, \c glXChooseFBConfig,
 * and \c glXChooseFBConfigSGIX.
 *
 * \param configs   Array of pointers to possible configs.  The elements of
 *                  this array that do not meet the criteria will be set to
 *                  NULL.  The remaining elements will be sorted according to
 *                  the various visual / FBConfig selection rules.
 * \param num_configs  Number of elements in the \c configs array.
 * \param attribList   Attributes used select from \c configs.  This array is
 *                     terminated by a \c None tag.  The array can either take
 *                     the form expected by \c glXChooseVisual (where boolean
 *                     tags do not have a value) or by \c glXChooseFBConfig
 *                     (where every tag has a value).
 * \param fbconfig_style_tags  Selects whether \c attribList is in
 *                             \c glXChooseVisual style or
 *                             \c glXChooseFBConfig style.
 * \returns The number of valid elements left in \c configs.
 *
 * \sa glXChooseVisual, glXChooseFBConfig, glXChooseFBConfigSGIX
 */
static int
choose_visual(struct glx_config ** configs, int num_configs,
              const int *attribList, GLboolean fbconfig_style_tags)
{
   struct glx_config test_config;
   int base;
   int i;

   /* This is a fairly direct implementation of the selection method
    * described by GLX_SGIX_fbconfig.  Start by culling out all the
    * configs that are not compatible with the selected parameter
    * list.
    */

   init_fbconfig_for_chooser(&test_config, fbconfig_style_tags);
   __glXInitializeVisualConfigFromTags(&test_config, 512,
                                       (const INT32 *) attribList,
                                       GL_TRUE, fbconfig_style_tags);

   base = 0;
   for (i = 0; i < num_configs; i++) {
      if (fbconfigs_compatible(&test_config, configs[i])) {
         configs[base] = configs[i];
         base++;
      }
   }

   if (base == 0) {
      return 0;
   }

   if (base < num_configs) {
      (void) memset(&configs[base], 0, sizeof(void *) * (num_configs - base));
   }

   /* After the incompatible configs are removed, the resulting
    * list is sorted according to the rules set out in the various
    * specifications.
    */

   qsort(configs, base, sizeof(struct glx_config *),
         (int (*)(const void *, const void *)) fbconfig_compare);
   return base;
}




/*
** Return the visual that best matches the template.  Return None if no
** visual matches the template.
*/
_X_EXPORT XVisualInfo *
glXChooseVisual(Display * dpy, int screen, int *attribList)
{
   XVisualInfo *visualList = NULL;
   struct glx_display *priv;
   struct glx_screen *psc;
   struct glx_config test_config;
   struct glx_config *config;
   struct glx_config *best_config = NULL;

   /*
    ** Get a list of all visuals, return if list is empty
    */
   if (GetGLXPrivScreenConfig(dpy, screen, &priv, &psc) != Success) {
      return None;
   }


   /*
    ** Build a template from the defaults and the attribute list
    ** Free visual list and return if an unexpected token is encountered
    */
   init_fbconfig_for_chooser(&test_config, GL_FALSE);
   __glXInitializeVisualConfigFromTags(&test_config, 512,
                                       (const INT32 *) attribList,
                                       GL_TRUE, GL_FALSE);

   /*
    ** Eliminate visuals that don't meet minimum requirements
    ** Compute a score for those that do
    ** Remember which visual, if any, got the highest score
    ** If no visual is acceptable, return None
    ** Otherwise, create an XVisualInfo list with just the selected X visual
    ** and return this.
    */
   for (config = psc->visuals; config != NULL; config = config->next) {
      if (fbconfigs_compatible(&test_config, config)
          && ((best_config == NULL) ||
              (fbconfig_compare (&config, &best_config) < 0))) {
         XVisualInfo visualTemplate;
         XVisualInfo *newList;
         int i;

         visualTemplate.screen = screen;
         visualTemplate.visualid = config->visualID;
         newList = XGetVisualInfo(dpy, VisualScreenMask | VisualIDMask,
                                  &visualTemplate, &i);

         if (newList) {
            Xfree(visualList);
            visualList = newList;
            best_config = config;
         }
      }
   }

#ifdef GLX_USE_APPLEGL
   if(visualList && getenv("LIBGL_DUMP_VISUALID")) {
      printf("visualid 0x%lx\n", visualList[0].visualid);
   }
#endif

   return visualList;
}


_X_EXPORT const char *
glXQueryExtensionsString(Display * dpy, int screen)
{
   struct glx_screen *psc;
   struct glx_display *priv;

   if (GetGLXPrivScreenConfig(dpy, screen, &priv, &psc) != Success) {
      return NULL;
   }

   if (!psc->effectiveGLXexts) {
      if (!psc->serverGLXexts) {
         psc->serverGLXexts =
            __glXQueryServerString(dpy, priv->majorOpcode, screen,
                                   GLX_EXTENSIONS);
      }

      __glXCalculateUsableExtensions(psc,
#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
                                     (psc->driScreen != NULL),
#else
                                     GL_FALSE,
#endif
                                     priv->minorVersion);
   }

   return psc->effectiveGLXexts;
}

_X_EXPORT const char *
glXGetClientString(Display * dpy, int name)
{
   (void) dpy;

   switch (name) {
   case GLX_VENDOR:
      return (__glXGLXClientVendorName);
   case GLX_VERSION:
      return (__glXGLXClientVersion);
   case GLX_EXTENSIONS:
      return (__glXGetClientExtensions());
   default:
      return NULL;
   }
}

_X_EXPORT const char *
glXQueryServerString(Display * dpy, int screen, int name)
{
   struct glx_screen *psc;
   struct glx_display *priv;
   const char **str;


   if (GetGLXPrivScreenConfig(dpy, screen, &priv, &psc) != Success) {
      return NULL;
   }

   switch (name) {
   case GLX_VENDOR:
      str = &priv->serverGLXvendor;
      break;
   case GLX_VERSION:
      str = &priv->serverGLXversion;
      break;
   case GLX_EXTENSIONS:
      str = &psc->serverGLXexts;
      break;
   default:
      return NULL;
   }

   if (*str == NULL) {
      *str = __glXQueryServerString(dpy, priv->majorOpcode, screen, name);
   }

   return *str;
}

void
__glXClientInfo(Display * dpy, int opcode)
{
   char *ext_str = __glXGetClientGLExtensionString();
   int size = strlen(ext_str) + 1;

#ifdef USE_XCB
   xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_glx_client_info(c,
                       GLX_MAJOR_VERSION, GLX_MINOR_VERSION, size, ext_str);
#else
   xGLXClientInfoReq *req;

   /* Send the glXClientInfo request */
   LockDisplay(dpy);
   GetReq(GLXClientInfo, req);
   req->reqType = opcode;
   req->glxCode = X_GLXClientInfo;
   req->major = GLX_MAJOR_VERSION;
   req->minor = GLX_MINOR_VERSION;

   req->length += (size + 3) >> 2;
   req->numbytes = size;
   Data(dpy, ext_str, size);

   UnlockDisplay(dpy);
   SyncHandle();
#endif /* USE_XCB */

   Xfree(ext_str);
}


/*
** EXT_import_context
*/

_X_EXPORT Display *
glXGetCurrentDisplay(void)
{
   struct glx_context *gc = __glXGetCurrentContext();
   if (NULL == gc)
      return NULL;
   return gc->currentDpy;
}

_X_EXPORT
GLX_ALIAS(Display *, glXGetCurrentDisplayEXT, (void), (),
          glXGetCurrentDisplay)

#ifndef GLX_USE_APPLEGL
_X_EXPORT GLXContext
glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
   struct glx_display *priv = __glXInitialize(dpy);
   struct glx_screen *psc = NULL;
   xGLXQueryContextReply reply;
   CARD8 opcode;
   struct glx_context *ctx;

   /* This GLX implementation knows about 5 different properties, so
    * allow the server to send us one of each.
    */
   int propList[5 * 2], *pProp, nPropListBytes;
   int numProps;
   int i, renderType;
   XID share;
   struct glx_config *mode;
   uint32_t fbconfigID = 0;
   uint32_t visualID = 0;
   uint32_t screen;
   Bool got_screen = False;

   /* The GLX_EXT_import_context spec says:
    *
    *     "If <contextID> does not refer to a valid context, then a BadContext
    *     error is generated; if <contextID> refers to direct rendering
    *     context then no error is generated but glXImportContextEXT returns
    *     NULL."
    *
    * If contextID is None, generate BadContext on the client-side.  Other
    * sorts of invalid contexts will be detected by the server in the
    * __glXIsDirect call.
    */
   if (contextID == None) {
      __glXSendError(dpy, GLXBadContext, contextID, X_GLXIsDirect, false);
      return NULL;
   }

   if (__glXIsDirect(dpy, contextID))
      return NULL;

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return 0;

   /* Send the glXQueryContextInfoEXT request */
   LockDisplay(dpy);

   if (priv->majorVersion > 1 || priv->minorVersion >= 3) {
      xGLXQueryContextReq *req;

      GetReq(GLXQueryContext, req);

      req->reqType = opcode;
      req->glxCode = X_GLXQueryContext;
      req->context = contextID;
   }
   else {
      xGLXVendorPrivateReq *vpreq;
      xGLXQueryContextInfoEXTReq *req;

      GetReqExtra(GLXVendorPrivate,
		  sz_xGLXQueryContextInfoEXTReq - sz_xGLXVendorPrivateReq,
		  vpreq);
      req = (xGLXQueryContextInfoEXTReq *) vpreq;
      req->reqType = opcode;
      req->glxCode = X_GLXVendorPrivateWithReply;
      req->vendorCode = X_GLXvop_QueryContextInfoEXT;
      req->context = contextID;
   }

   _XReply(dpy, (xReply *) & reply, 0, False);

   if (reply.n <= __GLX_MAX_CONTEXT_PROPS)
      nPropListBytes = reply.n * 2 * sizeof propList[0];
   else
      nPropListBytes = 0;
   _XRead(dpy, (char *) propList, nPropListBytes);
   UnlockDisplay(dpy);
   SyncHandle();

   numProps = nPropListBytes / (2 * sizeof(propList[0]));
   share = None;
   mode = NULL;
   renderType = 0;
   pProp = propList;

   for (i = 0, pProp = propList; i < numProps; i++, pProp += 2)
      switch (pProp[0]) {
      case GLX_SCREEN:
	 screen = pProp[1];
	 got_screen = True;
	 break;
      case GLX_SHARE_CONTEXT_EXT:
	 share = pProp[1];
	 break;
      case GLX_VISUAL_ID_EXT:
	 visualID = pProp[1];
	 break;
      case GLX_FBCONFIG_ID:
	 fbconfigID = pProp[1];
	 break;
      case GLX_RENDER_TYPE:
	 renderType = pProp[1];
	 break;
      }

   if (!got_screen)
      return NULL;

   psc = GetGLXScreenConfigs(dpy, screen);
   if (psc == NULL)
      return NULL;

   if (fbconfigID != 0) {
      mode = glx_config_find_fbconfig(psc->configs, fbconfigID);
   } else if (visualID != 0) {
      mode = glx_config_find_visual(psc->visuals, visualID);
   }

   if (mode == NULL)
      return NULL;

   ctx = indirect_create_context(psc, mode, NULL, renderType);
   if (ctx == NULL)
      return NULL;

   ctx->xid = contextID;
   ctx->imported = GL_TRUE;
   ctx->share_xid = share;

   return (GLXContext) ctx;
}

#endif

_X_EXPORT int
glXQueryContext(Display * dpy, GLXContext ctx_user, int attribute, int *value)
{
   struct glx_context *ctx = (struct glx_context *) ctx_user;

   switch (attribute) {
      case GLX_SHARE_CONTEXT_EXT:
      *value = ctx->share_xid;
      break;
   case GLX_VISUAL_ID_EXT:
      *value = ctx->config ? ctx->config->visualID : None;
      break;
   case GLX_SCREEN:
      *value = ctx->screen;
      break;
   case GLX_FBCONFIG_ID:
      *value = ctx->config ? ctx->config->fbconfigID : None;
      break;
   case GLX_RENDER_TYPE:
      *value = ctx->renderType;
      break;
   default:
      return GLX_BAD_ATTRIBUTE;
   }
   return Success;
}

_X_EXPORT
GLX_ALIAS(int, glXQueryContextInfoEXT,
          (Display * dpy, GLXContext ctx, int attribute, int *value),
          (dpy, ctx, attribute, value), glXQueryContext)

_X_EXPORT GLXContextID glXGetContextIDEXT(const GLXContext ctx_user)
{
   struct glx_context *ctx = (struct glx_context *) ctx_user;

   return (ctx == NULL) ? None : ctx->xid;
}

_X_EXPORT void
glXFreeContextEXT(Display *dpy, GLXContext ctx)
{
   struct glx_context *gc = (struct glx_context *) ctx;

   if (gc == NULL || gc->xid == None)
      return;

   /* The GLX_EXT_import_context spec says:
    *
    *     "glXFreeContext does not free the server-side context information or
    *     the XID associated with the server-side context."
    *
    * Don't send any protocol.  Just destroy the client-side tracking of the
    * context.  Also, only release the context structure if it's not current.
    */
   __glXLock();
   if (gc->currentDpy) {
      gc->xid = None;
   } else {
      gc->vtable->destroy(gc);
   }
   __glXUnlock();
}

_X_EXPORT GLXFBConfig *
glXChooseFBConfig(Display * dpy, int screen,
                  const int *attribList, int *nitems)
{
   struct glx_config **config_list;
   int list_size;


   config_list = (struct glx_config **)
      glXGetFBConfigs(dpy, screen, &list_size);

   if ((config_list != NULL) && (list_size > 0) && (attribList != NULL)) {
      list_size = choose_visual(config_list, list_size, attribList, GL_TRUE);
      if (list_size == 0) {
         XFree(config_list);
         config_list = NULL;
      }
   }

   *nitems = list_size;
   return (GLXFBConfig *) config_list;
}


_X_EXPORT GLXContext
glXCreateNewContext(Display * dpy, GLXFBConfig fbconfig,
                    int renderType, GLXContext shareList, Bool allowDirect)
{
   struct glx_config *config = (struct glx_config *) fbconfig;

   return CreateContext(dpy, config->fbconfigID, config, shareList,
			allowDirect, X_GLXCreateNewContext, renderType,
			config->screen);
}


_X_EXPORT GLXDrawable
glXGetCurrentReadDrawable(void)
{
   struct glx_context *gc = __glXGetCurrentContext();

   return gc->currentReadable;
}


_X_EXPORT GLXFBConfig *
glXGetFBConfigs(Display * dpy, int screen, int *nelements)
{
   struct glx_display *priv = __glXInitialize(dpy);
   struct glx_config **config_list = NULL;
   struct glx_config *config;
   unsigned num_configs = 0;
   int i;

   *nelements = 0;
   if (priv && (priv->screens != NULL)
       && (screen >= 0) && (screen <= ScreenCount(dpy))
       && (priv->screens[screen]->configs != NULL)
       && (priv->screens[screen]->configs->fbconfigID
	   != (int) GLX_DONT_CARE)) {

      for (config = priv->screens[screen]->configs; config != NULL;
           config = config->next) {
         if (config->fbconfigID != (int) GLX_DONT_CARE) {
            num_configs++;
         }
      }

      config_list = Xmalloc(num_configs * sizeof *config_list);
      if (config_list != NULL) {
         *nelements = num_configs;
         i = 0;
         for (config = priv->screens[screen]->configs; config != NULL;
              config = config->next) {
            if (config->fbconfigID != (int) GLX_DONT_CARE) {
               config_list[i] = config;
               i++;
            }
         }
      }
   }

   return (GLXFBConfig *) config_list;
}


_X_EXPORT int
glXGetFBConfigAttrib(Display * dpy, GLXFBConfig fbconfig,
                     int attribute, int *value)
{
   struct glx_config *config = ValidateGLXFBConfig(dpy, fbconfig);

   if (config == NULL)
      return GLXBadFBConfig;

   return glx_config_get(config, attribute, value);
}


_X_EXPORT XVisualInfo *
glXGetVisualFromFBConfig(Display * dpy, GLXFBConfig fbconfig)
{
   XVisualInfo visualTemplate;
   struct glx_config *config = (struct glx_config *) fbconfig;
   int count;

   /*
    ** Get a list of all visuals, return if list is empty
    */
   visualTemplate.visualid = config->visualID;
   return XGetVisualInfo(dpy, VisualIDMask, &visualTemplate, &count);
}

#ifndef GLX_USE_APPLEGL
/*
** GLX_SGI_swap_control
*/
static int
__glXSwapIntervalSGI(int interval)
{
   xGLXVendorPrivateReq *req;
   struct glx_context *gc = __glXGetCurrentContext();
   struct glx_screen *psc;
   Display *dpy;
   CARD32 *interval_ptr;
   CARD8 opcode;

   if (gc == NULL) {
      return GLX_BAD_CONTEXT;
   }

   if (interval <= 0) {
      return GLX_BAD_VALUE;
   }

   psc = GetGLXScreenConfigs( gc->currentDpy, gc->screen);

#ifdef GLX_DIRECT_RENDERING
   if (gc->isDirect && psc->driScreen && psc->driScreen->setSwapInterval) {
      __GLXDRIdrawable *pdraw =
	 GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);
      psc->driScreen->setSwapInterval(pdraw, interval);
      return 0;
   }
#endif

   dpy = gc->currentDpy;
   opcode = __glXSetupForCommand(dpy);
   if (!opcode) {
      return 0;
   }

   /* Send the glXSwapIntervalSGI request */
   LockDisplay(dpy);
   GetReqExtra(GLXVendorPrivate, sizeof(CARD32), req);
   req->reqType = opcode;
   req->glxCode = X_GLXVendorPrivate;
   req->vendorCode = X_GLXvop_SwapIntervalSGI;
   req->contextTag = gc->currentContextTag;

   interval_ptr = (CARD32 *) (req + 1);
   *interval_ptr = interval;

   UnlockDisplay(dpy);
   SyncHandle();
   XFlush(dpy);

   return 0;
}


/*
** GLX_MESA_swap_control
*/
static int
__glXSwapIntervalMESA(unsigned int interval)
{
#ifdef GLX_DIRECT_RENDERING
   struct glx_context *gc = __glXGetCurrentContext();

   if (gc != NULL && gc->isDirect) {
      struct glx_screen *psc;

      psc = GetGLXScreenConfigs( gc->currentDpy, gc->screen);
      if (psc->driScreen && psc->driScreen->setSwapInterval) {
         __GLXDRIdrawable *pdraw =
	    GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);
	 return psc->driScreen->setSwapInterval(pdraw, interval);
      }
   }
#endif

   return GLX_BAD_CONTEXT;
}


static int
__glXGetSwapIntervalMESA(void)
{
#ifdef GLX_DIRECT_RENDERING
   struct glx_context *gc = __glXGetCurrentContext();

   if (gc != NULL && gc->isDirect) {
      struct glx_screen *psc;

      psc = GetGLXScreenConfigs( gc->currentDpy, gc->screen);
      if (psc->driScreen && psc->driScreen->getSwapInterval) {
         __GLXDRIdrawable *pdraw =
	    GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);
	 return psc->driScreen->getSwapInterval(pdraw);
      }
   }
#endif

   return 0;
}


/*
** GLX_SGI_video_sync
*/
static int
__glXGetVideoSyncSGI(unsigned int *count)
{
   int64_t ust, msc, sbc;
   int ret;
   struct glx_context *gc = __glXGetCurrentContext();
   struct glx_screen *psc;
#ifdef GLX_DIRECT_RENDERING
   __GLXDRIdrawable *pdraw;
#endif

   if (!gc)
      return GLX_BAD_CONTEXT;

#ifdef GLX_DIRECT_RENDERING
   if (!gc->isDirect)
      return GLX_BAD_CONTEXT;
#endif

   psc = GetGLXScreenConfigs(gc->currentDpy, gc->screen);
#ifdef GLX_DIRECT_RENDERING
   pdraw = GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);
#endif

   /* FIXME: Looking at the GLX_SGI_video_sync spec in the extension registry,
    * FIXME: there should be a GLX encoding for this call.  I can find no
    * FIXME: documentation for the GLX encoding.
    */
#ifdef GLX_DIRECT_RENDERING
   if (psc->driScreen && psc->driScreen->getDrawableMSC) {
      ret = psc->driScreen->getDrawableMSC(psc, pdraw, &ust, &msc, &sbc);
      *count = (unsigned) msc;
      return (ret == True) ? 0 : GLX_BAD_CONTEXT;
   }
#endif

   return GLX_BAD_CONTEXT;
}

static int
__glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count)
{
   struct glx_context *gc = __glXGetCurrentContext();
   struct glx_screen *psc;
#ifdef GLX_DIRECT_RENDERING
   __GLXDRIdrawable *pdraw;
#endif
   int64_t ust, msc, sbc;
   int ret;

   if (divisor <= 0 || remainder < 0)
      return GLX_BAD_VALUE;

   if (!gc)
      return GLX_BAD_CONTEXT;

#ifdef GLX_DIRECT_RENDERING
   if (!gc->isDirect)
      return GLX_BAD_CONTEXT;
#endif

   psc = GetGLXScreenConfigs( gc->currentDpy, gc->screen);
#ifdef GLX_DIRECT_RENDERING
   pdraw = GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);
#endif

#ifdef GLX_DIRECT_RENDERING
   if (psc->driScreen && psc->driScreen->waitForMSC) {
      ret = psc->driScreen->waitForMSC(pdraw, 0, divisor, remainder, &ust, &msc,
				       &sbc);
      *count = (unsigned) msc;
      return (ret == True) ? 0 : GLX_BAD_CONTEXT;
   }
#endif

   return GLX_BAD_CONTEXT;
}

#endif /* GLX_USE_APPLEGL */

/*
** GLX_SGIX_fbconfig
** Many of these functions are aliased to GLX 1.3 entry points in the 
** GLX_functions table.
*/

_X_EXPORT
GLX_ALIAS(int, glXGetFBConfigAttribSGIX,
          (Display * dpy, GLXFBConfigSGIX config, int attribute, int *value),
          (dpy, config, attribute, value), glXGetFBConfigAttrib)

_X_EXPORT GLX_ALIAS(GLXFBConfigSGIX *, glXChooseFBConfigSGIX,
                 (Display * dpy, int screen, int *attrib_list,
                  int *nelements), (dpy, screen, attrib_list, nelements),
                 glXChooseFBConfig)

_X_EXPORT GLX_ALIAS(XVisualInfo *, glXGetVisualFromFBConfigSGIX,
                 (Display * dpy, GLXFBConfigSGIX config),
                 (dpy, config), glXGetVisualFromFBConfig)

_X_EXPORT GLXPixmap
glXCreateGLXPixmapWithConfigSGIX(Display * dpy,
                                 GLXFBConfigSGIX fbconfig,
                                 Pixmap pixmap)
{
#ifndef GLX_USE_APPLEGL
   xGLXVendorPrivateWithReplyReq *vpreq;
   xGLXCreateGLXPixmapWithConfigSGIXReq *req;
   GLXPixmap xid = None;
   CARD8 opcode;
   struct glx_screen *psc;
#endif
   struct glx_config *config = (struct glx_config *) fbconfig;


   if ((dpy == NULL) || (config == NULL)) {
      return None;
   }
#ifdef GLX_USE_APPLEGL
   if(apple_glx_pixmap_create(dpy, config->screen, pixmap, config))
      return None;
   return pixmap;
#else

   psc = GetGLXScreenConfigs(dpy, config->screen);
   if ((psc != NULL)
       && __glXExtensionBitIsEnabled(psc, SGIX_fbconfig_bit)) {
      opcode = __glXSetupForCommand(dpy);
      if (!opcode) {
         return None;
      }

      /* Send the glXCreateGLXPixmapWithConfigSGIX request */
      LockDisplay(dpy);
      GetReqExtra(GLXVendorPrivateWithReply,
                  sz_xGLXCreateGLXPixmapWithConfigSGIXReq -
                  sz_xGLXVendorPrivateWithReplyReq, vpreq);
      req = (xGLXCreateGLXPixmapWithConfigSGIXReq *) vpreq;
      req->reqType = opcode;
      req->glxCode = X_GLXVendorPrivateWithReply;
      req->vendorCode = X_GLXvop_CreateGLXPixmapWithConfigSGIX;
      req->screen = config->screen;
      req->fbconfig = config->fbconfigID;
      req->pixmap = pixmap;
      req->glxpixmap = xid = XAllocID(dpy);
      UnlockDisplay(dpy);
      SyncHandle();
   }

   return xid;
#endif
}

_X_EXPORT GLXContext
glXCreateContextWithConfigSGIX(Display * dpy,
                               GLXFBConfigSGIX fbconfig, int renderType,
                               GLXContext shareList, Bool allowDirect)
{
   GLXContext gc = NULL;
   struct glx_config *config = (struct glx_config *) fbconfig;
   struct glx_screen *psc;


   if ((dpy == NULL) || (config == NULL)) {
      return None;
   }

   psc = GetGLXScreenConfigs(dpy, config->screen);
   if ((psc != NULL)
       && __glXExtensionBitIsEnabled(psc, SGIX_fbconfig_bit)) {
      gc = CreateContext(dpy, config->fbconfigID, config, shareList,
                         allowDirect,
			 X_GLXvop_CreateContextWithConfigSGIX, renderType,
			 config->screen);
   }

   return gc;
}


_X_EXPORT GLXFBConfigSGIX
glXGetFBConfigFromVisualSGIX(Display * dpy, XVisualInfo * vis)
{
   struct glx_display *priv;
   struct glx_screen *psc = NULL;

   if ((GetGLXPrivScreenConfig(dpy, vis->screen, &priv, &psc) == Success)
       && __glXExtensionBitIsEnabled(psc, SGIX_fbconfig_bit)
       && (psc->configs->fbconfigID != (int) GLX_DONT_CARE)) {
      return (GLXFBConfigSGIX) glx_config_find_visual(psc->configs,
						      vis->visualid);
   }

   return NULL;
}

#ifndef GLX_USE_APPLEGL
/*
** GLX_SGIX_swap_group
*/
static void
__glXJoinSwapGroupSGIX(Display * dpy, GLXDrawable drawable,
                       GLXDrawable member)
{
   (void) dpy;
   (void) drawable;
   (void) member;
}


/*
** GLX_SGIX_swap_barrier
*/
static void
__glXBindSwapBarrierSGIX(Display * dpy, GLXDrawable drawable, int barrier)
{
   (void) dpy;
   (void) drawable;
   (void) barrier;
}

static Bool
__glXQueryMaxSwapBarriersSGIX(Display * dpy, int screen, int *max)
{
   (void) dpy;
   (void) screen;
   (void) max;
   return False;
}


/*
** GLX_OML_sync_control
*/
static Bool
__glXGetSyncValuesOML(Display * dpy, GLXDrawable drawable,
                      int64_t * ust, int64_t * msc, int64_t * sbc)
{
   struct glx_display * const priv = __glXInitialize(dpy);
   int ret;
#ifdef GLX_DIRECT_RENDERING
   __GLXDRIdrawable *pdraw;
#endif
   struct glx_screen *psc;

   if (!priv)
      return False;

#ifdef GLX_DIRECT_RENDERING
   pdraw = GetGLXDRIDrawable(dpy, drawable);
   psc = pdraw ? pdraw->psc : NULL;
   if (pdraw && psc->driScreen->getDrawableMSC) {
      ret = psc->driScreen->getDrawableMSC(psc, pdraw, ust, msc, sbc);
      return ret;
   }
#endif

   return False;
}

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
_X_HIDDEN GLboolean
__glxGetMscRate(__GLXDRIdrawable *glxDraw,
		int32_t * numerator, int32_t * denominator)
{
#ifdef XF86VIDMODE
   struct glx_screen *psc;
   XF86VidModeModeLine mode_line;
   int dot_clock;
   int i;

   psc = glxDraw->psc;
   if (XF86VidModeQueryVersion(psc->dpy, &i, &i) &&
       XF86VidModeGetModeLine(psc->dpy, psc->scr, &dot_clock, &mode_line)) {
      unsigned n = dot_clock * 1000;
      unsigned d = mode_line.vtotal * mode_line.htotal;

# define V_INTERLACE 0x010
# define V_DBLSCAN   0x020

      if (mode_line.flags & V_INTERLACE)
         n *= 2;
      else if (mode_line.flags & V_DBLSCAN)
         d *= 2;

      /* The OML_sync_control spec requires that if the refresh rate is a
       * whole number, that the returned numerator be equal to the refresh
       * rate and the denominator be 1.
       */

      if (n % d == 0) {
         n /= d;
         d = 1;
      }
      else {
         static const unsigned f[] = { 13, 11, 7, 5, 3, 2, 0 };

         /* This is a poor man's way to reduce a fraction.  It's far from
          * perfect, but it will work well enough for this situation.
          */

         for (i = 0; f[i] != 0; i++) {
            while (n % f[i] == 0 && d % f[i] == 0) {
               d /= f[i];
               n /= f[i];
            }
         }
      }

      *numerator = n;
      *denominator = d;

      return True;
   }
   else
#endif

   return False;
}
#endif

/**
 * Determine the refresh rate of the specified drawable and display.
 *
 * \param dpy          Display whose refresh rate is to be determined.
 * \param drawable     Drawable whose refresh rate is to be determined.
 * \param numerator    Numerator of the refresh rate.
 * \param demoninator  Denominator of the refresh rate.
 * \return  If the refresh rate for the specified display and drawable could
 *          be calculated, True is returned.  Otherwise False is returned.
 *
 * \note This function is implemented entirely client-side.  A lot of other
 *       functionality is required to export GLX_OML_sync_control, so on
 *       XFree86 this function can be called for direct-rendering contexts
 *       when GLX_OML_sync_control appears in the client extension string.
 */

_X_HIDDEN GLboolean
__glXGetMscRateOML(Display * dpy, GLXDrawable drawable,
                   int32_t * numerator, int32_t * denominator)
{
#if defined( GLX_DIRECT_RENDERING ) && defined( XF86VIDMODE )
   __GLXDRIdrawable *draw = GetGLXDRIDrawable(dpy, drawable);

   if (draw == NULL)
      return False;

   return __glxGetMscRate(draw, numerator, denominator);
#else
   (void) dpy;
   (void) drawable;
   (void) numerator;
   (void) denominator;
#endif
   return False;
}


static int64_t
__glXSwapBuffersMscOML(Display * dpy, GLXDrawable drawable,
                       int64_t target_msc, int64_t divisor, int64_t remainder)
{
   struct glx_context *gc = __glXGetCurrentContext();
#ifdef GLX_DIRECT_RENDERING
   __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
   struct glx_screen *psc = pdraw ? pdraw->psc : NULL;
#endif

   if (!gc) /* no GLX for this */
      return -1;

#ifdef GLX_DIRECT_RENDERING
   if (!pdraw || !gc->isDirect)
      return -1;
#endif

   /* The OML_sync_control spec says these should "generate a GLX_BAD_VALUE
    * error", but it also says "It [glXSwapBuffersMscOML] will return a value
    * of -1 if the function failed because of errors detected in the input
    * parameters"
    */
   if (divisor < 0 || remainder < 0 || target_msc < 0)
      return -1;
   if (divisor > 0 && remainder >= divisor)
      return -1;

   if (target_msc == 0 && divisor == 0 && remainder == 0)
      remainder = 1;

#ifdef GLX_DIRECT_RENDERING
   if (psc->driScreen && psc->driScreen->swapBuffers)
      return (*psc->driScreen->swapBuffers)(pdraw, target_msc, divisor,
					    remainder);
#endif

   return -1;
}


static Bool
__glXWaitForMscOML(Display * dpy, GLXDrawable drawable,
                   int64_t target_msc, int64_t divisor,
                   int64_t remainder, int64_t * ust,
                   int64_t * msc, int64_t * sbc)
{
#ifdef GLX_DIRECT_RENDERING
   __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
   struct glx_screen *psc = pdraw ? pdraw->psc : NULL;
   int ret;
#endif


   /* The OML_sync_control spec says these should "generate a GLX_BAD_VALUE
    * error", but the return type in the spec is Bool.
    */
   if (divisor < 0 || remainder < 0 || target_msc < 0)
      return False;
   if (divisor > 0 && remainder >= divisor)
      return False;

#ifdef GLX_DIRECT_RENDERING
   if (pdraw && psc->driScreen && psc->driScreen->waitForMSC) {
      ret = psc->driScreen->waitForMSC(pdraw, target_msc, divisor, remainder,
				       ust, msc, sbc);
      return ret;
   }
#endif

   return False;
}


static Bool
__glXWaitForSbcOML(Display * dpy, GLXDrawable drawable,
                   int64_t target_sbc, int64_t * ust,
                   int64_t * msc, int64_t * sbc)
{
#ifdef GLX_DIRECT_RENDERING
   __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
   struct glx_screen *psc = pdraw ? pdraw->psc : NULL;
   int ret;
#endif

   /* The OML_sync_control spec says this should "generate a GLX_BAD_VALUE
    * error", but the return type in the spec is Bool.
    */
   if (target_sbc < 0)
      return False;

#ifdef GLX_DIRECT_RENDERING
   if (pdraw && psc->driScreen && psc->driScreen->waitForSBC) {
      ret = psc->driScreen->waitForSBC(pdraw, target_sbc, ust, msc, sbc);
      return ret;
   }
#endif

   return False;
}

/*@}*/


/**
 * Mesa extension stubs.  These will help reduce portability problems.
 */
/*@{*/

/**
 * Release all buffers associated with the specified GLX drawable.
 *
 * \todo
 * This function was intended for stand-alone Mesa.  The issue there is that
 * the library doesn't get any notification when a window is closed.  In
 * DRI there is a similar but slightly different issue.  When GLX 1.3 is
 * supported, there are 3 different functions to destroy a drawable.  It
 * should be possible to create GLX protocol (or have it determine which
 * protocol to use based on the type of the drawable) to have one function
 * do the work of 3.  For the direct-rendering case, this function could
 * just call the driver's \c __DRIdrawableRec::destroyDrawable function.
 * This would reduce the frequency with which \c __driGarbageCollectDrawables
 * would need to be used.  This really should be done as part of the new DRI
 * interface work.
 *
 * \sa http://oss.sgi.com/projects/ogl-sample/registry/MESA/release_buffers.txt
 *     __driGarbageCollectDrawables
 *     glXDestroyGLXPixmap
 *     glXDestroyPbuffer glXDestroyPixmap glXDestroyWindow
 *     glXDestroyGLXPbufferSGIX glXDestroyGLXVideoSourceSGIX
 */
static Bool
__glXReleaseBuffersMESA(Display * dpy, GLXDrawable d)
{
   (void) dpy;
   (void) d;
   return False;
}


_X_EXPORT GLXPixmap
glXCreateGLXPixmapMESA(Display * dpy, XVisualInfo * visual,
                       Pixmap pixmap, Colormap cmap)
{
   (void) dpy;
   (void) visual;
   (void) pixmap;
   (void) cmap;
   return 0;
}

/*@}*/


/**
 * GLX_MESA_copy_sub_buffer
 */
#define X_GLXvop_CopySubBufferMESA 5154 /* temporary */
static void
__glXCopySubBufferMESA(Display * dpy, GLXDrawable drawable,
                       int x, int y, int width, int height)
{
   xGLXVendorPrivateReq *req;
   struct glx_context *gc;
   GLXContextTag tag;
   CARD32 *drawable_ptr;
   INT32 *x_ptr, *y_ptr, *w_ptr, *h_ptr;
   CARD8 opcode;

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
   if (pdraw != NULL) {
      struct glx_screen *psc = pdraw->psc;
      if (psc->driScreen->copySubBuffer != NULL) {
         glFlush();
         (*psc->driScreen->copySubBuffer) (pdraw, x, y, width, height);
      }

      return;
   }
#endif

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return;

   /*
    ** The calling thread may or may not have a current context.  If it
    ** does, send the context tag so the server can do a flush.
    */
   gc = __glXGetCurrentContext();
   if ((gc != NULL) && (dpy == gc->currentDpy) &&
       ((drawable == gc->currentDrawable) ||
        (drawable == gc->currentReadable))) {
      tag = gc->currentContextTag;
   }
   else {
      tag = 0;
   }

   LockDisplay(dpy);
   GetReqExtra(GLXVendorPrivate, sizeof(CARD32) + sizeof(INT32) * 4, req);
   req->reqType = opcode;
   req->glxCode = X_GLXVendorPrivate;
   req->vendorCode = X_GLXvop_CopySubBufferMESA;
   req->contextTag = tag;

   drawable_ptr = (CARD32 *) (req + 1);
   x_ptr = (INT32 *) (drawable_ptr + 1);
   y_ptr = (INT32 *) (drawable_ptr + 2);
   w_ptr = (INT32 *) (drawable_ptr + 3);
   h_ptr = (INT32 *) (drawable_ptr + 4);

   *drawable_ptr = drawable;
   *x_ptr = x;
   *y_ptr = y;
   *w_ptr = width;
   *h_ptr = height;

   UnlockDisplay(dpy);
   SyncHandle();
}

/*@{*/
static void
__glXBindTexImageEXT(Display * dpy,
                     GLXDrawable drawable, int buffer, const int *attrib_list)
{
   struct glx_context *gc = __glXGetCurrentContext();

   if (gc == NULL || gc->vtable->bind_tex_image == NULL)
      return;

   gc->vtable->bind_tex_image(dpy, drawable, buffer, attrib_list);
}

static void
__glXReleaseTexImageEXT(Display * dpy, GLXDrawable drawable, int buffer)
{
   struct glx_context *gc = __glXGetCurrentContext();

   if (gc == NULL || gc->vtable->release_tex_image == NULL)
      return;

   gc->vtable->release_tex_image(dpy, drawable, buffer);
}

/*@}*/

#endif /* GLX_USE_APPLEGL */

/**
 * \c strdup is actually not a standard ANSI C or POSIX routine.
 * Irix will not define it if ANSI mode is in effect.
 *
 * \sa strdup
 */
_X_HIDDEN char *
__glXstrdup(const char *str)
{
   char *copy;
   copy = (char *) Xmalloc(strlen(str) + 1);
   if (!copy)
      return NULL;
   strcpy(copy, str);
   return copy;
}

/*
** glXGetProcAddress support
*/

struct name_address_pair
{
   const char *Name;
   GLvoid *Address;
};

#define GLX_FUNCTION(f) { # f, (GLvoid *) f }
#define GLX_FUNCTION2(n,f) { # n, (GLvoid *) f }

static const struct name_address_pair GLX_functions[] = {
   /*** GLX_VERSION_1_0 ***/
   GLX_FUNCTION(glXChooseVisual),
   GLX_FUNCTION(glXCopyContext),
   GLX_FUNCTION(glXCreateContext),
   GLX_FUNCTION(glXCreateGLXPixmap),
   GLX_FUNCTION(glXDestroyContext),
   GLX_FUNCTION(glXDestroyGLXPixmap),
   GLX_FUNCTION(glXGetConfig),
   GLX_FUNCTION(glXGetCurrentContext),
   GLX_FUNCTION(glXGetCurrentDrawable),
   GLX_FUNCTION(glXIsDirect),
   GLX_FUNCTION(glXMakeCurrent),
   GLX_FUNCTION(glXQueryExtension),
   GLX_FUNCTION(glXQueryVersion),
   GLX_FUNCTION(glXSwapBuffers),
   GLX_FUNCTION(glXUseXFont),
   GLX_FUNCTION(glXWaitGL),
   GLX_FUNCTION(glXWaitX),

   /*** GLX_VERSION_1_1 ***/
   GLX_FUNCTION(glXGetClientString),
   GLX_FUNCTION(glXQueryExtensionsString),
   GLX_FUNCTION(glXQueryServerString),

   /*** GLX_VERSION_1_2 ***/
   GLX_FUNCTION(glXGetCurrentDisplay),

   /*** GLX_VERSION_1_3 ***/
   GLX_FUNCTION(glXChooseFBConfig),
   GLX_FUNCTION(glXCreateNewContext),
   GLX_FUNCTION(glXCreatePbuffer),
   GLX_FUNCTION(glXCreatePixmap),
   GLX_FUNCTION(glXCreateWindow),
   GLX_FUNCTION(glXDestroyPbuffer),
   GLX_FUNCTION(glXDestroyPixmap),
   GLX_FUNCTION(glXDestroyWindow),
   GLX_FUNCTION(glXGetCurrentReadDrawable),
   GLX_FUNCTION(glXGetFBConfigAttrib),
   GLX_FUNCTION(glXGetFBConfigs),
   GLX_FUNCTION(glXGetSelectedEvent),
   GLX_FUNCTION(glXGetVisualFromFBConfig),
   GLX_FUNCTION(glXMakeContextCurrent),
   GLX_FUNCTION(glXQueryContext),
   GLX_FUNCTION(glXQueryDrawable),
   GLX_FUNCTION(glXSelectEvent),

#ifndef GLX_USE_APPLEGL
   /*** GLX_SGI_swap_control ***/
   GLX_FUNCTION2(glXSwapIntervalSGI, __glXSwapIntervalSGI),

   /*** GLX_SGI_video_sync ***/
   GLX_FUNCTION2(glXGetVideoSyncSGI, __glXGetVideoSyncSGI),
   GLX_FUNCTION2(glXWaitVideoSyncSGI, __glXWaitVideoSyncSGI),

   /*** GLX_SGI_make_current_read ***/
   GLX_FUNCTION2(glXMakeCurrentReadSGI, glXMakeContextCurrent),
   GLX_FUNCTION2(glXGetCurrentReadDrawableSGI, glXGetCurrentReadDrawable),

   /*** GLX_EXT_import_context ***/
   GLX_FUNCTION(glXFreeContextEXT),
   GLX_FUNCTION(glXGetContextIDEXT),
   GLX_FUNCTION2(glXGetCurrentDisplayEXT, glXGetCurrentDisplay),
   GLX_FUNCTION(glXImportContextEXT),
   GLX_FUNCTION2(glXQueryContextInfoEXT, glXQueryContext),
#endif

   /*** GLX_SGIX_fbconfig ***/
   GLX_FUNCTION2(glXGetFBConfigAttribSGIX, glXGetFBConfigAttrib),
   GLX_FUNCTION2(glXChooseFBConfigSGIX, glXChooseFBConfig),
   GLX_FUNCTION(glXCreateGLXPixmapWithConfigSGIX),
   GLX_FUNCTION(glXCreateContextWithConfigSGIX),
   GLX_FUNCTION2(glXGetVisualFromFBConfigSGIX, glXGetVisualFromFBConfig),
   GLX_FUNCTION(glXGetFBConfigFromVisualSGIX),

#ifndef GLX_USE_APPLEGL
   /*** GLX_SGIX_pbuffer ***/
   GLX_FUNCTION(glXCreateGLXPbufferSGIX),
   GLX_FUNCTION(glXDestroyGLXPbufferSGIX),
   GLX_FUNCTION(glXQueryGLXPbufferSGIX),
   GLX_FUNCTION(glXSelectEventSGIX),
   GLX_FUNCTION(glXGetSelectedEventSGIX),

   /*** GLX_SGIX_swap_group ***/
   GLX_FUNCTION2(glXJoinSwapGroupSGIX, __glXJoinSwapGroupSGIX),

   /*** GLX_SGIX_swap_barrier ***/
   GLX_FUNCTION2(glXBindSwapBarrierSGIX, __glXBindSwapBarrierSGIX),
   GLX_FUNCTION2(glXQueryMaxSwapBarriersSGIX, __glXQueryMaxSwapBarriersSGIX),

   /*** GLX_MESA_copy_sub_buffer ***/
   GLX_FUNCTION2(glXCopySubBufferMESA, __glXCopySubBufferMESA),

   /*** GLX_MESA_pixmap_colormap ***/
   GLX_FUNCTION(glXCreateGLXPixmapMESA),

   /*** GLX_MESA_release_buffers ***/
   GLX_FUNCTION2(glXReleaseBuffersMESA, __glXReleaseBuffersMESA),

   /*** GLX_MESA_swap_control ***/
   GLX_FUNCTION2(glXSwapIntervalMESA, __glXSwapIntervalMESA),
   GLX_FUNCTION2(glXGetSwapIntervalMESA, __glXGetSwapIntervalMESA),
#endif

   /*** GLX_ARB_get_proc_address ***/
   GLX_FUNCTION(glXGetProcAddressARB),

   /*** GLX 1.4 ***/
   GLX_FUNCTION2(glXGetProcAddress, glXGetProcAddressARB),

#ifndef GLX_USE_APPLEGL
   /*** GLX_OML_sync_control ***/
   GLX_FUNCTION2(glXWaitForSbcOML, __glXWaitForSbcOML),
   GLX_FUNCTION2(glXWaitForMscOML, __glXWaitForMscOML),
   GLX_FUNCTION2(glXSwapBuffersMscOML, __glXSwapBuffersMscOML),
   GLX_FUNCTION2(glXGetMscRateOML, __glXGetMscRateOML),
   GLX_FUNCTION2(glXGetSyncValuesOML, __glXGetSyncValuesOML),

   /*** GLX_EXT_texture_from_pixmap ***/
   GLX_FUNCTION2(glXBindTexImageEXT, __glXBindTexImageEXT),
   GLX_FUNCTION2(glXReleaseTexImageEXT, __glXReleaseTexImageEXT),
#endif

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   /*** DRI configuration ***/
   GLX_FUNCTION(glXGetScreenDriver),
   GLX_FUNCTION(glXGetDriverConfig),
#endif

   /*** GLX_ARB_create_context and GLX_ARB_create_context_profile ***/
   GLX_FUNCTION(glXCreateContextAttribsARB),

   {NULL, NULL}                 /* end of list */
};

static const GLvoid *
get_glx_proc_address(const char *funcName)
{
   GLuint i;

   /* try static functions */
   for (i = 0; GLX_functions[i].Name; i++) {
      if (strcmp(GLX_functions[i].Name, funcName) == 0)
         return GLX_functions[i].Address;
   }

   return NULL;
}

/**
 * Get the address of a named GL function.  This is the pre-GLX 1.4 name for
 * \c glXGetProcAddress.
 *
 * \param procName  Name of a GL or GLX function.
 * \returns         A pointer to the named function
 *
 * \sa glXGetProcAddress
 */
_X_EXPORT void (*glXGetProcAddressARB(const GLubyte * procName)) (void)
{
   typedef void (*gl_function) (void);
   gl_function f;


   /* Search the table of GLX and internal functions first.  If that
    * fails and the supplied name could be a valid core GL name, try
    * searching the core GL function table.  This check is done to prevent
    * DRI based drivers from searching the core GL function table for
    * internal API functions.
    */
   f = (gl_function) get_glx_proc_address((const char *) procName);
   if ((f == NULL) && (procName[0] == 'g') && (procName[1] == 'l')
       && (procName[2] != 'X')) {
#ifdef GLX_SHARED_GLAPI
      f = (gl_function) __indirect_get_proc_address((const char *) procName);
#endif
      if (!f)
         f = (gl_function) _glapi_get_proc_address((const char *) procName);
      if (!f) {
         struct glx_context *gc = __glXGetCurrentContext();
      
         if (gc != NULL && gc->vtable->get_proc_address != NULL)
            f = gc->vtable->get_proc_address((const char *) procName);
      }
   }
   return f;
}

/**
 * Get the address of a named GL function.  This is the GLX 1.4 name for
 * \c glXGetProcAddressARB.
 *
 * \param procName  Name of a GL or GLX function.
 * \returns         A pointer to the named function
 *
 * \sa glXGetProcAddressARB
 */
_X_EXPORT void (*glXGetProcAddress(const GLubyte * procName)) (void)
#if defined(__GNUC__) && !defined(GLX_ALIAS_UNSUPPORTED)
   __attribute__ ((alias("glXGetProcAddressARB")));
#else
{
   return glXGetProcAddressARB(procName);
}
#endif /* __GNUC__ */


#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
/**
 * Get the unadjusted system time (UST).  Currently, the UST is measured in
 * microseconds since Epoc.  The actual resolution of the UST may vary from
 * system to system, and the units may vary from release to release.
 * Drivers should not call this function directly.  They should instead use
 * \c glXGetProcAddress to obtain a pointer to the function.
 *
 * \param ust Location to store the 64-bit UST
 * \returns Zero on success or a negative errno value on failure.
 *
 * \sa glXGetProcAddress, PFNGLXGETUSTPROC
 *
 * \since Internal API version 20030317.
 */
_X_HIDDEN int
__glXGetUST(int64_t * ust)
{
   struct timeval tv;

   if (ust == NULL) {
      return -EFAULT;
   }

   if (gettimeofday(&tv, NULL) == 0) {
      ust[0] = (tv.tv_sec * 1000000) + tv.tv_usec;
      return 0;
   }
   else {
      return -errno;
   }
}
#endif /* GLX_DIRECT_RENDERING */
