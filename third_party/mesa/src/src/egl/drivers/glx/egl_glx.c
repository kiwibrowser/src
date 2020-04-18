/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
 * Copyright 2010-2011 LunarG, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/**
 * This is an EGL driver that wraps GLX. This gives the benefit of being
 * completely agnostic of the direct rendering implementation.
 *
 * Authors: Alan Hourihane <alanh@tungstengraphics.com>
 */

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <dlfcn.h>
#include "GL/glx.h"

#include "eglconfig.h"
#include "eglcontext.h"
#include "egldefines.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"
#include "eglsurface.h"

#define CALLOC_STRUCT(T)   (struct T *) calloc(1, sizeof(struct T))

#ifndef GLX_VERSION_1_4
#error "GL/glx.h must be equal to or greater than GLX 1.4"
#endif

/* GLX 1.0 */
typedef GLXContext (*GLXCREATECONTEXTPROC)( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct );
typedef void (*GLXDESTROYCONTEXTPROC)( Display *dpy, GLXContext ctx );
typedef Bool (*GLXMAKECURRENTPROC)( Display *dpy, GLXDrawable drawable, GLXContext ctx);
typedef void (*GLXSWAPBUFFERSPROC)( Display *dpy, GLXDrawable drawable );
typedef GLXPixmap (*GLXCREATEGLXPIXMAPPROC)( Display *dpy, XVisualInfo *visual, Pixmap pixmap );
typedef void (*GLXDESTROYGLXPIXMAPPROC)( Display *dpy, GLXPixmap pixmap );
typedef Bool (*GLXQUERYVERSIONPROC)( Display *dpy, int *maj, int *min );
typedef int (*GLXGETCONFIGPROC)( Display *dpy, XVisualInfo *visual, int attrib, int *value );
typedef void (*GLXWAITGLPROC)( void );
typedef void (*GLXWAITXPROC)( void );

/* GLX 1.1 */
typedef const char *(*GLXQUERYEXTENSIONSSTRINGPROC)( Display *dpy, int screen );
typedef const char *(*GLXQUERYSERVERSTRINGPROC)( Display *dpy, int screen, int name );
typedef const char *(*GLXGETCLIENTSTRINGPROC)( Display *dpy, int name );

/** subclass of _EGLDriver */
struct GLX_egl_driver
{
   _EGLDriver Base;   /**< base class */

   void *handle;

   /* GLX 1.0 */
   GLXCREATECONTEXTPROC glXCreateContext;
   GLXDESTROYCONTEXTPROC glXDestroyContext;
   GLXMAKECURRENTPROC glXMakeCurrent;
   GLXSWAPBUFFERSPROC glXSwapBuffers;
   GLXCREATEGLXPIXMAPPROC glXCreateGLXPixmap;
   GLXDESTROYGLXPIXMAPPROC glXDestroyGLXPixmap;
   GLXQUERYVERSIONPROC glXQueryVersion;
   GLXGETCONFIGPROC glXGetConfig;
   GLXWAITGLPROC glXWaitGL;
   GLXWAITXPROC glXWaitX;

   /* GLX 1.1 */
   GLXQUERYEXTENSIONSSTRINGPROC glXQueryExtensionsString;
   GLXQUERYSERVERSTRINGPROC glXQueryServerString;
   GLXGETCLIENTSTRINGPROC glXGetClientString;

   /* GLX 1.3 or (GLX_SGI_make_current_read and GLX_SGIX_fbconfig) */
   PFNGLXGETFBCONFIGSPROC glXGetFBConfigs;
   PFNGLXGETFBCONFIGATTRIBPROC glXGetFBConfigAttrib;
   PFNGLXGETVISUALFROMFBCONFIGPROC glXGetVisualFromFBConfig;
   PFNGLXCREATEWINDOWPROC glXCreateWindow;
   PFNGLXDESTROYWINDOWPROC glXDestroyWindow;
   PFNGLXCREATEPIXMAPPROC glXCreatePixmap;
   PFNGLXDESTROYPIXMAPPROC glXDestroyPixmap;
   PFNGLXCREATEPBUFFERPROC glXCreatePbuffer;
   PFNGLXDESTROYPBUFFERPROC glXDestroyPbuffer;
   PFNGLXCREATENEWCONTEXTPROC glXCreateNewContext;
   PFNGLXMAKECONTEXTCURRENTPROC glXMakeContextCurrent;

   /* GLX 1.4 or GLX_ARB_get_proc_address */
   PFNGLXGETPROCADDRESSPROC glXGetProcAddress;

   /* GLX_SGIX_pbuffer */
   PFNGLXCREATEGLXPBUFFERSGIXPROC glXCreateGLXPbufferSGIX;
   PFNGLXDESTROYGLXPBUFFERSGIXPROC glXDestroyGLXPbufferSGIX;
};


/** driver data of _EGLDisplay */
struct GLX_egl_display
{
   Display *dpy;
   XVisualInfo *visuals;
   GLXFBConfig *fbconfigs;

   int glx_maj, glx_min;

   const char *extensions;
   EGLBoolean have_1_3;
   EGLBoolean have_make_current_read;
   EGLBoolean have_fbconfig;
   EGLBoolean have_pbuffer;

   /* workaround quirks of different GLX implementations */
   EGLBoolean single_buffered_quirk;
   EGLBoolean glx_window_quirk;
};


/** subclass of _EGLContext */
struct GLX_egl_context
{
   _EGLContext Base;   /**< base class */

   GLXContext context;
};


/** subclass of _EGLSurface */
struct GLX_egl_surface
{
   _EGLSurface Base;   /**< base class */

