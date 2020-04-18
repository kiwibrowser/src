/*
 * Mesa 3-D graphics library
 * Version: 7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Chia-I Wu <olv@lunarg.com>
 */

#include "dxgi_private.h"
#include <stdio.h>
extern "C"
{
#include "state_tracker/drm_driver.h"
#include "util/u_dl.h"
}
#define PIPE_PREFIX "pipe_"

static const char *
get_search_path(void)
{
 static const char *search_path;

 if (!search_path) {
	 static char buffer[1024];
	 const char *p;
	 int ret;

	 p = getenv("DXGI_DRIVERS_PATH");
	 if(!p)
		 p = getenv("EGL_DRIVERS_PATH");
#ifdef __unix__
	 if (p && (geteuid() != getuid() || getegid() != getgid())) {
	 p = NULL;
	 }
#endif

	 if (p) {
	 ret = snprintf(buffer, sizeof(buffer),
		 "%s:%s", p, DXGI_DRIVER_SEARCH_DIR);
	 if (ret > 0 && ret < (int)sizeof(buffer))
		search_path = buffer;
	 }
 }
 if (!search_path)
	 search_path = DXGI_DRIVER_SEARCH_DIR;

 return search_path;
}

static void
for_each_colon_separated(const char *search_path,
		 bool (*loader)(const char *, size_t, void *),
		 void *loader_data)
{
 const char *cur, *next;
 size_t len;

 cur = search_path;
 while (cur) {
	 next = strchr(cur, ':');
	 len = (next) ? next - cur : strlen(cur);

	 if (!loader(cur, len, loader_data))
	 break;

	 cur = (next) ? next + 1 : NULL;
 }
}

void
for_each_in_search_path(bool (*callback)(const char *, size_t, void *),
			 void *callback_data)
{
 const char *search_path = get_search_path();
 for_each_colon_separated(search_path, callback, callback_data);
}

static struct pipe_module {
 boolean initialized;
 char *name;
 struct util_dl_library *lib;
 const struct drm_driver_descriptor *drmdd;
 struct pipe_screen *(*swrast_create_screen)(struct sw_winsys *);
} pipe_modules[16];

static bool
dlopen_pipe_module_cb(const char *dir, size_t len, void *callback_data)
{
 struct pipe_module *pmod = (struct pipe_module *) callback_data;
 char path[1024];
 int ret;

 if (len) {
	 ret = snprintf(path, sizeof(path),
		"%.*s/" PIPE_PREFIX "%s" UTIL_DL_EXT, len, dir, pmod->name);
 }
 else {
	 ret = snprintf(path, sizeof(path),
		PIPE_PREFIX "%s" UTIL_DL_EXT, pmod->name);
 }
 if (ret > 0 && ret < (int)sizeof(path)) {
	 pmod->lib = util_dl_open(path);
 }

 return !(pmod->lib);
}

static bool
load_pipe_module(struct pipe_module *pmod, const char *name)
{
 pmod->name = strdup(name);
 if (!pmod->name)
	 return FALSE;

 for_each_in_search_path(dlopen_pipe_module_cb, (void *) pmod);
 if (pmod->lib) {
	 pmod->drmdd = (const struct drm_driver_descriptor *)
	 util_dl_get_proc_address(pmod->lib, "driver_descriptor");

	 /* sanity check on the name */
	 if (pmod->drmdd && strcmp(pmod->drmdd->name, pmod->name) != 0)
	 pmod->drmdd = NULL;

	 /* swrast */
	 if (pmod->drmdd && !pmod->drmdd->driver_name) {
	 pmod->swrast_create_screen =
		(struct pipe_screen *(*)(struct sw_winsys *))
		util_dl_get_proc_address(pmod->lib, "swrast_create_screen");
	 if (!pmod->swrast_create_screen)
		pmod->drmdd = NULL;
	 }

	 if (!pmod->drmdd) {
	 util_dl_close(pmod->lib);
	 pmod->lib = NULL;
	 }
 }

 return (pmod->drmdd != NULL);
}


static struct pipe_module *
get_pipe_module(const char *name)
{
 struct pipe_module *pmod = NULL;
 unsigned i;

 if (!name)
	 return NULL;

 for (i = 0; i < sizeof(pipe_modules) / sizeof(pipe_modules[0]); i++) {
	 if (!pipe_modules[i].initialized ||
	 strcmp(pipe_modules[i].name, name) == 0) {
	 pmod = &pipe_modules[i];
	 break;
	 }
 }
 if (!pmod)
	 return NULL;

 if (!pmod->initialized) {
	 load_pipe_module(pmod, name);
	 pmod->initialized = TRUE;
 }

 return pmod;
}

struct native_display;

struct pipe_screen *
dxgi_loader_create_drm_screen(struct native_display* dpy, const char *name, int fd)
{
 struct pipe_module *pmod = get_pipe_module(name);
 return (pmod && pmod->drmdd && pmod->drmdd->create_screen) ?
	 pmod->drmdd->create_screen(fd) : NULL;
}

struct pipe_screen *
dxgi_loader_create_sw_screen(struct native_display* dpy, struct sw_winsys *ws)
{
 struct pipe_module *pmod = get_pipe_module("swrast");
 return (pmod && pmod->swrast_create_screen) ?
	 pmod->swrast_create_screen(ws) : NULL;
}
