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


#ifndef EGLAPI_INCLUDED
#define EGLAPI_INCLUDED

/**
 * A generic function ptr type
 */
typedef void (*_EGLProc)(void);


/**
 * Typedefs for all EGL API entrypoint functions.
 */

/* driver funcs */
typedef EGLBoolean (*Initialize_t)(_EGLDriver *, _EGLDisplay *dpy);
typedef EGLBoolean (*Terminate_t)(_EGLDriver *, _EGLDisplay *dpy);

/* config funcs */
typedef EGLBoolean (*GetConfigs_t)(_EGLDriver *drv, _EGLDisplay *dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
typedef EGLBoolean (*ChooseConfig_t)(_EGLDriver *drv, _EGLDisplay *dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
typedef EGLBoolean (*GetConfigAttrib_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *config, EGLint attribute, EGLint *value);

/* context funcs */
typedef _EGLContext *(*CreateContext_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *config, _EGLContext *share_list, const EGLint *attrib_list);
typedef EGLBoolean (*DestroyContext_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx);
/* this is the only function (other than Initialize) that may be called with an uninitialized display */
typedef EGLBoolean (*MakeCurrent_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw, _EGLSurface *read, _EGLContext *ctx);
typedef EGLBoolean (*QueryContext_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx, EGLint attribute, EGLint *value);

/* surface funcs */
typedef _EGLSurface *(*CreateWindowSurface_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *config, EGLNativeWindowType window, const EGLint *attrib_list);
typedef _EGLSurface *(*CreatePixmapSurface_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *config, EGLNativePixmapType pixmap, const EGLint *attrib_list);
typedef _EGLSurface *(*CreatePbufferSurface_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *config, const EGLint *attrib_list);
typedef EGLBoolean (*DestroySurface_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface);
typedef EGLBoolean (*QuerySurface_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface, EGLint attribute, EGLint *value);
typedef EGLBoolean (*SurfaceAttrib_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface, EGLint attribute, EGLint value);
typedef EGLBoolean (*BindTexImage_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface, EGLint buffer);
typedef EGLBoolean (*ReleaseTexImage_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface, EGLint buffer);
typedef EGLBoolean (*SwapInterval_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint interval);
typedef EGLBoolean (*SwapBuffers_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw);
typedef EGLBoolean (*CopyBuffers_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface, EGLNativePixmapType target);

/* misc funcs */
typedef const char *(*QueryString_t)(_EGLDriver *drv, _EGLDisplay *dpy, EGLint name);
typedef EGLBoolean (*WaitClient_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx);
typedef EGLBoolean (*WaitNative_t)(_EGLDriver *drv, _EGLDisplay *dpy, EGLint engine);

/* this function may be called from multiple threads at the same time */
typedef _EGLProc (*GetProcAddress_t)(_EGLDriver *drv, const char *procname);



#ifdef EGL_MESA_screen_surface
typedef EGLBoolean (*ChooseModeMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *screen, const EGLint *attrib_list, EGLModeMESA *modes, EGLint modes_size, EGLint *num_modes);
typedef EGLBoolean (*GetModesMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *screen, EGLModeMESA *modes, EGLint mode_size, EGLint *num_mode);
typedef EGLBoolean (*GetModeAttribMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLMode *mode, EGLint attribute, EGLint *value);
typedef EGLBoolean (*CopyContextMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *source, _EGLContext *dest, EGLint mask);
typedef EGLBoolean (*GetScreensMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, EGLScreenMESA *screens, EGLint max_screens, EGLint *num_screens);
typedef _EGLSurface *(*CreateScreenSurfaceMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *config, const EGLint *attrib_list);
typedef EGLBoolean (*ShowScreenSurfaceMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *screen, _EGLSurface *surface, _EGLMode *mode);
typedef EGLBoolean (*ScreenPositionMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *screen, EGLint x, EGLint y);
typedef EGLBoolean (*QueryScreenMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *screen, EGLint attribute, EGLint *value);
typedef EGLBoolean (*QueryScreenSurfaceMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *screen, _EGLSurface **surface);
typedef EGLBoolean (*QueryScreenModeMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *screen, _EGLMode **mode);
typedef const char * (*QueryModeStringMESA_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLMode *mode);
#endif /* EGL_MESA_screen_surface */


typedef _EGLSurface *(*CreatePbufferFromClientBuffer_t)(_EGLDriver *drv, _EGLDisplay *dpy, EGLenum buftype, EGLClientBuffer buffer, _EGLConfig *config, const EGLint *attrib_list);


typedef _EGLImage *(*CreateImageKHR_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attr_list);
typedef EGLBoolean (*DestroyImageKHR_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *image);


typedef _EGLSync *(*CreateSyncKHR_t)(_EGLDriver *drv, _EGLDisplay *dpy, EGLenum type, const EGLint *attrib_list);
typedef EGLBoolean (*DestroySyncKHR_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync);
typedef EGLint (*ClientWaitSyncKHR_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync, EGLint flags, EGLTimeKHR timeout);
typedef EGLBoolean (*SignalSyncKHR_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync, EGLenum mode);
typedef EGLBoolean (*GetSyncAttribKHR_t)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync, EGLint attribute, EGLint *value);


#ifdef EGL_NOK_swap_region
typedef EGLBoolean (*SwapBuffersRegionNOK_t)(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf, EGLint numRects, const EGLint *rects);
#endif

#ifdef EGL_MESA_drm_image
typedef _EGLImage *(*CreateDRMImageMESA_t)(_EGLDriver *drv, _EGLDisplay *disp, const EGLint *attr_list);
typedef EGLBoolean (*ExportDRMImageMESA_t)(_EGLDriver *drv, _EGLDisplay *disp, _EGLImage *img, EGLint *name, EGLint *handle, EGLint *stride);
#endif

#ifdef EGL_WL_bind_wayland_display
struct wl_display;
typedef EGLBoolean (*BindWaylandDisplayWL_t)(_EGLDriver *drv, _EGLDisplay *disp, struct wl_display *display);
typedef EGLBoolean (*UnbindWaylandDisplayWL_t)(_EGLDriver *drv, _EGLDisplay *disp, struct wl_display *display);
typedef EGLBoolean (*QueryWaylandBufferWL_t)(_EGLDriver *drv, _EGLDisplay *displ, struct wl_buffer *buffer, EGLint attribute, EGLint *value);
#endif

typedef EGLBoolean (*PostSubBufferNV_t)(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surface, EGLint x, EGLint y, EGLint width, EGLint height);

/**
 * The API dispatcher jumps through these functions
 */
struct _egl_api
{
   Initialize_t Initialize;
   Terminate_t Terminate;

   GetConfigs_t GetConfigs;
   ChooseConfig_t ChooseConfig;
   GetConfigAttrib_t GetConfigAttrib;

   CreateContext_t CreateContext;
   DestroyContext_t DestroyContext;
   MakeCurrent_t MakeCurrent;
   QueryContext_t QueryContext;

   CreateWindowSurface_t CreateWindowSurface;
   CreatePixmapSurface_t CreatePixmapSurface;
   CreatePbufferSurface_t CreatePbufferSurface;
   DestroySurface_t DestroySurface;
   QuerySurface_t QuerySurface;
   SurfaceAttrib_t SurfaceAttrib;
   BindTexImage_t BindTexImage;
   ReleaseTexImage_t ReleaseTexImage;
   SwapInterval_t SwapInterval;
   SwapBuffers_t SwapBuffers;
   CopyBuffers_t CopyBuffers;

   QueryString_t QueryString;
   WaitClient_t WaitClient;
   WaitNative_t WaitNative;
   GetProcAddress_t GetProcAddress;

#ifdef EGL_MESA_screen_surface
   ChooseModeMESA_t ChooseModeMESA;
   GetModesMESA_t GetModesMESA;
   GetModeAttribMESA_t GetModeAttribMESA;
   CopyContextMESA_t CopyContextMESA;
   GetScreensMESA_t GetScreensMESA;
   CreateScreenSurfaceMESA_t CreateScreenSurfaceMESA;
   ShowScreenSurfaceMESA_t ShowScreenSurfaceMESA;
   ScreenPositionMESA_t ScreenPositionMESA;
   QueryScreenMESA_t QueryScreenMESA;
   QueryScreenSurfaceMESA_t QueryScreenSurfaceMESA;
   QueryScreenModeMESA_t QueryScreenModeMESA;
   QueryModeStringMESA_t QueryModeStringMESA;
#endif /* EGL_MESA_screen_surface */

   CreatePbufferFromClientBuffer_t CreatePbufferFromClientBuffer;

   CreateImageKHR_t CreateImageKHR;
   DestroyImageKHR_t DestroyImageKHR;

   CreateSyncKHR_t CreateSyncKHR;
   DestroySyncKHR_t DestroySyncKHR;
   ClientWaitSyncKHR_t ClientWaitSyncKHR;
   SignalSyncKHR_t SignalSyncKHR;
   GetSyncAttribKHR_t GetSyncAttribKHR;

#ifdef EGL_NOK_swap_region
   SwapBuffersRegionNOK_t SwapBuffersRegionNOK;
#endif

#ifdef EGL_MESA_drm_image
   CreateDRMImageMESA_t CreateDRMImageMESA;
   ExportDRMImageMESA_t ExportDRMImageMESA;
#endif

#ifdef EGL_WL_bind_wayland_display
   BindWaylandDisplayWL_t BindWaylandDisplayWL;
   UnbindWaylandDisplayWL_t UnbindWaylandDisplayWL;
   QueryWaylandBufferWL_t QueryWaylandBufferWL;
#endif

   PostSubBufferNV_t PostSubBufferNV;
};

#endif /* EGLAPI_INCLUDED */