   Drawable drawable;
   GLXDrawable glx_drawable;

   void (*destroy)(Display *, GLXDrawable);
};


/** subclass of _EGLConfig */
struct GLX_egl_config
{
   _EGLConfig Base;   /**< base class */
   EGLBoolean double_buffered;
   int index;
};

/* standard typecasts */
_EGL_DRIVER_STANDARD_TYPECASTS(GLX_egl)

static int
GLX_egl_config_index(_EGLConfig *conf)
{
   struct GLX_egl_config *GLX_conf = GLX_egl_config(conf);
   return GLX_conf->index;
}


static const struct {
   int attr;
   int egl_attr;
} fbconfig_attributes[] = {
   /* table 3.1 of GLX 1.4 */
   { GLX_FBCONFIG_ID,                  0 },
   { GLX_BUFFER_SIZE,                  EGL_BUFFER_SIZE },
   { GLX_LEVEL,                        EGL_LEVEL },
   { GLX_DOUBLEBUFFER,                 0 },
   { GLX_STEREO,                       0 },
   { GLX_AUX_BUFFERS,                  0 },
   { GLX_RED_SIZE,                     EGL_RED_SIZE },
   { GLX_GREEN_SIZE,                   EGL_GREEN_SIZE },
   { GLX_BLUE_SIZE,                    EGL_BLUE_SIZE },
   { GLX_ALPHA_SIZE,                   EGL_ALPHA_SIZE },
   { GLX_DEPTH_SIZE,                   EGL_DEPTH_SIZE },
   { GLX_STENCIL_SIZE,                 EGL_STENCIL_SIZE },
   { GLX_ACCUM_RED_SIZE,               0 },
   { GLX_ACCUM_GREEN_SIZE,             0 },
   { GLX_ACCUM_BLUE_SIZE,              0 },
   { GLX_ACCUM_ALPHA_SIZE,             0 },
   { GLX_SAMPLE_BUFFERS,               EGL_SAMPLE_BUFFERS },
   { GLX_SAMPLES,                      EGL_SAMPLES },
   { GLX_RENDER_TYPE,                  0 },
   { GLX_DRAWABLE_TYPE,                EGL_SURFACE_TYPE },
   { GLX_X_RENDERABLE,                 EGL_NATIVE_RENDERABLE },
   { GLX_X_VISUAL_TYPE,                EGL_NATIVE_VISUAL_TYPE },
   { GLX_CONFIG_CAVEAT,                EGL_CONFIG_CAVEAT },
   { GLX_TRANSPARENT_TYPE,             EGL_TRANSPARENT_TYPE },
   { GLX_TRANSPARENT_INDEX_VALUE,      0 },
   { GLX_TRANSPARENT_RED_VALUE,        EGL_TRANSPARENT_RED_VALUE },
   { GLX_TRANSPARENT_GREEN_VALUE,      EGL_TRANSPARENT_GREEN_VALUE },
   { GLX_TRANSPARENT_BLUE_VALUE,       EGL_TRANSPARENT_BLUE_VALUE },
   { GLX_MAX_PBUFFER_WIDTH,            EGL_MAX_PBUFFER_WIDTH },
   { GLX_MAX_PBUFFER_HEIGHT,           EGL_MAX_PBUFFER_HEIGHT },
   { GLX_MAX_PBUFFER_PIXELS,           EGL_MAX_PBUFFER_PIXELS },
   { GLX_VISUAL_ID,                    EGL_NATIVE_VISUAL_ID }
};


static EGLBoolean
convert_fbconfig(struct GLX_egl_driver *GLX_drv,
                 struct GLX_egl_display *GLX_dpy, GLXFBConfig fbconfig,
                 struct GLX_egl_config *GLX_conf)
{
   Display *dpy = GLX_dpy->dpy;
   int err, attr, val;
   unsigned i;

   /* must have rgba bit */
   err = GLX_drv->glXGetFBConfigAttrib(dpy, fbconfig, GLX_RENDER_TYPE, &val);
   if (err || !(val & GLX_RGBA_BIT))
      return EGL_FALSE;

   /* must know whether it is double-buffered */
   err = GLX_drv->glXGetFBConfigAttrib(dpy, fbconfig, GLX_DOUBLEBUFFER, &val);
   if (err)
      return EGL_FALSE;
   GLX_conf->double_buffered = val;

   GLX_conf->Base.RenderableType = EGL_OPENGL_BIT;
   GLX_conf->Base.Conformant = EGL_OPENGL_BIT;

