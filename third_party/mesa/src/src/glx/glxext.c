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
 * \file glxext.c
 * GLX protocol interface boot-strap code.
 *
 * Direct rendering support added by Precision Insight, Inc.
 *
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 */

#include <assert.h>
#include "glxclient.h"
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>
#ifdef GLX_USE_APPLEGL
#include "apple_glx.h"
#include "apple_visual.h"
#endif
#include "glxextensions.h"

#ifdef USE_XCB
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/glx.h>
#endif


#ifdef DEBUG
void __glXDumpDrawBuffer(struct glx_context * ctx);
#endif

/*
** You can set this cell to 1 to force the gl drawing stuff to be
** one command per packet
*/
_X_HIDDEN int __glXDebug = 0;

/* Extension required boiler plate */

static const char __glXExtensionName[] = GLX_EXTENSION_NAME;
  static struct glx_display *glx_displays;

static /* const */ char *error_list[] = {
   "GLXBadContext",
   "GLXBadContextState",
   "GLXBadDrawable",
   "GLXBadPixmap",
   "GLXBadContextTag",
   "GLXBadCurrentWindow",
   "GLXBadRenderRequest",
   "GLXBadLargeRequest",
   "GLXUnsupportedPrivateRequest",
   "GLXBadFBConfig",
   "GLXBadPbuffer",
   "GLXBadCurrentDrawable",
   "GLXBadWindow",
   "GLXBadProfileARB",
};

#ifdef GLX_USE_APPLEGL
static char *__glXErrorString(Display *dpy, int code, XExtCodes *codes, 
                              char *buf, int n);
#endif

static
XEXT_GENERATE_ERROR_STRING(__glXErrorString, __glXExtensionName,
                           __GLX_NUMBER_ERRORS, error_list)

/*
 * GLX events are a bit funky.  We don't stuff the X event code into
 * our user exposed (via XNextEvent) structure.  Instead we use the GLX
 * private event code namespace (and hope it doesn't conflict).  Clients
 * have to know that bit 15 in the event type field means they're getting
 * a GLX event, and then handle the various sub-event types there, rather
 * than simply checking the event code and handling it directly.
 */

static Bool
__glXWireToEvent(Display *dpy, XEvent *event, xEvent *wire)
{
     struct glx_display *glx_dpy = __glXInitialize(dpy);

   if (glx_dpy == NULL)
      return False;

   switch ((wire->u.u.type & 0x7f) - glx_dpy->codes->first_event) {
   case GLX_PbufferClobber:
   {
      GLXPbufferClobberEvent *aevent = (GLXPbufferClobberEvent *)event;
      xGLXPbufferClobberEvent *awire = (xGLXPbufferClobberEvent *)wire;
      aevent->event_type = awire->type;
      aevent->serial = awire->sequenceNumber;
      aevent->event_type = awire->event_type;
      aevent->draw_type = awire->draw_type;
      aevent->drawable = awire->drawable;
      aevent->buffer_mask = awire->buffer_mask;
      aevent->aux_buffer = awire->aux_buffer;
      aevent->x = awire->x;
      aevent->y = awire->y;
      aevent->width = awire->width;
      aevent->height = awire->height;
      aevent->count = awire->count;
      return True;
   }
   case GLX_BufferSwapComplete:
   {
      GLXBufferSwapComplete *aevent = (GLXBufferSwapComplete *)event;
      xGLXBufferSwapComplete2 *awire = (xGLXBufferSwapComplete2 *)wire;
      struct glx_drawable *glxDraw = GetGLXDrawable(dpy, awire->drawable);
      aevent->event_type = awire->event_type;
      aevent->drawable = awire->drawable;
      aevent->ust = ((CARD64)awire->ust_hi << 32) | awire->ust_lo;
      aevent->msc = ((CARD64)awire->msc_hi << 32) | awire->msc_lo;

      if (!glxDraw)
	 return False;

      if (awire->sbc < glxDraw->lastEventSbc)
	 glxDraw->eventSbcWrap += 0x100000000;
      glxDraw->lastEventSbc = awire->sbc;
      aevent->sbc = awire->sbc + glxDraw->eventSbcWrap;
      return True;
   }
   default:
      /* client doesn't support server event */
      break;
   }

   return False;
}

/* We don't actually support this.  It doesn't make sense for clients to
 * send each other GLX events.
 */
