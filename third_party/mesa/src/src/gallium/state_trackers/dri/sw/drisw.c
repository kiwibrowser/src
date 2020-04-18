/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
 * All Rights Reserved.
 * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
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

/* TODO:
 *
 * xshm / EGLImage:
 *
 * Allow the loaders to use the XSHM extension. It probably requires callbacks
 * for createImage/destroyImage similar to DRI2 getBuffers.
 */

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "pipe/p_context.h"
#include "state_tracker/drisw_api.h"
#include "state_tracker/st_context.h"

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"

DEBUG_GET_ONCE_BOOL_OPTION(swrast_no_present, "SWRAST_NO_PRESENT", FALSE);
static boolean swrast_no_present = FALSE;

static INLINE void
get_drawable_info(__DRIdrawable *dPriv, int *x, int *y, int *w, int *h)
{
   __DRIscreen *sPriv = dPriv->driScreenPriv;
   const __DRIswrastLoaderExtension *loader = sPriv->swrast_loader;

   loader->getDrawableInfo(dPriv,
                           x, y, w, h,
                           dPriv->loaderPrivate);
}

static INLINE void
put_image(__DRIdrawable *dPriv, void *data, unsigned width, unsigned height)
{
   __DRIscreen *sPriv = dPriv->driScreenPriv;
   const __DRIswrastLoaderExtension *loader = sPriv->swrast_loader;

   loader->putImage(dPriv, __DRI_SWRAST_IMAGE_OP_SWAP,
                    0, 0, width, height,
                    data, dPriv->loaderPrivate);
}

static INLINE void
get_image(__DRIdrawable *dPriv, int x, int y, int width, int height, void *data)
{
   __DRIscreen *sPriv = dPriv->driScreenPriv;
   const __DRIswrastLoaderExtension *loader = sPriv->swrast_loader;

   loader->getImage(dPriv,
                    x, y, width, height,
                    data, dPriv->loaderPrivate);
}

static void
drisw_update_drawable_info(struct dri_drawable *drawable)
{
   __DRIdrawable *dPriv = drawable->dPriv;
   int x, y;

   get_drawable_info(dPriv, &x, &y, &dPriv->w, &dPriv->h);
}

static void
drisw_put_image(struct dri_drawable *drawable,
                void *data, unsigned width, unsigned height)
{
   __DRIdrawable *dPriv = drawable->dPriv;

   put_image(dPriv, data, width, height);
}

static INLINE void
drisw_present_texture(__DRIdrawable *dPriv,
                      struct pipe_resource *ptex)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct dri_screen *screen = dri_screen(drawable->sPriv);

   if (swrast_no_present)
      return;

   screen->base.screen->flush_frontbuffer(screen->base.screen, ptex, 0, 0, drawable);
}

static INLINE void
drisw_invalidate_drawable(__DRIdrawable *dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);

   drawable->texture_stamp = dPriv->lastStamp - 1;

   p_atomic_inc(&drawable->base.stamp);
}

static INLINE void
drisw_copy_to_front(__DRIdrawable * dPriv,
                    struct pipe_resource *ptex)
{
   drisw_present_texture(dPriv, ptex);

   drisw_invalidate_drawable(dPriv);
}

/*
 * Backend functions for st_framebuffer interface and swap_buffers.
 */

static void
drisw_swap_buffers(__DRIdrawable *dPriv)
{
   struct dri_context *ctx = dri_get_current(dPriv->driScreenPriv);
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_resource *ptex;

   if (!ctx)
      return;

   ptex = drawable->textures[ST_ATTACHMENT_BACK_LEFT];

   if (ptex) {
      if (ctx->pp && drawable->textures[ST_ATTACHMENT_DEPTH_STENCIL])
         pp_run(ctx->pp, ptex, ptex, drawable->textures[ST_ATTACHMENT_DEPTH_STENCIL]);

      ctx->st->flush(ctx->st, ST_FLUSH_FRONT, NULL);

      drisw_copy_to_front(dPriv, ptex);
   }
}

static void
drisw_flush_frontbuffer(struct dri_drawable *drawable,
                        enum st_attachment_type statt)
{
   struct dri_context *ctx = dri_get_current(drawable->sPriv);
   struct pipe_resource *ptex;

   if (!ctx)
      return;

   ptex = drawable->textures[statt];

   if (ptex) {
      drisw_copy_to_front(ctx->dPriv, ptex);
   }
}

/**
 * Allocate framebuffer attachments.
 *
 * During fixed-size operation, the function keeps allocating new attachments
 * as they are requested. Unused attachments are not removed, not until the
 * framebuffer is resized or destroyed.
 */
