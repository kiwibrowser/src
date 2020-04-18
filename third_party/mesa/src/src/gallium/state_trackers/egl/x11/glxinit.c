/**
 * GLX initialization.  Code based on glxext.c, glx_query.c, and
 * glcontextmodes.c under src/glx/.  The major difference is that DRI
 * related code is stripped out.
 *
 * If the maintenance of this file takes too much time, we should consider
 * refactoring glxext.c.
 */

#include <assert.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xlibint.h>
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>
#include <sys/time.h>

#include "GL/glxproto.h"
#include "GL/glxtokens.h"
#include "GL/gl.h" /* for GL types needed by __GLcontextModes */
#include "glcore.h"  /* for __GLcontextModes */

#include "glxinit.h"

#ifdef GLX_DIRECT_RENDERING

typedef struct GLXGenericGetString
{
   CARD8 reqType;
   CARD8 glxCode;
   CARD16 length B16;
   CARD32 for_whom B32;
   CARD32 name B32;
} xGLXGenericGetStringReq;

#define sz_xGLXGenericGetStringReq 12
#define X_GLXGenericGetString 0

/* Extension required boiler plate */

static char *__glXExtensionName = GLX_EXTENSION_NAME;
static XExtensionInfo *__glXExtensionInfo = NULL;

static int
__glXCloseDisplay(Display * dpy, XExtCodes * codes)
{
   return XextRemoveDisplay(__glXExtensionInfo, dpy);
}

static /* const */ XExtensionHooks __glXExtensionHooks = {
  NULL,                   /* create_gc */
  NULL,                   /* copy_gc */
  NULL,                   /* flush_gc */
  NULL,                   /* free_gc */
  NULL,                   /* create_font */
  NULL,                   /* free_font */
  __glXCloseDisplay,      /* close_display */
  NULL,                   /* wire_to_event */
  NULL,                   /* event_to_wire */
  NULL,                   /* error */
  NULL,                   /* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY(__glXFindDisplay, __glXExtensionInfo,
				  __glXExtensionName, &__glXExtensionHooks,
				  __GLX_NUMBER_EVENTS, NULL)

static GLint
_gl_convert_from_x_visual_type(int visualType)
{
#define NUM_VISUAL_TYPES   6
   static const int glx_visual_types[NUM_VISUAL_TYPES] = {
      GLX_STATIC_GRAY, GLX_GRAY_SCALE,
      GLX_STATIC_COLOR, GLX_PSEUDO_COLOR,
      GLX_TRUE_COLOR, GLX_DIRECT_COLOR
   };

   return ((unsigned) visualType < NUM_VISUAL_TYPES)
      ? glx_visual_types[visualType] : GLX_NONE;
}

static void
_gl_context_modes_destroy(__GLcontextModes * modes)
{
   while (modes != NULL) {
      __GLcontextModes *const next = modes->next;

      Xfree(modes);
      modes = next;
   }
}

static __GLcontextModes *
_gl_context_modes_create(unsigned count, size_t minimum_size)
{
   const size_t size = (minimum_size > sizeof(__GLcontextModes))
      ? minimum_size : sizeof(__GLcontextModes);
   __GLcontextModes *base = NULL;
   __GLcontextModes **next;
   unsigned i;

   next = &base;
   for (i = 0; i < count; i++) {
      *next = (__GLcontextModes *) Xmalloc(size);
      if (*next == NULL) {
         _gl_context_modes_destroy(base);
         base = NULL;
         break;
      }

      memset(*next, 0, size);
      (*next)->visualID = GLX_DONT_CARE;
      (*next)->visualType = GLX_DONT_CARE;
      (*next)->visualRating = GLX_NONE;
      (*next)->transparentPixel = GLX_NONE;
      (*next)->transparentRed = GLX_DONT_CARE;
      (*next)->transparentGreen = GLX_DONT_CARE;
      (*next)->transparentBlue = GLX_DONT_CARE;
      (*next)->transparentAlpha = GLX_DONT_CARE;
      (*next)->transparentIndex = GLX_DONT_CARE;
      (*next)->xRenderable = GLX_DONT_CARE;
      (*next)->fbconfigID = GLX_DONT_CARE;
      (*next)->swapMethod = GLX_SWAP_UNDEFINED_OML;
      (*next)->bindToTextureRgb = GLX_DONT_CARE;
      (*next)->bindToTextureRgba = GLX_DONT_CARE;
      (*next)->bindToMipmapTexture = GLX_DONT_CARE;
      (*next)->bindToTextureTargets = GLX_DONT_CARE;
      (*next)->yInverted = GLX_DONT_CARE;

      next = &((*next)->next);
   }

   return base;
}

static char *
__glXQueryServerString(Display * dpy, int opcode, CARD32 screen, CARD32 name)
{
   xGLXGenericGetStringReq *req;
   xGLXSingleReply reply;
   int length;
   int numbytes;
   char *buf;
   CARD32 for_whom = screen;
   CARD32 glxCode = X_GLXQueryServerString;


   LockDisplay(dpy);


   /* All of the GLX protocol requests for getting a string from the server
    * look the same.  The exact meaning of the for_whom field is usually
    * either the screen number (for glXQueryServerString) or the context tag
    * (for GLXSingle).
    */

   GetReq(GLXGenericGetString, req);
   req->reqType = opcode;
   req->glxCode = glxCode;
   req->for_whom = for_whom;
   req->name = name;

   _XReply(dpy, (xReply *) & reply, 0, False);

   length = reply.length * 4;
   numbytes = reply.size;

   buf = (char *) Xmalloc(numbytes);
   if (buf != NULL) {
      _XRead(dpy, buf, numbytes);
      length -= numbytes;
   }

   _XEatData(dpy, length);

   UnlockDisplay(dpy);
   SyncHandle();

   return buf;
}