static Status
__glXEventToWire(Display *dpy, XEvent *event, xEvent *wire)
{
     struct glx_display *glx_dpy = __glXInitialize(dpy);

   if (glx_dpy == NULL)
      return False;

   switch (event->type) {
   case GLX_DAMAGED:
      break;
   case GLX_SAVED:
      break;
   case GLX_EXCHANGE_COMPLETE_INTEL:
      break;
   case GLX_COPY_COMPLETE_INTEL:
      break;
   case GLX_FLIP_COMPLETE_INTEL:
      break;
   default:
      /* client doesn't support server event */
      break;
   }

   return Success;
}

/************************************************************************/
/*
** Free the per screen configs data as well as the array of
** __glXScreenConfigs.
*/
static void
FreeScreenConfigs(struct glx_display * priv)
{
   struct glx_screen *psc;
   GLint i, screens;

   /* Free screen configuration information */
   screens = ScreenCount(priv->dpy);
   for (i = 0; i < screens; i++) {
      psc = priv->screens[i];
      glx_screen_cleanup(psc);

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      if (psc->driScreen) {
         psc->driScreen->destroyScreen(psc);
      } else {
	 Xfree(psc);
      }
#else
      Xfree(psc);
#endif
   }
   XFree((char *) priv->screens);
   priv->screens = NULL;
}

static void
glx_display_free(struct glx_display *priv)
{
   struct glx_context *gc;

   gc = __glXGetCurrentContext();
   if (priv->dpy == gc->currentDpy) {
      gc->vtable->destroy(gc);
      __glXSetCurrentContextNull();
   }

   FreeScreenConfigs(priv);
   if (priv->serverGLXvendor)
      Xfree((char *) priv->serverGLXvendor);
   if (priv->serverGLXversion)
      Xfree((char *) priv->serverGLXversion);

   __glxHashDestroy(priv->glXDrawHash);

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   __glxHashDestroy(priv->drawHash);

   /* Free the direct rendering per display data */
   if (priv->driswDisplay)
      (*priv->driswDisplay->destroyDisplay) (priv->driswDisplay);
   priv->driswDisplay = NULL;

   if (priv->driDisplay)
      (*priv->driDisplay->destroyDisplay) (priv->driDisplay);
   priv->driDisplay = NULL;

   if (priv->dri2Display)
      (*priv->dri2Display->destroyDisplay) (priv->dri2Display);
   priv->dri2Display = NULL;
#endif

   Xfree((char *) priv);
}

static int
__glXCloseDisplay(Display * dpy, XExtCodes * codes)
{
   struct glx_display *priv, **prev;

   _XLockMutex(_Xglobal_lock);
   prev = &glx_displays;
   for (priv = glx_displays; priv; prev = &priv->next, priv = priv->next) {
      if (priv->dpy == dpy) {
         *prev = priv->next;
	 break;
      }
   }
   _XUnlockMutex(_Xglobal_lock);

   glx_display_free(priv);

   return 1;
}

/*
** Query the version of the GLX extension.  This procedure works even if
** the client extension is not completely set up.
*/
static Bool
QueryVersion(Display * dpy, int opcode, int *major, int *minor)
{
#ifdef USE_XCB
   xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_glx_query_version_reply_t *reply = xcb_glx_query_version_reply(c,
                                                                      xcb_glx_query_version
                                                                      (c,
                                                                       GLX_MAJOR_VERSION,
                                                                       GLX_MINOR_VERSION),
                                                                      NULL);

   if (!reply)
     return GL_FALSE;

   if (reply->major_version != GLX_MAJOR_VERSION) {
      free(reply);
      return GL_FALSE;
   }
   *major = reply->major_version;
   *minor = min(reply->minor_version, GLX_MINOR_VERSION);
   free(reply);
   return GL_TRUE;
#else
   xGLXQueryVersionReq *req;
   xGLXQueryVersionReply reply;

   /* Send the glXQueryVersion request */
   LockDisplay(dpy);
   GetReq(GLXQueryVersion, req);
   req->reqType = opcode;
   req->glxCode = X_GLXQueryVersion;
   req->majorVersion = GLX_MAJOR_VERSION;
   req->minorVersion = GLX_MINOR_VERSION;
   _XReply(dpy, (xReply *) & reply, 0, False);
   UnlockDisplay(dpy);
   SyncHandle();

   if (reply.majorVersion != GLX_MAJOR_VERSION) {
      /*
       ** The server does not support the same major release as this
       ** client.
       */
      return GL_FALSE;
   }
   *major = reply.majorVersion;
   *minor = min(reply.minorVersion, GLX_MINOR_VERSION);
   return GL_TRUE;
#endif /* USE_XCB */
}

