/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "util/u_simple_list.h"

#include "rbug_public.h"
#include "rbug_screen.h"
#include "rbug_context.h"
#include "rbug_objects.h"

DEBUG_GET_ONCE_BOOL_OPTION(rbug, "GALLIUM_RBUG", FALSE)

static void
rbug_screen_destroy(struct pipe_screen *_screen)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;

   screen->destroy(screen);

   FREE(rb_screen);
}

static const char *
rbug_screen_get_name(struct pipe_screen *_screen)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;

   return screen->get_name(screen);
}

static const char *
rbug_screen_get_vendor(struct pipe_screen *_screen)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;

   return screen->get_vendor(screen);
}

static int
rbug_screen_get_param(struct pipe_screen *_screen,
                      enum pipe_cap param)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;

   return screen->get_param(screen,
                            param);
}

static int
rbug_screen_get_shader_param(struct pipe_screen *_screen,
                      unsigned shader, enum pipe_shader_cap param)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;

   return screen->get_shader_param(screen, shader,
                            param);
}

static float
rbug_screen_get_paramf(struct pipe_screen *_screen,
                       enum pipe_capf param)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;

   return screen->get_paramf(screen,
                             param);
}

static boolean
rbug_screen_is_format_supported(struct pipe_screen *_screen,
                                enum pipe_format format,
                                enum pipe_texture_target target,
                                unsigned sample_count,
                                unsigned tex_usage)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;

   return screen->is_format_supported(screen,
                                      format,
                                      target,
                                      sample_count,
                                      tex_usage);
}

static struct pipe_context *
rbug_screen_context_create(struct pipe_screen *_screen,
                           void *priv)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;
   struct pipe_context *result;

   result = screen->context_create(screen, priv);
   if (result)
      return rbug_context_create(_screen, result);
   return NULL;
}

static struct pipe_resource *
rbug_screen_resource_create(struct pipe_screen *_screen,
                            const struct pipe_resource *templat)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;
   struct pipe_resource *result;

   result = screen->resource_create(screen,
                                    templat);

   if (result)
      return rbug_resource_create(rb_screen, result);
   return NULL;
}

static struct pipe_resource *
rbug_screen_resource_from_handle(struct pipe_screen *_screen,
                                 const struct pipe_resource *templ,
                                 struct winsys_handle *handle)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;
   struct pipe_resource *result;

   result = screen->resource_from_handle(screen, templ, handle);

   result = rbug_resource_create(rbug_screen(_screen), result);

   return result;
}

static boolean
rbug_screen_resource_get_handle(struct pipe_screen *_screen,
                                struct pipe_resource *_resource,
                                struct winsys_handle *handle)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct rbug_resource *rb_resource = rbug_resource(_resource);
   struct pipe_screen *screen = rb_screen->screen;
   struct pipe_resource *resource = rb_resource->resource;

   return screen->resource_get_handle(screen, resource, handle);
}



static void
rbug_screen_resource_destroy(struct pipe_screen *screen,
                             struct pipe_resource *_resource)
{
   rbug_resource_destroy(rbug_resource(_resource));
}

static void
rbug_screen_flush_frontbuffer(struct pipe_screen *_screen,
                              struct pipe_resource *_resource,
                              unsigned level, unsigned layer,
                              void *context_private)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct rbug_resource *rb_resource = rbug_resource(_resource);
   struct pipe_screen *screen = rb_screen->screen;
   struct pipe_resource *resource = rb_resource->resource;

   screen->flush_frontbuffer(screen,
                             resource,
                             level, layer,
                             context_private);
}

static void
rbug_screen_fence_reference(struct pipe_screen *_screen,
                            struct pipe_fence_handle **ptr,
                            struct pipe_fence_handle *fence)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;

   screen->fence_reference(screen,
                           ptr,
                           fence);
}

static boolean
rbug_screen_fence_signalled(struct pipe_screen *_screen,
                            struct pipe_fence_handle *fence)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;

   return screen->fence_signalled(screen,
                                  fence);
}

static boolean
rbug_screen_fence_finish(struct pipe_screen *_screen,
                         struct pipe_fence_handle *fence,
                         uint64_t timeout)
{
   struct rbug_screen *rb_screen = rbug_screen(_screen);
   struct pipe_screen *screen = rb_screen->screen;

   return screen->fence_finish(screen,
                               fence,
                               timeout);
}

boolean
rbug_enabled()
{
   return debug_get_option_rbug();
}

struct pipe_screen *
rbug_screen_create(struct pipe_screen *screen)
{
   struct rbug_screen *rb_screen;

   if (!debug_get_option_rbug())
      return screen;

   rb_screen = CALLOC_STRUCT(rbug_screen);
   if (!rb_screen)
      return screen;

   pipe_mutex_init(rb_screen->list_mutex);
   make_empty_list(&rb_screen->contexts);
   make_empty_list(&rb_screen->resources);
   make_empty_list(&rb_screen->surfaces);
   make_empty_list(&rb_screen->transfers);

   rb_screen->base.destroy = rbug_screen_destroy;
   rb_screen->base.get_name = rbug_screen_get_name;
   rb_screen->base.get_vendor = rbug_screen_get_vendor;
   rb_screen->base.get_param = rbug_screen_get_param;
   rb_screen->base.get_shader_param = rbug_screen_get_shader_param;
   rb_screen->base.get_paramf = rbug_screen_get_paramf;
   rb_screen->base.is_format_supported = rbug_screen_is_format_supported;
   rb_screen->base.context_create = rbug_screen_context_create;
   rb_screen->base.resource_create = rbug_screen_resource_create;
   rb_screen->base.resource_from_handle = rbug_screen_resource_from_handle;
   rb_screen->base.resource_get_handle = rbug_screen_resource_get_handle;
   rb_screen->base.resource_destroy = rbug_screen_resource_destroy;
   rb_screen->base.flush_frontbuffer = rbug_screen_flush_frontbuffer;
   rb_screen->base.fence_reference = rbug_screen_fence_reference;
   rb_screen->base.fence_signalled = rbug_screen_fence_signalled;
   rb_screen->base.fence_finish = rbug_screen_fence_finish;

   rb_screen->screen = screen;

   rb_screen->private_context = screen->context_create(screen, NULL);
   if (!rb_screen->private_context)
      goto err_free;

   rb_screen->rbug = rbug_start(rb_screen);

   if (!rb_screen->rbug)
      goto err_context;

   return &rb_screen->base;

err_context:
   rb_screen->private_context->destroy(rb_screen->private_context);
err_free:
   FREE(rb_screen);
   return screen;
}
