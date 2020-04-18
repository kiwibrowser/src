/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"

#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_string.h"
#include "util/u_atomic.h"

#include "egl_g3d.h"
#include "egl_g3d_api.h"
#include "egl_g3d_st.h"
#include "egl_g3d_loader.h"
#include "native.h"

static void
egl_g3d_invalid_surface(struct native_display *ndpy,
                        struct native_surface *nsurf,
                        unsigned int seq_num)
{
   /* XXX not thread safe? */
   struct egl_g3d_surface *gsurf = egl_g3d_surface(nsurf->user_data);

   if (gsurf && gsurf->stfbi)
      p_atomic_inc(&gsurf->stfbi->stamp);
}

static struct pipe_screen *
egl_g3d_new_drm_screen(struct native_display *ndpy, const char *name, int fd)
{
   _EGLDisplay *dpy = (_EGLDisplay *) ndpy->user_data;
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   return gdpy->loader->create_drm_screen(name, fd);
}

static struct pipe_screen *
egl_g3d_new_sw_screen(struct native_display *ndpy, struct sw_winsys *ws)
{
   _EGLDisplay *dpy = (_EGLDisplay *) ndpy->user_data;
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   return gdpy->loader->create_sw_screen(ws);
}

static struct pipe_resource *
egl_g3d_lookup_egl_image(struct native_display *ndpy, void *egl_image)
{
   _EGLDisplay *dpy = (_EGLDisplay *) ndpy->user_data;
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct st_egl_image img;
   struct pipe_resource *resource = NULL;

   memset(&img, 0, sizeof(img));
   if (gdpy->smapi->get_egl_image(gdpy->smapi, egl_image, &img))
      resource = img.texture;

   return resource;
}

static const struct native_event_handler egl_g3d_native_event_handler = {
   egl_g3d_invalid_surface,
   egl_g3d_new_drm_screen,
   egl_g3d_new_sw_screen,
   egl_g3d_lookup_egl_image
};

/**
 * Get the native platform.
 */
static const struct native_platform *
egl_g3d_get_platform(_EGLDriver *drv, _EGLPlatformType plat)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);

   if (!gdrv->platforms[plat]) {
      const char *plat_name = NULL;
      const struct native_platform *nplat = NULL;

      switch (plat) {
      case _EGL_PLATFORM_WINDOWS:
         plat_name = "Windows";
#ifdef HAVE_GDI_BACKEND
         nplat = native_get_gdi_platform(&egl_g3d_native_event_handler);
#endif
         break;
      case _EGL_PLATFORM_X11:
         plat_name = "X11";
#ifdef HAVE_X11_BACKEND
         nplat = native_get_x11_platform(&egl_g3d_native_event_handler);
#endif
	 break;
      case _EGL_PLATFORM_WAYLAND:
         plat_name = "wayland";
#ifdef HAVE_WAYLAND_BACKEND
         nplat = native_get_wayland_platform(&egl_g3d_native_event_handler);
#endif
         break;
      case _EGL_PLATFORM_DRM:
         plat_name = "DRM";
#ifdef HAVE_DRM_BACKEND
         nplat = native_get_drm_platform(&egl_g3d_native_event_handler);
#endif
         break;
      case _EGL_PLATFORM_FBDEV:
         plat_name = "FBDEV";
#ifdef HAVE_FBDEV_BACKEND
         nplat = native_get_fbdev_platform(&egl_g3d_native_event_handler);
#endif
         break;
      case _EGL_PLATFORM_NULL:
         plat_name = "NULL";
#ifdef HAVE_NULL_BACKEND
         nplat = native_get_null_platform(&egl_g3d_native_event_handler);
#endif
         break;
      case _EGL_PLATFORM_ANDROID:
         plat_name = "Android";
#ifdef HAVE_ANDROID_BACKEND
         nplat = native_get_android_platform(&egl_g3d_native_event_handler);
#endif
         break;
      default:
         break;
      }

      if (!nplat)
         _eglLog(_EGL_WARNING, "unsupported platform %s", plat_name);

      gdrv->platforms[plat] = nplat;
   }

   return gdrv->platforms[plat];
}

#ifdef EGL_MESA_screen_surface

