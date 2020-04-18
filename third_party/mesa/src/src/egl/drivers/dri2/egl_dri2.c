/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <xf86drm.h>
#include <GL/gl.h>
#include <GL/internal/dri_interface.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "egl_dri2.h"

const __DRIuseInvalidateExtension use_invalidate = {
   { __DRI_USE_INVALIDATE, 1 }
};

EGLint dri2_to_egl_attribute_map[] = {
   0,
   EGL_BUFFER_SIZE,		/* __DRI_ATTRIB_BUFFER_SIZE */
   EGL_LEVEL,			/* __DRI_ATTRIB_LEVEL */
   EGL_RED_SIZE,		/* __DRI_ATTRIB_RED_SIZE */
   EGL_GREEN_SIZE,		/* __DRI_ATTRIB_GREEN_SIZE */
   EGL_BLUE_SIZE,		/* __DRI_ATTRIB_BLUE_SIZE */
   EGL_LUMINANCE_SIZE,		/* __DRI_ATTRIB_LUMINANCE_SIZE */
   EGL_ALPHA_SIZE,		/* __DRI_ATTRIB_ALPHA_SIZE */
   0,				/* __DRI_ATTRIB_ALPHA_MASK_SIZE */
   EGL_DEPTH_SIZE,		/* __DRI_ATTRIB_DEPTH_SIZE */
   EGL_STENCIL_SIZE,		/* __DRI_ATTRIB_STENCIL_SIZE */
   0,				/* __DRI_ATTRIB_ACCUM_RED_SIZE */
   0,				/* __DRI_ATTRIB_ACCUM_GREEN_SIZE */
   0,				/* __DRI_ATTRIB_ACCUM_BLUE_SIZE */
   0,				/* __DRI_ATTRIB_ACCUM_ALPHA_SIZE */
   EGL_SAMPLE_BUFFERS,		/* __DRI_ATTRIB_SAMPLE_BUFFERS */
   EGL_SAMPLES,			/* __DRI_ATTRIB_SAMPLES */
   0,				/* __DRI_ATTRIB_RENDER_TYPE, */
   0,				/* __DRI_ATTRIB_CONFIG_CAVEAT */
   0,				/* __DRI_ATTRIB_CONFORMANT */
   0,				/* __DRI_ATTRIB_DOUBLE_BUFFER */
   0,				/* __DRI_ATTRIB_STEREO */
   0,				/* __DRI_ATTRIB_AUX_BUFFERS */
   0,				/* __DRI_ATTRIB_TRANSPARENT_TYPE */
   0,				/* __DRI_ATTRIB_TRANSPARENT_INDEX_VALUE */
   0,				/* __DRI_ATTRIB_TRANSPARENT_RED_VALUE */
   0,				/* __DRI_ATTRIB_TRANSPARENT_GREEN_VALUE */
   0,				/* __DRI_ATTRIB_TRANSPARENT_BLUE_VALUE */
   0,				/* __DRI_ATTRIB_TRANSPARENT_ALPHA_VALUE */
   0,				/* __DRI_ATTRIB_FLOAT_MODE */
   0,				/* __DRI_ATTRIB_RED_MASK */
   0,				/* __DRI_ATTRIB_GREEN_MASK */
   0,				/* __DRI_ATTRIB_BLUE_MASK */
   0,				/* __DRI_ATTRIB_ALPHA_MASK */
   EGL_MAX_PBUFFER_WIDTH,	/* __DRI_ATTRIB_MAX_PBUFFER_WIDTH */
   EGL_MAX_PBUFFER_HEIGHT,	/* __DRI_ATTRIB_MAX_PBUFFER_HEIGHT */
   EGL_MAX_PBUFFER_PIXELS,	/* __DRI_ATTRIB_MAX_PBUFFER_PIXELS */
   0,				/* __DRI_ATTRIB_OPTIMAL_PBUFFER_WIDTH */
   0,				/* __DRI_ATTRIB_OPTIMAL_PBUFFER_HEIGHT */
   0,				/* __DRI_ATTRIB_VISUAL_SELECT_GROUP */
   0,				/* __DRI_ATTRIB_SWAP_METHOD */
   EGL_MAX_SWAP_INTERVAL,	/* __DRI_ATTRIB_MAX_SWAP_INTERVAL */
   EGL_MIN_SWAP_INTERVAL,	/* __DRI_ATTRIB_MIN_SWAP_INTERVAL */
   0,				/* __DRI_ATTRIB_BIND_TO_TEXTURE_RGB */
   0,				/* __DRI_ATTRIB_BIND_TO_TEXTURE_RGBA */
   0,				/* __DRI_ATTRIB_BIND_TO_MIPMAP_TEXTURE */
   0,				/* __DRI_ATTRIB_BIND_TO_TEXTURE_TARGETS */
   EGL_Y_INVERTED_NOK,		/* __DRI_ATTRIB_YINVERTED */
   0,				/* __DRI_ATTRIB_FRAMEBUFFER_SRGB_CAPABLE */
};

static EGLBoolean
dri2_match_config(const _EGLConfig *conf, const _EGLConfig *criteria)
{
   if (_eglCompareConfigs(conf, criteria, NULL, EGL_FALSE) != 0)
      return EGL_FALSE;

   if (!_eglMatchConfig(conf, criteria))
      return EGL_FALSE;

   return EGL_TRUE;
}