/* 
 * We don't want to enable this GLX_OML_swap_method in glxext.h, 
 * because we can't support it.  The X server writes it out though,
 * so we should handle it somehow, to avoid false warnings.
 */
enum {
    IGNORE_GLX_SWAP_METHOD_OML = 0x8060
};


static GLint
convert_from_x_visual_type(int visualType)
{
   static const int glx_visual_types[] = {
      GLX_STATIC_GRAY, GLX_GRAY_SCALE,
      GLX_STATIC_COLOR, GLX_PSEUDO_COLOR,
      GLX_TRUE_COLOR, GLX_DIRECT_COLOR
   };

   if (visualType < ARRAY_SIZE(glx_visual_types))
      return glx_visual_types[visualType];

   return GLX_NONE;
}

/*
 * getVisualConfigs uses the !tagged_only path.
 * getFBConfigs uses the tagged_only path.
 */
_X_HIDDEN void
__glXInitializeVisualConfigFromTags(struct glx_config * config, int count,
                                    const INT32 * bp, Bool tagged_only,
                                    Bool fbconfig_style_tags)
{
   int i;

   if (!tagged_only) {
      /* Copy in the first set of properties */
      config->visualID = *bp++;

      config->visualType = convert_from_x_visual_type(*bp++);

      config->rgbMode = *bp++;

      config->redBits = *bp++;
      config->greenBits = *bp++;
      config->blueBits = *bp++;
      config->alphaBits = *bp++;
      config->accumRedBits = *bp++;
      config->accumGreenBits = *bp++;
      config->accumBlueBits = *bp++;
      config->accumAlphaBits = *bp++;

      config->doubleBufferMode = *bp++;
      config->stereoMode = *bp++;

      config->rgbBits = *bp++;
      config->depthBits = *bp++;
      config->stencilBits = *bp++;
      config->numAuxBuffers = *bp++;
      config->level = *bp++;

#ifdef GLX_USE_APPLEGL
       /* AppleSGLX supports pixmap and pbuffers with all config. */
       config->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT;
       /* Unfortunately this can create an ABI compatibility problem. */
       count -= 18;
#else
      count -= __GLX_MIN_CONFIG_PROPS;
#endif
   }

   config->sRGBCapable = GL_FALSE;

   /*
    ** Additional properties may be in a list at the end
    ** of the reply.  They are in pairs of property type
    ** and property value.
    */

#define FETCH_OR_SET(tag) \
    config-> tag = ( fbconfig_style_tags ) ? *bp++ : 1

   for (i = 0; i < count; i += 2) {
      long int tag = *bp++;
      
      switch (tag) {
      case GLX_RGBA:
         FETCH_OR_SET(rgbMode);
         break;
      case GLX_BUFFER_SIZE:
         config->rgbBits = *bp++;
         break;
      case GLX_LEVEL:
         config->level = *bp++;
         break;
      case GLX_DOUBLEBUFFER:
         FETCH_OR_SET(doubleBufferMode);
         break;
      case GLX_STEREO:
         FETCH_OR_SET(stereoMode);
         break;
      case GLX_AUX_BUFFERS:
         config->numAuxBuffers = *bp++;
         break;
      case GLX_RED_SIZE:
         config->redBits = *bp++;
         break;
      case GLX_GREEN_SIZE:
         config->greenBits = *bp++;
         break;
      case GLX_BLUE_SIZE:
         config->blueBits = *bp++;
         break;
      case GLX_ALPHA_SIZE:
         config->alphaBits = *bp++;
         break;
      case GLX_DEPTH_SIZE:
         config->depthBits = *bp++;
         break;
      case GLX_STENCIL_SIZE:
         config->stencilBits = *bp++;
         break;
      case GLX_ACCUM_RED_SIZE:
         config->accumRedBits = *bp++;
         break;
      case GLX_ACCUM_GREEN_SIZE:
         config->accumGreenBits = *bp++;
         break;
      case GLX_ACCUM_BLUE_SIZE:
         config->accumBlueBits = *bp++;
         break;
      case GLX_ACCUM_ALPHA_SIZE:
         config->accumAlphaBits = *bp++;
         break;
      case GLX_VISUAL_CAVEAT_EXT:
         config->visualRating = *bp++;
         break;
      case GLX_X_VISUAL_TYPE:
         config->visualType = *bp++;
         break;
      case GLX_TRANSPARENT_TYPE:
         config->transparentPixel = *bp++;
         break;
      case GLX_TRANSPARENT_INDEX_VALUE:
         config->transparentIndex = *bp++;
         break;
      case GLX_TRANSPARENT_RED_VALUE:
         config->transparentRed = *bp++;
         break;
      case GLX_TRANSPARENT_GREEN_VALUE:
         config->transparentGreen = *bp++;
         break;
      case GLX_TRANSPARENT_BLUE_VALUE:
         config->transparentBlue = *bp++;
         break;
      case GLX_TRANSPARENT_ALPHA_VALUE:
         config->transparentAlpha = *bp++;
         break;
      case GLX_VISUAL_ID:
         config->visualID = *bp++;
         break;
      case GLX_DRAWABLE_TYPE:
         config->drawableType = *bp++;
#ifdef GLX_USE_APPLEGL
         /* AppleSGLX supports pixmap and pbuffers with all config. */
         config->drawableType |= GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT;              
#endif
         break;
      case GLX_RENDER_TYPE:
         config->renderType = *bp++;
         break;
      case GLX_X_RENDERABLE:
         config->xRenderable = *bp++;
         break;
      case GLX_FBCONFIG_ID:
         config->fbconfigID = *bp++;
         break;
      case GLX_MAX_PBUFFER_WIDTH:
         config->maxPbufferWidth = *bp++;
         break;
      case GLX_MAX_PBUFFER_HEIGHT:
         config->maxPbufferHeight = *bp++;
         break;
      case GLX_MAX_PBUFFER_PIXELS:
         config->maxPbufferPixels = *bp++;
         break;
#ifndef GLX_USE_APPLEGL
      case GLX_OPTIMAL_PBUFFER_WIDTH_SGIX:
         config->optimalPbufferWidth = *bp++;
         break;
      case GLX_OPTIMAL_PBUFFER_HEIGHT_SGIX:
         config->optimalPbufferHeight = *bp++;
         break;
      case GLX_VISUAL_SELECT_GROUP_SGIX:
         config->visualSelectGroup = *bp++;
         break;
      case GLX_SWAP_METHOD_OML:
         config->swapMethod = *bp++;
         break;
#endif
      case GLX_SAMPLE_BUFFERS_SGIS:
         config->sampleBuffers = *bp++;
         break;
      case GLX_SAMPLES_SGIS:
         config->samples = *bp++;
         break;
#ifdef GLX_USE_APPLEGL
      case IGNORE_GLX_SWAP_METHOD_OML:
         /* We ignore this tag.  See the comment above this function. */
         ++bp;
         break;
#else
      case GLX_BIND_TO_TEXTURE_RGB_EXT:
         config->bindToTextureRgb = *bp++;
         break;
      case GLX_BIND_TO_TEXTURE_RGBA_EXT:
         config->bindToTextureRgba = *bp++;
         break;
      case GLX_BIND_TO_MIPMAP_TEXTURE_EXT:
         config->bindToMipmapTexture = *bp++;
         break;
      case GLX_BIND_TO_TEXTURE_TARGETS_EXT:
         config->bindToTextureTargets = *bp++;
         break;
      case GLX_Y_INVERTED_EXT:
         config->yInverted = *bp++;
         break;
#endif
      case GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT:
         config->sRGBCapable = *bp++;
         break;

      case GLX_USE_GL:
         if (fbconfig_style_tags)
            bp++;
         break;
      case None:
         i = count;
         break;
      default:
         if(getenv("LIBGL_DIAGNOSTIC")) {
             long int tagvalue = *bp++;
             fprintf(stderr, "WARNING: unknown GLX tag from server: "
                     "tag 0x%lx value 0x%lx\n", tag, tagvalue);
         } else {
             /* Ignore the unrecognized tag's value */
             bp++;
         }
         break;
      }
   }

   config->renderType =
      (config->rgbMode) ? GLX_RGBA_BIT : GLX_COLOR_INDEX_BIT;
}

