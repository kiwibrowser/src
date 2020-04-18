/*
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@redhat.com)
 */

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include "glapi.h"
#include "glxclient.h"
#include <X11/extensions/dri2proto.h>
#include "xf86dri.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "xf86drm.h"
#include "dri2.h"
#include "dri_common.h"

/* From xmlpool/options.h, user exposed so should be stable */
#define DRI_CONF_VBLANK_NEVER 0
#define DRI_CONF_VBLANK_DEF_INTERVAL_0 1
#define DRI_CONF_VBLANK_DEF_INTERVAL_1 2
#define DRI_CONF_VBLANK_ALWAYS_SYNC 3

#undef DRI2_MINOR
#define DRI2_MINOR 1

struct dri2_display
{
   __GLXDRIdisplay base;

   /*
    ** XFree86-DRI version information
    */
   int driMajor;
   int driMinor;
   int driPatch;
   int swapAvailable;
   int invalidateAvailable;

   __glxHashTable *dri2Hash;

   const __DRIextension *loader_extensions[4];
};

struct dri2_screen {
   struct glx_screen base;

   __DRIscreen *driScreen;
   __GLXDRIscreen vtable;
   const __DRIdri2Extension *dri2;
   const __DRIcoreExtension *core;

   const __DRI2flushExtension *f;
   const __DRI2configQueryExtension *config;
   const __DRItexBufferExtension *texBuffer;
   const __DRI2throttleExtension *throttle;
   const __DRIconfig **driver_configs;

   void *driver;
   int fd;

   Bool show_fps;
};

struct dri2_context
{
   struct glx_context base;
   __DRIcontext *driContext;
};

struct dri2_drawable
{
   __GLXDRIdrawable base;
   __DRIdrawable *driDrawable;
   __DRIbuffer buffers[5];
   int bufferCount;
   int width, height;
   int have_back;
   int have_fake_front;
   int swap_interval;

   double previous_time;
   unsigned frames;
};

static const struct glx_context_vtable dri2_context_vtable;

static void
dri2_destroy_context(struct glx_context *context)
{
   struct dri2_context *pcp = (struct dri2_context *) context;
   struct dri2_screen *psc = (struct dri2_screen *) context->psc;

   driReleaseDrawables(&pcp->base);

   if (context->extensions)
      XFree((char *) context->extensions);

   (*psc->core->destroyContext) (pcp->driContext);

   Xfree(pcp);
}

static Bool
dri2_bind_context(struct glx_context *context, struct glx_context *old,
		  GLXDrawable draw, GLXDrawable read)
{
   struct dri2_context *pcp = (struct dri2_context *) context;
   struct dri2_screen *psc = (struct dri2_screen *) pcp->base.psc;
   struct dri2_drawable *pdraw, *pread;
   struct dri2_display *pdp;

   pdraw = (struct dri2_drawable *) driFetchDrawable(context, draw);
   pread = (struct dri2_drawable *) driFetchDrawable(context, read);

   driReleaseDrawables(&pcp->base);

   if (pdraw == NULL || pread == NULL)
      return GLXBadDrawable;

   if (!(*psc->core->bindContext) (pcp->driContext,
				   pdraw->driDrawable, pread->driDrawable))
      return GLXBadContext;

   /* If the server doesn't send invalidate events, we may miss a
    * resize before the rendering starts.  Invalidate the buffers now
    * so the driver will recheck before rendering starts. */
   pdp = (struct dri2_display *) psc->base.display;
   if (!pdp->invalidateAvailable) {
      dri2InvalidateBuffers(psc->base.dpy, pdraw->base.xDrawable);
      if (pread != pdraw)
	 dri2InvalidateBuffers(psc->base.dpy, pread->base.xDrawable);
   }

   return Success;
}

static void
dri2_unbind_context(struct glx_context *context, struct glx_context *new)
{
   struct dri2_context *pcp = (struct dri2_context *) context;
   struct dri2_screen *psc = (struct dri2_screen *) pcp->base.psc;

   (*psc->core->unbindContext) (pcp->driContext);
}