   for (i = 0; i < ARRAY_SIZE(fbconfig_attributes); i++) {
      EGLint egl_attr, egl_val;

      attr = fbconfig_attributes[i].attr;
      egl_attr = fbconfig_attributes[i].egl_attr;
      if (!egl_attr)
         continue;

      err = GLX_drv->glXGetFBConfigAttrib(dpy, fbconfig, attr, &val);
      if (err) {
         if (err == GLX_BAD_ATTRIBUTE) {
            err = 0;
            continue;
         }
         break;
      }

      switch (egl_attr) {
      case EGL_SURFACE_TYPE:
         egl_val = 0;
         if (val & GLX_WINDOW_BIT)
            egl_val |= EGL_WINDOW_BIT;
         /* pixmap and pbuffer surfaces must be single-buffered in EGL */
         if (!GLX_conf->double_buffered) {
            if (val & GLX_PIXMAP_BIT)
               egl_val |= EGL_PIXMAP_BIT;
            if (val & GLX_PBUFFER_BIT)
               egl_val |= EGL_PBUFFER_BIT;
         }
         break;
      case EGL_NATIVE_VISUAL_TYPE:
         switch (val) {
         case GLX_TRUE_COLOR:
            egl_val = TrueColor;
            break;
         case GLX_DIRECT_COLOR:
            egl_val = DirectColor;
            break;
         case GLX_PSEUDO_COLOR:
            egl_val = PseudoColor;
            break;
         case GLX_STATIC_COLOR:
            egl_val = StaticColor;
            break;
         case GLX_GRAY_SCALE:
            egl_val = GrayScale;
            break;
         case GLX_STATIC_GRAY:
            egl_val = StaticGray;
            break;
         default:
            egl_val = EGL_NONE;
            break;
         }
         break;
      case EGL_CONFIG_CAVEAT:
         egl_val = EGL_NONE;
         if (val == GLX_SLOW_CONFIG) {
            egl_val = EGL_SLOW_CONFIG;
         }
         else if (val == GLX_NON_CONFORMANT_CONFIG) {
            GLX_conf->Base.Conformant &= ~EGL_OPENGL_BIT;
            egl_val = EGL_NONE;
         }
         break;
      case EGL_TRANSPARENT_TYPE:
         egl_val = (val == GLX_TRANSPARENT_RGB) ?
            EGL_TRANSPARENT_RGB : EGL_NONE;
         break;
      default:
         egl_val = val;
         break;
      }

      _eglSetConfigKey(&GLX_conf->Base, egl_attr, egl_val);
   }
   if (err)
      return EGL_FALSE;

   if (!GLX_conf->Base.SurfaceType)
      return EGL_FALSE;

   return EGL_TRUE;
}

static const struct {
   int attr;
   int egl_attr;
} visual_attributes[] = {
   /* table 3.7 of GLX 1.4 */
   { GLX_USE_GL,              0 },
   { GLX_BUFFER_SIZE,         EGL_BUFFER_SIZE },
   { GLX_LEVEL,               EGL_LEVEL },
   { GLX_RGBA,                0 },
   { GLX_DOUBLEBUFFER,        0 },
   { GLX_STEREO,              0 },
   { GLX_AUX_BUFFERS,         0 },
   { GLX_RED_SIZE,            EGL_RED_SIZE },
   { GLX_GREEN_SIZE,          EGL_GREEN_SIZE },
   { GLX_BLUE_SIZE,           EGL_BLUE_SIZE },
   { GLX_ALPHA_SIZE,          EGL_ALPHA_SIZE },
   { GLX_DEPTH_SIZE,          EGL_DEPTH_SIZE },
   { GLX_STENCIL_SIZE,        EGL_STENCIL_SIZE },
   { GLX_ACCUM_RED_SIZE,      0 },
   { GLX_ACCUM_GREEN_SIZE,    0 },
   { GLX_ACCUM_BLUE_SIZE,     0 },
   { GLX_ACCUM_ALPHA_SIZE,    0 },
   { GLX_SAMPLE_BUFFERS,      EGL_SAMPLE_BUFFERS },
   { GLX_SAMPLES,             EGL_SAMPLES },
   { GLX_FBCONFIG_ID,         0 },
   /* GLX_EXT_visual_rating */
   { GLX_VISUAL_CAVEAT_EXT,   EGL_CONFIG_CAVEAT }
};

static EGLBoolean
convert_visual(struct GLX_egl_driver *GLX_drv,
               struct GLX_egl_display *GLX_dpy, XVisualInfo *vinfo,
               struct GLX_egl_config *GLX_conf)
{
   Display *dpy = GLX_dpy->dpy;
   int err, attr, val;
   unsigned i;

   /* the visual must support OpenGL and RGBA buffer */
   err = GLX_drv->glXGetConfig(dpy, vinfo, GLX_USE_GL, &val);
   if (!err && val)
      err = GLX_drv->glXGetConfig(dpy, vinfo, GLX_RGBA, &val);
   if (err || !val)
      return EGL_FALSE;

   /* must know whether it is double-buffered */
   err = GLX_drv->glXGetConfig(dpy, vinfo, GLX_DOUBLEBUFFER, &val);
   if (err)
      return EGL_FALSE;
   GLX_conf->double_buffered = val;

   GLX_conf->Base.RenderableType = EGL_OPENGL_BIT;
   GLX_conf->Base.Conformant = EGL_OPENGL_BIT;
   GLX_conf->Base.SurfaceType = EGL_WINDOW_BIT;
   /* pixmap surfaces must be single-buffered in EGL */
   if (!GLX_conf->double_buffered)
      GLX_conf->Base.SurfaceType |= EGL_PIXMAP_BIT;

   GLX_conf->Base.NativeVisualID = vinfo->visualid;
   GLX_conf->Base.NativeVisualType = vinfo->class;
   GLX_conf->Base.NativeRenderable = EGL_TRUE;

   for (i = 0; i < ARRAY_SIZE(visual_attributes); i++) {
      EGLint egl_attr, egl_val;

      attr = visual_attributes[i].attr;
      egl_attr = visual_attributes[i].egl_attr;
      if (!egl_attr)
         continue;

      err = GLX_drv->glXGetConfig(dpy, vinfo, attr, &val);
      if (err) {
         if (err == GLX_BAD_ATTRIBUTE) {
            err = 0;
            continue;
         }
         break;
      }

      switch (egl_attr) {
      case EGL_CONFIG_CAVEAT:
         egl_val = EGL_NONE;
         if (val == GLX_SLOW_VISUAL_EXT) {
            egl_val = EGL_SLOW_CONFIG;
         }
         else if (val == GLX_NON_CONFORMANT_VISUAL_EXT) {
            GLX_conf->Base.Conformant &= ~EGL_OPENGL_BIT;
            egl_val = EGL_NONE;
         }
         break;
         break;
      default:
         egl_val = val;
         break;
      }
      _eglSetConfigKey(&GLX_conf->Base, egl_attr, egl_val);
   }