static struct glx_config *
createConfigsFromProperties(Display * dpy, int nvisuals, int nprops,
                            int screen, GLboolean tagged_only)
{
   INT32 buf[__GLX_TOTAL_CONFIG], *props;
   unsigned prop_size;
   struct glx_config *modes, *m;
   int i;

   if (nprops == 0)
      return NULL;

   /* FIXME: Is the __GLX_MIN_CONFIG_PROPS test correct for FBconfigs? */

   /* Check number of properties */
   if (nprops < __GLX_MIN_CONFIG_PROPS || nprops > __GLX_MAX_CONFIG_PROPS)
      return NULL;

   /* Allocate memory for our config structure */
   modes = glx_config_create_list(nvisuals);
   if (!modes)
      return NULL;

   prop_size = nprops * __GLX_SIZE_INT32;
   if (prop_size <= sizeof(buf))
      props = buf;
   else
      props = Xmalloc(prop_size);

   /* Read each config structure and convert it into our format */
   m = modes;
   for (i = 0; i < nvisuals; i++) {
      _XRead(dpy, (char *) props, prop_size);
#ifdef GLX_USE_APPLEGL
       /* Older X servers don't send this so we default it here. */
      m->drawableType = GLX_WINDOW_BIT;
#else
      /* 
       * The XQuartz 2.3.2.1 X server doesn't set this properly, so
       * set the proper bits here.
       * AppleSGLX supports windows, pixmaps, and pbuffers with all config.
       */
      m->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT;
#endif
       __glXInitializeVisualConfigFromTags(m, nprops, props,
                                          tagged_only, GL_TRUE);
      m->screen = screen;
      m = m->next;
   }

   if (props != buf)
      Xfree(props);

   return modes;
}

