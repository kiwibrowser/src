/*
 * Copyright © 2011 Intel Corporation
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
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#include "gbm_gallium_drmint.h"

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "pipe-loader/pipe_loader.h"

static const char *
get_library_search_path(void)
{
   const char *search_path = NULL;

   /* don't allow setuid apps to use GBM_BACKENDS_PATH */
   if (geteuid() == getuid())
      search_path = getenv("GBM_BACKENDS_PATH");
   if (search_path == NULL)
      search_path = PIPE_SEARCH_DIR;

   return search_path;
}

int
gallium_screen_create(struct gbm_gallium_drm_device *gdrm)
{
   struct pipe_loader_device *dev;
#ifdef HAVE_PIPE_LOADER_DRM
   int ret;

   ret = pipe_loader_drm_probe_fd(&dev, gdrm->base.base.fd);
   if (!ret)
      return -1;
#endif /* HAVE_PIPE_LOADER_DRM */

   gdrm->screen = pipe_loader_create_screen(dev, get_library_search_path());
   if (gdrm->screen == NULL) {
      debug_printf("failed to load driver: %s\n", gdrm->base.driver_name);
      pipe_loader_release(&dev, 1);
      return -1;
   };

   gdrm->driver = dev;
   gdrm->base.driver_name = strdup(dev->driver_name);
   return 0;
}

void
gallium_screen_destroy(struct gbm_gallium_drm_device *gdrm)
{
   FREE(gdrm->base.driver_name);
   gdrm->screen->destroy(gdrm->screen);
   pipe_loader_release((struct pipe_loader_device **)&gdrm->driver, 1);
}

GBM_EXPORT struct gbm_backend gbm_backend = {
   .backend_name = "gallium_drm",
   .create_device = gbm_gallium_drm_device_create,
};