   return (err) ? EGL_FALSE : EGL_TRUE;
}


static void
fix_config(struct GLX_egl_display *GLX_dpy, struct GLX_egl_config *GLX_conf)
{
   _EGLConfig *conf = &GLX_conf->Base;

   if (!GLX_conf->double_buffered && GLX_dpy->single_buffered_quirk) {
      /* some GLX impls do not like single-buffered window surface */
      conf->SurfaceType &= ~EGL_WINDOW_BIT;
      /* pbuffer bit is usually not set */
      if (GLX_dpy->have_pbuffer)
         conf->SurfaceType |= EGL_PBUFFER_BIT;
   }

   /* no visual attribs unless window bit is set */
   if (!(conf->SurfaceType & EGL_WINDOW_BIT)) {
      conf->NativeVisualID = 0;
      conf->NativeVisualType = EGL_NONE;
   }

   if (conf->TransparentType != EGL_TRANSPARENT_RGB) {
      /* some impls set them to -1 (GLX_DONT_CARE) */
      conf->TransparentRedValue = 0;
      conf->TransparentGreenValue = 0;
      conf->TransparentBlueValue = 0;
   }

   /* make sure buffer size is set correctly */
   conf->BufferSize =
      conf->RedSize + conf->GreenSize + conf->BlueSize + conf->AlphaSize;
}


static EGLBoolean
create_configs(_EGLDriver *drv, _EGLDisplay *dpy, EGLint screen)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(dpy);
   EGLint num_configs = 0, i;
   EGLint id = 1;

   if (GLX_dpy->have_fbconfig) {
      GLX_dpy->fbconfigs =
         GLX_drv->glXGetFBConfigs(GLX_dpy->dpy, screen, &num_configs);
   }
   else {
      XVisualInfo vinfo_template;
      long mask;

      vinfo_template.screen = screen;
      mask = VisualScreenMask;
      GLX_dpy->visuals = XGetVisualInfo(GLX_dpy->dpy, mask, &vinfo_template,
                                        &num_configs);
   }

   if (!num_configs)
      return EGL_FALSE;

   for (i = 0; i < num_configs; i++) {
      struct GLX_egl_config *GLX_conf, template;
      EGLBoolean ok;

      memset(&template, 0, sizeof(template));
      _eglInitConfig(&template.Base, dpy, id);
      if (GLX_dpy->have_fbconfig) {
         ok = convert_fbconfig(GLX_drv, GLX_dpy,
               GLX_dpy->fbconfigs[i], &template);
      }
      else {
         ok = convert_visual(GLX_drv, GLX_dpy,
               &GLX_dpy->visuals[i], &template);
      }
      if (!ok)
        continue;

      fix_config(GLX_dpy, &template);
      if (!_eglValidateConfig(&template.Base, EGL_FALSE)) {
         _eglLog(_EGL_DEBUG, "GLX: failed to validate config %d", i);
         continue;
      }

      GLX_conf = CALLOC_STRUCT(GLX_egl_config);
      if (GLX_conf) {
         memcpy(GLX_conf, &template, sizeof(template));
         GLX_conf->index = i;

         _eglLinkConfig(&GLX_conf->Base);
         id++;
      }
   }

   return EGL_TRUE;
}


static void
check_extensions(struct GLX_egl_driver *GLX_drv,
                 struct GLX_egl_display *GLX_dpy, EGLint screen)
{
   GLX_dpy->extensions =
      GLX_drv->glXQueryExtensionsString(GLX_dpy->dpy, screen);
   if (GLX_dpy->extensions) {
      if (strstr(GLX_dpy->extensions, "GLX_SGI_make_current_read")) {
         /* GLX 1.3 entry points are used */
         GLX_dpy->have_make_current_read = EGL_TRUE;
      }

      if (strstr(GLX_dpy->extensions, "GLX_SGIX_fbconfig")) {
         /* GLX 1.3 entry points are used */
         GLX_dpy->have_fbconfig = EGL_TRUE;
      }

      if (strstr(GLX_dpy->extensions, "GLX_SGIX_pbuffer")) {
         if (GLX_drv->glXCreateGLXPbufferSGIX &&
             GLX_drv->glXDestroyGLXPbufferSGIX &&
             GLX_dpy->have_fbconfig)
            GLX_dpy->have_pbuffer = EGL_TRUE;
      }
   }

   if (GLX_dpy->glx_maj == 1 && GLX_dpy->glx_min >= 3) {
      GLX_dpy->have_1_3 = EGL_TRUE;
      GLX_dpy->have_make_current_read = EGL_TRUE;
      GLX_dpy->have_fbconfig = EGL_TRUE;
      GLX_dpy->have_pbuffer = EGL_TRUE;
   }
}


static void
check_quirks(struct GLX_egl_driver *GLX_drv,
             struct GLX_egl_display *GLX_dpy, EGLint screen)
{
   const char *vendor;

   GLX_dpy->single_buffered_quirk = EGL_TRUE;
   GLX_dpy->glx_window_quirk = EGL_TRUE;

   vendor = GLX_drv->glXGetClientString(GLX_dpy->dpy, GLX_VENDOR);
   if (vendor && strstr(vendor, "NVIDIA")) {
      vendor = GLX_drv->glXQueryServerString(GLX_dpy->dpy, screen, GLX_VENDOR);
      if (vendor && strstr(vendor, "NVIDIA")) {
         _eglLog(_EGL_DEBUG, "disable quirks");
         GLX_dpy->single_buffered_quirk = EGL_FALSE;
         GLX_dpy->glx_window_quirk = EGL_FALSE;
      }
   }
}