static GLboolean
getVisualConfigs(struct glx_screen *psc,
		  struct glx_display *priv, int screen)
{
   xGLXGetVisualConfigsReq *req;
   xGLXGetVisualConfigsReply reply;
   Display *dpy = priv->dpy;

   LockDisplay(dpy);

   psc->visuals = NULL;
   GetReq(GLXGetVisualConfigs, req);
   req->reqType = priv->majorOpcode;
   req->glxCode = X_GLXGetVisualConfigs;
   req->screen = screen;

   if (!_XReply(dpy, (xReply *) & reply, 0, False))
      goto out;

   psc->visuals = createConfigsFromProperties(dpy,
                                              reply.numVisuals,
                                              reply.numProps,
                                              screen, GL_FALSE);

 out:
   UnlockDisplay(dpy);
   return psc->visuals != NULL;
}

static GLboolean
 getFBConfigs(struct glx_screen *psc, struct glx_display *priv, int screen)
{
   xGLXGetFBConfigsReq *fb_req;
   xGLXGetFBConfigsSGIXReq *sgi_req;
   xGLXVendorPrivateWithReplyReq *vpreq;
   xGLXGetFBConfigsReply reply;
   Display *dpy = priv->dpy;

   psc->serverGLXexts =
      __glXQueryServerString(dpy, priv->majorOpcode, screen, GLX_EXTENSIONS);

   LockDisplay(dpy);

   psc->configs = NULL;
   if (atof(priv->serverGLXversion) >= 1.3) {
      GetReq(GLXGetFBConfigs, fb_req);
      fb_req->reqType = priv->majorOpcode;
      fb_req->glxCode = X_GLXGetFBConfigs;
      fb_req->screen = screen;
   }
   else if (strstr(psc->serverGLXexts, "GLX_SGIX_fbconfig") != NULL) {
      GetReqExtra(GLXVendorPrivateWithReply,
                  sz_xGLXGetFBConfigsSGIXReq -
                  sz_xGLXVendorPrivateWithReplyReq, vpreq);
      sgi_req = (xGLXGetFBConfigsSGIXReq *) vpreq;
      sgi_req->reqType = priv->majorOpcode;
      sgi_req->glxCode = X_GLXVendorPrivateWithReply;
      sgi_req->vendorCode = X_GLXvop_GetFBConfigsSGIX;
      sgi_req->screen = screen;
   }
   else
      goto out;

   if (!_XReply(dpy, (xReply *) & reply, 0, False))
      goto out;

   psc->configs = createConfigsFromProperties(dpy,
                                              reply.numFBConfigs,
                                              reply.numAttribs * 2,
                                              screen, GL_TRUE);

 out:
   UnlockDisplay(dpy);
   return psc->configs != NULL;
}

