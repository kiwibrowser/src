/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

/* directly referenced from target Makefile, because of X dependencies */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <X11/Xlib-xcb.h>
#include <xcb/dri2.h>
#include <xf86drm.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "state_tracker/drm_driver.h"

#include "util/u_memory.h"
#include "util/u_hash.h"
#include "util/u_hash_table.h"
#include "util/u_inlines.h"

#include "vl/vl_compositor.h"
#include "vl/vl_winsys.h"

struct vl_dri_screen
{
   struct vl_screen base;
   xcb_connection_t *conn;
   xcb_drawable_t drawable;

   unsigned width, height;

   bool current_buffer;
   uint32_t buffer_names[2];
   struct u_rect dirty_areas[2];

   bool flushed;
   xcb_dri2_swap_buffers_cookie_t swap_cookie;
   xcb_dri2_wait_sbc_cookie_t wait_cookie;
   xcb_dri2_get_buffers_cookie_t buffers_cookie;

   int64_t last_ust, ns_frame, last_msc, next_msc, skew_msc;
};

static const unsigned int attachments[1] = { XCB_DRI2_ATTACHMENT_BUFFER_BACK_LEFT };

static void
vl_dri2_handle_stamps(struct vl_dri_screen* scrn,
                      uint32_t ust_hi, uint32_t ust_lo,
                      uint32_t msc_hi, uint32_t msc_lo)
{
   int64_t ust = ((((uint64_t)ust_hi) << 32) | ust_lo) * 1000;
   int64_t msc = (((uint64_t)msc_hi) << 32) | msc_lo;

   if (scrn->last_ust && scrn->last_msc && (ust > scrn->last_ust) && (msc > scrn->last_msc))
      scrn->ns_frame = (ust - scrn->last_ust) / (msc - scrn->last_msc);

   if (scrn->next_msc && (scrn->next_msc < msc))
      scrn->skew_msc++;

   scrn->last_ust = ust;
   scrn->last_msc = msc;
}

static xcb_dri2_get_buffers_reply_t*
vl_dri2_get_flush_reply(struct vl_dri_screen *scrn)
{
   xcb_dri2_wait_sbc_reply_t *wait_sbc_reply;

   assert(scrn);

   if (!scrn->flushed)
      return NULL;

   scrn->flushed = false;

   free(xcb_dri2_swap_buffers_reply(scrn->conn, scrn->swap_cookie, NULL));

   wait_sbc_reply = xcb_dri2_wait_sbc_reply(scrn->conn, scrn->wait_cookie, NULL);
   if (!wait_sbc_reply)
      return NULL;
   vl_dri2_handle_stamps(scrn, wait_sbc_reply->ust_hi, wait_sbc_reply->ust_lo,
                         wait_sbc_reply->msc_hi, wait_sbc_reply->msc_lo);
   free(wait_sbc_reply);

   return xcb_dri2_get_buffers_reply(scrn->conn, scrn->buffers_cookie, NULL);
}

static void
vl_dri2_flush_frontbuffer(struct pipe_screen *screen,
                          struct pipe_resource *resource,
                          unsigned level, unsigned layer,
                          void *context_private)
{
   struct vl_dri_screen *scrn = (struct vl_dri_screen*)context_private;
   uint32_t msc_hi, msc_lo;

   assert(screen);
   assert(resource);
   assert(context_private);

   free(vl_dri2_get_flush_reply(scrn));

   msc_hi = scrn->next_msc >> 32;
   msc_lo = scrn->next_msc & 0xFFFFFFFF;

   scrn->swap_cookie = xcb_dri2_swap_buffers_unchecked(scrn->conn, scrn->drawable, msc_hi, msc_lo, 0, 0, 0, 0);
   scrn->wait_cookie = xcb_dri2_wait_sbc_unchecked(scrn->conn, scrn->drawable, 0, 0);
   scrn->buffers_cookie = xcb_dri2_get_buffers_unchecked(scrn->conn, scrn->drawable, 1, 1, attachments);

   scrn->flushed = true;
   scrn->current_buffer = !scrn->current_buffer;
}

static void
vl_dri2_destroy_drawable(struct vl_dri_screen *scrn)
{
   xcb_void_cookie_t destroy_cookie;
   if (scrn->drawable) {
      free(vl_dri2_get_flush_reply(scrn));
      destroy_cookie = xcb_dri2_destroy_drawable_checked(scrn->conn, scrn->drawable);
      /* ignore any error here, since the drawable can be destroyed long ago */
      free(xcb_request_check(scrn->conn, destroy_cookie));
   }
}