/**
 * Called via eglInitialize(), GLX_drv->API.Initialize().
 */
static EGLBoolean
GLX_eglInitialize(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   struct GLX_egl_display *GLX_dpy;

   if (disp->Platform != _EGL_PLATFORM_X11)
      return EGL_FALSE;

   /* this is a fallback driver */
   if (!disp->Options.UseFallback)
      return EGL_FALSE;

   if (disp->Options.TestOnly)
      return EGL_TRUE;

   GLX_dpy = CALLOC_STRUCT(GLX_egl_display);
   if (!GLX_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   GLX_dpy->dpy = (Display *) disp->PlatformDisplay;
   if (!GLX_dpy->dpy) {
      GLX_dpy->dpy = XOpenDisplay(NULL);
      if (!GLX_dpy->dpy) {
         _eglLog(_EGL_WARNING, "GLX: XOpenDisplay failed");
         free(GLX_dpy);
         return EGL_FALSE;
      }
   }

   if (!GLX_drv->glXQueryVersion(GLX_dpy->dpy,
            &GLX_dpy->glx_maj, &GLX_dpy->glx_min)) {
      _eglLog(_EGL_WARNING, "GLX: glXQueryVersion failed");
      if (!disp->PlatformDisplay)
         XCloseDisplay(GLX_dpy->dpy);
      free(GLX_dpy);
      return EGL_FALSE;
   }

   disp->DriverData = (void *) GLX_dpy;
   disp->ClientAPIs = EGL_OPENGL_BIT;

   check_extensions(GLX_drv, GLX_dpy, DefaultScreen(GLX_dpy->dpy));
   check_quirks(GLX_drv, GLX_dpy, DefaultScreen(GLX_dpy->dpy));

   create_configs(drv, disp, DefaultScreen(GLX_dpy->dpy));
   if (!_eglGetArraySize(disp->Configs)) {
      _eglLog(_EGL_WARNING, "GLX: failed to create any config");
      if (!disp->PlatformDisplay)
         XCloseDisplay(GLX_dpy->dpy);
      free(GLX_dpy);
      return EGL_FALSE;
   }

   /* we're supporting EGL 1.4 */
   disp->VersionMajor = 1;
   disp->VersionMinor = 4;

   return EGL_TRUE;
}


/**
 * Called via eglTerminate(), drv->API.Terminate().
 */
static EGLBoolean
GLX_eglTerminate(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);

   _eglReleaseDisplayResources(drv, disp);
   _eglCleanupDisplay(disp);

   if (GLX_dpy->visuals)
      XFree(GLX_dpy->visuals);
   if (GLX_dpy->fbconfigs)
      XFree(GLX_dpy->fbconfigs);

   if (!disp->PlatformDisplay)
      XCloseDisplay(GLX_dpy->dpy);
   free(GLX_dpy);

   disp->DriverData = NULL;

   return EGL_TRUE;
}


/**
 * Called via eglCreateContext(), drv->API.CreateContext().
 */
static _EGLContext *
GLX_eglCreateContext(_EGLDriver *drv, _EGLDisplay *disp, _EGLConfig *conf,
                      _EGLContext *share_list, const EGLint *attrib_list)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   struct GLX_egl_context *GLX_ctx = CALLOC_STRUCT(GLX_egl_context);
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_context *GLX_ctx_shared = GLX_egl_context(share_list);

   if (!GLX_ctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      return NULL;
   }

   if (!_eglInitContext(&GLX_ctx->Base, disp, conf, attrib_list)) {
      free(GLX_ctx);
      return NULL;
   }

   if (GLX_dpy->have_fbconfig) {
      GLX_ctx->context = GLX_drv->glXCreateNewContext(GLX_dpy->dpy,
            GLX_dpy->fbconfigs[GLX_egl_config_index(conf)],
            GLX_RGBA_TYPE,
            GLX_ctx_shared ? GLX_ctx_shared->context : NULL,
            GL_TRUE);
   }
   else {
      GLX_ctx->context = GLX_drv->glXCreateContext(GLX_dpy->dpy,
            &GLX_dpy->visuals[GLX_egl_config_index(conf)],
            GLX_ctx_shared ? GLX_ctx_shared->context : NULL,
            GL_TRUE);
   }
   if (!GLX_ctx->context) {
      free(GLX_ctx);
      return NULL;
   }

   return &GLX_ctx->Base;
}

/**
 * Called via eglDestroyContext(), drv->API.DestroyContext().
 */
static EGLBoolean
GLX_eglDestroyContext(_EGLDriver *drv, _EGLDisplay *disp, _EGLContext *ctx)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_context *GLX_ctx = GLX_egl_context(ctx);

   if (_eglPutContext(ctx)) {
      assert(GLX_ctx);
      GLX_drv->glXDestroyContext(GLX_dpy->dpy, GLX_ctx->context);

      free(GLX_ctx);
   }

   return EGL_TRUE;
}

/**
 * Destroy a surface.  The display is allowed to be uninitialized.
 */
static void
destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
{
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_surf = GLX_egl_surface(surf);

   if (GLX_surf->destroy)
      GLX_surf->destroy(GLX_dpy->dpy, GLX_surf->glx_drawable);

   free(GLX_surf);
}


/**
 * Called via eglMakeCurrent(), drv->API.MakeCurrent().
 */