_X_HIDDEN Bool
glx_screen_init(struct glx_screen *psc,
		 int screen, struct glx_display * priv)
{
   /* Initialize per screen dynamic client GLX extensions */
   psc->ext_list_first_time = GL_TRUE;
   psc->scr = screen;
   psc->dpy = priv->dpy;
   psc->display = priv;

   getVisualConfigs(psc, priv, screen);
   getFBConfigs(psc, priv, screen);

   return GL_TRUE;
}

_X_HIDDEN void
glx_screen_cleanup(struct glx_screen *psc)
{
   if (psc->configs) {
      glx_config_destroy_list(psc->configs);
      if (psc->effectiveGLXexts)
          Xfree(psc->effectiveGLXexts);
      psc->configs = NULL;   /* NOTE: just for paranoia */
   }
   if (psc->visuals) {
      glx_config_destroy_list(psc->visuals);
      psc->visuals = NULL;   /* NOTE: just for paranoia */
   }
   Xfree((char *) psc->serverGLXexts);
}

/*
** Allocate the memory for the per screen configs for each screen.
** If that works then fetch the per screen configs data.
*/
static Bool
AllocAndFetchScreenConfigs(Display * dpy, struct glx_display * priv)
{
   struct glx_screen *psc;
   GLint i, screens;

   /*
    ** First allocate memory for the array of per screen configs.
    */
   screens = ScreenCount(dpy);
   priv->screens = Xmalloc(screens * sizeof *priv->screens);
   if (!priv->screens)
      return GL_FALSE;

   priv->serverGLXversion =
      __glXQueryServerString(dpy, priv->majorOpcode, 0, GLX_VERSION);
   if (priv->serverGLXversion == NULL) {
      FreeScreenConfigs(priv);
      return GL_FALSE;
   }

   for (i = 0; i < screens; i++, psc++) {
      psc = NULL;
#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      if (priv->dri2Display)
	 psc = (*priv->dri2Display->createScreen) (i, priv);
      if (psc == NULL && priv->driDisplay)
	 psc = (*priv->driDisplay->createScreen) (i, priv);
      if (psc == NULL && priv->driswDisplay)
	 psc = (*priv->driswDisplay->createScreen) (i, priv);
#endif
#if defined(GLX_USE_APPLEGL)
      if (psc == NULL)
         psc = applegl_create_screen(i, priv);
#else
      if (psc == NULL)
	 psc = indirect_create_screen(i, priv);
#endif
      priv->screens[i] = psc;
   }
   SyncHandle();
   return GL_TRUE;
}

/*
** Initialize the client side extension code.
*/
 _X_HIDDEN struct glx_display *
