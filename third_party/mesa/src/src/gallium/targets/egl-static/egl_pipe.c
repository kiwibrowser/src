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
#include "target-helpers/inline_debug_helper.h"
#include "target-helpers/inline_sw_helper.h"
#include "egl_pipe.h"

/* for i915 */
#include "i915/drm/i915_drm_public.h"
#include "i915/i915_public.h"
#include "target-helpers/inline_wrapper_sw_helper.h"
/* for nouveau */
#include "nouveau/drm/nouveau_drm_public.h"
/* for r300 */
#include "radeon/drm/radeon_drm_public.h"
#include "r300/r300_public.h"
/* for r600 */
#include "r600/r600_public.h"
/* for radeonsi */
#include "radeonsi/radeonsi_public.h"
/* for vmwgfx */
#include "svga/drm/svga_drm_public.h"
#include "svga/svga_public.h"

static struct pipe_screen *
pipe_i915_create_screen(int fd)
{
#if _EGL_PIPE_I915
   struct i915_winsys *iws;
   struct pipe_screen *screen;

   iws = i915_drm_winsys_create(fd);
   if (!iws)
      return NULL;

   screen = i915_screen_create(iws);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
#else
   return NULL;
#endif
}

static struct pipe_screen *
pipe_nouveau_create_screen(int fd)
{
#if _EGL_PIPE_NOUVEAU
   struct pipe_screen *screen;

   screen = nouveau_drm_screen_create(fd);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
#else
   return NULL;
#endif
}

static struct pipe_screen *
pipe_r300_create_screen(int fd)
{
#if _EGL_PIPE_R300
   struct radeon_winsys *sws;
   struct pipe_screen *screen;

   sws = radeon_drm_winsys_create(fd);
   if (!sws)
      return NULL;

   screen = r300_screen_create(sws);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
#else
   return NULL;
#endif
}

static struct pipe_screen *
pipe_r600_create_screen(int fd)
{
#if _EGL_PIPE_R600
   struct radeon_winsys *rw;
   struct pipe_screen *screen;

   rw = radeon_drm_winsys_create(fd);
   if (!rw)
      return NULL;

   screen = r600_screen_create(rw);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
#else
   return NULL;
#endif
}

static struct pipe_screen *
pipe_radeonsi_create_screen(int fd)
{
#if _EGL_PIPE_RADEONSI
   struct radeon_winsys *rw;
   struct pipe_screen *screen;

   rw = radeon_drm_winsys_create(fd);
   if (!rw)
      return NULL;

   screen = radeonsi_screen_create(rw);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
#else
   return NULL;
#endif
}

static struct pipe_screen *
pipe_vmwgfx_create_screen(int fd)
{
#if _EGL_PIPE_VMWGFX
   struct svga_winsys_screen *sws;
   struct pipe_screen *screen;

   sws = svga_drm_winsys_screen_create(fd);
   if (!sws)
      return NULL;

   screen = svga_screen_create(sws);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
#else
   return NULL;
#endif
}

struct pipe_screen *
egl_pipe_create_drm_screen(const char *name, int fd)
{
   if (strcmp(name, "i915") == 0)
      return pipe_i915_create_screen(fd);
   else if (strcmp(name, "nouveau") == 0)
      return pipe_nouveau_create_screen(fd);
   else if (strcmp(name, "r300") == 0)
      return pipe_r300_create_screen(fd);
   else if (strcmp(name, "r600") == 0)
      return pipe_r600_create_screen(fd);
   else if (strcmp(name, "radeonsi") == 0)
      return pipe_radeonsi_create_screen(fd);
   else if (strcmp(name, "vmwgfx") == 0)
      return pipe_vmwgfx_create_screen(fd);
   else
      return NULL;
}

struct pipe_screen *
egl_pipe_create_swrast_screen(struct sw_winsys *ws)
{
   struct pipe_screen *screen;

   screen = sw_screen_create(ws);
   if (screen)
      screen = debug_screen_wrap(screen);

   return screen;
}