struct dri2_egl_config *
dri2_add_config(_EGLDisplay *disp, const __DRIconfig *dri_config, int id,
		int depth, EGLint surface_type, const EGLint *attr_list,
		const unsigned int *rgba_masks)
{
   struct dri2_egl_config *conf;
   struct dri2_egl_display *dri2_dpy;
   _EGLConfig base;
   unsigned int attrib, value, double_buffer;
   EGLint key, bind_to_texture_rgb, bind_to_texture_rgba;
   unsigned int dri_masks[4] = { 0, 0, 0, 0 };
   _EGLConfig *matching_config;
   EGLint num_configs = 0;
   EGLint config_id;
   int i;

   dri2_dpy = disp->DriverData;
   _eglInitConfig(&base, disp, id);
   
   i = 0;
   double_buffer = 0;
   bind_to_texture_rgb = 0;
   bind_to_texture_rgba = 0;

   while (dri2_dpy->core->indexConfigAttrib(dri_config, i++, &attrib, &value)) {
      switch (attrib) {
      case __DRI_ATTRIB_RENDER_TYPE:
	 if (value & __DRI_ATTRIB_RGBA_BIT)
	    value = EGL_RGB_BUFFER;
	 else if (value & __DRI_ATTRIB_LUMINANCE_BIT)
	    value = EGL_LUMINANCE_BUFFER;
	 else
	    /* not valid */;
	 _eglSetConfigKey(&base, EGL_COLOR_BUFFER_TYPE, value);
	 break;	 

      case __DRI_ATTRIB_CONFIG_CAVEAT:
         if (value & __DRI_ATTRIB_NON_CONFORMANT_CONFIG)
            value = EGL_NON_CONFORMANT_CONFIG;
         else if (value & __DRI_ATTRIB_SLOW_BIT)
            value = EGL_SLOW_CONFIG;
	 else
	    value = EGL_NONE;
	 _eglSetConfigKey(&base, EGL_CONFIG_CAVEAT, value);
         break;

      case __DRI_ATTRIB_BIND_TO_TEXTURE_RGB:
	 bind_to_texture_rgb = value;
	 break;

      case __DRI_ATTRIB_BIND_TO_TEXTURE_RGBA:
	 bind_to_texture_rgba = value;
	 break;

      case __DRI_ATTRIB_DOUBLE_BUFFER:
	 double_buffer = value;
	 break;

      case __DRI_ATTRIB_RED_MASK:
         dri_masks[0] = value;
         break;

      case __DRI_ATTRIB_GREEN_MASK:
         dri_masks[1] = value;
         break;

      case __DRI_ATTRIB_BLUE_MASK:
         dri_masks[2] = value;
         break;

      case __DRI_ATTRIB_ALPHA_MASK:
         dri_masks[3] = value;
         break;

      default:
	 key = dri2_to_egl_attribute_map[attrib];
	 if (key != 0)
	    _eglSetConfigKey(&base, key, value);
	 break;
      }
   }

   if (attr_list)
      for (i = 0; attr_list[i] != EGL_NONE; i += 2)
         _eglSetConfigKey(&base, attr_list[i], attr_list[i+1]);

   if (depth > 0 && depth != base.BufferSize)
      return NULL;

   if (rgba_masks && memcmp(rgba_masks, dri_masks, sizeof(dri_masks)))
      return NULL;

   base.NativeRenderable = EGL_TRUE;

   base.SurfaceType = surface_type;
   if (surface_type & (EGL_PBUFFER_BIT |
       (disp->Extensions.NOK_texture_from_pixmap ? EGL_PIXMAP_BIT : 0))) {
      base.BindToTextureRGB = bind_to_texture_rgb;
      if (base.AlphaSize > 0)
         base.BindToTextureRGBA = bind_to_texture_rgba;
   }

   base.RenderableType = disp->ClientAPIs;
   base.Conformant = disp->ClientAPIs;

   if (!_eglValidateConfig(&base, EGL_FALSE)) {
      _eglLog(_EGL_DEBUG, "DRI2: failed to validate config %d", id);
      return NULL;
   }

   config_id = base.ConfigID;
   base.ConfigID    = EGL_DONT_CARE;
   base.SurfaceType = EGL_DONT_CARE;
   num_configs = _eglFilterArray(disp->Configs, (void **) &matching_config, 1,
                                 (_EGLArrayForEach) dri2_match_config, &base);

   if (num_configs == 1) {
      conf = (struct dri2_egl_config *) matching_config;

      if (double_buffer && !conf->dri_double_config)
         conf->dri_double_config = dri_config;
      else if (!double_buffer && !conf->dri_single_config)
         conf->dri_single_config = dri_config;
      else
         /* a similar config type is already added (unlikely) => discard */
         return NULL;
   }
   else if (num_configs == 0) {
      conf = malloc(sizeof *conf);
      if (conf == NULL)
         return NULL;

      memcpy(&conf->base, &base, sizeof base);
      if (double_buffer) {
         conf->dri_double_config = dri_config;
         conf->dri_single_config = NULL;
      } else {
         conf->dri_single_config = dri_config;
         conf->dri_double_config = NULL;
      }
      conf->base.SurfaceType = 0;
      conf->base.ConfigID = config_id;

      _eglLinkConfig(&conf->base);
   }
   else {
      assert(0);
      return NULL;
   }

   if (double_buffer) {
      surface_type &= ~EGL_PIXMAP_BIT;

      if (dri2_dpy->swap_available) {
         conf->base.MinSwapInterval = 0;
         conf->base.MaxSwapInterval = 1000; /* XXX arbitrary value */
      }
   }

   conf->base.SurfaceType |= surface_type;

   return conf;
}

__DRIimage *
dri2_lookup_egl_image(__DRIscreen *screen, void *image, void *data)
{
   _EGLDisplay *disp = data;
   struct dri2_egl_image *dri2_img;
   _EGLImage *img;

   (void) screen;

   img = _eglLookupImage(image, disp);
   if (img == NULL) {
      _eglError(EGL_BAD_PARAMETER, "dri2_lookup_egl_image");
      return NULL;
   }

   dri2_img = dri2_egl_image(image);

   return dri2_img->dri_image;
}

const __DRIimageLookupExtension image_lookup_extension = {
   { __DRI_IMAGE_LOOKUP, 1 },
   dri2_lookup_egl_image
};

static const char dri_driver_path[] = DEFAULT_DRIVER_DIR;

struct dri2_extension_match {
   const char *name;
   int version;
   int offset;
};

static struct dri2_extension_match dri2_driver_extensions[] = {
   { __DRI_CORE, 1, offsetof(struct dri2_egl_display, core) },
   { __DRI_DRI2, 2, offsetof(struct dri2_egl_display, dri2) },
   { NULL, 0, 0 }
};

static struct dri2_extension_match dri2_core_extensions[] = {
   { __DRI2_FLUSH, 1, offsetof(struct dri2_egl_display, flush) },
   { __DRI_TEX_BUFFER, 2, offsetof(struct dri2_egl_display, tex_buffer) },
   { __DRI_IMAGE, 1, offsetof(struct dri2_egl_display, image) },
   { NULL, 0, 0 }
};

static struct dri2_extension_match swrast_driver_extensions[] = {
   { __DRI_CORE, 1, offsetof(struct dri2_egl_display, core) },
   { __DRI_SWRAST, 2, offsetof(struct dri2_egl_display, swrast) },
   { NULL, 0, 0 }
};

static struct dri2_extension_match swrast_core_extensions[] = {
   { __DRI_TEX_BUFFER, 2, offsetof(struct dri2_egl_display, tex_buffer) },
   { NULL, 0, 0 }
};

