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
 * Functions for choosing and opening/loading device drivers.
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "eglstring.h"
#include "egldefines.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "egllog.h"
#include "eglmutex.h"

#if defined(_EGL_OS_UNIX)
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif


typedef struct _egl_module {
   char *Path;
   _EGLMain_t BuiltIn;
   void *Handle;
   _EGLDriver *Driver;
} _EGLModule;

static _EGL_DECLARE_MUTEX(_eglModuleMutex);
static _EGLArray *_eglModules;

const struct {
   const char *name;
   _EGLMain_t main;
} _eglBuiltInDrivers[] = {
#ifdef _EGL_BUILT_IN_DRIVER_GALLIUM
   { "egl_gallium", _eglBuiltInDriverGALLIUM },
#endif
#ifdef _EGL_BUILT_IN_DRIVER_DRI2
   { "egl_dri2", _eglBuiltInDriverDRI2 },
#endif
#ifdef _EGL_BUILT_IN_DRIVER_GLX
   { "egl_glx", _eglBuiltInDriverGLX },
#endif
   { NULL, NULL }
};

/**
 * Wrappers for dlopen/dlclose()
 */
#if defined(_EGL_OS_WINDOWS)


typedef HMODULE lib_handle;

static HMODULE
open_library(const char *filename)
{
   return LoadLibrary(filename);
}

static void
close_library(HMODULE lib)
{
   FreeLibrary(lib);
}


static const char *
library_suffix(void)
{
   return ".dll";
}


#elif defined(_EGL_OS_UNIX)


typedef void * lib_handle;

static void *
open_library(const char *filename)
{
   return dlopen(filename, RTLD_LAZY);
}

static void
close_library(void *lib)
{
   dlclose(lib);
}


static const char *
library_suffix(void)
{
   return ".so";
}


#endif


/**
 * Open the named driver and find its bootstrap function: _eglMain().
 */
static _EGLMain_t
_eglOpenLibrary(const char *driverPath, lib_handle *handle)
{
   lib_handle lib;
   _EGLMain_t mainFunc = NULL;
   const char *error = "unknown error";

   assert(driverPath);

   _eglLog(_EGL_DEBUG, "dlopen(%s)", driverPath);
   lib = open_library(driverPath);

#if defined(_EGL_OS_WINDOWS)
   /* XXX untested */
   if (lib)
      mainFunc = (_EGLMain_t) GetProcAddress(lib, "_eglMain");
#elif defined(_EGL_OS_UNIX)
   if (lib) {
      union {
         _EGLMain_t func;
         void *ptr;
      } tmp = { NULL };
      /* direct cast gives a warning when compiled with -pedantic */
      tmp.ptr = dlsym(lib, "_eglMain");
      mainFunc = tmp.func;
      if (!mainFunc)
         error = dlerror();
   }
   else {
      error = dlerror();
   }
#endif

   if (!lib) {
      _eglLog(_EGL_WARNING, "Could not open driver %s (%s)",
              driverPath, error);
      return NULL;
   }

   if (!mainFunc) {
      _eglLog(_EGL_WARNING, "_eglMain not found in %s (%s)",
              driverPath, error);
      if (lib)
         close_library(lib);
      return NULL;
   }

   *handle = lib;
   return mainFunc;
}


/**
 * Load a module and create the driver object.
 */
static EGLBoolean
_eglLoadModule(_EGLModule *mod)
{
   _EGLMain_t mainFunc;
   lib_handle lib;
   _EGLDriver *drv;

   if (mod->Driver)
      return EGL_TRUE;

   if (mod->BuiltIn) {
      lib = (lib_handle) NULL;
      mainFunc = mod->BuiltIn;
   }
   else {
      mainFunc = _eglOpenLibrary(mod->Path, &lib);
      if (!mainFunc)
         return EGL_FALSE;
   }

   drv = mainFunc(NULL);
   if (!drv) {
      if (lib)
         close_library(lib);
      return EGL_FALSE;
   }

   if (!drv->Name) {
      _eglLog(_EGL_WARNING, "Driver loaded from %s has no name", mod->Path);
      drv->Name = "UNNAMED";
   }

   mod->Handle = (void *) lib;
   mod->Driver = drv;

   return EGL_TRUE;
}