static EGLBoolean
GLX_eglMakeCurrent(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *dsurf,
                   _EGLSurface *rsurf, _EGLContext *ctx)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_dsurf = GLX_egl_surface(dsurf);
   struct GLX_egl_surface *GLX_rsurf = GLX_egl_surface(rsurf);
   struct GLX_egl_context *GLX_ctx = GLX_egl_context(ctx);
   _EGLContext *old_ctx;
   _EGLSurface *old_dsurf, *old_rsurf;
   GLXDrawable ddraw, rdraw;
   GLXContext cctx;
   EGLBoolean ret = EGL_FALSE;

   /* make new bindings */
   if (!_eglBindContext(ctx, dsurf, rsurf, &old_ctx, &old_dsurf, &old_rsurf))
      return EGL_FALSE;

   ddraw = (GLX_dsurf) ? GLX_dsurf->glx_drawable : None;
   rdraw = (GLX_rsurf) ? GLX_rsurf->glx_drawable : None;
   cctx = (GLX_ctx) ? GLX_ctx->context : NULL;

   if (GLX_dpy->have_make_current_read)
      ret = GLX_drv->glXMakeContextCurrent(GLX_dpy->dpy, ddraw, rdraw, cctx);
   else if (ddraw == rdraw)
      ret = GLX_drv->glXMakeCurrent(GLX_dpy->dpy, ddraw, cctx);

   if (ret) {
      if (_eglPutSurface(old_dsurf))
         destroy_surface(disp, old_dsurf);
      if (_eglPutSurface(old_rsurf))
         destroy_surface(disp, old_rsurf);
      /* no destroy? */
      _eglPutContext(old_ctx);
   }
   else {
      /* undo the previous _eglBindContext */
      _eglBindContext(old_ctx, old_dsurf, old_rsurf, &ctx, &dsurf, &rsurf);
      assert(&GLX_ctx->Base == ctx &&
             &GLX_dsurf->Base == dsurf &&
             &GLX_rsurf->Base == rsurf);

      _eglPutSurface(dsurf);
      _eglPutSurface(rsurf);
      _eglPutContext(ctx);

      _eglPutSurface(old_dsurf);
      _eglPutSurface(old_rsurf);
      _eglPutContext(old_ctx);
   }

   return ret;
}

/** Get size of given window */
static Status
get_drawable_size(Display *dpy, Drawable d, unsigned *width, unsigned *height)
{
   Window root;
   Status stat;
   int xpos, ypos;
   unsigned int w, h, bw, depth;
   stat = XGetGeometry(dpy, d, &root, &xpos, &ypos, &w, &h, &bw, &depth);
   *width = w;
   *height = h;
   return stat;
}

/**
 * Called via eglCreateWindowSurface(), drv->API.CreateWindowSurface().
 */
static _EGLSurface *
GLX_eglCreateWindowSurface(_EGLDriver *drv, _EGLDisplay *disp,
                           _EGLConfig *conf, EGLNativeWindowType window,
                           const EGLint *attrib_list)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_surf;
   unsigned width, height;

   GLX_surf = CALLOC_STRUCT(GLX_egl_surface);
   if (!GLX_surf) {
      _eglError(EGL_BAD_ALLOC, "eglCreateWindowSurface");
      return NULL;
   }

   if (!_eglInitSurface(&GLX_surf->Base, disp, EGL_WINDOW_BIT,
                        conf, attrib_list)) {
      free(GLX_surf);
      return NULL;
   }

   GLX_surf->drawable = window;

   if (GLX_dpy->have_1_3 && !GLX_dpy->glx_window_quirk) {
      GLX_surf->glx_drawable = GLX_drv->glXCreateWindow(GLX_dpy->dpy,
            GLX_dpy->fbconfigs[GLX_egl_config_index(conf)],
            GLX_surf->drawable, NULL);
   }
   else {
      GLX_surf->glx_drawable = GLX_surf->drawable;
   }

   if (!GLX_surf->glx_drawable) {
      free(GLX_surf);
      return NULL;
   }

   if (GLX_dpy->have_1_3 && !GLX_dpy->glx_window_quirk)
      GLX_surf->destroy = GLX_drv->glXDestroyWindow;

   get_drawable_size(GLX_dpy->dpy, window, &width, &height);
   GLX_surf->Base.Width = width;
   GLX_surf->Base.Height = height;

   return &GLX_surf->Base;
}

static _EGLSurface *
GLX_eglCreatePixmapSurface(_EGLDriver *drv, _EGLDisplay *disp,
                           _EGLConfig *conf, EGLNativePixmapType pixmap,
                           const EGLint *attrib_list)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_surf;
   unsigned width, height;

   GLX_surf = CALLOC_STRUCT(GLX_egl_surface);
   if (!GLX_surf) {
      _eglError(EGL_BAD_ALLOC, "eglCreatePixmapSurface");
      return NULL;
   }

   if (!_eglInitSurface(&GLX_surf->Base, disp, EGL_PIXMAP_BIT,
                        conf, attrib_list)) {
      free(GLX_surf);
      return NULL;
   }

   GLX_surf->drawable = pixmap;

   if (GLX_dpy->have_1_3) {
      GLX_surf->glx_drawable = GLX_drv->glXCreatePixmap(GLX_dpy->dpy,
            GLX_dpy->fbconfigs[GLX_egl_config_index(conf)],
            GLX_surf->drawable, NULL);
   }
   else if (GLX_dpy->have_fbconfig) {
      GLXFBConfig fbconfig = GLX_dpy->fbconfigs[GLX_egl_config_index(conf)];
      XVisualInfo *vinfo;

      vinfo = GLX_drv->glXGetVisualFromFBConfig(GLX_dpy->dpy, fbconfig);
      if (vinfo) {
         GLX_surf->glx_drawable = GLX_drv->glXCreateGLXPixmap(GLX_dpy->dpy,
               vinfo, GLX_surf->drawable);
         XFree(vinfo);
      }
   }
   else {
      GLX_surf->glx_drawable = GLX_drv->glXCreateGLXPixmap(GLX_dpy->dpy,
            &GLX_dpy->visuals[GLX_egl_config_index(conf)],
            GLX_surf->drawable);
   }

   if (!GLX_surf->glx_drawable) {
      free(GLX_surf);
      return NULL;
   }

   GLX_surf->destroy = (GLX_dpy->have_1_3) ?
      GLX_drv->glXDestroyPixmap : GLX_drv->glXDestroyGLXPixmap;

   get_drawable_size(GLX_dpy->dpy, pixmap, &width, &height);
   GLX_surf->Base.Width = width;
   GLX_surf->Base.Height = height;

   return &GLX_surf->Base;
}