static EGLBoolean
dri2_bind_extensions(struct dri2_egl_display *dri2_dpy,
		     struct dri2_extension_match *matches,
		     const __DRIextension **extensions)
{
   int i, j, ret = EGL_TRUE;
   void *field;

   for (i = 0; extensions[i]; i++) {
      _eglLog(_EGL_DEBUG, "DRI2: found extension `%s'", extensions[i]->name);
      for (j = 0; matches[j].name; j++) {
	 if (strcmp(extensions[i]->name, matches[j].name) == 0 &&
	     extensions[i]->version >= matches[j].version) {
	    field = ((char *) dri2_dpy + matches[j].offset);
	    *(const __DRIextension **) field = extensions[i];
	    _eglLog(_EGL_INFO, "DRI2: found extension %s version %d",
		    extensions[i]->name, extensions[i]->version);
	 }
      }
   }
   
   for (j = 0; matches[j].name; j++) {
      field = ((char *) dri2_dpy + matches[j].offset);
      if (*(const __DRIextension **) field == NULL) {
	 _eglLog(_EGL_FATAL, "DRI2: did not find extension %s version %d",
		 matches[j].name, matches[j].version);
	 ret = EGL_FALSE;
      }
   }

   return ret;
}

static const __DRIextension **
dri2_open_driver(_EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = disp->DriverData;
   const __DRIextension **extensions;
   char path[PATH_MAX], *search_paths, *p, *next, *end;

   search_paths = NULL;
   if (geteuid() == getuid()) {
      /* don't allow setuid apps to use LIBGL_DRIVERS_PATH */
      search_paths = getenv("LIBGL_DRIVERS_PATH");
   }
   if (search_paths == NULL)
      search_paths = DEFAULT_DRIVER_DIR;

   dri2_dpy->driver = NULL;
   end = search_paths + strlen(search_paths);
   for (p = search_paths; p < end && dri2_dpy->driver == NULL; p = next + 1) {
      int len;
      next = strchr(p, ':');
      if (next == NULL)
         next = end;

      len = next - p;
#if GLX_USE_TLS
      snprintf(path, sizeof path,
	       "%.*s/tls/%s_dri.so", len, p, dri2_dpy->driver_name);
      dri2_dpy->driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
#endif
      if (dri2_dpy->driver == NULL) {
	 snprintf(path, sizeof path,
		  "%.*s/%s_dri.so", len, p, dri2_dpy->driver_name);
	 dri2_dpy->driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
	 if (dri2_dpy->driver == NULL)
	    _eglLog(_EGL_DEBUG, "failed to open %s: %s\n", path, dlerror());
      }
   }

   if (dri2_dpy->driver == NULL) {
      _eglLog(_EGL_WARNING,
	      "DRI2: failed to open %s (search paths %s)",
	      dri2_dpy->driver_name, search_paths);
      return NULL;
   }

   _eglLog(_EGL_DEBUG, "DRI2: dlopen(%s)", path);
   extensions = dlsym(dri2_dpy->driver, __DRI_DRIVER_EXTENSIONS);
   if (extensions == NULL) {
      _eglLog(_EGL_WARNING,
	      "DRI2: driver exports no extensions (%s)", dlerror());
      dlclose(dri2_dpy->driver);
   }

   return extensions;
}

EGLBoolean
dri2_load_driver(_EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = disp->DriverData;
   const __DRIextension **extensions;

   extensions = dri2_open_driver(disp);
   if (!extensions)
      return EGL_FALSE;

   if (!dri2_bind_extensions(dri2_dpy, dri2_driver_extensions, extensions)) {
      dlclose(dri2_dpy->driver);
      return EGL_FALSE;
   }

   return EGL_TRUE;
}

EGLBoolean
dri2_load_driver_swrast(_EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = disp->DriverData;
   const __DRIextension **extensions;

   dri2_dpy->driver_name = "swrast";
   extensions = dri2_open_driver(disp);

   if (!extensions)
      return EGL_FALSE;

   if (!dri2_bind_extensions(dri2_dpy, swrast_driver_extensions, extensions)) {
      dlclose(dri2_dpy->driver);
      return EGL_FALSE;
   }

   return EGL_TRUE;
}

void
dri2_setup_screen(_EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   unsigned int api_mask;

   if (dri2_dpy->dri2) {
      api_mask = dri2_dpy->dri2->getAPIMask(dri2_dpy->dri_screen);
   } else {
      assert(dri2_dpy->swrast);
      api_mask = 1 << __DRI_API_OPENGL | 1 << __DRI_API_GLES | 1 << __DRI_API_GLES2;
   }

   disp->ClientAPIs = 0;
   if (api_mask & (1 <<__DRI_API_OPENGL))
      disp->ClientAPIs |= EGL_OPENGL_BIT;
   if (api_mask & (1 <<__DRI_API_GLES))
      disp->ClientAPIs |= EGL_OPENGL_ES_BIT;
   if (api_mask & (1 << __DRI_API_GLES2))
      disp->ClientAPIs |= EGL_OPENGL_ES2_BIT;

   assert(dri2_dpy->dri2 || dri2_dpy->swrast);
   disp->Extensions.KHR_surfaceless_context = EGL_TRUE;

   if (dri2_dpy->dri2 && dri2_dpy->dri2->base.version >= 3) {
      disp->Extensions.KHR_create_context = EGL_TRUE;

      if (dri2_dpy->robustness)
         disp->Extensions.EXT_create_context_robustness = EGL_TRUE;
   }

   if (dri2_dpy->image) {
      disp->Extensions.MESA_drm_image = EGL_TRUE;
      disp->Extensions.KHR_image_base = EGL_TRUE;
      disp->Extensions.KHR_gl_renderbuffer_image = EGL_TRUE;
   }
}

EGLBoolean
dri2_create_screen(_EGLDisplay *disp)
{
   const __DRIextension **extensions;
   struct dri2_egl_display *dri2_dpy;

   dri2_dpy = disp->DriverData;

   if (dri2_dpy->dri2) {
      dri2_dpy->dri_screen =
         dri2_dpy->dri2->createNewScreen(0, dri2_dpy->fd, dri2_dpy->extensions,
				         &dri2_dpy->driver_configs, disp);
   } else {
      assert(dri2_dpy->swrast);
      dri2_dpy->dri_screen =
         dri2_dpy->swrast->createNewScreen(0, dri2_dpy->extensions,
                                           &dri2_dpy->driver_configs, disp);
   }

   if (dri2_dpy->dri_screen == NULL) {
      _eglLog(_EGL_WARNING, "DRI2: failed to create dri screen");
      return EGL_FALSE;
   }

   dri2_dpy->own_dri_screen = 1;

   extensions = dri2_dpy->core->getExtensions(dri2_dpy->dri_screen);
   
   if (dri2_dpy->dri2) {
      unsigned i;

      if (!dri2_bind_extensions(dri2_dpy, dri2_core_extensions, extensions))
         goto cleanup_dri_screen;

      for (i = 0; extensions[i]; i++) {
	 if (strcmp(extensions[i]->name, __DRI2_ROBUSTNESS) == 0) {
            dri2_dpy->robustness = (__DRIrobustnessExtension *) extensions[i];
	 }
      }
   } else {
      assert(dri2_dpy->swrast);
      if (!dri2_bind_extensions(dri2_dpy, swrast_core_extensions, extensions))
         goto cleanup_dri_screen;
   }

   dri2_setup_screen(disp);

   return EGL_TRUE;

 cleanup_dri_screen:
   dri2_dpy->core->destroyScreen(dri2_dpy->dri_screen);

   return EGL_FALSE;
}

