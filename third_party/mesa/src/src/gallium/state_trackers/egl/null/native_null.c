/*
 * Mesa 3-D graphics library
 * Version:  7.12
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

/**
 * For NULL window system,
 *
 *  - the only valid native display is EGL_DEFAULT_DISPLAY
 *  - there is no window or pixmap
 */

#include "util/u_memory.h"
#include "null/null_sw_winsys.h"
#include "common/native.h"

struct null_display {
   struct native_display base;

   const struct native_event_handler *event_handler;

   struct native_config *configs;
   int num_configs;
};

static INLINE struct null_display *
null_display(const struct native_display *ndpy)
{
   return (struct null_display *) ndpy;
}

static const struct native_config **
null_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct null_display *null = null_display(ndpy);
   const struct native_config **configs;
   int i;

   configs = MALLOC(sizeof(*configs) * null->num_configs);
   if (configs) {
      for (i = 0; i < null->num_configs; i++)
         configs[i] = &null->configs[i];
      if (num_configs)
         *num_configs = null->num_configs;
   }

   return configs;
}

static int
null_display_get_param(struct native_display *ndpy,
                      enum native_param_type param)
{
   return 0;
}

static void
null_display_destroy(struct native_display *ndpy)
{
   struct null_display *null = null_display(ndpy);

   FREE(null->configs);
   ndpy_uninit(&null->base);
   FREE(null);
}

static boolean
null_display_init_config(struct native_display *ndpy)
{
   const enum pipe_format color_formats[] =  {
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_B8G8R8X8_UNORM,
      PIPE_FORMAT_B5G6R5_UNORM,
      PIPE_FORMAT_NONE
   };
   struct null_display *null = null_display(ndpy);
   int i;

   null->configs = CALLOC(Elements(color_formats) - 1, sizeof(*null->configs));
   if (!null->configs)
      return FALSE;

   /* add configs */
   for (i = 0; color_formats[i] != PIPE_FORMAT_NONE; i++) {
      if (null->base.screen->is_format_supported(null->base.screen,
               color_formats[i], PIPE_TEXTURE_2D, 0,
               PIPE_BIND_RENDER_TARGET)) {
         struct native_config *nconf = &null->configs[null->num_configs];

         nconf->color_format = color_formats[i];
         nconf->buffer_mask = 1 << NATIVE_ATTACHMENT_BACK_LEFT;
         null->num_configs++;
      }
   }

   return TRUE;
}

static boolean
null_display_init_screen(struct native_display *ndpy)
{
   struct null_display *null = null_display(ndpy);
   struct sw_winsys *ws;

   ws = null_sw_create();
   if (!ws)
      return FALSE;

   null->base.screen = null->event_handler->new_sw_screen(&null->base, ws);
   if (!null->base.screen) {
      if (ws->destroy)
         ws->destroy(ws);
      return FALSE;
   }

   if (!null_display_init_config(&null->base)) {
      ndpy_uninit(&null->base);
      return FALSE;
   }

   return TRUE;
}

static struct native_display *
null_display_create(const struct native_event_handler *event_handler)
{
   struct null_display *null;

   null = CALLOC_STRUCT(null_display);
   if (!null)
      return NULL;

   null->event_handler = event_handler;

   null->base.init_screen = null_display_init_screen;
   null->base.destroy = null_display_destroy;
   null->base.get_param = null_display_get_param;
   null->base.get_configs = null_display_get_configs;

   return &null->base;
}

static const struct native_event_handler *null_event_handler;

static struct native_display *
native_create_display(void *dpy, boolean use_sw)
{
   struct native_display *ndpy = NULL;

   /* the only valid display is NULL */
   if (!dpy)
      ndpy = null_display_create(null_event_handler);

   return ndpy;
}

static const struct native_platform null_platform = {
   "NULL", /* name */
   native_create_display
};

const struct native_platform *
native_get_null_platform(const struct native_event_handler *event_handler)
{
   null_event_handler = event_handler;
   return &null_platform;
}