static struct glx_context *
dri2_create_context(struct glx_screen *base,
		    struct glx_config *config_base,
		    struct glx_context *shareList, int renderType)
{
   struct dri2_context *pcp, *pcp_shared;
   struct dri2_screen *psc = (struct dri2_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   __DRIcontext *shared = NULL;

   if (shareList) {
      /* If the shareList context is not a DRI2 context, we cannot possibly
       * create a DRI2 context that shares it.
       */
      if (shareList->vtable->destroy != dri2_destroy_context) {
	 return NULL;
      }

      pcp_shared = (struct dri2_context *) shareList;
      shared = pcp_shared->driContext;
   }

   pcp = Xmalloc(sizeof *pcp);
   if (pcp == NULL)
      return NULL;

   memset(pcp, 0, sizeof *pcp);
   if (!glx_context_init(&pcp->base, &psc->base, &config->base)) {
      Xfree(pcp);
      return NULL;
   }

   pcp->driContext =
      (*psc->dri2->createNewContext) (psc->driScreen,
                                      config->driConfig, shared, pcp);

   if (pcp->driContext == NULL) {
      Xfree(pcp);
      return NULL;
   }

   pcp->base.vtable = &dri2_context_vtable;

   return &pcp->base;
}

static struct glx_context *
dri2_create_context_attribs(struct glx_screen *base,
			    struct glx_config *config_base,
			    struct glx_context *shareList,
			    unsigned num_attribs,
			    const uint32_t *attribs,
			    unsigned *error)
{
   struct dri2_context *pcp = NULL;
   struct dri2_context *pcp_shared = NULL;
   struct dri2_screen *psc = (struct dri2_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   __DRIcontext *shared = NULL;

   uint32_t minor_ver = 1;
   uint32_t major_ver = 2;
   uint32_t flags = 0;
   unsigned api;
   int reset = __DRI_CTX_RESET_NO_NOTIFICATION;
   uint32_t ctx_attribs[2 * 5];
   unsigned num_ctx_attribs = 0;

   if (psc->dri2->base.version < 3) {
      *error = __DRI_CTX_ERROR_NO_MEMORY;
      goto error_exit;
   }

   /* Remap the GLX tokens to DRI2 tokens.
    */
   if (!dri2_convert_glx_attribs(num_attribs, attribs,
				 &major_ver, &minor_ver, &flags, &api, &reset,
                                 error))
      goto error_exit;

   if (shareList) {
      pcp_shared = (struct dri2_context *) shareList;
      shared = pcp_shared->driContext;
   }

   pcp = Xmalloc(sizeof *pcp);
   if (pcp == NULL) {
      *error = __DRI_CTX_ERROR_NO_MEMORY;
      goto error_exit;
   }

   memset(pcp, 0, sizeof *pcp);
   if (!glx_context_init(&pcp->base, &psc->base, &config->base))
      goto error_exit;

   ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_MAJOR_VERSION;
   ctx_attribs[num_ctx_attribs++] = major_ver;
   ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_MINOR_VERSION;
   ctx_attribs[num_ctx_attribs++] = minor_ver;

   /* Only send a value when the non-default value is requested.  By doing
    * this we don't have to check the driver's DRI2 version before sending the
    * default value.
    */
   if (reset != __DRI_CTX_RESET_NO_NOTIFICATION) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_RESET_STRATEGY;
      ctx_attribs[num_ctx_attribs++] = reset;
   }

   if (flags != 0) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_FLAGS;

      /* The current __DRI_CTX_FLAG_* values are identical to the
       * GLX_CONTEXT_*_BIT values.
       */
      ctx_attribs[num_ctx_attribs++] = flags;
   }

   pcp->driContext =
      (*psc->dri2->createContextAttribs) (psc->driScreen,
					  api,
					  config->driConfig,
					  shared,
					  num_ctx_attribs / 2,
					  ctx_attribs,
					  error,
					  pcp);

   if (pcp->driContext == NULL)
      goto error_exit;

   pcp->base.vtable = &dri2_context_vtable;

   return &pcp->base;

error_exit:
   if (pcp != NULL)
      Xfree(pcp);

   return NULL;
}

static void
dri2DestroyDrawable(__GLXDRIdrawable *base)
{
   struct dri2_screen *psc = (struct dri2_screen *) base->psc;
   struct dri2_drawable *pdraw = (struct dri2_drawable *) base;
   struct glx_display *dpyPriv = psc->base.display;
   struct dri2_display *pdp = (struct dri2_display *)dpyPriv->dri2Display;

   __glxHashDelete(pdp->dri2Hash, pdraw->base.xDrawable);
   (*psc->core->destroyDrawable) (pdraw->driDrawable);

   /* If it's a GLX 1.3 drawables, we can destroy the DRI2 drawable
    * now, as the application explicitly asked to destroy the GLX
    * drawable.  Otherwise, for legacy drawables, we let the DRI2
    * drawable linger on the server, since there's no good way of
    * knowing when the application is done with it.  The server will
    * destroy the DRI2 drawable when it destroys the X drawable or the
    * client exits anyway. */
   if (pdraw->base.xDrawable != pdraw->base.drawable)
      DRI2DestroyDrawable(psc->base.dpy, pdraw->base.xDrawable);

   Xfree(pdraw);
}