/**
 * Called via eglInitialize(), GLX_drv->API.Initialize().
 */
static EGLBoolean
dri2_initialize(_EGLDriver *drv, _EGLDisplay *disp)
{
   /* not until swrast_dri is supported */
   if (disp->Options.UseFallback)
      return EGL_FALSE;

   switch (disp->Platform) {
#ifdef HAVE_X11_PLATFORM
   case _EGL_PLATFORM_X11:
      if (disp->Options.TestOnly)
         return EGL_TRUE;
      return dri2_initialize_x11(drv, disp);
#endif

#ifdef HAVE_LIBUDEV
#ifdef HAVE_DRM_PLATFORM
   case _EGL_PLATFORM_DRM:
      if (disp->Options.TestOnly)
         return EGL_TRUE;
      return dri2_initialize_drm(drv, disp);
#endif
#ifdef HAVE_WAYLAND_PLATFORM
   case _EGL_PLATFORM_WAYLAND:
      if (disp->Options.TestOnly)
         return EGL_TRUE;
      return dri2_initialize_wayland(drv, disp);
#endif
#endif
#ifdef HAVE_ANDROID_PLATFORM
   case _EGL_PLATFORM_ANDROID:
      if (disp->Options.TestOnly)
         return EGL_TRUE;
      return dri2_initialize_android(drv, disp);
#endif

   default:
      return EGL_FALSE;
   }
}

/**
 * Called via eglTerminate(), drv->API.Terminate().
 */
static EGLBoolean
dri2_terminate(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   _eglReleaseDisplayResources(drv, disp);
   _eglCleanupDisplay(disp);

   if (dri2_dpy->own_dri_screen)
      dri2_dpy->core->destroyScreen(dri2_dpy->dri_screen);
   if (dri2_dpy->fd)
      close(dri2_dpy->fd);
   if (dri2_dpy->driver)
      dlclose(dri2_dpy->driver);
   if (dri2_dpy->device_name)
      free(dri2_dpy->device_name);

   if (disp->PlatformDisplay == NULL) {
      switch (disp->Platform) {
#ifdef HAVE_X11_PLATFORM
      case _EGL_PLATFORM_X11:
         xcb_disconnect(dri2_dpy->conn);
         break;
#endif
#ifdef HAVE_DRM_PLATFORM
      case _EGL_PLATFORM_DRM:
         if (dri2_dpy->own_device) {
            gbm_device_destroy(&dri2_dpy->gbm_dri->base.base);
         }
         break;
#endif
      default:
         break;
      }
   }

   free(dri2_dpy);
   disp->DriverData = NULL;

   return EGL_TRUE;
}

/**
 * Set the error code after a call to
 * dri2_egl_display::dri2::createContextAttribs.
 */
static void
dri2_create_context_attribs_error(int dri_error)
{
   EGLint egl_error;

   switch (dri_error) {
   case __DRI_CTX_ERROR_SUCCESS:
      return;

   case __DRI_CTX_ERROR_NO_MEMORY:
      egl_error = EGL_BAD_ALLOC;
      break;

  /* From the EGL_KHR_create_context spec, section "Errors":
   *
   *   * If <config> does not support a client API context compatible
   *     with the requested API major and minor version, [...] context flags,
   *     and context reset notification behavior (for client API types where
   *     these attributes are supported), then an EGL_BAD_MATCH error is
   *     generated.
   *
   *   * If an OpenGL ES context is requested and the values for
   *     attributes EGL_CONTEXT_MAJOR_VERSION_KHR and
   *     EGL_CONTEXT_MINOR_VERSION_KHR specify an OpenGL ES version that
   *     is not defined, than an EGL_BAD_MATCH error is generated.
   *
   *   * If an OpenGL context is requested, the requested version is
   *     greater than 3.2, and the value for attribute
   *     EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR has no bits set; has any
   *     bits set other than EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR and
   *     EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR; has more than
   *     one of these bits set; or if the implementation does not support
   *     the requested profile, then an EGL_BAD_MATCH error is generated.
   */
   case __DRI_CTX_ERROR_BAD_API:
   case __DRI_CTX_ERROR_BAD_VERSION:
   case __DRI_CTX_ERROR_BAD_FLAG:
      egl_error = EGL_BAD_MATCH;
      break;

  /* From the EGL_KHR_create_context spec, section "Errors":
   *
   *   * If an attribute name or attribute value in <attrib_list> is not
   *     recognized (including unrecognized bits in bitmask attributes),
   *     then an EGL_BAD_ATTRIBUTE error is generated."
   */
   case __DRI_CTX_ERROR_UNKNOWN_ATTRIBUTE:
   case __DRI_CTX_ERROR_UNKNOWN_FLAG:
      egl_error = EGL_BAD_ATTRIBUTE;
      break;

   default:
      assert(0);
      egl_error = EGL_BAD_MATCH;
      break;
   }

   _eglError(egl_error, "dri2_create_context");
}

/**
 * Called via eglCreateContext(), drv->API.CreateContext().
 */