/************************************************************************/
/*
** Free the per screen configs data as well as the array of
** __glXScreenConfigs.
*/
static void
FreeScreenConfigs(__GLXdisplayPrivate * priv)
{
   __GLXscreenConfigs *psc;
   GLint i, screens;

   /* Free screen configuration information */
   screens = ScreenCount(priv->dpy);
   for (i = 0; i < screens; i++) {
      psc = priv->screenConfigs[i];
      if (!psc)
         continue;
      if (psc->configs) {
         _gl_context_modes_destroy(psc->configs);
         psc->configs = NULL;   /* NOTE: just for paranoia */
      }
      Xfree((char *) psc->serverGLXexts);
   }
   XFree((char *) priv->screenConfigs);
   priv->screenConfigs = NULL;
}

/*
** Release the private memory referred to in a display private
** structure.  The caller will free the extension structure.
*/
static int
__glXFreeDisplayPrivate(XExtData * extension)
{
   __GLXdisplayPrivate *priv;

   priv = (__GLXdisplayPrivate *) extension->private_data;
   FreeScreenConfigs(priv);
   if (priv->serverGLXversion)
      Xfree((char *) priv->serverGLXversion);

   Xfree((char *) priv);
   return 0;
}

/************************************************************************/

/*
** Query the version of the GLX extension.  This procedure works even if
** the client extension is not completely set up.
*/

#define GLX_MAJOR_VERSION 1       /* current version numbers */
#define GLX_MINOR_VERSION 4

static Bool
QueryVersion(Display * dpy, int opcode, int *major, int *minor)
{
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
}

#define __GLX_MIN_CONFIG_PROPS	18
#define __GLX_MAX_CONFIG_PROPS	500
#define __GLX_EXT_CONFIG_PROPS 	10
#define __GLX_TOTAL_CONFIG       (__GLX_MIN_CONFIG_PROPS +      \
                                    2 * __GLX_EXT_CONFIG_PROPS)

static void
__glXInitializeVisualConfigFromTags(__GLcontextModes * config, int count,
                                    const INT32 * bp, Bool tagged_only,
                                    Bool fbconfig_style_tags)
{
   int i;

   if (!tagged_only) {
      /* Copy in the first set of properties */
      config->visualID = *bp++;

      config->visualType = _gl_convert_from_x_visual_type(*bp++);

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

      count -= __GLX_MIN_CONFIG_PROPS;
   }

   /*
    ** Additional properties may be in a list at the end
    ** of the reply.  They are in pairs of property type
    ** and property value.
    */

#define FETCH_OR_SET(tag) \
    config-> tag = ( fbconfig_style_tags ) ? *bp++ : 1

   for (i = 0; i < count; i += 2) {
      switch (*bp++) {
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
      case GLX_SAMPLE_BUFFERS_SGIS:
         config->sampleBuffers = *bp++;
         break;
      case GLX_SAMPLES_SGIS:
         config->samples = *bp++;
         break;
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
      case None:
         i = count;
         break;
      default:
         break;
      }
   }

   config->renderType =
      (config->rgbMode) ? GLX_RGBA_BIT : GLX_COLOR_INDEX_BIT;

   config->haveAccumBuffer = ((config->accumRedBits +
                               config->accumGreenBits +
                               config->accumBlueBits +
                               config->accumAlphaBits) > 0);
   config->haveDepthBuffer = (config->depthBits > 0);
   config->haveStencilBuffer = (config->stencilBits > 0);
}