static void
vl_dri2_set_drawable(struct vl_dri_screen *scrn, Drawable drawable)
{
   assert(scrn);
   assert(drawable);

   if (scrn->drawable == drawable)
      return;

   vl_dri2_destroy_drawable(scrn);

   xcb_dri2_create_drawable(scrn->conn, drawable);
   scrn->current_buffer = false;
   vl_compositor_reset_dirty_area(&scrn->dirty_areas[0]);
   vl_compositor_reset_dirty_area(&scrn->dirty_areas[1]);
   scrn->drawable = drawable;
}

struct pipe_resource*
vl_screen_texture_from_drawable(struct vl_screen *vscreen, Drawable drawable)
{
   struct vl_dri_screen *scrn = (struct vl_dri_screen*)vscreen;

   struct winsys_handle dri2_handle;
   struct pipe_resource template, *tex;

   xcb_dri2_get_buffers_reply_t *reply;
   xcb_dri2_dri2_buffer_t *buffers, *back_left;

   unsigned i;

   assert(scrn);

   vl_dri2_set_drawable(scrn, drawable);
   reply = vl_dri2_get_flush_reply(scrn);
   if (!reply) {
      xcb_dri2_get_buffers_cookie_t cookie;
      cookie = xcb_dri2_get_buffers_unchecked(scrn->conn, drawable, 1, 1, attachments);
      reply = xcb_dri2_get_buffers_reply(scrn->conn, cookie, NULL);
   }
   if (!reply)
      return NULL;

   buffers = xcb_dri2_get_buffers_buffers(reply);
   if (!buffers)  {
      free(reply);
      return NULL;
   }

   for (i = 0; i < reply->count; ++i) {
      if (buffers[i].attachment == XCB_DRI2_ATTACHMENT_BUFFER_BACK_LEFT) {
         back_left = &buffers[i];
         break;
      }
   }

   if (i == reply->count) {
      free(reply);
      return NULL;
   }

   if (reply->width != scrn->width || reply->height != scrn->height) {
      vl_compositor_reset_dirty_area(&scrn->dirty_areas[0]);
      vl_compositor_reset_dirty_area(&scrn->dirty_areas[1]);
      scrn->width = reply->width;
      scrn->height = reply->height;

   } else if (back_left->name != scrn->buffer_names[scrn->current_buffer]) {
      vl_compositor_reset_dirty_area(&scrn->dirty_areas[scrn->current_buffer]);
      scrn->buffer_names[scrn->current_buffer] = back_left->name;
   }

   memset(&dri2_handle, 0, sizeof(dri2_handle));
   dri2_handle.type = DRM_API_HANDLE_TYPE_SHARED;
   dri2_handle.handle = back_left->name;
   dri2_handle.stride = back_left->pitch;

   memset(&template, 0, sizeof(template));
   template.target = PIPE_TEXTURE_2D;
   template.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   template.last_level = 0;
   template.width0 = reply->width;
   template.height0 = reply->height;
   template.depth0 = 1;
   template.array_size = 1;
   template.usage = PIPE_USAGE_STATIC;
   template.bind = PIPE_BIND_RENDER_TARGET;
   template.flags = 0;

   tex = scrn->base.pscreen->resource_from_handle(scrn->base.pscreen, &template, &dri2_handle);
   free(reply);

   return tex;
}

struct u_rect *
vl_screen_get_dirty_area(struct vl_screen *vscreen)
{
   struct vl_dri_screen *scrn = (struct vl_dri_screen*)vscreen;
   assert(scrn);
   return &scrn->dirty_areas[scrn->current_buffer];
}

uint64_t
vl_screen_get_timestamp(struct vl_screen *vscreen, Drawable drawable)
{
   struct vl_dri_screen *scrn = (struct vl_dri_screen*)vscreen;
   xcb_dri2_get_msc_cookie_t cookie;
   xcb_dri2_get_msc_reply_t *reply;

   assert(scrn);

   vl_dri2_set_drawable(scrn, drawable);
   if (!scrn->last_ust) {
      cookie = xcb_dri2_get_msc_unchecked(scrn->conn, drawable);
      reply = xcb_dri2_get_msc_reply(scrn->conn, cookie, NULL);

      if (reply) {
         vl_dri2_handle_stamps(scrn, reply->ust_hi, reply->ust_lo,
                               reply->msc_hi, reply->msc_lo);
         free(reply);
      }
   }
   return scrn->last_ust;
}