static __GLXDRIdrawable *
dri2CreateDrawable(struct glx_screen *base, XID xDrawable,
		   GLXDrawable drawable, struct glx_config *config_base)
{
   struct dri2_drawable *pdraw;
   struct dri2_screen *psc = (struct dri2_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   struct glx_display *dpyPriv;
   struct dri2_display *pdp;
   GLint vblank_mode = DRI_CONF_VBLANK_DEF_INTERVAL_1;

   pdraw = Xmalloc(sizeof(*pdraw));
   if (!pdraw)
      return NULL;

   memset(pdraw, 0, sizeof *pdraw);
   pdraw->base.destroyDrawable = dri2DestroyDrawable;
   pdraw->base.xDrawable = xDrawable;
   pdraw->base.drawable = drawable;
   pdraw->base.psc = &psc->base;
   pdraw->bufferCount = 0;
   pdraw->swap_interval = 1; /* default may be overridden below */
   pdraw->have_back = 0;

   if (psc->config)
      psc->config->configQueryi(psc->driScreen,
				"vblank_mode", &vblank_mode);

   switch (vblank_mode) {
   case DRI_CONF_VBLANK_NEVER:
   case DRI_CONF_VBLANK_DEF_INTERVAL_0:
      pdraw->swap_interval = 0;
      break;
   case DRI_CONF_VBLANK_DEF_INTERVAL_1:
   case DRI_CONF_VBLANK_ALWAYS_SYNC:
   default:
      pdraw->swap_interval = 1;
      break;
   }

   DRI2CreateDrawable(psc->base.dpy, xDrawable);

   dpyPriv = __glXInitialize(psc->base.dpy);
   pdp = (struct dri2_display *)dpyPriv->dri2Display;;
   /* Create a new drawable */
   pdraw->driDrawable =
      (*psc->dri2->createNewDrawable) (psc->driScreen,
                                       config->driConfig, pdraw);

   if (!pdraw->driDrawable) {
      DRI2DestroyDrawable(psc->base.dpy, xDrawable);
      Xfree(pdraw);
      return NULL;
   }

   if (__glxHashInsert(pdp->dri2Hash, xDrawable, pdraw)) {
      (*psc->core->destroyDrawable) (pdraw->driDrawable);
      DRI2DestroyDrawable(psc->base.dpy, xDrawable);
      Xfree(pdraw);
      return None;
   }


#ifdef X_DRI2SwapInterval
   /*
    * Make sure server has the same swap interval we do for the new
    * drawable.
    */
   if (pdp->swapAvailable)
      DRI2SwapInterval(psc->base.dpy, xDrawable, pdraw->swap_interval);
#endif

   return &pdraw->base;
}

#ifdef X_DRI2GetMSC

static int
dri2DrawableGetMSC(struct glx_screen *psc, __GLXDRIdrawable *pdraw,
		   int64_t *ust, int64_t *msc, int64_t *sbc)
{
   CARD64 dri2_ust, dri2_msc, dri2_sbc;
   int ret;

   ret = DRI2GetMSC(psc->dpy, pdraw->xDrawable,
		    &dri2_ust, &dri2_msc, &dri2_sbc);
   *ust = dri2_ust;
   *msc = dri2_msc;
   *sbc = dri2_sbc;

   return ret;
}

#endif


#ifdef X_DRI2WaitMSC

static int
dri2WaitForMSC(__GLXDRIdrawable *pdraw, int64_t target_msc, int64_t divisor,
	       int64_t remainder, int64_t *ust, int64_t *msc, int64_t *sbc)
{
   CARD64 dri2_ust, dri2_msc, dri2_sbc;
   int ret;

   ret = DRI2WaitMSC(pdraw->psc->dpy, pdraw->xDrawable, target_msc, divisor,
		     remainder, &dri2_ust, &dri2_msc, &dri2_sbc);
   *ust = dri2_ust;
   *msc = dri2_msc;
   *sbc = dri2_sbc;

   return ret;
}

static int
dri2WaitForSBC(__GLXDRIdrawable *pdraw, int64_t target_sbc, int64_t *ust,
	       int64_t *msc, int64_t *sbc)
{
   CARD64 dri2_ust, dri2_msc, dri2_sbc;
   int ret;

   ret = DRI2WaitSBC(pdraw->psc->dpy, pdraw->xDrawable,
		     target_sbc, &dri2_ust, &dri2_msc, &dri2_sbc);
   *ust = dri2_ust;
   *msc = dri2_msc;
   *sbc = dri2_sbc;

   return ret;
}

#endif /* X_DRI2WaitMSC */

/**
 * dri2Throttle - Request driver throttling
 *
 * This function uses the DRI2 throttle extension to give the
 * driver the opportunity to throttle on flush front, copysubbuffer
 * and swapbuffers.
 */
static void
dri2Throttle(struct dri2_screen *psc,
	     struct dri2_drawable *draw,
	     enum __DRI2throttleReason reason)
{
   if (psc->throttle) {
      struct glx_context *gc = __glXGetCurrentContext();
      struct dri2_context *dri2Ctx = (struct dri2_context *)gc;
      __DRIcontext *ctx =
	 (dri2Ctx) ? dri2Ctx->driContext : NULL;

      psc->throttle->throttle(ctx, draw->driDrawable, reason);
   }
}

static void
__dri2CopySubBuffer(__GLXDRIdrawable *pdraw, int x, int y,
		    int width, int height,
		    enum __DRI2throttleReason reason)
{
   struct dri2_drawable *priv = (struct dri2_drawable *) pdraw;
   struct dri2_screen *psc = (struct dri2_screen *) pdraw->psc;
   XRectangle xrect;
   XserverRegion region;

   /* Check we have the right attachments */
   if (!priv->have_back)
      return;

   xrect.x = x;
   xrect.y = priv->height - y - height;
   xrect.width = width;
   xrect.height = height;

#ifdef __DRI2_FLUSH
   if (psc->f)
      (*psc->f->flush) (priv->driDrawable);
#endif

   dri2Throttle(psc, priv, reason);

   region = XFixesCreateRegion(psc->base.dpy, &xrect, 1);
   DRI2CopyRegion(psc->base.dpy, pdraw->xDrawable, region,
                  DRI2BufferFrontLeft, DRI2BufferBackLeft);

   /* Refresh the fake front (if present) after we just damaged the real
    * front.
    */
   if (priv->have_fake_front)
      DRI2CopyRegion(psc->base.dpy, pdraw->xDrawable, region,
		     DRI2BufferFakeFrontLeft, DRI2BufferFrontLeft);

   XFixesDestroyRegion(psc->base.dpy, region);
}

static void
dri2CopySubBuffer(__GLXDRIdrawable *pdraw, int x, int y,
		  int width, int height)
{
   __dri2CopySubBuffer(pdraw, x, y, width, height,
		       __DRI2_THROTTLE_COPYSUBBUFFER);
}


static void
dri2_copy_drawable(struct dri2_drawable *priv, int dest, int src)
{
   XRectangle xrect;
   XserverRegion region;
   struct dri2_screen *psc = (struct dri2_screen *) priv->base.psc;

   xrect.x = 0;
   xrect.y = 0;
   xrect.width = priv->width;
   xrect.height = priv->height;

#ifdef __DRI2_FLUSH
   if (psc->f)
      (*psc->f->flush) (priv->driDrawable);
#endif

   region = XFixesCreateRegion(psc->base.dpy, &xrect, 1);
   DRI2CopyRegion(psc->base.dpy, priv->base.xDrawable, region, dest, src);
   XFixesDestroyRegion(psc->base.dpy, region);

}

static void
dri2_wait_x(struct glx_context *gc)
{
   struct dri2_drawable *priv = (struct dri2_drawable *)
      GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);

   if (priv == NULL || !priv->have_fake_front)
      return;

   dri2_copy_drawable(priv, DRI2BufferFakeFrontLeft, DRI2BufferFrontLeft);
}