static __GLcontextModes *
createConfigsFromProperties(Display * dpy, int nvisuals, int nprops,
                            int screen, GLboolean tagged_only)
{
   INT32 buf[__GLX_TOTAL_CONFIG], *props;
   unsigned prop_size;
   __GLcontextModes *modes, *m;
   int i;

   if (nprops == 0)
      return NULL;

   /* FIXME: Is the __GLX_MIN_CONFIG_PROPS test correct for FBconfigs? */

   /* Check number of properties */
   if (nprops < __GLX_MIN_CONFIG_PROPS || nprops > __GLX_MAX_CONFIG_PROPS)
      return NULL;

   /* Allocate memory for our config structure */
   modes = _gl_context_modes_create(nvisuals, sizeof(__GLcontextModes));
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
      /* Older X servers don't send this so we default it here. */
      m->drawableType = GLX_WINDOW_BIT;
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
getFBConfigs(__GLXscreenConfigs *psc, __GLXdisplayPrivate *priv, int screen)
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
                  sz_xGLXGetFBConfigsSGIXReq +
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

static GLboolean
AllocAndFetchScreenConfigs(Display * dpy, __GLXdisplayPrivate * priv)
{
   __GLXscreenConfigs *psc;
   GLint i, screens;

   /*
    ** First allocate memory for the array of per screen configs.
    */
   screens = ScreenCount(dpy);
   priv->screenConfigs = Xmalloc(screens * sizeof *priv->screenConfigs);
   if (!priv->screenConfigs) {
      return GL_FALSE;
   }

   priv->serverGLXversion =
      __glXQueryServerString(dpy, priv->majorOpcode, 0, GLX_VERSION);
   if (priv->serverGLXversion == NULL) {
      FreeScreenConfigs(priv);
      return GL_FALSE;
   }

   for (i = 0; i < screens; i++) {
      psc = Xcalloc(1, sizeof *psc);
      if (!psc)
         return GL_FALSE;
      getFBConfigs(psc, priv, i);
      priv->screenConfigs[i] = psc;
   }

   SyncHandle();

   return GL_TRUE;
}

_X_HIDDEN __GLXdisplayPrivate *
__glXInitialize(Display * dpy)
{
   XExtDisplayInfo *info = __glXFindDisplay(dpy);
   XExtData **privList, *private, *found;
   __GLXdisplayPrivate *dpyPriv;
   XEDataObject dataObj;
   int major, minor;

   if (!XextHasExtension(info))
      return NULL;

   /* See if a display private already exists.  If so, return it */
   dataObj.display = dpy;
   privList = XEHeadOfExtensionList(dataObj);
   found = XFindOnExtensionList(privList, info->codes->extension);
   if (found)
      return (__GLXdisplayPrivate *) found->private_data;

   /* See if the versions are compatible */
   if (!QueryVersion(dpy, info->codes->major_opcode, &major, &minor))
      return NULL;

   /*
    ** Allocate memory for all the pieces needed for this buffer.
    */
   private = (XExtData *) Xmalloc(sizeof(XExtData));
   if (!private)
      return NULL;
   dpyPriv = (__GLXdisplayPrivate *) Xcalloc(1, sizeof(__GLXdisplayPrivate));
   if (!dpyPriv) {
      Xfree(private);
      return NULL;
   }

   /*
    ** Init the display private and then read in the screen config
    ** structures from the server.
    */
   dpyPriv->majorOpcode = info->codes->major_opcode;
   dpyPriv->dpy = dpy;

   if (!AllocAndFetchScreenConfigs(dpy, dpyPriv)) {
      Xfree(dpyPriv);
      Xfree(private);
      return NULL;
   }

   /*
    ** Fill in the private structure.  This is the actual structure that
    ** hangs off of the Display structure.  Our private structure is
    ** referred to by this structure.  Got that?
    */
   private->number = info->codes->extension;
   private->next = 0;
   private->free_private = __glXFreeDisplayPrivate;
   private->private_data = (char *) dpyPriv;
   XAddToExtensionList(privList, private);

   return dpyPriv;
}

#endif /* GLX_DIRECT_RENDERING */