static void
drisw_allocate_textures(struct dri_drawable *drawable,
                        const enum st_attachment_type *statts,
                        unsigned count)
{
   struct dri_screen *screen = dri_screen(drawable->sPriv);
   struct pipe_resource templ;
   unsigned width, height;
   boolean resized;
   unsigned i;

   width  = drawable->dPriv->w;
   height = drawable->dPriv->h;

   resized = (drawable->old_w != width ||
              drawable->old_h != height);

   /* remove outdated textures */
   if (resized) {
      for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
         pipe_resource_reference(&drawable->textures[i], NULL);
   }

   memset(&templ, 0, sizeof(templ));
   templ.target = screen->target;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.last_level = 0;

   for (i = 0; i < count; i++) {
      enum pipe_format format;
      unsigned bind;

      /* the texture already exists or not requested */
      if (drawable->textures[statts[i]])
         continue;

      dri_drawable_get_format(drawable, statts[i], &format, &bind);

      /* if we don't do any present, no need for display targets */
      if (statts[i] != ST_ATTACHMENT_DEPTH_STENCIL && !swrast_no_present)
         bind |= PIPE_BIND_DISPLAY_TARGET;

      if (format == PIPE_FORMAT_NONE)
         continue;

      templ.format = format;
      templ.bind = bind;

      drawable->textures[statts[i]] =
         screen->base.screen->resource_create(screen->base.screen, &templ);
   }

   drawable->old_w = width;
   drawable->old_h = height;
}

static void
drisw_update_tex_buffer(struct dri_drawable *drawable,
                        struct dri_context *ctx,
                        struct pipe_resource *res)
{
   __DRIdrawable *dPriv = drawable->dPriv;

   struct st_context *st_ctx = (struct st_context *)ctx->st;
   struct pipe_context *pipe = st_ctx->pipe;
   struct pipe_transfer *transfer;
   char *map;
   int x, y, w, h;
   int ximage_stride, line;
   int cpp = util_format_get_blocksize(res->format);

   get_drawable_info(dPriv, &x, &y, &w, &h);

   transfer = pipe_get_transfer(pipe, res,
                                0, 0, // level, layer,
                                PIPE_TRANSFER_WRITE,
                                x, y, w, h);
   map = pipe_transfer_map(pipe, transfer);

   /* Copy the Drawable content to the mapped texture buffer */
   get_image(dPriv, x, y, w, h, map);

   /* The pipe transfer has a pitch rounded up to the nearest 64 pixels. */
   ximage_stride = w * cpp;
   for (line = h-1; line; --line) {
      memmove(&map[line * transfer->stride],
              &map[line * ximage_stride],
              ximage_stride);
   }

   pipe_transfer_unmap(pipe, transfer);
   pipe_transfer_destroy(pipe, transfer);
}

/*
 * Backend function for init_screen.
 */

static const __DRIextension *drisw_screen_extensions[] = {
   &driTexBufferExtension.base,
   NULL
};

static struct drisw_loader_funcs drisw_lf = {
   .put_image = drisw_put_image
};

static const __DRIconfig **
drisw_init_screen(__DRIscreen * sPriv)
{
   const __DRIconfig **configs;
   struct dri_screen *screen;
   struct pipe_screen *pscreen;

   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      return NULL;

   screen->sPriv = sPriv;
   screen->fd = -1;

   swrast_no_present = debug_get_option_swrast_no_present();

   sPriv->driverPrivate = (void *)screen;
   sPriv->extensions = drisw_screen_extensions;

   pscreen = drisw_create_screen(&drisw_lf);
   /* dri_init_screen_helper checks pscreen for us */

   configs = dri_init_screen_helper(screen, pscreen, 32);
   if (!configs)
      goto fail;

   return configs;
fail:
   dri_destroy_screen_helper(screen);
   FREE(screen);
   return NULL;
}

static boolean
drisw_create_buffer(__DRIscreen * sPriv,
                    __DRIdrawable * dPriv,
                    const struct gl_config * visual, boolean isPixmap)
{
   struct dri_drawable *drawable = NULL;

   if (!dri_create_buffer(sPriv, dPriv, visual, isPixmap))
      return FALSE;

   drawable = dPriv->driverPrivate;

   drawable->allocate_textures = drisw_allocate_textures;
   drawable->update_drawable_info = drisw_update_drawable_info;
   drawable->flush_frontbuffer = drisw_flush_frontbuffer;
   drawable->update_tex_buffer = drisw_update_tex_buffer;

   return TRUE;
}

/**
 * DRI driver virtual function table.
 *
 * DRI versions differ in their implementation of init_screen and swap_buffers.
 */
const struct __DriverAPIRec driDriverAPI = {
   .InitScreen = drisw_init_screen,
   .DestroyScreen = dri_destroy_screen,
   .CreateContext = dri_create_context,
   .DestroyContext = dri_destroy_context,
   .CreateBuffer = drisw_create_buffer,
   .DestroyBuffer = dri_destroy_buffer,
   .SwapBuffers = drisw_swap_buffers,
   .MakeCurrent = dri_make_current,
   .UnbindContext = dri_unbind_context,
};

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driSWRastExtension.base,
    NULL
};

/* vim: set sw=3 ts=8 sts=3 expandtab: */