/**
 * Unload a module.
 */
static void
_eglUnloadModule(_EGLModule *mod)
{
#if defined(_EGL_OS_UNIX)
   /* destroy the driver */
   if (mod->Driver && mod->Driver->Unload)
      mod->Driver->Unload(mod->Driver);

   /*
    * XXX At this point (atexit), the module might be the last reference to
    * libEGL.  Closing the module might unmap libEGL and give problems.
    */
#if 0
   if (mod->Handle)
      close_library(mod->Handle);
#endif
#elif defined(_EGL_OS_WINDOWS)
   /* XXX Windows unloads DLLs before atexit */
#endif

   mod->Driver = NULL;
   mod->Handle = NULL;
}


/**
 * Add a module to the module array.
 */
static _EGLModule *
_eglAddModule(const char *path)
{
   _EGLModule *mod;
   EGLint i;

   if (!_eglModules) {
      _eglModules = _eglCreateArray("Module", 8);
      if (!_eglModules)
         return NULL;
   }

   /* find duplicates */
   for (i = 0; i < _eglModules->Size; i++) {
      mod = _eglModules->Elements[i];
      if (strcmp(mod->Path, path) == 0)
         return mod;
   }

   /* allocate a new one */
   mod = calloc(1, sizeof(*mod));
   if (mod) {
      mod->Path = _eglstrdup(path);
      if (!mod->Path) {
         free(mod);
         mod = NULL;
      }
   }
   if (mod) {
      _eglAppendArray(_eglModules, (void *) mod);
      _eglLog(_EGL_DEBUG, "added %s to module array", mod->Path);
   }

   return mod;
}


/**
 * Free a module.
 */
static void
_eglFreeModule(void *module)
{
   _EGLModule *mod = (_EGLModule *) module;

   _eglUnloadModule(mod);
   free(mod->Path);
   free(mod);
}


/**
 * A loader function for use with _eglPreloadForEach.  The loader data is the
 * filename of the driver.   This function stops on the first valid driver.
 */
static EGLBoolean
_eglLoaderFile(const char *dir, size_t len, void *loader_data)
{
   char path[1024];
   const char *filename = (const char *) loader_data;
   size_t flen = strlen(filename);

   /* make a full path */
   if (len + flen + 2 > sizeof(path))
      return EGL_TRUE;
   if (len) {
      memcpy(path, dir, len);
      path[len++] = '/';
   }
   memcpy(path + len, filename, flen);
   len += flen;
   path[len] = '\0';

   if (library_suffix()) {
      const char *suffix = library_suffix();
      size_t slen = strlen(suffix);
      const char *p;
      EGLBoolean need_suffix;

      p = filename + flen - slen;
      need_suffix = (p < filename || strcmp(p, suffix) != 0);
      if (need_suffix) {
         /* overflow */
         if (len + slen + 1 > sizeof(path))
            return EGL_TRUE;
         strcpy(path + len, suffix);
      }
   }

#if defined(_EGL_OS_UNIX)
   /* check if the file exists */
   if (access(path, F_OK))
      return EGL_TRUE;
#endif

   _eglAddModule(path);

   return EGL_TRUE;
}


/**
 * Run the callback function on each driver directory.
 *
 * The process may end prematurely if the callback function returns false.
 */