static void
egl_g3d_add_screens(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   const struct native_connector **native_connectors;
   EGLint num_connectors, i;

   native_connectors =
      gdpy->native->modeset->get_connectors(gdpy->native, &num_connectors, NULL);
   if (!num_connectors) {
      if (native_connectors)
         FREE(native_connectors);
      return;
   }

   for (i = 0; i < num_connectors; i++) {
      const struct native_connector *nconn = native_connectors[i];
      struct egl_g3d_screen *gscr;
      const struct native_mode **native_modes;
      EGLint num_modes, j;

      /* TODO support for hotplug */
      native_modes =
         gdpy->native->modeset->get_modes(gdpy->native, nconn, &num_modes);
      if (!num_modes) {
         if (native_modes)
            FREE(native_modes);
         continue;
      }

      gscr = CALLOC_STRUCT(egl_g3d_screen);
      if (!gscr) {
         FREE(native_modes);
         continue;
      }

      _eglInitScreen(&gscr->base, dpy, num_modes);
      for (j = 0; j < gscr->base.NumModes; j++) {
         const struct native_mode *nmode = native_modes[j];
         _EGLMode *mode = &gscr->base.Modes[j];

         mode->Width = nmode->width;
         mode->Height = nmode->height;
         mode->RefreshRate = nmode->refresh_rate;
         mode->Optimal = EGL_FALSE;
         mode->Interlaced = EGL_FALSE;
         /* no need to strdup() */
         mode->Name = nmode->desc;
      }

      gscr->native = nconn;
      gscr->native_modes = native_modes;

      _eglLinkScreen(&gscr->base);
   }

   FREE(native_connectors);
}

#endif /* EGL_MESA_screen_surface */

/**
 * Initialize and validate the EGL config attributes.
 */
static EGLBoolean
init_config_attributes(_EGLConfig *conf, const struct native_config *nconf,
                       EGLint api_mask, enum pipe_format depth_stencil_format,
                       EGLint preserve_buffer, EGLint max_swap_interval,
                       EGLBoolean pre_alpha)
{
   uint rgba[4], depth_stencil[2], buffer_size;
   EGLint surface_type;
   EGLint i;

   /* get the color and depth/stencil component sizes */
   assert(nconf->color_format != PIPE_FORMAT_NONE);
   buffer_size = 0;
   for (i = 0; i < 4; i++) {
      rgba[i] = util_format_get_component_bits(nconf->color_format,
            UTIL_FORMAT_COLORSPACE_RGB, i);
      buffer_size += rgba[i];
   }
   for (i = 0; i < 2; i++) {
      if (depth_stencil_format != PIPE_FORMAT_NONE) {
         depth_stencil[i] =
            util_format_get_component_bits(depth_stencil_format,
               UTIL_FORMAT_COLORSPACE_ZS, i);
      }
      else {
         depth_stencil[i] = 0;
      }
   }

   surface_type = 0x0;
   /* pixmap surfaces should be EGL_SINGLE_BUFFER */
   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_FRONT_LEFT)) {
      if (nconf->pixmap_bit)
         surface_type |= EGL_PIXMAP_BIT;
   }
   /* the others surfaces should be EGL_BACK_BUFFER (or settable) */
   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_BACK_LEFT)) {
      if (nconf->window_bit)
         surface_type |= EGL_WINDOW_BIT;
#ifdef EGL_MESA_screen_surface
      if (nconf->scanout_bit)
         surface_type |= EGL_SCREEN_BIT_MESA;
#endif
      surface_type |= EGL_PBUFFER_BIT;
   }

   if (preserve_buffer)
      surface_type |= EGL_SWAP_BEHAVIOR_PRESERVED_BIT;

   if (pre_alpha && rgba[3]) {
      surface_type |= EGL_VG_ALPHA_FORMAT_PRE_BIT;
      /* st/vega does not support premultiplied alpha yet */
      api_mask &= ~EGL_OPENVG_BIT;
   }

   conf->Conformant = api_mask;
   conf->RenderableType = api_mask;

   conf->RedSize = rgba[0];
   conf->GreenSize = rgba[1];
   conf->BlueSize = rgba[2];
   conf->AlphaSize = rgba[3];
   conf->BufferSize = buffer_size;

   conf->DepthSize = depth_stencil[0];
   conf->StencilSize = depth_stencil[1];

   /* st/vega will allocate the mask on demand */
   if (api_mask & EGL_OPENVG_BIT)
      conf->AlphaMaskSize = 8;

   conf->SurfaceType = surface_type;

   conf->NativeRenderable = EGL_TRUE;
   if (surface_type & EGL_WINDOW_BIT) {
      conf->NativeVisualID = nconf->native_visual_id;
      conf->NativeVisualType = nconf->native_visual_type;
   }

   if (surface_type & EGL_PBUFFER_BIT) {
      conf->BindToTextureRGB = EGL_TRUE;
      if (rgba[3])
         conf->BindToTextureRGBA = EGL_TRUE;

      conf->MaxPbufferWidth = 4096;
      conf->MaxPbufferHeight = 4096;
      conf->MaxPbufferPixels = 4096 * 4096;
   }

   conf->Level = nconf->level;

   if (nconf->transparent_rgb) {
      conf->TransparentType = EGL_TRANSPARENT_RGB;
      conf->TransparentRedValue = nconf->transparent_rgb_values[0];
      conf->TransparentGreenValue = nconf->transparent_rgb_values[1];
      conf->TransparentBlueValue = nconf->transparent_rgb_values[2];
   }

   conf->MinSwapInterval = 0;
   conf->MaxSwapInterval = max_swap_interval;

   return _eglValidateConfig(conf, EGL_FALSE);
}