__glXInitialize(Display * dpy)
{
   struct glx_display *dpyPriv, *d;
#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   Bool glx_direct, glx_accel;
#endif
   int i;

   _XLockMutex(_Xglobal_lock);

   for (dpyPriv = glx_displays; dpyPriv; dpyPriv = dpyPriv->next) {
      if (dpyPriv->dpy == dpy) {
	 _XUnlockMutex(_Xglobal_lock);
	 return dpyPriv;
      }
   }

   /* Drop the lock while we create the display private. */
   _XUnlockMutex(_Xglobal_lock);

   dpyPriv = Xcalloc(1, sizeof *dpyPriv);
   if (!dpyPriv)
      return NULL;

   dpyPriv->codes = XInitExtension(dpy, __glXExtensionName);
   if (!dpyPriv->codes) {
      Xfree(dpyPriv);
      _XUnlockMutex(_Xglobal_lock);
      return NULL;
   }

   dpyPriv->dpy = dpy;
   dpyPriv->majorOpcode = dpyPriv->codes->major_opcode;
   dpyPriv->serverGLXvendor = 0x0;
   dpyPriv->serverGLXversion = 0x0;

   /* See if the versions are compatible.  This GLX implementation does not
    * work with servers that only support GLX 1.0.
    */
   if (!QueryVersion(dpy, dpyPriv->majorOpcode,
		     &dpyPriv->majorVersion, &dpyPriv->minorVersion)
       || (dpyPriv->majorVersion == 1 && dpyPriv->minorVersion < 1)) {
      Xfree(dpyPriv);
      _XUnlockMutex(_Xglobal_lock);
      return NULL;
   }

   for (i = 0; i < __GLX_NUMBER_EVENTS; i++) {
      XESetWireToEvent(dpy, dpyPriv->codes->first_event + i, __glXWireToEvent);
      XESetEventToWire(dpy, dpyPriv->codes->first_event + i, __glXEventToWire);
   }

   XESetCloseDisplay(dpy, dpyPriv->codes->extension, __glXCloseDisplay);
   XESetErrorString (dpy, dpyPriv->codes->extension,__glXErrorString);

   dpyPriv->glXDrawHash = __glxHashCreate();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   glx_direct = (getenv("LIBGL_ALWAYS_INDIRECT") == NULL);
   glx_accel = (getenv("LIBGL_ALWAYS_SOFTWARE") == NULL);

   dpyPriv->drawHash = __glxHashCreate();

   /*
    ** Initialize the direct rendering per display data and functions.
    ** Note: This _must_ be done before calling any other DRI routines
    ** (e.g., those called in AllocAndFetchScreenConfigs).
    */
   if (glx_direct && glx_accel) {
      dpyPriv->dri2Display = dri2CreateDisplay(dpy);
      dpyPriv->driDisplay = driCreateDisplay(dpy);
   }
   if (glx_direct)
      dpyPriv->driswDisplay = driswCreateDisplay(dpy);
#endif

#ifdef GLX_USE_APPLEGL
   if (!applegl_create_display(dpyPriv)) {
      Xfree(dpyPriv);
      return NULL;
   }
#endif
   if (!AllocAndFetchScreenConfigs(dpy, dpyPriv)) {
      Xfree(dpyPriv);
      return NULL;
   }

#ifdef USE_XCB
   __glX_send_client_info(dpyPriv);
#else
   __glXClientInfo(dpy, dpyPriv->majorOpcode);
#endif

   /* Grab the lock again and add the dispay private, unless somebody
    * beat us to initializing on this display in the meantime. */
   _XLockMutex(_Xglobal_lock);

   for (d = glx_displays; d; d = d->next) {
      if (d->dpy == dpy) {
	 _XUnlockMutex(_Xglobal_lock);
	 glx_display_free(dpyPriv);
	 return d;
      }
   }

   dpyPriv->next = glx_displays;
   glx_displays = dpyPriv;

    _XUnlockMutex(_Xglobal_lock);

   return dpyPriv;
}

/*
** Setup for sending a GLX command on dpy.  Make sure the extension is
** initialized.  Try to avoid calling __glXInitialize as its kinda slow.
*/
_X_HIDDEN CARD8
__glXSetupForCommand(Display * dpy)
{
    struct glx_context *gc;
    struct glx_display *priv;

   /* If this thread has a current context, flush its rendering commands */
   gc = __glXGetCurrentContext();
   if (gc->currentDpy) {
      /* Flush rendering buffer of the current context, if any */
      (void) __glXFlushRenderBuffer(gc, gc->pc);

      if (gc->currentDpy == dpy) {
         /* Use opcode from gc because its right */
         return gc->majorOpcode;
      }
      else {
         /*
          ** Have to get info about argument dpy because it might be to
          ** a different server
          */
      }
   }

   /* Forced to lookup extension via the slow initialize route */
   priv = __glXInitialize(dpy);
   if (!priv) {
      return 0;
   }
   return priv->majorOpcode;
}

/**
 * Flush the drawing command transport buffer.
 *
 * \param ctx  Context whose transport buffer is to be flushed.
 * \param pc   Pointer to first unused buffer location.
 *
 * \todo
 * Modify this function to use \c ctx->pc instead of the explicit
 * \c pc parameter.
 */
_X_HIDDEN GLubyte *
__glXFlushRenderBuffer(struct glx_context * ctx, GLubyte * pc)
{
   Display *const dpy = ctx->currentDpy;
#ifdef USE_XCB
   xcb_connection_t *c = XGetXCBConnection(dpy);
#else
   xGLXRenderReq *req;
#endif /* USE_XCB */
   const GLint size = pc - ctx->buf;

   if ((dpy != NULL) && (size > 0)) {
#ifdef USE_XCB
      xcb_glx_render(c, ctx->currentContextTag, size,
                     (const uint8_t *) ctx->buf);
#else
      /* Send the entire buffer as an X request */
      LockDisplay(dpy);
      GetReq(GLXRender, req);
      req->reqType = ctx->majorOpcode;
      req->glxCode = X_GLXRender;
      req->contextTag = ctx->currentContextTag;
      req->length += (size + 3) >> 2;
      _XSend(dpy, (char *) ctx->buf, size);
      UnlockDisplay(dpy);
      SyncHandle();
#endif
   }

   /* Reset pointer and return it */
   ctx->pc = ctx->buf;
   return ctx->pc;
}