static _EGLContext *
dri2_create_context(_EGLDriver *drv, _EGLDisplay *disp, _EGLConfig *conf,
		    _EGLContext *share_list, const EGLint *attrib_list)
{
   struct dri2_egl_context *dri2_ctx;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_context *dri2_ctx_shared = dri2_egl_context(share_list);
   __DRIcontext *shared =
      dri2_ctx_shared ? dri2_ctx_shared->dri_context : NULL;
   struct dri2_egl_config *dri2_config = dri2_egl_config(conf);
   const __DRIconfig *dri_config;
   int api;

   (void) drv;

   dri2_ctx = malloc(sizeof *dri2_ctx);
   if (!dri2_ctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      return NULL;
   }

   if (!_eglInitContext(&dri2_ctx->base, disp, conf, attrib_list))
      goto cleanup;

   switch (dri2_ctx->base.ClientAPI) {
   case EGL_OPENGL_ES_API:
      switch (dri2_ctx->base.ClientMajorVersion) {
      case 1:
         api = __DRI_API_GLES;
         break;
      case 2:
      case 3:
         api = __DRI_API_GLES2;
         break;
      default:
	 _eglError(EGL_BAD_PARAMETER, "eglCreateContext");
	 return NULL;
      }
      break;
   case EGL_OPENGL_API:
      if ((dri2_ctx->base.ClientMajorVersion >= 4
           || (dri2_ctx->base.ClientMajorVersion == 3
               && dri2_ctx->base.ClientMinorVersion >= 2))
          && dri2_ctx->base.Profile == EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR)
         api = __DRI_API_OPENGL_CORE;
      else
         api = __DRI_API_OPENGL;
      break;
   default:
      _eglError(EGL_BAD_PARAMETER, "eglCreateContext");
      return NULL;
   }

   if (conf != NULL) {
      /* The config chosen here isn't necessarily
       * used for surfaces later.
       * A pixmap surface will use the single config.
       * This opportunity depends on disabling the
       * doubleBufferMode check in
       * src/mesa/main/context.c:check_compatible()
       */
      if (dri2_config->dri_double_config)
         dri_config = dri2_config->dri_double_config;
      else
         dri_config = dri2_config->dri_single_config;

      /* EGL_WINDOW_BIT is set only when there is a dri_double_config.  This
       * makes sure the back buffer will always be used.
       */
      if (conf->SurfaceType & EGL_WINDOW_BIT)
         dri2_ctx->base.WindowRenderBuffer = EGL_BACK_BUFFER;
   }
   else
      dri_config = NULL;

   if (dri2_dpy->dri2) {
      if (dri2_dpy->dri2->base.version >= 3) {
         unsigned error;
         unsigned num_attribs = 0;
         uint32_t ctx_attribs[8];

         ctx_attribs[num_attribs++] = __DRI_CTX_ATTRIB_MAJOR_VERSION;
         ctx_attribs[num_attribs++] = dri2_ctx->base.ClientMajorVersion;
         ctx_attribs[num_attribs++] = __DRI_CTX_ATTRIB_MINOR_VERSION;
         ctx_attribs[num_attribs++] = dri2_ctx->base.ClientMinorVersion;

         if (dri2_ctx->base.Flags != 0) {
            /* If the implementation doesn't support the __DRI2_ROBUSTNESS
             * extension, don't even try to send it the robust-access flag.
             * It may explode.  Instead, generate the required EGL error here.
             */
            if ((dri2_ctx->base.Flags & EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR) != 0
                && !dri2_dpy->robustness) {
               _eglError(EGL_BAD_MATCH, "eglCreateContext");
               goto cleanup;
            }

            ctx_attribs[num_attribs++] = __DRI_CTX_ATTRIB_FLAGS;
            ctx_attribs[num_attribs++] = dri2_ctx->base.Flags;
         }

         if (dri2_ctx->base.ResetNotificationStrategy != EGL_NO_RESET_NOTIFICATION_KHR) {
            /* If the implementation doesn't support the __DRI2_ROBUSTNESS
             * extension, don't even try to send it a reset strategy.  It may
             * explode.  Instead, generate the required EGL error here.
             */
            if (!dri2_dpy->robustness) {
               _eglError(EGL_BAD_CONFIG, "eglCreateContext");
               goto cleanup;
            }

            ctx_attribs[num_attribs++] = __DRI_CTX_ATTRIB_RESET_STRATEGY;
            ctx_attribs[num_attribs++] = __DRI_CTX_RESET_LOSE_CONTEXT;
         }

         assert(num_attribs <= ARRAY_SIZE(ctx_attribs));

	 dri2_ctx->dri_context =
	    dri2_dpy->dri2->createContextAttribs(dri2_dpy->dri_screen,
                                                 api,
                                                 dri_config,
                                                 shared,
                                                 num_attribs / 2,
                                                 ctx_attribs,
                                                 & error,
                                                 dri2_ctx);
	 dri2_create_context_attribs_error(error);
      } else {
	 dri2_ctx->dri_context =
	    dri2_dpy->dri2->createNewContextForAPI(dri2_dpy->dri_screen,
						   api,
						   dri_config,
                                                   shared,
						   dri2_ctx);
      }
   } else {
      assert(dri2_dpy->swrast);
      dri2_ctx->dri_context =
         dri2_dpy->swrast->createNewContextForAPI(dri2_dpy->dri_screen,
                                                  api,
                                                  dri_config,
                                                  shared,
                                                  dri2_ctx);
   }

   if (!dri2_ctx->dri_context)
      goto cleanup;

   return &dri2_ctx->base;

 cleanup:
   free(dri2_ctx);
   return NULL;
}

/**
 * Called via eglDestroyContext(), drv->API.DestroyContext().
 */
static EGLBoolean
dri2_destroy_context(_EGLDriver *drv, _EGLDisplay *disp, _EGLContext *ctx)
{
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   if (_eglPutContext(ctx)) {
      dri2_dpy->core->destroyContext(dri2_ctx->dri_context);
      free(dri2_ctx);
   }

   return EGL_TRUE;
}

/**
 * Called via eglMakeCurrent(), drv->API.MakeCurrent().
 */
static EGLBoolean
dri2_make_current(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *dsurf,
		  _EGLSurface *rsurf, _EGLContext *ctx)
{
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_dsurf = dri2_egl_surface(dsurf);
   struct dri2_egl_surface *dri2_rsurf = dri2_egl_surface(rsurf);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   _EGLContext *old_ctx;
   _EGLSurface *old_dsurf, *old_rsurf;
   __DRIdrawable *ddraw, *rdraw;
   __DRIcontext *cctx;

   /* make new bindings */
   if (!_eglBindContext(ctx, dsurf, rsurf, &old_ctx, &old_dsurf, &old_rsurf))
      return EGL_FALSE;

   /* flush before context switch */
   if (old_ctx && dri2_drv->glFlush)
      dri2_drv->glFlush();

   ddraw = (dri2_dsurf) ? dri2_dsurf->dri_drawable : NULL;
   rdraw = (dri2_rsurf) ? dri2_rsurf->dri_drawable : NULL;
   cctx = (dri2_ctx) ? dri2_ctx->dri_context : NULL;

   if (old_ctx) {
      __DRIcontext *old_cctx = dri2_egl_context(old_ctx)->dri_context;
      dri2_dpy->core->unbindContext(old_cctx);
   }

   if ((cctx == NULL && ddraw == NULL && rdraw == NULL) ||
       dri2_dpy->core->bindContext(cctx, ddraw, rdraw)) {
      if (old_dsurf)
         drv->API.DestroySurface(drv, disp, old_dsurf);
      if (old_rsurf)
         drv->API.DestroySurface(drv, disp, old_rsurf);
      if (old_ctx)
         drv->API.DestroyContext(drv, disp, old_ctx);

      return EGL_TRUE;
   } else {
      /* undo the previous _eglBindContext */
      _eglBindContext(old_ctx, old_dsurf, old_rsurf, &ctx, &dsurf, &rsurf);
      assert(&dri2_ctx->base == ctx &&
             &dri2_dsurf->base == dsurf &&
             &dri2_rsurf->base == rsurf);

      _eglPutSurface(dsurf);
      _eglPutSurface(rsurf);
      _eglPutContext(ctx);

      _eglPutSurface(old_dsurf);
      _eglPutSurface(old_rsurf);
      _eglPutContext(old_ctx);

      return EGL_FALSE;
   }
}