/**
 * Initialize an EGL config from the native config.
 */
static EGLBoolean
egl_g3d_init_config(_EGLDriver *drv, _EGLDisplay *dpy,
                    _EGLConfig *conf, const struct native_config *nconf,
                    enum pipe_format depth_stencil_format,
                    int preserve_buffer, int max_swap_interval,
                    int pre_alpha)
{
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   EGLint buffer_mask;
   EGLBoolean valid;

   buffer_mask = 0x0;
   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_FRONT_LEFT))
      buffer_mask |= ST_ATTACHMENT_FRONT_LEFT_MASK;
   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_BACK_LEFT))
      buffer_mask |= ST_ATTACHMENT_BACK_LEFT_MASK;
   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_FRONT_RIGHT))
      buffer_mask |= ST_ATTACHMENT_FRONT_RIGHT_MASK;
   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_BACK_RIGHT))
      buffer_mask |= ST_ATTACHMENT_BACK_RIGHT_MASK;

   gconf->stvis.buffer_mask = buffer_mask;
   gconf->stvis.color_format = nconf->color_format;
   gconf->stvis.depth_stencil_format = depth_stencil_format;
   gconf->stvis.accum_format = PIPE_FORMAT_NONE;
   gconf->stvis.samples = 0;

   /* will be overridden per surface */
   gconf->stvis.render_buffer = (buffer_mask & ST_ATTACHMENT_BACK_LEFT_MASK) ?
      ST_ATTACHMENT_BACK_LEFT : ST_ATTACHMENT_FRONT_LEFT;

   valid = init_config_attributes(&gconf->base,
         nconf, dpy->ClientAPIs, depth_stencil_format,
         preserve_buffer, max_swap_interval, pre_alpha);
   if (!valid) {
      _eglLog(_EGL_DEBUG, "skip invalid config 0x%x", nconf->native_visual_id);
      return EGL_FALSE;
   }

   gconf->native = nconf;

   return EGL_TRUE;
}

/**
 * Get all interested depth/stencil formats of a display.
 */
static EGLint
egl_g3d_fill_depth_stencil_formats(_EGLDisplay *dpy,
                                   enum pipe_format formats[8])
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct pipe_screen *screen = gdpy->native->screen;
   const EGLint candidates[] = {
      1, PIPE_FORMAT_Z16_UNORM,
      1, PIPE_FORMAT_Z32_UNORM,
      2, PIPE_FORMAT_Z24_UNORM_S8_UINT, PIPE_FORMAT_S8_UINT_Z24_UNORM,
      2, PIPE_FORMAT_Z24X8_UNORM, PIPE_FORMAT_X8Z24_UNORM,
      0
   };
   const EGLint *fmt = candidates;
   EGLint count;

   count = 0;
   formats[count++] = PIPE_FORMAT_NONE;

   while (*fmt) {
      EGLint i, n = *fmt++;

      /* pick the first supported format */
      for (i = 0; i < n; i++) {
         if (screen->is_format_supported(screen, fmt[i],
                  PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL)) {
            formats[count++] = fmt[i];
            break;
         }
      }

      fmt += n;
   }

   return count;
}

/**
 * Add configs to display and return the next config ID.
 */
