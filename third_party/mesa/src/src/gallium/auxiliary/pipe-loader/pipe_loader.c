/**************************************************************************
 *
 * Copyright 2012 Francisco Jerez
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe_loader_priv.h"

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_string.h"
#include "util/u_dl.h"

#define MODULE_PREFIX "pipe_"

static int (*backends[])(struct pipe_loader_device **, int) = {
#ifdef HAVE_PIPE_LOADER_DRM
   &pipe_loader_drm_probe,
#endif
   &pipe_loader_sw_probe
};

int
pipe_loader_probe(struct pipe_loader_device **devs, int ndev)
{
   int i, n = 0;

   for (i = 0; i < Elements(backends); i++)
      n += backends[i](&devs[n], MAX2(0, ndev - n));

   return n;
}

void
pipe_loader_release(struct pipe_loader_device **devs, int ndev)
{
   int i;

   for (i = 0; i < ndev; i++)
      devs[i]->ops->release(&devs[i]);
}

struct pipe_screen *
pipe_loader_create_screen(struct pipe_loader_device *dev,
                          const char *library_paths)
{
   return dev->ops->create_screen(dev, library_paths);
}

struct util_dl_library *
pipe_loader_find_module(struct pipe_loader_device *dev,
                        const char *library_paths)
{
   struct util_dl_library *lib;
   const char *next;
   char path[PATH_MAX];
   int len, ret;

   for (next = library_paths; *next; library_paths = next + 1) {
      next = util_strchrnul(library_paths, ':');
      len = next - library_paths;

      if (len)
         ret = util_snprintf(path, sizeof(path), "%.*s/%s%s%s",
                             len, library_paths,
                             MODULE_PREFIX, dev->driver_name, UTIL_DL_EXT);
      else
         ret = util_snprintf(path, sizeof(path), "%s%s%s",
                             MODULE_PREFIX, dev->driver_name, UTIL_DL_EXT);

      if (ret > 0 && ret < sizeof(path)) {
         lib = util_dl_open(path);
         if (lib) {
            debug_printf("loaded %s\n", path);
            return lib;
         }
      }
   }

   return NULL;
}