/*
 * Called from eglGetProcAddress() via drv->API.GetProcAddress().
 */
static _EGLProc
dri2_get_proc_address(_EGLDriver *drv, const char *procname)
{
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);

   return dri2_drv->get_proc_address(procname);
}

static EGLBoolean
dri2_wait_client(_EGLDriver *drv, _EGLDisplay *disp, _EGLContext *ctx)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(ctx->DrawSurface);

   (void) drv;

   /* FIXME: If EGL allows frontbuffer rendering for window surfaces,
    * we need to copy fake to real here.*/

   (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);

   return EGL_TRUE;
}

static EGLBoolean
dri2_wait_native(_EGLDriver *drv, _EGLDisplay *disp, EGLint engine)
{
   (void) drv;
   (void) disp;

   if (engine != EGL_CORE_NATIVE_ENGINE)
      return _eglError(EGL_BAD_PARAMETER, "eglWaitNative");
   /* glXWaitX(); */

   return EGL_TRUE;
}

static EGLBoolean
dri2_bind_tex_image(_EGLDriver *drv,
		    _EGLDisplay *disp, _EGLSurface *surf, EGLint buffer)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   struct dri2_egl_context *dri2_ctx;
   _EGLContext *ctx;
   GLint format, target;

   ctx = _eglGetCurrentContext();
   dri2_ctx = dri2_egl_context(ctx);

   if (!_eglBindTexImage(drv, disp, surf, buffer))
      return EGL_FALSE;

   switch (dri2_surf->base.TextureFormat) {
   case EGL_TEXTURE_RGB:
      format = __DRI_TEXTURE_FORMAT_RGB;
      break;
   case EGL_TEXTURE_RGBA:
      format = __DRI_TEXTURE_FORMAT_RGBA;
      break;
   default:
      assert(0);
   }

   switch (dri2_surf->base.TextureTarget) {
   case EGL_TEXTURE_2D:
      target = GL_TEXTURE_2D;
      break;
   default:
      assert(0);
   }

   (*dri2_dpy->tex_buffer->setTexBuffer2)(dri2_ctx->dri_context,
					  target, format,
					  dri2_surf->dri_drawable);

   return EGL_TRUE;
}

static EGLBoolean
dri2_release_tex_image(_EGLDriver *drv,
		       _EGLDisplay *disp, _EGLSurface *surf, EGLint buffer)
{
#if __DRI_TEX_BUFFER_VERSION >= 3
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   struct dri2_egl_context *dri2_ctx;
   _EGLContext *ctx;
   GLint  target;

   ctx = _eglGetCurrentContext();
   dri2_ctx = dri2_egl_context(ctx);

   if (!_eglReleaseTexImage(drv, disp, surf, buffer))
      return EGL_FALSE;

   switch (dri2_surf->base.TextureTarget) {
   case EGL_TEXTURE_2D:
      target = GL_TEXTURE_2D;
      break;
   default:
      assert(0);
   }
   if (dri2_dpy->tex_buffer->releaseTexBuffer!=NULL)
    (*dri2_dpy->tex_buffer->releaseTexBuffer)(dri2_ctx->dri_context,
                                             target,
                                             dri2_surf->dri_drawable);
#endif

   return EGL_TRUE;
}

static _EGLImage *
dri2_create_image(_EGLDisplay *disp, __DRIimage *dri_image)
{
   struct dri2_egl_image *dri2_img;

   if (dri_image == NULL) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image");
      return NULL;
   }

   dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image");
      return NULL;
   }

   if (!_eglInitImage(&dri2_img->base, disp)) {
      free(dri2_img);
      return NULL;
   }

   dri2_img->dri_image = dri_image;

   return &dri2_img->base;
}

static _EGLImage *
dri2_create_image_khr_renderbuffer(_EGLDisplay *disp, _EGLContext *ctx,
				   EGLClientBuffer buffer,
				   const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   GLuint renderbuffer = (GLuint) (uintptr_t) buffer;
   __DRIimage *dri_image;

   if (renderbuffer == 0) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
      return EGL_NO_IMAGE_KHR;
   }

   dri_image =
      dri2_dpy->image->createImageFromRenderbuffer(dri2_ctx->dri_context,
                                                   renderbuffer, NULL);

   return dri2_create_image(disp, dri_image);
}

static _EGLImage *
dri2_create_image_mesa_drm_buffer(_EGLDisplay *disp, _EGLContext *ctx,
				  EGLClientBuffer buffer, const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   EGLint format, name, pitch, err;
   _EGLImageAttribs attrs;
   __DRIimage *dri_image;

   name = (EGLint) (uintptr_t) buffer;

   err = _eglParseImageAttribList(&attrs, disp, attr_list);
   if (err != EGL_SUCCESS)
      return NULL;

   if (attrs.Width <= 0 || attrs.Height <= 0 ||
       attrs.DRMBufferStrideMESA <= 0) {
      _eglError(EGL_BAD_PARAMETER,
		"bad width, height or stride");
      return NULL;
   }

   switch (attrs.DRMBufferFormatMESA) {
   case EGL_DRM_BUFFER_FORMAT_ARGB32_MESA:
      format = __DRI_IMAGE_FORMAT_ARGB8888;
      pitch = attrs.DRMBufferStrideMESA;
      break;
   default:
      _eglError(EGL_BAD_PARAMETER,
		"dri2_create_image_khr: unsupported pixmap depth");
      return NULL;
   }

   dri_image =
      dri2_dpy->image->createImageFromName(dri2_dpy->dri_screen,
					   attrs.Width,
					   attrs.Height,
					   format,
					   name,
					   pitch,
					   NULL);

   return dri2_create_image(disp, dri_image);
}

#ifdef HAVE_WAYLAND_PLATFORM

/* This structure describes how a wl_buffer maps to one or more
 * __DRIimages.  A wl_drm_buffer stores the wl_drm format code and the
 * offsets and strides of the planes in the buffer.  This table maps a
 * wl_drm format code to a description of the planes in the buffer
 * that lets us create a __DRIimage for each of the planes. */

static const struct wl_drm_components_descriptor {
   uint32_t dri_components;
   EGLint components;
   int nplanes;
} wl_drm_components[] = {
   { __DRI_IMAGE_COMPONENTS_RGB, EGL_TEXTURE_RGB, 1 },
   { __DRI_IMAGE_COMPONENTS_RGBA, EGL_TEXTURE_RGBA, 1 },
   { __DRI_IMAGE_COMPONENTS_Y_U_V, EGL_TEXTURE_Y_U_V_WL, 3 },
   { __DRI_IMAGE_COMPONENTS_Y_UV, EGL_TEXTURE_Y_UV_WL, 2 },
   { __DRI_IMAGE_COMPONENTS_Y_XUXV, EGL_TEXTURE_Y_XUXV_WL, 2 },
};