static void
dri2_wait_gl(struct glx_context *gc)
{
   struct dri2_drawable *priv = (struct dri2_drawable *)
      GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);

   if (priv == NULL || !priv->have_fake_front)
      return;

   dri2_copy_drawable(priv, DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft);
}

static void
dri2FlushFrontBuffer(__DRIdrawable *driDrawable, void *loaderPrivate)
{
   struct glx_display *priv;
   struct dri2_display *pdp;
   struct glx_context *gc;
   struct dri2_drawable *pdraw = loaderPrivate;
   struct dri2_screen *psc;

   if (!pdraw)
      return;

   if (!pdraw->base.psc)
      return;

   psc = (struct dri2_screen *) pdraw->base.psc;

   priv = __glXInitialize(psc->base.dpy);
   pdp = (struct dri2_display *) priv->dri2Display;
   gc = __glXGetCurrentContext();

   dri2Throttle(psc, pdraw, __DRI2_THROTTLE_FLUSHFRONT);

   /* Old servers don't send invalidate events */
   if (!pdp->invalidateAvailable)
       dri2InvalidateBuffers(priv->dpy, pdraw->base.xDrawable);

   dri2_wait_gl(gc);
}


static void
dri2DestroyScreen(struct glx_screen *base)
{
   struct dri2_screen *psc = (struct dri2_screen *) base;

   /* Free the direct rendering per screen data */
   (*psc->core->destroyScreen) (psc->driScreen);
   driDestroyConfigs(psc->driver_configs);
   close(psc->fd);
   Xfree(psc);
}

/**
 * Process list of buffer received from the server
 *
 * Processes the list of buffers received in a reply from the server to either
 * \c DRI2GetBuffers or \c DRI2GetBuffersWithFormat.
 */
static void
process_buffers(struct dri2_drawable * pdraw, DRI2Buffer * buffers,
                unsigned count)
{
   int i;

   pdraw->bufferCount = count;
   pdraw->have_fake_front = 0;
   pdraw->have_back = 0;

   /* This assumes the DRI2 buffer attachment tokens matches the
    * __DRIbuffer tokens. */
   for (i = 0; i < count; i++) {
      pdraw->buffers[i].attachment = buffers[i].attachment;
      pdraw->buffers[i].name = buffers[i].name;
      pdraw->buffers[i].pitch = buffers[i].pitch;
      pdraw->buffers[i].cpp = buffers[i].cpp;
      pdraw->buffers[i].flags = buffers[i].flags;
      if (pdraw->buffers[i].attachment == __DRI_BUFFER_FAKE_FRONT_LEFT)
         pdraw->have_fake_front = 1;
      if (pdraw->buffers[i].attachment == __DRI_BUFFER_BACK_LEFT)
         pdraw->have_back = 1;
   }

}

unsigned dri2GetSwapEventType(Display* dpy, XID drawable)
{
      struct glx_display *glx_dpy = __glXInitialize(dpy);
      __GLXDRIdrawable *pdraw;
      pdraw = dri2GetGlxDrawableFromXDrawableId(dpy, drawable);
      if (!pdraw || !(pdraw->eventMask & GLX_BUFFER_SWAP_COMPLETE_INTEL_MASK))
         return 0;
      return glx_dpy->codes->first_event + GLX_BufferSwapComplete;
}

