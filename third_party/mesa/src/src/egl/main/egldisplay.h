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


#ifndef EGLDISPLAY_INCLUDED
#define EGLDISPLAY_INCLUDED


#include "egltypedefs.h"
#include "egldefines.h"
#include "eglmutex.h"
#include "eglarray.h"


enum _egl_platform_type {
   _EGL_PLATFORM_WINDOWS,
   _EGL_PLATFORM_X11,
   _EGL_PLATFORM_WAYLAND,
   _EGL_PLATFORM_DRM,
   _EGL_PLATFORM_FBDEV,
   _EGL_PLATFORM_NULL,
   _EGL_PLATFORM_ANDROID,

   _EGL_NUM_PLATFORMS,
   _EGL_INVALID_PLATFORM = -1
};
typedef enum _egl_platform_type _EGLPlatformType;


enum _egl_resource_type {
   _EGL_RESOURCE_CONTEXT,
   _EGL_RESOURCE_SURFACE,
   _EGL_RESOURCE_IMAGE,
   _EGL_RESOURCE_SYNC,

   _EGL_NUM_RESOURCES
};
/* this cannot and need not go into egltypedefs.h */
typedef enum _egl_resource_type _EGLResourceType;


/**
 * A resource of a display.
 */
struct _egl_resource
{
   /* which display the resource belongs to */
   _EGLDisplay *Display;
   EGLBoolean IsLinked;
   EGLint RefCount;

   /* used to link resources of the same type */
   _EGLResource *Next;
};


/**
 * Optional EGL extensions info.
 */
struct _egl_extensions
{
   EGLBoolean MESA_screen_surface;
   EGLBoolean MESA_copy_context;
   EGLBoolean MESA_drm_display;
   EGLBoolean MESA_drm_image;

   EGLBoolean WL_bind_wayland_display;

   EGLBoolean KHR_image_base;
   EGLBoolean KHR_image_pixmap;
   EGLBoolean KHR_vg_parent_image;
   EGLBoolean KHR_gl_texture_2D_image;
   EGLBoolean KHR_gl_texture_cubemap_image;
   EGLBoolean KHR_gl_texture_3D_image;
   EGLBoolean KHR_gl_renderbuffer_image;

   EGLBoolean KHR_reusable_sync;
   EGLBoolean KHR_fence_sync;

   EGLBoolean KHR_surfaceless_context;
   EGLBoolean KHR_create_context;

   EGLBoolean NOK_swap_region;
   EGLBoolean NOK_texture_from_pixmap;

   EGLBoolean ANDROID_image_native_buffer;

   EGLBoolean NV_post_sub_buffer;

   EGLBoolean EXT_create_context_robustness;
};


struct _egl_display
{
   /* used to link displays */
   _EGLDisplay *Next;

   _EGLMutex Mutex;

   _EGLPlatformType Platform; /**< The type of the platform display */
   void *PlatformDisplay;     /**< A pointer to the platform display */

   _EGLDriver *Driver;        /**< Matched driver of the display */
   EGLBoolean Initialized;    /**< True if the display is initialized */

   /* options that affect how the driver initializes the display */
   struct {
      EGLBoolean TestOnly;    /**< Driver should not set fields when true */
      EGLBoolean UseFallback; /**< Use fallback driver (sw or less features) */
   } Options;

   /* these fields are set by the driver during init */
   void *DriverData;          /**< Driver private data */
   EGLint VersionMajor;       /**< EGL major version */
   EGLint VersionMinor;       /**< EGL minor version */
   EGLint ClientAPIs;         /**< Bitmask of APIs supported (EGL_xxx_BIT) */
   _EGLExtensions Extensions; /**< Extensions supported */

   /* these fields are derived from above */
   char VersionString[1000];                       /**< EGL_VERSION */
   char ClientAPIsString[1000];                    /**< EGL_CLIENT_APIS */
   char ExtensionsString[_EGL_MAX_EXTENSIONS_LEN]; /**< EGL_EXTENSIONS */

   _EGLArray *Screens;
   _EGLArray *Configs;

   /* lists of resources */
   _EGLResource *ResourceLists[_EGL_NUM_RESOURCES];
};


extern _EGLPlatformType
_eglGetNativePlatform(EGLNativeDisplayType nativeDisplay);


extern void
_eglFiniDisplay(void);


extern _EGLDisplay *
_eglFindDisplay(_EGLPlatformType plat, void *plat_dpy);


PUBLIC void
_eglReleaseDisplayResources(_EGLDriver *drv, _EGLDisplay *dpy);


PUBLIC void
_eglCleanupDisplay(_EGLDisplay *disp);


extern EGLBoolean
_eglCheckDisplayHandle(EGLDisplay dpy);


PUBLIC EGLBoolean
_eglCheckResource(void *res, _EGLResourceType type, _EGLDisplay *dpy);


/**
 * Lookup a handle to find the linked display.
 * Return NULL if the handle has no corresponding linked display.
 */
static INLINE _EGLDisplay *
_eglLookupDisplay(EGLDisplay display)
{
   _EGLDisplay *dpy = (_EGLDisplay *) display;
   if (!_eglCheckDisplayHandle(display))
      dpy = NULL;
   return dpy;
}


/**
 * Return the handle of a linked display, or EGL_NO_DISPLAY.
 */
static INLINE EGLDisplay
_eglGetDisplayHandle(_EGLDisplay *dpy)
{
   return (EGLDisplay) ((dpy) ? dpy : EGL_NO_DISPLAY);
}


extern void
_eglInitResource(_EGLResource *res, EGLint size, _EGLDisplay *dpy);


PUBLIC void
_eglGetResource(_EGLResource *res);


PUBLIC EGLBoolean
_eglPutResource(_EGLResource *res);


extern void
_eglLinkResource(_EGLResource *res, _EGLResourceType type);


extern void
_eglUnlinkResource(_EGLResource *res, _EGLResourceType type);


/**
 * Return true if the resource is linked.
 */
static INLINE EGLBoolean
_eglIsResourceLinked(_EGLResource *res)
{
   return res->IsLinked;
}


#endif /* EGLDISPLAY_INCLUDED */
