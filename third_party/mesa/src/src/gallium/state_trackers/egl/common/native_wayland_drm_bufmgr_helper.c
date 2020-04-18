#include <stdint.h>
#include <string.h>

#include "native.h"
#include "util/u_inlines.h"
#include "state_tracker/drm_driver.h"

#ifdef HAVE_WAYLAND_BACKEND

#include <wayland-server.h>
#include <wayland-drm-server-protocol.h>

#include "native_wayland_drm_bufmgr_helper.h"

void
egl_g3d_wl_drm_helper_reference_buffer(void *user_data, uint32_t name,
                                       struct wl_drm_buffer *buffer)
{
   struct native_display *ndpy = user_data;
   struct pipe_resource templ;
   struct winsys_handle wsh;
   enum pipe_format pf;

   switch (buffer->format) {
   case WL_DRM_FORMAT_ARGB8888:
      pf = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   case WL_DRM_FORMAT_XRGB8888:
      pf = PIPE_FORMAT_B8G8R8X8_UNORM;
      break;
   default:
      pf = PIPE_FORMAT_NONE;
      break;
   }

   if (pf == PIPE_FORMAT_NONE)
      return;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = pf;
   templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   templ.width0 = buffer->buffer.width;
   templ.height0 = buffer->buffer.height;
   templ.depth0 = 1;
   templ.array_size = 1;

   memset(&wsh, 0, sizeof(wsh));
   wsh.handle = name;
   wsh.stride = buffer->stride[0];

   buffer->driver_buffer =
      ndpy->screen->resource_from_handle(ndpy->screen, &templ, &wsh);
}

void
egl_g3d_wl_drm_helper_unreference_buffer(void *user_data,
                                         struct wl_drm_buffer *buffer)
{
   struct pipe_resource *resource = buffer->driver_buffer;

   pipe_resource_reference(&resource, NULL);
}

struct pipe_resource *
egl_g3d_wl_drm_common_wl_buffer_get_resource(struct native_display *ndpy,
                                             struct wl_buffer *buffer)
{
   return wayland_drm_buffer_get_buffer(buffer);
}

EGLBoolean
egl_g3d_wl_drm_common_query_buffer(struct native_display *ndpy,
                                   struct wl_buffer *_buffer,
                                   EGLint attribute, EGLint *value)
{
   struct wl_drm_buffer *buffer = (struct wl_drm_buffer *) _buffer;
   struct pipe_resource *resource = buffer->driver_buffer;

   if (!wayland_buffer_is_drm(&buffer->buffer))
      return EGL_FALSE;

   switch (attribute) {
   case EGL_TEXTURE_FORMAT:
      switch (resource->format) {
      case PIPE_FORMAT_B8G8R8A8_UNORM:
         *value = EGL_TEXTURE_RGBA;
         return EGL_TRUE;
      case PIPE_FORMAT_B8G8R8X8_UNORM:
         *value = EGL_TEXTURE_RGB;
         return EGL_TRUE;
      default:
         return EGL_FALSE;
      }
   case EGL_WIDTH:
      *value = buffer->buffer.width;
      return EGL_TRUE;
   case EGL_HEIGHT:
      *value = buffer->buffer.height;
      return EGL_TRUE;
   default:
      return EGL_FALSE;
   }
}

#endif