/**
 * Send a portion of a GLXRenderLarge command to the server.  The advantage of
 * this function over \c __glXSendLargeCommand is that callers can use the
 * data buffer in the GLX context and may be able to avoid allocating an
 * extra buffer.  The disadvantage is the clients will have to do more
 * GLX protocol work (i.e., calculating \c totalRequests, etc.).
 *
 * \sa __glXSendLargeCommand
 *
 * \param gc             GLX context
 * \param requestNumber  Which part of the whole command is this?  The first
 *                       request is 1.
 * \param totalRequests  How many requests will there be?
 * \param data           Command data.
 * \param dataLen        Size, in bytes, of the command data.
 */
_X_HIDDEN void
__glXSendLargeChunk(struct glx_context * gc, GLint requestNumber,
                    GLint totalRequests, const GLvoid * data, GLint dataLen)
{
   Display *dpy = gc->currentDpy;
#ifdef USE_XCB
   xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_glx_render_large(c, gc->currentContextTag, requestNumber,
                        totalRequests, dataLen, data);
#else
   xGLXRenderLargeReq *req;

   if (requestNumber == 1) {
      LockDisplay(dpy);
   }

   GetReq(GLXRenderLarge, req);
   req->reqType = gc->majorOpcode;
   req->glxCode = X_GLXRenderLarge;
   req->contextTag = gc->currentContextTag;
   req->length += (dataLen + 3) >> 2;
   req->requestNumber = requestNumber;
   req->requestTotal = totalRequests;
   req->dataBytes = dataLen;
   Data(dpy, data, dataLen);

   if (requestNumber == totalRequests) {
      UnlockDisplay(dpy);
      SyncHandle();
   }
#endif /* USE_XCB */
}


/**
 * Send a command that is too large for the GLXRender protocol request.
 *
 * Send a large command, one that is too large for some reason to
 * send using the GLXRender protocol request.  One reason to send
 * a large command is to avoid copying the data.
 *
 * \param ctx        GLX context
 * \param header     Header data.
 * \param headerLen  Size, in bytes, of the header data.  It is assumed that
 *                   the header data will always be small enough to fit in
 *                   a single X protocol packet.
 * \param data       Command data.
 * \param dataLen    Size, in bytes, of the command data.
 */
_X_HIDDEN void
__glXSendLargeCommand(struct glx_context * ctx,
                      const GLvoid * header, GLint headerLen,
                      const GLvoid * data, GLint dataLen)
{
   GLint maxSize;
   GLint totalRequests, requestNumber;

   /*
    ** Calculate the maximum amount of data can be stuffed into a single
    ** packet.  sz_xGLXRenderReq is added because bufSize is the maximum
    ** packet size minus sz_xGLXRenderReq.
    */
   maxSize = (ctx->bufSize + sz_xGLXRenderReq) - sz_xGLXRenderLargeReq;
   totalRequests = 1 + (dataLen / maxSize);
   if (dataLen % maxSize)
      totalRequests++;

   /*
    ** Send all of the command, except the large array, as one request.
    */
   assert(headerLen <= maxSize);
   __glXSendLargeChunk(ctx, 1, totalRequests, header, headerLen);

   /*
    ** Send enough requests until the whole array is sent.
    */
   for (requestNumber = 2; requestNumber <= (totalRequests - 1);
        requestNumber++) {
      __glXSendLargeChunk(ctx, requestNumber, totalRequests, data, maxSize);
      data = (const GLvoid *) (((const GLubyte *) data) + maxSize);
      dataLen -= maxSize;
      assert(dataLen > 0);
   }

   assert(dataLen <= maxSize);
   __glXSendLargeChunk(ctx, requestNumber, totalRequests, data, dataLen);
}

/************************************************************************/

#ifdef DEBUG
_X_HIDDEN void
__glXDumpDrawBuffer(struct glx_context * ctx)
{
   GLubyte *p = ctx->buf;
   GLubyte *end = ctx->pc;
   GLushort opcode, length;

   while (p < end) {
      /* Fetch opcode */
      opcode = *((GLushort *) p);
      length = *((GLushort *) (p + 2));
      printf("%2x: %5d: ", opcode, length);
      length -= 4;
      p += 4;
      while (length > 0) {
         printf("%08x ", *((unsigned *) p));
         p += 4;
         length -= 4;
      }
      printf("\n");
   }
}
#endif