static EGLint
egl_g3d_add_configs(_EGLDriver *drv, _EGLDisplay *dpy, EGLint id)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   const struct native_config **native_configs;
   enum pipe_format depth_stencil_formats[8];
   int num_formats, num_configs, i, j;
   int preserve_buffer, max_swap_interval, premultiplied_alpha;

   native_configs = gdpy->native->get_configs(gdpy->native, &num_configs);
   if (!num_configs) {
      if (native_configs)
         FREE(native_configs);
      return id;
   }

   preserve_buffer =
      gdpy->native->get_param(gdpy->native, NATIVE_PARAM_PRESERVE_BUFFER);
   max_swap_interval =
      gdpy->native->get_param(gdpy->native, NATIVE_PARAM_MAX_SWAP_INTERVAL);
   premultiplied_alpha =
      gdpy->native->get_param(gdpy->native, NATIVE_PARAM_PREMULTIPLIED_ALPHA);

   num_formats = egl_g3d_fill_depth_stencil_formats(dpy,
         depth_stencil_formats);

   for (i = 0; i < num_configs; i++) {
      for (j = 0; j < num_formats; j++) {
         struct egl_g3d_config *gconf;

         gconf = CALLOC_STRUCT(egl_g3d_config);
         if (gconf) {
            _eglInitConfig(&gconf->base, dpy, id);
            if (!egl_g3d_init_config(drv, dpy, &gconf->base,
                     native_configs[i], depth_stencil_formats[j],
                     preserve_buffer, max_swap_interval,
                     premultiplied_alpha)) {
               FREE(gconf);
               break;
            }

            _eglLinkConfig(&gconf->base);
            id++;
         }
      }
   }

   FREE(native_configs);
   return id;
}

static void
egl_g3d_free_config(void *conf)
{
   struct egl_g3d_config *gconf = egl_g3d_config((_EGLConfig *) conf);
   FREE(gconf);
}

static void
egl_g3d_free_screen(void *scr)
{
#ifdef EGL_MESA_screen_surface
   struct egl_g3d_screen *gscr = egl_g3d_screen((_EGLScreen *) scr);
   FREE(gscr->native_modes);
   FREE(gscr);
#endif
}