static void
_eglPreloadForEach(const char *search_path,
                   EGLBoolean (*loader)(const char *, size_t, void *),
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


/**
 * Return a list of colon-separated driver directories.
 */
static const char *
_eglGetSearchPath(void)
{
   static char search_path[1024];

#if defined(_EGL_OS_UNIX) || defined(_EGL_OS_WINDOWS)
   if (search_path[0] == '\0') {
      char *buf = search_path;
      size_t len = sizeof(search_path);
      EGLBoolean use_env;
      char dir_sep;
      int ret;

#if defined(_EGL_OS_UNIX)
      use_env = (geteuid() == getuid() && getegid() == getgid());
      dir_sep = '/';
#else
      use_env = EGL_TRUE;
      dir_sep = '\\';
#endif

      if (use_env) {
         char *p;

         /* extract the dirname from EGL_DRIVER */
         p = getenv("EGL_DRIVER");
         if (p && strchr(p, dir_sep)) {
            ret = _eglsnprintf(buf, len, "%s", p);
            if (ret > 0 && ret < len) {
               p = strrchr(buf, dir_sep);
               *p++ = ':';

               len -= p - buf;
               buf = p;
            }
         }

         /* append EGL_DRIVERS_PATH */
         p = getenv("EGL_DRIVERS_PATH");
         if (p) {
            ret = _eglsnprintf(buf, len, "%s:", p);
            if (ret > 0 && ret < len) {
               buf += ret;
               len -= ret;
            }
         }
      }
      else {
         _eglLog(_EGL_DEBUG,
               "ignore EGL_DRIVERS_PATH for setuid/setgid binaries");
      }

      ret = _eglsnprintf(buf, len, "%s", _EGL_DRIVER_SEARCH_DIR);
      if (ret < 0 || ret >= len)
         search_path[0] = '\0';

      _eglLog(_EGL_DEBUG, "EGL search path is %s", search_path);
   }
#endif /* defined(_EGL_OS_UNIX) || defined(_EGL_OS_WINDOWS) */

   return search_path;
}


/**
 * Add the user driver to the module array.
 *
 * The user driver is specified by EGL_DRIVER.
 */
static EGLBoolean
_eglAddUserDriver(void)
{
   const char *search_path = _eglGetSearchPath();
   char *env;
   size_t name_len = 0;

   env = getenv("EGL_DRIVER");
#if defined(_EGL_OS_UNIX)
   if (env && strchr(env, '/')) {
      search_path = "";
      if ((geteuid() != getuid() || getegid() != getgid())) {
         _eglLog(_EGL_DEBUG,
               "ignore EGL_DRIVER for setuid/setgid binaries");
         env = NULL;
      }
   }
   else if (env) {
      char *suffix = strchr(env, '.');
      name_len = (suffix) ? suffix - env : strlen(env);
   }
#else
   if (env)
      name_len = strlen(env);
#endif /* _EGL_OS_UNIX */

   /*
    * Try built-in drivers first if we know the driver name.  This makes sure
    * we do not load the outdated external driver that is still on the
    * filesystem.
    */
   if (name_len) {
      _EGLModule *mod;
      EGLint i;

      for (i = 0; _eglBuiltInDrivers[i].name; i++) {
         if (strlen(_eglBuiltInDrivers[i].name) == name_len &&
             !strncmp(_eglBuiltInDrivers[i].name, env, name_len)) {
            mod = _eglAddModule(env);
            if (mod)
               mod->BuiltIn = _eglBuiltInDrivers[i].main;

            return EGL_TRUE;
         }
      }
   }

   /* otherwise, treat env as a path */
   if (env) {
      _eglPreloadForEach(search_path, _eglLoaderFile, (void *) env);

      return EGL_TRUE;
   }

   return EGL_FALSE;
}


/**
 * Add egl_gallium to the module array.
 */
static void
_eglAddGalliumDriver(void)
{
#ifndef _EGL_BUILT_IN_DRIVER_GALLIUM
   void *external = (void *) "egl_gallium";
   _eglPreloadForEach(_eglGetSearchPath(), _eglLoaderFile, external);
#endif
}


/**
 * Add built-in drivers to the module array.
 */
static void
_eglAddBuiltInDrivers(void)
{
   _EGLModule *mod;
   EGLint i;

   for (i = 0; _eglBuiltInDrivers[i].name; i++) {
      mod = _eglAddModule(_eglBuiltInDrivers[i].name);
      if (mod)
         mod->BuiltIn = _eglBuiltInDrivers[i].main;
   }
}


/**
 * Add drivers to the module array.  Drivers will be loaded as they are matched
 * to displays.
 */
static EGLBoolean
_eglAddDrivers(void)
{
   if (_eglModules)
      return EGL_TRUE;

   if (!_eglAddUserDriver()) {
      /*
       * Add other drivers only when EGL_DRIVER is not set.  The order here
       * decides the priorities.
       */
      _eglAddGalliumDriver();
      _eglAddBuiltInDrivers();
   }

   return (_eglModules != NULL);
}


/**
 * A helper function for _eglMatchDriver.  It finds the first driver that can
 * initialize the display and return.
 */
static _EGLDriver *
_eglMatchAndInitialize(_EGLDisplay *dpy)
{
   _EGLDriver *drv = NULL;
   EGLint i = 0;

   if (!_eglAddDrivers()) {
      _eglLog(_EGL_WARNING, "failed to find any driver");
      return NULL;
   }

   if (dpy->Driver) {
      drv = dpy->Driver;
      /* no re-matching? */
      if (!drv->API.Initialize(drv, dpy))
         drv = NULL;
      return drv;
   }

   while (i < _eglModules->Size) {
      _EGLModule *mod = (_EGLModule *) _eglModules->Elements[i];

      if (!_eglLoadModule(mod)) {
         /* remove invalid modules */
         _eglEraseArray(_eglModules, i, _eglFreeModule);
         continue;
      }

      if (mod->Driver->API.Initialize(mod->Driver, dpy)) {
         drv = mod->Driver;
         break;
      }
      else {
         i++;
      }
   }

   return drv;
}


/**
 * Match a display to a driver.  The display is initialized unless test_only is
 * true.  The matching is done by finding the first driver that can initialize
 * the display.
 */
_EGLDriver *
_eglMatchDriver(_EGLDisplay *dpy, EGLBoolean test_only)
{
   _EGLDriver *best_drv;

   assert(!dpy->Initialized);

   _eglLockMutex(&_eglModuleMutex);

   /* set options */
   dpy->Options.TestOnly = test_only;
   dpy->Options.UseFallback = EGL_FALSE;

   best_drv = _eglMatchAndInitialize(dpy);
   if (!best_drv) {
      dpy->Options.UseFallback = EGL_TRUE;
      best_drv = _eglMatchAndInitialize(dpy);
   }

   _eglUnlockMutex(&_eglModuleMutex);

   if (best_drv) {
      _eglLog(_EGL_DEBUG, "the best driver is %s%s",
            best_drv->Name, (test_only) ? " (test only) " : "");
      if (!test_only) {
         dpy->Driver = best_drv;
         dpy->Initialized = EGL_TRUE;
      }
   }

   return best_drv;
}


__eglMustCastToProperFunctionPointerType
_eglGetDriverProc(const char *procname)
{
   EGLint i;
   _EGLProc proc = NULL;

   if (!_eglModules) {
      /* load the driver for the default display */
      EGLDisplay egldpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
      _EGLDisplay *dpy = _eglLookupDisplay(egldpy);
      if (!dpy || !_eglMatchDriver(dpy, EGL_TRUE))
         return NULL;
   }

   for (i = 0; i < _eglModules->Size; i++) {
      _EGLModule *mod = (_EGLModule *) _eglModules->Elements[i];

      if (!mod->Driver)
         break;
      proc = mod->Driver->API.GetProcAddress(mod->Driver, procname);
      if (proc)
         break;
   }

   return proc;
}


/**
 * Unload all drivers.
 */
void
_eglUnloadDrivers(void)
{
   /* this is called at atexit time */
   if (_eglModules) {
      _eglDestroyArray(_eglModules, _eglFreeModule);
      _eglModules = NULL;
   }
}


/**
 * Invoke a callback function on each EGL search path.
 *
 * The first argument of the callback function is the name of the search path.
 * The second argument is the length of the name.
 */
void
_eglSearchPathForEach(EGLBoolean (*callback)(const char *, size_t, void *),
                      void *callback_data)
{
   const char *search_path = _eglGetSearchPath();
   _eglPreloadForEach(search_path, callback, callback_data);
}