static _EGLSurface *
GLX_eglCreatePbufferSurface(_EGLDriver *drv, _EGLDisplay *disp,
                            _EGLConfig *conf, const EGLint *attrib_list)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_surf;
   int attribs[5];
   int i;

   GLX_surf = CALLOC_STRUCT(GLX_egl_surface);
   if (!GLX_surf) {
      _eglError(EGL_BAD_ALLOC, "eglCreatePbufferSurface");
      return NULL;
   }

   if (!_eglInitSurface(&GLX_surf->Base, disp, EGL_PBUFFER_BIT,
                        conf, attrib_list)) {
      free(GLX_surf);
      return NULL;
   }

   i = 0;
   attribs[i] = None;

   GLX_surf->drawable = None;

   if (GLX_dpy->have_1_3) {
      /* put geometry in attribs */
      if (GLX_surf->Base.Width) {
         attribs[i++] = GLX_PBUFFER_WIDTH;
         attribs[i++] = GLX_surf->Base.Width;
      }
      if (GLX_surf->Base.Height) {
         attribs[i++] = GLX_PBUFFER_HEIGHT;
         attribs[i++] = GLX_surf->Base.Height;
      }
      attribs[i] = None;

      GLX_surf->glx_drawable = GLX_drv->glXCreatePbuffer(GLX_dpy->dpy,
            GLX_dpy->fbconfigs[GLX_egl_config_index(conf)], attribs);
   }
   else if (GLX_dpy->have_pbuffer) {
      GLX_surf->glx_drawable = GLX_drv->glXCreateGLXPbufferSGIX(GLX_dpy->dpy,
            GLX_dpy->fbconfigs[GLX_egl_config_index(conf)],
            GLX_surf->Base.Width,
            GLX_surf->Base.Height,
            attribs);
   }

   if (!GLX_surf->glx_drawable) {
      free(GLX_surf);
      return NULL;
   }

   GLX_surf->destroy = (GLX_dpy->have_1_3) ?
      GLX_drv->glXDestroyPbuffer : GLX_drv->glXDestroyGLXPbufferSGIX;

   return &GLX_surf->Base;
}


static EGLBoolean
GLX_eglDestroySurface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
   (void) drv;

   if (_eglPutSurface(surf))
      destroy_surface(disp, surf);

   return EGL_TRUE;
}


static EGLBoolean
GLX_eglSwapBuffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_surf = GLX_egl_surface(draw);

   GLX_drv->glXSwapBuffers(GLX_dpy->dpy, GLX_surf->glx_drawable);

   return EGL_TRUE;
}

/*
 * Called from eglGetProcAddress() via drv->API.GetProcAddress().
 */
static _EGLProc
GLX_eglGetProcAddress(_EGLDriver *drv, const char *procname)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);

   return (_EGLProc) GLX_drv->glXGetProcAddress((const GLubyte *) procname);
}

static EGLBoolean
GLX_eglWaitClient(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);

   (void) dpy;
   (void) ctx;

   GLX_drv->glXWaitGL();
   return EGL_TRUE;
}

static EGLBoolean
GLX_eglWaitNative(_EGLDriver *drv, _EGLDisplay *dpy, EGLint engine)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);

   (void) dpy;

   if (engine != EGL_CORE_NATIVE_ENGINE)
      return _eglError(EGL_BAD_PARAMETER, "eglWaitNative");
   GLX_drv->glXWaitX();
   return EGL_TRUE;
}

static void
GLX_Unload(_EGLDriver *drv)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);

   if (GLX_drv->handle)
      dlclose(GLX_drv->handle);
   free(GLX_drv);
}


static EGLBoolean
GLX_Load(_EGLDriver *drv)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   void *handle = NULL;

   GLX_drv->glXGetProcAddress = dlsym(RTLD_DEFAULT, "glXGetProcAddress");
   if (!GLX_drv->glXGetProcAddress)
      GLX_drv->glXGetProcAddress = dlsym(RTLD_DEFAULT, "glXGetProcAddressARB");
   if (!GLX_drv->glXGetProcAddress) {
      handle = dlopen("libGL.so", RTLD_LAZY | RTLD_LOCAL);
      if (!handle)
         goto fail;

      GLX_drv->glXGetProcAddress = dlsym(handle, "glXGetProcAddress");
      if (!GLX_drv->glXGetProcAddress)
         GLX_drv->glXGetProcAddress = dlsym(handle, "glXGetProcAddressARB");
      if (!GLX_drv->glXGetProcAddress)
         goto fail;
   }