static EGLBoolean
egl_g3d_terminate(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);

   _eglReleaseDisplayResources(drv, dpy);

   if (dpy->Configs) {
      _eglDestroyArray(dpy->Configs, egl_g3d_free_config);
      dpy->Configs = NULL;
   }
   if (dpy->Screens) {
      _eglDestroyArray(dpy->Screens, egl_g3d_free_screen);
      dpy->Screens = NULL;
   }

   _eglCleanupDisplay(dpy);

   if (gdpy->smapi)
      egl_g3d_destroy_st_manager(gdpy->smapi);

   if (gdpy->native)
      gdpy->native->destroy(gdpy->native);

   FREE(gdpy);
   dpy->DriverData = NULL;

   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_initialize(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   struct egl_g3d_display *gdpy;
   const struct native_platform *nplat;

   nplat = egl_g3d_get_platform(drv, dpy->Platform);
   if (!nplat)
      return EGL_FALSE;

   if (dpy->Options.TestOnly)
      return EGL_TRUE;

   gdpy = CALLOC_STRUCT(egl_g3d_display);
   if (!gdpy) {
      _eglError(EGL_BAD_ALLOC, "eglInitialize");
      goto fail;
   }
   gdpy->loader = gdrv->loader;
   dpy->DriverData = gdpy;

   _eglLog(_EGL_INFO, "use %s for display %p",
         nplat->name, dpy->PlatformDisplay);
   gdpy->native =
      nplat->create_display(dpy->PlatformDisplay, dpy->Options.UseFallback);
   if (!gdpy->native) {
      _eglError(EGL_NOT_INITIALIZED, "eglInitialize(no usable display)");
      goto fail;
   }
   gdpy->native->user_data = (void *) dpy;
   if (!gdpy->native->init_screen(gdpy->native)) {
      _eglError(EGL_NOT_INITIALIZED,
            "eglInitialize(failed to initialize screen)");
      goto fail;
   }

   if (gdpy->loader->profile_masks[ST_API_OPENGL] & ST_PROFILE_DEFAULT_MASK)
      dpy->ClientAPIs |= EGL_OPENGL_BIT;
   if (gdpy->loader->profile_masks[ST_API_OPENGL] & ST_PROFILE_OPENGL_ES1_MASK)
      dpy->ClientAPIs |= EGL_OPENGL_ES_BIT;
   if (gdpy->loader->profile_masks[ST_API_OPENGL] & ST_PROFILE_OPENGL_ES2_MASK)
      dpy->ClientAPIs |= EGL_OPENGL_ES2_BIT;
   if (gdpy->loader->profile_masks[ST_API_OPENVG] & ST_PROFILE_DEFAULT_MASK)
      dpy->ClientAPIs |= EGL_OPENVG_BIT;

   gdpy->smapi = egl_g3d_create_st_manager(dpy);
   if (!gdpy->smapi) {
      _eglError(EGL_NOT_INITIALIZED,
            "eglInitialize(failed to create st manager)");
      goto fail;
   }

#ifdef EGL_MESA_screen_surface
   /* enable MESA_screen_surface before adding (and validating) configs */
   if (gdpy->native->modeset) {
      dpy->Extensions.MESA_screen_surface = EGL_TRUE;
      egl_g3d_add_screens(drv, dpy);
   }
#endif

   dpy->Extensions.KHR_image_base = EGL_TRUE;
   if (gdpy->native->get_param(gdpy->native, NATIVE_PARAM_USE_NATIVE_BUFFER))
      dpy->Extensions.KHR_image_pixmap = EGL_TRUE;

   dpy->Extensions.KHR_reusable_sync = EGL_TRUE;
   dpy->Extensions.KHR_fence_sync = EGL_TRUE;

   dpy->Extensions.KHR_surfaceless_context = EGL_TRUE;

   if (dpy->Platform == _EGL_PLATFORM_DRM) {
      dpy->Extensions.MESA_drm_display = EGL_TRUE;
      if (gdpy->native->buffer)
         dpy->Extensions.MESA_drm_image = EGL_TRUE;
   }

   if (dpy->Platform == _EGL_PLATFORM_WAYLAND && gdpy->native->buffer)
      dpy->Extensions.MESA_drm_image = EGL_TRUE;

#ifdef EGL_ANDROID_image_native_buffer
   if (dpy->Platform == _EGL_PLATFORM_ANDROID && gdpy->native->buffer)
      dpy->Extensions.ANDROID_image_native_buffer = EGL_TRUE;
#endif

#ifdef EGL_WL_bind_wayland_display
   if (gdpy->native->wayland_bufmgr)
      dpy->Extensions.WL_bind_wayland_display = EGL_TRUE;
#endif

   if (gdpy->native->get_param(gdpy->native, NATIVE_PARAM_PRESENT_REGION) &&
       gdpy->native->get_param(gdpy->native, NATIVE_PARAM_PRESERVE_BUFFER)) {
#ifdef EGL_NOK_swap_region
      dpy->Extensions.NOK_swap_region = EGL_TRUE;
#endif
      dpy->Extensions.NV_post_sub_buffer = EGL_TRUE;
   }

   if (egl_g3d_add_configs(drv, dpy, 1) == 1) {
      _eglError(EGL_NOT_INITIALIZED, "eglInitialize(unable to add configs)");
      goto fail;
   }

   dpy->VersionMajor = 1;
   dpy->VersionMinor = 4;

   return EGL_TRUE;

fail:
   if (gdpy)
      egl_g3d_terminate(drv, dpy);
   return EGL_FALSE;
}

static _EGLProc
egl_g3d_get_proc_address(_EGLDriver *drv, const char *procname)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   struct st_api *stapi = NULL;

   if (procname && procname[0] == 'v' && procname[1] == 'g')
      stapi = gdrv->loader->get_st_api(ST_API_OPENVG);
   else if (procname && procname[0] == 'g' && procname[1] == 'l')
      stapi = gdrv->loader->get_st_api(ST_API_OPENGL);

   return (_EGLProc) ((stapi) ?
         stapi->get_proc_address(stapi, procname) : NULL);
}

_EGLDriver *
egl_g3d_create_driver(const struct egl_g3d_loader *loader)
{
   struct egl_g3d_driver *gdrv;

   gdrv = CALLOC_STRUCT(egl_g3d_driver);
   if (!gdrv)
      return NULL;

   gdrv->loader = loader;

   egl_g3d_init_driver_api(&gdrv->base);
   gdrv->base.API.Initialize = egl_g3d_initialize;
   gdrv->base.API.Terminate = egl_g3d_terminate;
   gdrv->base.API.GetProcAddress = egl_g3d_get_proc_address;

   /* to be filled by the caller */
   gdrv->base.Name = NULL;
   gdrv->base.Unload = NULL;

   return &gdrv->base;
}

void
egl_g3d_destroy_driver(_EGLDriver *drv)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   FREE(gdrv);
}