static _EGLImage *
dri2_create_image_wayland_wl_buffer(_EGLDisplay *disp, _EGLContext *ctx,
				    EGLClientBuffer _buffer,
				    const EGLint *attr_list)
{
   struct wl_drm_buffer *buffer = (struct wl_drm_buffer *) _buffer;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   const struct wl_drm_components_descriptor *f;
   __DRIimage *dri_image;
   _EGLImageAttribs attrs;
   EGLint err;
   int32_t plane;

   if (!wayland_buffer_is_drm(&buffer->buffer))
       return NULL;

   err = _eglParseImageAttribList(&attrs, disp, attr_list);
   plane = attrs.PlaneWL;
   if (err != EGL_SUCCESS) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_wayland_wl_buffer");
      return NULL;
   }

   f = buffer->driver_format;
   if (plane < 0 || plane >= f->nplanes) {
      _eglError(EGL_BAD_PARAMETER,
                "dri2_create_image_wayland_wl_buffer (plane out of bounds)");
      return NULL;
   }

   dri_image = dri2_dpy->image->fromPlanar(buffer->driver_buffer, plane, NULL);

   if (dri_image == NULL) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_wayland_wl_buffer");
      return NULL;
   }

   return dri2_create_image(disp, dri_image);
}
#endif

_EGLImage *
dri2_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
		      _EGLContext *ctx, EGLenum target,
		      EGLClientBuffer buffer, const EGLint *attr_list)
{
   (void) drv;

   switch (target) {
   case EGL_GL_RENDERBUFFER_KHR:
      return dri2_create_image_khr_renderbuffer(disp, ctx, buffer, attr_list);
   case EGL_DRM_BUFFER_MESA:
      return dri2_create_image_mesa_drm_buffer(disp, ctx, buffer, attr_list);
#ifdef HAVE_WAYLAND_PLATFORM
   case EGL_WAYLAND_BUFFER_WL:
      return dri2_create_image_wayland_wl_buffer(disp, ctx, buffer, attr_list);
#endif
   default:
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
      return EGL_NO_IMAGE_KHR;
   }
}

static EGLBoolean
dri2_destroy_image_khr(_EGLDriver *drv, _EGLDisplay *disp, _EGLImage *image)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img = dri2_egl_image(image);

   (void) drv;

   dri2_dpy->image->destroyImage(dri2_img->dri_image);
   free(dri2_img);

   return EGL_TRUE;
}

static _EGLImage *
dri2_create_drm_image_mesa(_EGLDriver *drv, _EGLDisplay *disp,
			   const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img;
   _EGLImageAttribs attrs;
   unsigned int dri_use, valid_mask;
   int format;
   EGLint err = EGL_SUCCESS;

   (void) drv;

   dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr");
      return EGL_NO_IMAGE_KHR;
   }

   if (!attr_list) {
      err = EGL_BAD_PARAMETER;
      goto cleanup_img;
   }

   if (!_eglInitImage(&dri2_img->base, disp)) {
      err = EGL_BAD_PARAMETER;
      goto cleanup_img;
   }

   err = _eglParseImageAttribList(&attrs, disp, attr_list);
   if (err != EGL_SUCCESS)
      goto cleanup_img;

   if (attrs.Width <= 0 || attrs.Height <= 0) {
      _eglLog(_EGL_WARNING, "bad width or height (%dx%d)",
            attrs.Width, attrs.Height);
      goto cleanup_img;
   }

   switch (attrs.DRMBufferFormatMESA) {
   case EGL_DRM_BUFFER_FORMAT_ARGB32_MESA:
      format = __DRI_IMAGE_FORMAT_ARGB8888;
      break;
   default:
      _eglLog(_EGL_WARNING, "bad image format value 0x%04x",
            attrs.DRMBufferFormatMESA);
      goto cleanup_img;
   }

   valid_mask =
      EGL_DRM_BUFFER_USE_SCANOUT_MESA |
      EGL_DRM_BUFFER_USE_SHARE_MESA |
      EGL_DRM_BUFFER_USE_CURSOR_MESA;
   if (attrs.DRMBufferUseMESA & ~valid_mask) {
      _eglLog(_EGL_WARNING, "bad image use bit 0x%04x",
            attrs.DRMBufferUseMESA & ~valid_mask);
      goto cleanup_img;
   }

   dri_use = 0;
   if (attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_SHARE_MESA)
      dri_use |= __DRI_IMAGE_USE_SHARE;
   if (attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_SCANOUT_MESA)
      dri_use |= __DRI_IMAGE_USE_SCANOUT;
   if (attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_CURSOR_MESA)
      dri_use |= __DRI_IMAGE_USE_CURSOR;

   dri2_img->dri_image = 
      dri2_dpy->image->createImage(dri2_dpy->dri_screen,
				   attrs.Width, attrs.Height,
                                   format, dri_use, dri2_img);
   if (dri2_img->dri_image == NULL) {
      err = EGL_BAD_ALLOC;
      goto cleanup_img;
   }

   return &dri2_img->base;

 cleanup_img:
   free(dri2_img);
   _eglError(err, "dri2_create_drm_image_mesa");

   return EGL_NO_IMAGE_KHR;
}

static EGLBoolean
dri2_export_drm_image_mesa(_EGLDriver *drv, _EGLDisplay *disp, _EGLImage *img,
			  EGLint *name, EGLint *handle, EGLint *stride)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img = dri2_egl_image(img);

   (void) drv;

   if (name && !dri2_dpy->image->queryImage(dri2_img->dri_image,
					    __DRI_IMAGE_ATTRIB_NAME, name)) {
      _eglError(EGL_BAD_ALLOC, "dri2_export_drm_image_mesa");
      return EGL_FALSE;
   }

   if (handle)
      dri2_dpy->image->queryImage(dri2_img->dri_image,
				  __DRI_IMAGE_ATTRIB_HANDLE, handle);

   if (stride)
      dri2_dpy->image->queryImage(dri2_img->dri_image,
				  __DRI_IMAGE_ATTRIB_STRIDE, stride);

   return EGL_TRUE;
}

#ifdef HAVE_WAYLAND_PLATFORM

static void
dri2_wl_reference_buffer(void *user_data, uint32_t name,
                         struct wl_drm_buffer *buffer)
{
   _EGLDisplay *disp = user_data;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   __DRIimage *img;
   int i, dri_components = 0;

   img = dri2_dpy->image->createImageFromNames(dri2_dpy->dri_screen,
                                               buffer->buffer.width,
                                               buffer->buffer.height,
                                               buffer->format, (int*)&name, 1,
                                               buffer->stride,
                                               buffer->offset,
                                               NULL);

