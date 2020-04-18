#ifndef _WAYLAND_EGL_PRIV_H
#define _WAYLAND_EGL_PRIV_H

#ifdef  __cplusplus
extern "C" {
#endif

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WL_EGL_EXPORT __attribute__ ((visibility("default")))
#else
#define WL_EGL_EXPORT
#endif

#include <wayland-client.h>

struct wl_egl_window {
	struct wl_surface *surface;

	int width;
	int height;
	int dx;
	int dy;

	int attached_width;
	int attached_height;

	void *private;
	void (*resize_callback)(struct wl_egl_window *, void *);
};

#ifdef  __cplusplus
}
#endif

#endif
