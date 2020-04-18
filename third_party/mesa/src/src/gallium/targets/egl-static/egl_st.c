/*
 * Mesa 3-D graphics library
 * Version:  7.10
 *
 * Copyright (C) 2011 LunarG Inc.
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
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */
#include "util/u_debug.h"
#include "state_tracker/st_api.h"
#include "egl_st.h"

#if FEATURE_GL || FEATURE_ES1 || FEATURE_ES2
#include "state_tracker/st_gl_api.h"
#endif

#if FEATURE_VG
#include "vg_api.h"
#endif

#if _EGL_EXTERNAL_GL

#include "util/u_string.h"
#include "util/u_dl.h"
#include "egldriver.h"
#include "egllog.h"

static struct util_dl_library *egl_st_gl_lib;

static EGLBoolean
dlopen_gl_lib_cb(const char *dir, size_t len, void *callback_data)
{
   const char *name = (const char *) callback_data;
   char path[1024];
   int ret;

   if (len) {
      assert(len <= INT_MAX && "path is insanely long!");
      ret = util_snprintf(path, sizeof(path), "%.*s/%s" UTIL_DL_EXT,
            (int)len, dir, name);
   }
   else {
      ret = util_snprintf(path, sizeof(path), "%s" UTIL_DL_EXT, name);
   }

   if (ret > 0 && ret < sizeof(path)) {
      egl_st_gl_lib = util_dl_open(path);
      if (egl_st_gl_lib)
         _eglLog(_EGL_DEBUG, "loaded %s", path);
   }

   return !egl_st_gl_lib;
}

static struct st_api *
load_gl(const char *name, const char *procname)
{
   struct st_api *(*create_api)(void);
   struct st_api *stapi = NULL;

   _eglSearchPathForEach(dlopen_gl_lib_cb, (void *) name);
   if (!egl_st_gl_lib)
      return NULL;

   create_api = (struct st_api *(*)(void))
      util_dl_get_proc_address(egl_st_gl_lib, procname);
   if (create_api)
      stapi = create_api();

   if (!stapi) {
      util_dl_close(egl_st_gl_lib);
      egl_st_gl_lib = NULL;
   }

   return stapi;
}

static struct st_api *
egl_st_load_gl(void)
{
   const char module[] = "st_GL";
   const char symbol[] = "st_api_create_OpenGL";
   struct st_api *stapi;

   stapi = load_gl(module, symbol);

   /* try again with libglapi.so loaded */
   if (!stapi) {
      struct util_dl_library *glapi = util_dl_open("libglapi" UTIL_DL_EXT);

      if (glapi) {
         _eglLog(_EGL_DEBUG, "retry with libglapi" UTIL_DL_EXT " loaded");

         stapi = load_gl(module, symbol);
         util_dl_close(glapi);
      }
   }
   if (!stapi)
      _eglLog(_EGL_WARNING, "unable to load %s" UTIL_DL_EXT, module);

   return stapi;
}

#endif /* _EGL_EXTERNAL_GL */

struct st_api *
egl_st_create_api(enum st_api_type api)
{
   struct st_api *stapi = NULL;

   switch (api) {
   case ST_API_OPENGL:
#if FEATURE_GL || FEATURE_ES1 || FEATURE_ES2
#if _EGL_EXTERNAL_GL
      stapi = egl_st_load_gl();
#else
      stapi = st_gl_api_create();
#endif
#endif
      break;
   case ST_API_OPENVG:
#if FEATURE_VG
      stapi = (struct st_api *) vg_api_get();
#endif
      break;
   default:
      assert(!"Unknown API Type\n");
      break;
   }

   return stapi;
}

void
egl_st_destroy_api(struct st_api *stapi)
{
#if _EGL_EXTERNAL_GL
   boolean is_gl = (stapi->api == ST_API_OPENGL);

   stapi->destroy(stapi);

   if (is_gl) {
      util_dl_close(egl_st_gl_lib);
      egl_st_gl_lib = NULL;
   }
#else
   stapi->destroy(stapi);
#endif
}

uint
egl_st_get_profile_mask(enum st_api_type api)
{
   uint mask = 0x0;

   switch (api) {
   case ST_API_OPENGL:
#if FEATURE_GL
      mask |= ST_PROFILE_DEFAULT_MASK;
#endif
#if FEATURE_ES1
      mask |= ST_PROFILE_OPENGL_ES1_MASK;
#endif
#if FEATURE_ES2
      mask |= ST_PROFILE_OPENGL_ES2_MASK;
#endif
      break;
   case ST_API_OPENVG:
#if FEATURE_VG
      mask |= ST_PROFILE_DEFAULT_MASK;
#endif
      break;
   default:
      break;
   }

   return mask;
}