   if (img == NULL)
      return;

   dri2_dpy->image->queryImage(img, __DRI_IMAGE_ATTRIB_COMPONENTS, &dri_components);

   buffer->driver_format = NULL;
   for (i = 0; i < ARRAY_SIZE(wl_drm_components); i++)
      if (wl_drm_components[i].dri_components == dri_components)
         buffer->driver_format = &wl_drm_components[i];

   if (buffer->driver_format == NULL)
      dri2_dpy->image->destroyImage(img);
   else
      buffer->driver_buffer = img;
}

static void
dri2_wl_release_buffer(void *user_data, struct wl_drm_buffer *buffer)
{
   _EGLDisplay *disp = user_data;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   dri2_dpy->image->destroyImage(buffer->driver_buffer);
}

static struct wayland_drm_callbacks wl_drm_callbacks = {
	.authenticate = NULL,
	.reference_buffer = dri2_wl_reference_buffer,
	.release_buffer = dri2_wl_release_buffer
};

static EGLBoolean
dri2_bind_wayland_display_wl(_EGLDriver *drv, _EGLDisplay *disp,
			     struct wl_display *wl_dpy)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   (void) drv;

   if (dri2_dpy->wl_server_drm)
	   return EGL_FALSE;

   wl_drm_callbacks.authenticate =
      (int(*)(void *, uint32_t)) dri2_dpy->authenticate;

   dri2_dpy->wl_server_drm =
	   wayland_drm_init(wl_dpy, dri2_dpy->device_name,
                            &wl_drm_callbacks, disp);

   if (!dri2_dpy->wl_server_drm)
	   return EGL_FALSE;

   return EGL_TRUE;
}

static EGLBoolean
dri2_unbind_wayland_display_wl(_EGLDriver *drv, _EGLDisplay *disp,
			       struct wl_display *wl_dpy)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   (void) drv;

   if (!dri2_dpy->wl_server_drm)
	   return EGL_FALSE;

   wayland_drm_uninit(dri2_dpy->wl_server_drm);
   dri2_dpy->wl_server_drm = NULL;

   return EGL_TRUE;
}

static EGLBoolean
dri2_query_wayland_buffer_wl(_EGLDriver *drv, _EGLDisplay *disp,
                             struct wl_buffer *_buffer,
                             EGLint attribute, EGLint *value)
{
   struct wl_drm_buffer *buffer = (struct wl_drm_buffer *) _buffer;
   const struct wl_drm_components_descriptor *format;

   if (!wayland_buffer_is_drm(&buffer->buffer))
      return EGL_FALSE;

   format = buffer->driver_format;
   switch (attribute) {
   case EGL_TEXTURE_FORMAT:
      *value = format->components;
      return EGL_TRUE;
   case EGL_WIDTH:
      *value = buffer->buffer.width;
      break;
   case EGL_HEIGHT:
      *value = buffer->buffer.height;
      break;
   }

   return EGL_FALSE;
}
#endif

static void
dri2_unload(_EGLDriver *drv)
{
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);

   if (dri2_drv->handle)
      dlclose(dri2_drv->handle);
   free(dri2_drv);
}

static EGLBoolean
dri2_load(_EGLDriver *drv)
{
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);
#ifdef HAVE_SHARED_GLAPI
#ifdef HAVE_ANDROID_PLATFORM
   const char *libname = "libglapi.so";
#else
   const char *libname = "libglapi.so.0";
#endif
#else
   /*
    * Both libGL.so and libglapi.so are glapi providers.  There is no way to
    * tell which one to load.
    */
   const char *libname = NULL;
#endif
   void *handle;

   /* RTLD_GLOBAL to make sure glapi symbols are visible to DRI drivers */
   handle = dlopen(libname, RTLD_LAZY | RTLD_GLOBAL);
   if (handle) {
      dri2_drv->get_proc_address = (_EGLProc (*)(const char *))
         dlsym(handle, "_glapi_get_proc_address");
      if (!dri2_drv->get_proc_address || !libname) {
         /* no need to keep a reference */
         dlclose(handle);
         handle = NULL;
      }
   }

   /* if glapi is not available, loading DRI drivers will fail */
   if (!dri2_drv->get_proc_address) {
      _eglLog(_EGL_WARNING, "DRI2: failed to find _glapi_get_proc_address");
      return EGL_FALSE;
   }

   dri2_drv->glFlush = (void (*)(void))
      dri2_drv->get_proc_address("glFlush");

   dri2_drv->handle = handle;

   return EGL_TRUE;
}

/**
 * This is the main entrypoint into the driver, called by libEGL.
 * Create a new _EGLDriver object and init its dispatch table.
 */
_EGLDriver *
_eglBuiltInDriverDRI2(const char *args)
{
   struct dri2_egl_driver *dri2_drv;

   (void) args;

   dri2_drv = malloc(sizeof *dri2_drv);
   if (!dri2_drv)
      return NULL;

   memset(dri2_drv, 0, sizeof *dri2_drv);

   if (!dri2_load(&dri2_drv->base)) {
      free(dri2_drv);
      return NULL;
   }

   _eglInitDriverFallbacks(&dri2_drv->base);
   dri2_drv->base.API.Initialize = dri2_initialize;
   dri2_drv->base.API.Terminate = dri2_terminate;
   dri2_drv->base.API.CreateContext = dri2_create_context;
   dri2_drv->base.API.DestroyContext = dri2_destroy_context;
   dri2_drv->base.API.MakeCurrent = dri2_make_current;
   dri2_drv->base.API.GetProcAddress = dri2_get_proc_address;
   dri2_drv->base.API.WaitClient = dri2_wait_client;
   dri2_drv->base.API.WaitNative = dri2_wait_native;
   dri2_drv->base.API.BindTexImage = dri2_bind_tex_image;
   dri2_drv->base.API.ReleaseTexImage = dri2_release_tex_image;
   dri2_drv->base.API.CreateImageKHR = dri2_create_image_khr;
   dri2_drv->base.API.DestroyImageKHR = dri2_destroy_image_khr;
   dri2_drv->base.API.CreateDRMImageMESA = dri2_create_drm_image_mesa;
   dri2_drv->base.API.ExportDRMImageMESA = dri2_export_drm_image_mesa;
#ifdef HAVE_WAYLAND_PLATFORM
   dri2_drv->base.API.BindWaylandDisplayWL = dri2_bind_wayland_display_wl;
   dri2_drv->base.API.UnbindWaylandDisplayWL = dri2_unbind_wayland_display_wl;
   dri2_drv->base.API.QueryWaylandBufferWL = dri2_query_wayland_buffer_wl;
#endif

   dri2_drv->base.Name = "DRI2";
   dri2_drv->base.Unload = dri2_unload;

   return &dri2_drv->base;
}