static void show_fps(struct dri2_drawable *draw)
{
   struct timeval tv;
   double current_time;

   gettimeofday(&tv, 0);
   current_time = (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;

   draw->frames++;

   if (draw->previous_time + 1 < current_time) {
      if (draw->previous_time) {
         fprintf(stderr, "libGL: FPS = %.1f\n",
                 draw->frames / (current_time - draw->previous_time));
      }
      draw->frames = 0;
      draw->previous_time = current_time;
   }
}

static int64_t
dri2SwapBuffers(__GLXDRIdrawable *pdraw, int64_t target_msc, int64_t divisor,
		int64_t remainder)
{
    struct dri2_drawable *priv = (struct dri2_drawable *) pdraw;
    struct glx_display *dpyPriv = __glXInitialize(priv->base.psc->dpy);
    struct dri2_screen *psc = (struct dri2_screen *) priv->base.psc;
    struct dri2_display *pdp =
	(struct dri2_display *)dpyPriv->dri2Display;
    CARD64 ret = 0;

    /* Check we have the right attachments */
    if (!priv->have_back)
	return ret;

    /* Old servers can't handle swapbuffers */
    if (!pdp->swapAvailable) {
       __dri2CopySubBuffer(pdraw, 0, 0, priv->width, priv->height,
			   __DRI2_THROTTLE_SWAPBUFFER);
    } else {
#ifdef X_DRI2SwapBuffers
#ifdef __DRI2_FLUSH
    if (psc->f) {
       struct glx_context *gc = __glXGetCurrentContext();

       if (gc) {
	  (*psc->f->flush)(priv->driDrawable);
       }
    }
#endif

       dri2Throttle(psc, priv, __DRI2_THROTTLE_SWAPBUFFER);

       DRI2SwapBuffers(psc->base.dpy, pdraw->xDrawable,
		       target_msc, divisor, remainder, &ret);
#endif
    }

    if (psc->show_fps) {
       show_fps(priv);
    }

    /* Old servers don't send invalidate events */
    if (!pdp->invalidateAvailable)
       dri2InvalidateBuffers(dpyPriv->dpy, pdraw->xDrawable);

    return ret;
}

static __DRIbuffer *
dri2GetBuffers(__DRIdrawable * driDrawable,
               int *width, int *height,
               unsigned int *attachments, int count,
               int *out_count, void *loaderPrivate)
{
   struct dri2_drawable *pdraw = loaderPrivate;
   DRI2Buffer *buffers;

   buffers = DRI2GetBuffers(pdraw->base.psc->dpy, pdraw->base.xDrawable,
                            width, height, attachments, count, out_count);
   if (buffers == NULL)
      return NULL;

   pdraw->width = *width;
   pdraw->height = *height;
   process_buffers(pdraw, buffers, *out_count);

   Xfree(buffers);

   return pdraw->buffers;
}

static __DRIbuffer *
dri2GetBuffersWithFormat(__DRIdrawable * driDrawable,
                         int *width, int *height,
                         unsigned int *attachments, int count,
                         int *out_count, void *loaderPrivate)
{
   struct dri2_drawable *pdraw = loaderPrivate;
   DRI2Buffer *buffers;

   buffers = DRI2GetBuffersWithFormat(pdraw->base.psc->dpy,
                                      pdraw->base.xDrawable,
                                      width, height, attachments,
                                      count, out_count);
   if (buffers == NULL)
      return NULL;

   pdraw->width = *width;
   pdraw->height = *height;
   process_buffers(pdraw, buffers, *out_count);

   Xfree(buffers);

   return pdraw->buffers;
}

#ifdef X_DRI2SwapInterval

static int
dri2SetSwapInterval(__GLXDRIdrawable *pdraw, int interval)
{
   struct dri2_drawable *priv =  (struct dri2_drawable *) pdraw;
   GLint vblank_mode = DRI_CONF_VBLANK_DEF_INTERVAL_1;
   struct dri2_screen *psc = (struct dri2_screen *) priv->base.psc;

   if (psc->config)
      psc->config->configQueryi(psc->driScreen,
				"vblank_mode", &vblank_mode);

   switch (vblank_mode) {
   case DRI_CONF_VBLANK_NEVER:
      return GLX_BAD_VALUE;
   case DRI_CONF_VBLANK_ALWAYS_SYNC:
      if (interval <= 0)
	 return GLX_BAD_VALUE;
      break;
   default:
      break;
   }

   DRI2SwapInterval(priv->base.psc->dpy, priv->base.xDrawable, interval);
   priv->swap_interval = interval;

   return 0;
}

static int
dri2GetSwapInterval(__GLXDRIdrawable *pdraw)
{
   struct dri2_drawable *priv =  (struct dri2_drawable *) pdraw;

  return priv->swap_interval;
}

#endif /* X_DRI2SwapInterval */

static const __DRIdri2LoaderExtension dri2LoaderExtension = {
   {__DRI_DRI2_LOADER, __DRI_DRI2_LOADER_VERSION},
   dri2GetBuffers,
   dri2FlushFrontBuffer,
   dri2GetBuffersWithFormat,
};

static const __DRIdri2LoaderExtension dri2LoaderExtension_old = {
   {__DRI_DRI2_LOADER, __DRI_DRI2_LOADER_VERSION},
   dri2GetBuffers,
   dri2FlushFrontBuffer,
   NULL,
};

#ifdef __DRI_USE_INVALIDATE
static const __DRIuseInvalidateExtension dri2UseInvalidate = {
   { __DRI_USE_INVALIDATE, __DRI_USE_INVALIDATE_VERSION }
};
#endif

_X_HIDDEN void
dri2InvalidateBuffers(Display *dpy, XID drawable)
{
   __GLXDRIdrawable *pdraw =
      dri2GetGlxDrawableFromXDrawableId(dpy, drawable);
   struct dri2_screen *psc;
   struct dri2_drawable *pdp = (struct dri2_drawable *) pdraw;

   if (!pdraw)
      return;

   psc = (struct dri2_screen *) pdraw->psc;

#if __DRI2_FLUSH_VERSION >= 3
   if (pdraw && psc->f && psc->f->base.version >= 3 && psc->f->invalidate)
       psc->f->invalidate(pdp->driDrawable);
#endif
}

static void
dri2_bind_tex_image(Display * dpy,
		    GLXDrawable drawable,
		    int buffer, const int *attrib_list)
{
   struct glx_context *gc = __glXGetCurrentContext();
   struct dri2_context *pcp = (struct dri2_context *) gc;
   __GLXDRIdrawable *base = GetGLXDRIDrawable(dpy, drawable);
   struct glx_display *dpyPriv = __glXInitialize(dpy);
   struct dri2_drawable *pdraw = (struct dri2_drawable *) base;
   struct dri2_display *pdp =
      (struct dri2_display *) dpyPriv->dri2Display;
   struct dri2_screen *psc;

   if (pdraw != NULL) {
      psc = (struct dri2_screen *) base->psc;

#if __DRI2_FLUSH_VERSION >= 3
      if (!pdp->invalidateAvailable && psc->f &&
           psc->f->base.version >= 3 && psc->f->invalidate)
	 psc->f->invalidate(pdraw->driDrawable);
#endif

      if (psc->texBuffer->base.version >= 2 &&
	  psc->texBuffer->setTexBuffer2 != NULL) {
	 (*psc->texBuffer->setTexBuffer2) (pcp->driContext,
					   pdraw->base.textureTarget,
					   pdraw->base.textureFormat,
					   pdraw->driDrawable);
      }
      else {
	 (*psc->texBuffer->setTexBuffer) (pcp->driContext,
					  pdraw->base.textureTarget,
					  pdraw->driDrawable);
      }
   }
}

static void
dri2_release_tex_image(Display * dpy, GLXDrawable drawable, int buffer)
{
#if __DRI_TEX_BUFFER_VERSION >= 3
   struct glx_context *gc = __glXGetCurrentContext();
   struct dri2_context *pcp = (struct dri2_context *) gc;
   __GLXDRIdrawable *base = GetGLXDRIDrawable(dpy, drawable);
   struct glx_display *dpyPriv = __glXInitialize(dpy);
   struct dri2_drawable *pdraw = (struct dri2_drawable *) base;
   struct dri2_display *pdp =
      (struct dri2_display *) dpyPriv->dri2Display;
   struct dri2_screen *psc;

   if (pdraw != NULL) {
      psc = (struct dri2_screen *) base->psc;

      if (psc->texBuffer->base.version >= 3 &&
          psc->texBuffer->releaseTexBuffer != NULL) {
         (*psc->texBuffer->releaseTexBuffer) (pcp->driContext,
                                           pdraw->base.textureTarget,
                                           pdraw->driDrawable);
      }
   }
#endif
}

static const struct glx_context_vtable dri2_context_vtable = {
   dri2_destroy_context,
   dri2_bind_context,
   dri2_unbind_context,
   dri2_wait_gl,
   dri2_wait_x,
   DRI_glXUseXFont,
   dri2_bind_tex_image,
   dri2_release_tex_image,
   NULL, /* get_proc_address */
};

static void
dri2BindExtensions(struct dri2_screen *psc, const __DRIextension **extensions,
                   const char *driverName)
{
   int i;

   __glXEnableDirectExtension(&psc->base, "GLX_SGI_video_sync");
   __glXEnableDirectExtension(&psc->base, "GLX_SGI_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_SGI_make_current_read");

   /*
    * GLX_INTEL_swap_event is broken on the server side, where it's
    * currently unconditionally enabled. This completely breaks
    * systems running on drivers which don't support that extension.
    * There's no way to test for its presence on this side, so instead
    * of disabling it uncondtionally, just disable it for drivers
    * which are known to not support it.
    */
   if (strcmp(driverName, "vmwgfx") != 0) {
      __glXEnableDirectExtension(&psc->base, "GLX_INTEL_swap_event");
   }

   if (psc->dri2->base.version >= 3) {
      const unsigned mask = psc->dri2->getAPIMask(psc->driScreen);

      __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context");
      __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context_profile");

      if ((mask & (1 << __DRI_API_GLES2)) != 0)
	 __glXEnableDirectExtension(&psc->base,
				    "GLX_EXT_create_context_es2_profile");
   }

   for (i = 0; extensions[i]; i++) {
      if ((strcmp(extensions[i]->name, __DRI_TEX_BUFFER) == 0)) {
	 psc->texBuffer = (__DRItexBufferExtension *) extensions[i];
	 __glXEnableDirectExtension(&psc->base, "GLX_EXT_texture_from_pixmap");
      }

      if ((strcmp(extensions[i]->name, __DRI2_FLUSH) == 0)) {
	 psc->f = (__DRI2flushExtension *) extensions[i];
	 /* internal driver extension, no GL extension exposed */
      }

      if ((strcmp(extensions[i]->name, __DRI2_CONFIG_QUERY) == 0))
	 psc->config = (__DRI2configQueryExtension *) extensions[i];

      if (((strcmp(extensions[i]->name, __DRI2_THROTTLE) == 0)))
	 psc->throttle = (__DRI2throttleExtension *) extensions[i];

      /* DRI2 version 3 is also required because
       * GLX_ARB_create_context_robustness requires GLX_ARB_create_context.
       */
      if (psc->dri2->base.version >= 3
          && strcmp(extensions[i]->name, __DRI2_ROBUSTNESS) == 0)
         __glXEnableDirectExtension(&psc->base,
                                    "GLX_ARB_create_context_robustness");
   }
}

static const struct glx_screen_vtable dri2_screen_vtable = {
   dri2_create_context,
   dri2_create_context_attribs
};

static struct glx_screen *
dri2CreateScreen(int screen, struct glx_display * priv)
{
   const __DRIconfig **driver_configs;
   const __DRIextension **extensions;
   const struct dri2_display *const pdp = (struct dri2_display *)
      priv->dri2Display;
   struct dri2_screen *psc;
   __GLXDRIscreen *psp;
   struct glx_config *configs = NULL, *visuals = NULL;
   char *driverName, *deviceName, *tmp;
   drm_magic_t magic;
   int i;

   psc = Xmalloc(sizeof *psc);
   if (psc == NULL)
      return NULL;

   memset(psc, 0, sizeof *psc);
   psc->fd = -1;

   if (!glx_screen_init(&psc->base, screen, priv)) {
      Xfree(psc);
      return NULL;
   }

   if (!DRI2Connect(priv->dpy, RootWindow(priv->dpy, screen),
		    &driverName, &deviceName)) {
      glx_screen_cleanup(&psc->base);
      XFree(psc);
      InfoMessageF("screen %d does not appear to be DRI2 capable\n", screen);
      return NULL;
   }

   psc->driver = driOpenDriver(driverName);
   if (psc->driver == NULL) {
      ErrorMessageF("driver pointer missing\n");
      goto handle_error;
   }

   extensions = dlsym(psc->driver, __DRI_DRIVER_EXTENSIONS);
   if (extensions == NULL) {
      ErrorMessageF("driver exports no extensions (%s)\n", dlerror());
      goto handle_error;
   }

   for (i = 0; extensions[i]; i++) {
      if (strcmp(extensions[i]->name, __DRI_CORE) == 0)
	 psc->core = (__DRIcoreExtension *) extensions[i];
      if (strcmp(extensions[i]->name, __DRI_DRI2) == 0)
	 psc->dri2 = (__DRIdri2Extension *) extensions[i];
   }

   if (psc->core == NULL || psc->dri2 == NULL) {
      ErrorMessageF("core dri or dri2 extension not found\n");
      goto handle_error;
   }

#ifdef O_CLOEXEC
   psc->fd = open(deviceName, O_RDWR | O_CLOEXEC);
   if (psc->fd == -1 && errno == EINVAL)
#endif
   {
      psc->fd = open(deviceName, O_RDWR);
      if (psc->fd != -1)
         fcntl(psc->fd, F_SETFD, fcntl(psc->fd, F_GETFD) | FD_CLOEXEC);
   }
   if (psc->fd < 0) {
      ErrorMessageF("failed to open drm device: %s\n", strerror(errno));
      goto handle_error;
   }

   if (drmGetMagic(psc->fd, &magic)) {
      ErrorMessageF("failed to get magic\n");
      goto handle_error;
   }

   if (!DRI2Authenticate(priv->dpy, RootWindow(priv->dpy, screen), magic)) {
      ErrorMessageF("failed to authenticate magic %d\n", magic);
      goto handle_error;
   }

   
   /* If the server does not support the protocol for
    * DRI2GetBuffersWithFormat, don't supply that interface to the driver.
    */
   psc->driScreen =
      psc->dri2->createNewScreen(screen, psc->fd,
				 (const __DRIextension **)
				 &pdp->loader_extensions[0],
				 &driver_configs, psc);

   if (psc->driScreen == NULL) {
      ErrorMessageF("failed to create dri screen\n");
      goto handle_error;
   }

   extensions = psc->core->getExtensions(psc->driScreen);
   dri2BindExtensions(psc, extensions, driverName);

   configs = driConvertConfigs(psc->core, psc->base.configs, driver_configs);
   visuals = driConvertConfigs(psc->core, psc->base.visuals, driver_configs);

   if (!configs || !visuals)
       goto handle_error;

   glx_config_destroy_list(psc->base.configs);
   psc->base.configs = configs;
   glx_config_destroy_list(psc->base.visuals);
   psc->base.visuals = visuals;

   psc->driver_configs = driver_configs;

   psc->base.vtable = &dri2_screen_vtable;
   psp = &psc->vtable;
   psc->base.driScreen = psp;
   psp->destroyScreen = dri2DestroyScreen;
   psp->createDrawable = dri2CreateDrawable;
   psp->swapBuffers = dri2SwapBuffers;
   psp->getDrawableMSC = NULL;
   psp->waitForMSC = NULL;
   psp->waitForSBC = NULL;
   psp->setSwapInterval = NULL;
   psp->getSwapInterval = NULL;

   if (pdp->driMinor >= 2) {
#ifdef X_DRI2GetMSC
      psp->getDrawableMSC = dri2DrawableGetMSC;
#endif
#ifdef X_DRI2WaitMSC
      psp->waitForMSC = dri2WaitForMSC;
      psp->waitForSBC = dri2WaitForSBC;
#endif
#ifdef X_DRI2SwapInterval
      psp->setSwapInterval = dri2SetSwapInterval;
      psp->getSwapInterval = dri2GetSwapInterval;
#endif
#if defined(X_DRI2GetMSC) && defined(X_DRI2WaitMSC) && defined(X_DRI2SwapInterval)
      __glXEnableDirectExtension(&psc->base, "GLX_OML_sync_control");
#endif
   }

   /* DRI2 suports SubBuffer through DRI2CopyRegion, so it's always
    * available.*/
   psp->copySubBuffer = dri2CopySubBuffer;
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_copy_sub_buffer");

   Xfree(driverName);
   Xfree(deviceName);

   tmp = getenv("LIBGL_SHOW_FPS");
   psc->show_fps = tmp && strcmp(tmp, "1") == 0;

   return &psc->base;

handle_error:
   CriticalErrorMessageF("failed to load driver: %s\n", driverName);

   if (configs)
       glx_config_destroy_list(configs);
   if (visuals)
       glx_config_destroy_list(visuals);
   if (psc->driScreen)
       psc->core->destroyScreen(psc->driScreen);
   psc->driScreen = NULL;
   if (psc->fd >= 0)
      close(psc->fd);
   if (psc->driver)
      dlclose(psc->driver);

   Xfree(driverName);
   Xfree(deviceName);
   glx_screen_cleanup(&psc->base);
   XFree(psc);

   return NULL;
}

/* Called from __glXFreeDisplayPrivate.
 */
static void
dri2DestroyDisplay(__GLXDRIdisplay * dpy)
{
   struct dri2_display *pdp = (struct dri2_display *) dpy;

   __glxHashDestroy(pdp->dri2Hash);
   Xfree(dpy);
}

_X_HIDDEN __GLXDRIdrawable *
dri2GetGlxDrawableFromXDrawableId(Display *dpy, XID id)
{
   struct glx_display *d = __glXInitialize(dpy);
   struct dri2_display *pdp = (struct dri2_display *) d->dri2Display;
   __GLXDRIdrawable *pdraw;

   if (__glxHashLookup(pdp->dri2Hash, id, (void *) &pdraw) == 0)
      return pdraw;

   return NULL;
}

/*
 * Allocate, initialize and return a __DRIdisplayPrivate object.
 * This is called from __glXInitialize() when we are given a new
 * display pointer.
 */
_X_HIDDEN __GLXDRIdisplay *
dri2CreateDisplay(Display * dpy)
{
   struct dri2_display *pdp;
   int eventBase, errorBase, i;

   if (!DRI2QueryExtension(dpy, &eventBase, &errorBase))
      return NULL;

   pdp = Xmalloc(sizeof *pdp);
   if (pdp == NULL)
      return NULL;

   if (!DRI2QueryVersion(dpy, &pdp->driMajor, &pdp->driMinor)) {
      Xfree(pdp);
      return NULL;
   }

   pdp->driPatch = 0;
   pdp->swapAvailable = (pdp->driMinor >= 2);
   pdp->invalidateAvailable = (pdp->driMinor >= 3);

   pdp->base.destroyDisplay = dri2DestroyDisplay;
   pdp->base.createScreen = dri2CreateScreen;

   i = 0;
   if (pdp->driMinor < 1)
      pdp->loader_extensions[i++] = &dri2LoaderExtension_old.base;
   else
      pdp->loader_extensions[i++] = &dri2LoaderExtension.base;
   
   pdp->loader_extensions[i++] = &systemTimeExtension.base;

#ifdef __DRI_USE_INVALIDATE
   pdp->loader_extensions[i++] = &dri2UseInvalidate.base;
#endif
   pdp->loader_extensions[i++] = NULL;

   pdp->dri2Hash = __glxHashCreate();
   if (pdp->dri2Hash == NULL) {
      Xfree(pdp);
      return NULL;
   }

   return &pdp->base;
}

#endif /* GLX_DIRECT_RENDERING */