void
vl_screen_set_next_timestamp(struct vl_screen *vscreen, uint64_t stamp)
{
   struct vl_dri_screen *scrn = (struct vl_dri_screen*)vscreen;
   assert(scrn);
   if (stamp && scrn->last_ust && scrn->ns_frame && scrn->last_msc)
      scrn->next_msc = ((int64_t)stamp - scrn->last_ust) / scrn->ns_frame + scrn->last_msc + scrn->skew_msc;
   else
      scrn->next_msc = 0;
}

void*
vl_screen_get_private(struct vl_screen *vscreen)
{
   return vscreen;
}

struct vl_screen*
vl_screen_create(Display *display, int screen)
{
   struct vl_dri_screen *scrn;
   const xcb_query_extension_reply_t *extension;
   xcb_dri2_query_version_cookie_t dri2_query_cookie;
   xcb_dri2_query_version_reply_t *dri2_query = NULL;
   xcb_dri2_connect_cookie_t connect_cookie;
   xcb_dri2_connect_reply_t *connect = NULL;
   xcb_dri2_authenticate_cookie_t authenticate_cookie;
   xcb_dri2_authenticate_reply_t *authenticate = NULL;
   xcb_screen_iterator_t s;
   xcb_generic_error_t *error = NULL;
   char *device_name;
   int fd, device_name_length;

   drm_magic_t magic;

   assert(display);

   scrn = CALLOC_STRUCT(vl_dri_screen);
   if (!scrn)
      return NULL;

   scrn->conn = XGetXCBConnection(display);
   if (!scrn->conn)
      goto free_screen;

   xcb_prefetch_extension_data(scrn->conn, &xcb_dri2_id);

   extension = xcb_get_extension_data(scrn->conn, &xcb_dri2_id);
   if (!(extension && extension->present))
      goto free_screen;

   dri2_query_cookie = xcb_dri2_query_version (scrn->conn, XCB_DRI2_MAJOR_VERSION, XCB_DRI2_MINOR_VERSION);
   dri2_query = xcb_dri2_query_version_reply (scrn->conn, dri2_query_cookie, &error);
   if (dri2_query == NULL || error != NULL || dri2_query->minor_version < 2)
      goto free_screen;

   s = xcb_setup_roots_iterator(xcb_get_setup(scrn->conn));
   connect_cookie = xcb_dri2_connect_unchecked(scrn->conn, s.data->root, XCB_DRI2_DRIVER_TYPE_DRI);
   connect = xcb_dri2_connect_reply(scrn->conn, connect_cookie, NULL);
   if (connect == NULL || connect->driver_name_length + connect->device_name_length == 0)
      goto free_screen;

   device_name_length = xcb_dri2_connect_device_name_length(connect);
   device_name = CALLOC(1, device_name_length);
   memcpy(device_name, xcb_dri2_connect_device_name(connect), device_name_length);
   device_name[device_name_length] = 0;
   fd = open(device_name, O_RDWR);
   free(device_name);

   if (fd < 0)
      goto free_screen;

   if (drmGetMagic(fd, &magic))
      goto free_screen;

   authenticate_cookie = xcb_dri2_authenticate_unchecked(scrn->conn, s.data->root, magic);
   authenticate = xcb_dri2_authenticate_reply(scrn->conn, authenticate_cookie, NULL);

   if (authenticate == NULL || !authenticate->authenticated)
      goto free_screen;

   scrn->base.pscreen = driver_descriptor.create_screen(fd);
   if (!scrn->base.pscreen)
      goto free_screen;

   scrn->base.pscreen->flush_frontbuffer = vl_dri2_flush_frontbuffer;
   vl_compositor_reset_dirty_area(&scrn->dirty_areas[0]);
   vl_compositor_reset_dirty_area(&scrn->dirty_areas[1]);

   free(dri2_query);
   free(connect);
   free(authenticate);

   return &scrn->base;

free_screen:
   FREE(scrn);

   free(dri2_query);
   free(connect);
   free(authenticate);
   free(error);

   return NULL;
}

void vl_screen_destroy(struct vl_screen *vscreen)
{
   struct vl_dri_screen *scrn = (struct vl_dri_screen*)vscreen;

   assert(vscreen);

   if (scrn->flushed) {
      free(xcb_dri2_swap_buffers_reply(scrn->conn, scrn->swap_cookie, NULL));
      free(xcb_dri2_wait_sbc_reply(scrn->conn, scrn->wait_cookie, NULL));
      free(xcb_dri2_get_buffers_reply(scrn->conn, scrn->buffers_cookie, NULL));
   }

   vl_dri2_destroy_drawable(scrn);
   scrn->base.pscreen->destroy(scrn->base.pscreen);
   FREE(scrn);
}