#define GET_PROC(proc_type, proc_name, check)                        \
   do {                                                              \
      GLX_drv->proc_name = (proc_type)                               \
         GLX_drv->glXGetProcAddress((const GLubyte *) #proc_name);   \
      if (check && !GLX_drv->proc_name) goto fail;                   \
   } while (0)

   /* GLX 1.0 */
   GET_PROC(GLXCREATECONTEXTPROC, glXCreateContext, EGL_TRUE);
   GET_PROC(GLXDESTROYCONTEXTPROC, glXDestroyContext, EGL_TRUE);
   GET_PROC(GLXMAKECURRENTPROC, glXMakeCurrent, EGL_TRUE);
   GET_PROC(GLXSWAPBUFFERSPROC, glXSwapBuffers, EGL_TRUE);
   GET_PROC(GLXCREATEGLXPIXMAPPROC, glXCreateGLXPixmap, EGL_TRUE);
   GET_PROC(GLXDESTROYGLXPIXMAPPROC, glXDestroyGLXPixmap, EGL_TRUE);
   GET_PROC(GLXQUERYVERSIONPROC, glXQueryVersion, EGL_TRUE);
   GET_PROC(GLXGETCONFIGPROC, glXGetConfig, EGL_TRUE);
   GET_PROC(GLXWAITGLPROC, glXWaitGL, EGL_TRUE);
   GET_PROC(GLXWAITXPROC, glXWaitX, EGL_TRUE);

   /* GLX 1.1 */
   GET_PROC(GLXQUERYEXTENSIONSSTRINGPROC, glXQueryExtensionsString, EGL_TRUE);
   GET_PROC(GLXQUERYSERVERSTRINGPROC, glXQueryServerString, EGL_TRUE);
   GET_PROC(GLXGETCLIENTSTRINGPROC, glXGetClientString, EGL_TRUE);

   /* GLX 1.3 */
   GET_PROC(PFNGLXGETFBCONFIGSPROC, glXGetFBConfigs, EGL_FALSE);
   GET_PROC(PFNGLXGETFBCONFIGATTRIBPROC, glXGetFBConfigAttrib, EGL_FALSE);
   GET_PROC(PFNGLXGETVISUALFROMFBCONFIGPROC, glXGetVisualFromFBConfig, EGL_FALSE);
   GET_PROC(PFNGLXCREATEWINDOWPROC, glXCreateWindow, EGL_FALSE);
   GET_PROC(PFNGLXDESTROYWINDOWPROC, glXDestroyWindow, EGL_FALSE);
   GET_PROC(PFNGLXCREATEPIXMAPPROC, glXCreatePixmap, EGL_FALSE);
   GET_PROC(PFNGLXDESTROYPIXMAPPROC, glXDestroyPixmap, EGL_FALSE);
   GET_PROC(PFNGLXCREATEPBUFFERPROC, glXCreatePbuffer, EGL_FALSE);
   GET_PROC(PFNGLXDESTROYPBUFFERPROC, glXDestroyPbuffer, EGL_FALSE);
   GET_PROC(PFNGLXCREATENEWCONTEXTPROC, glXCreateNewContext, EGL_FALSE);
   GET_PROC(PFNGLXMAKECONTEXTCURRENTPROC, glXMakeContextCurrent, EGL_FALSE);

   /* GLX_SGIX_pbuffer */
   GET_PROC(PFNGLXCREATEGLXPBUFFERSGIXPROC,
         glXCreateGLXPbufferSGIX, EGL_FALSE);
   GET_PROC(PFNGLXDESTROYGLXPBUFFERSGIXPROC,
         glXDestroyGLXPbufferSGIX, EGL_FALSE);
#undef GET_PROC

   GLX_drv->handle = handle;

   return EGL_TRUE;

fail:
   if (handle)
      dlclose(handle);
   return EGL_FALSE;
}


/**
 * This is the main entrypoint into the driver, called by libEGL.
 * Create a new _EGLDriver object and init its dispatch table.
 */
_EGLDriver *
_eglBuiltInDriverGLX(const char *args)
{
   struct GLX_egl_driver *GLX_drv = CALLOC_STRUCT(GLX_egl_driver);

   (void) args;

   if (!GLX_drv)
      return NULL;

   if (!GLX_Load(&GLX_drv->Base)) {
      _eglLog(_EGL_WARNING, "GLX: failed to load GLX");
      free(GLX_drv);
      return NULL;
   }

   _eglInitDriverFallbacks(&GLX_drv->Base);
   GLX_drv->Base.API.Initialize = GLX_eglInitialize;
   GLX_drv->Base.API.Terminate = GLX_eglTerminate;
   GLX_drv->Base.API.CreateContext = GLX_eglCreateContext;
   GLX_drv->Base.API.DestroyContext = GLX_eglDestroyContext;
   GLX_drv->Base.API.MakeCurrent = GLX_eglMakeCurrent;
   GLX_drv->Base.API.CreateWindowSurface = GLX_eglCreateWindowSurface;
   GLX_drv->Base.API.CreatePixmapSurface = GLX_eglCreatePixmapSurface;
   GLX_drv->Base.API.CreatePbufferSurface = GLX_eglCreatePbufferSurface;
   GLX_drv->Base.API.DestroySurface = GLX_eglDestroySurface;
   GLX_drv->Base.API.SwapBuffers = GLX_eglSwapBuffers;
   GLX_drv->Base.API.GetProcAddress = GLX_eglGetProcAddress;
   GLX_drv->Base.API.WaitClient = GLX_eglWaitClient;
   GLX_drv->Base.API.WaitNative = GLX_eglWaitNative;

   GLX_drv->Base.Name = "GLX";
   GLX_drv->Base.Unload = GLX_Unload;

   return &GLX_drv->Base;
}
