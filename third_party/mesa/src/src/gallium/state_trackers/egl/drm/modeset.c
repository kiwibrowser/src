/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
 * Copyright (C) 2011 VMware Inc. All rights reserved.
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
 *    Thomas Hellstrom <thellstrom@vmware.com>
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "egllog.h"

#include "native_drm.h"

static boolean
drm_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                     unsigned int *seq_num, struct pipe_resource **textures,
                     int *width, int *height)
{
   struct drm_surface *drmsurf = drm_surface(nsurf);

   if (!resource_surface_add_resources(drmsurf->rsurf, attachment_mask))
      return FALSE;
   if (textures)
      resource_surface_get_resources(drmsurf->rsurf, textures, attachment_mask);

   if (seq_num)
      *seq_num = drmsurf->sequence_number;
   if (width)
      *width = drmsurf->width;
   if (height)
      *height = drmsurf->height;

   return TRUE;
}

/**
 * Add textures as DRM framebuffers.
 */
static boolean
drm_surface_init_framebuffers(struct native_surface *nsurf, boolean need_back)
{
   struct drm_surface *drmsurf = drm_surface(nsurf);
   struct drm_display *drmdpy = drmsurf->drmdpy;
   int num_framebuffers = (need_back) ? 2 : 1;
   int i, err;

   for (i = 0; i < num_framebuffers; i++) {
      struct drm_framebuffer *fb;
      enum native_attachment natt;
      struct winsys_handle whandle;
      uint block_bits;

      if (i == 0) {
         fb = &drmsurf->front_fb;
         natt = NATIVE_ATTACHMENT_FRONT_LEFT;
      }
      else {
         fb = &drmsurf->back_fb;
         natt = NATIVE_ATTACHMENT_BACK_LEFT;
      }

      if (!fb->texture) {
         /* make sure the texture has been allocated */
         resource_surface_add_resources(drmsurf->rsurf, 1 << natt);
         fb->texture =
            resource_surface_get_single_resource(drmsurf->rsurf, natt);
         if (!fb->texture)
            return FALSE;
      }

      /* already initialized */
      if (fb->buffer_id)
         continue;

      /* TODO detect the real value */
      fb->is_passive = TRUE;

      memset(&whandle, 0, sizeof(whandle));
      whandle.type = DRM_API_HANDLE_TYPE_KMS;

      if (!drmdpy->base.screen->resource_get_handle(drmdpy->base.screen,
               fb->texture, &whandle))
         return FALSE;

      block_bits = util_format_get_blocksizebits(drmsurf->color_format);
      err = drmModeAddFB(drmdpy->fd, drmsurf->width, drmsurf->height,
            block_bits, block_bits, whandle.stride, whandle.handle,
            &fb->buffer_id);
      if (err) {
         fb->buffer_id = 0;
         return FALSE;
      }
   }

   return TRUE;
}

static boolean
drm_surface_flush_frontbuffer(struct native_surface *nsurf)
{
#ifdef DRM_MODE_FEATURE_DIRTYFB
   struct drm_surface *drmsurf = drm_surface(nsurf);
   struct drm_display *drmdpy = drmsurf->drmdpy;

   if (drmsurf->front_fb.is_passive)
      drmModeDirtyFB(drmdpy->fd, drmsurf->front_fb.buffer_id, NULL, 0);
#endif

   return TRUE;
}

static boolean
drm_surface_copy_swap(struct native_surface *nsurf)
{
   struct drm_surface *drmsurf = drm_surface(nsurf);
   struct drm_display *drmdpy = drmsurf->drmdpy;

   (void) resource_surface_throttle(drmsurf->rsurf);
   if (!resource_surface_copy_swap(drmsurf->rsurf, &drmdpy->base))
      return FALSE;

   (void) resource_surface_flush(drmsurf->rsurf, &drmdpy->base);
   if (!drm_surface_flush_frontbuffer(nsurf))
      return FALSE;

   drmsurf->sequence_number++;

   return TRUE;
}

static boolean
drm_surface_swap_buffers(struct native_surface *nsurf)
{
   struct drm_surface *drmsurf = drm_surface(nsurf);
   struct drm_crtc *drmcrtc = &drmsurf->current_crtc;
   struct drm_display *drmdpy = drmsurf->drmdpy;
   struct drm_framebuffer tmp_fb;
   int err;

   if (!drmsurf->have_pageflip)
      return drm_surface_copy_swap(nsurf);

   if (!drmsurf->back_fb.buffer_id) {
      if (!drm_surface_init_framebuffers(&drmsurf->base, TRUE))
         return FALSE;
   }

   if (drmsurf->is_shown && drmcrtc->crtc) {
      err = drmModePageFlip(drmdpy->fd, drmcrtc->crtc->crtc_id,
			    drmsurf->back_fb.buffer_id, 0, NULL);
      if (err) {
	 drmsurf->have_pageflip = FALSE;
         return drm_surface_copy_swap(nsurf);
      }
   }

   /* swap the buffers */
   tmp_fb = drmsurf->front_fb;
   drmsurf->front_fb = drmsurf->back_fb;
   drmsurf->back_fb = tmp_fb;

   resource_surface_swap_buffers(drmsurf->rsurf,
         NATIVE_ATTACHMENT_FRONT_LEFT, NATIVE_ATTACHMENT_BACK_LEFT, FALSE);
   /* the front/back textures are swapped */
   drmsurf->sequence_number++;
   drmdpy->event_handler->invalid_surface(&drmdpy->base,
         &drmsurf->base, drmsurf->sequence_number);

   return TRUE;
}

static boolean
drm_surface_present(struct native_surface *nsurf,
                    const struct native_present_control *ctrl)
{
   boolean ret;

   if (ctrl->swap_interval)
      return FALSE;

   switch (ctrl->natt) {
   case NATIVE_ATTACHMENT_FRONT_LEFT:
      ret = drm_surface_flush_frontbuffer(nsurf);
      break;
   case NATIVE_ATTACHMENT_BACK_LEFT:
      if (ctrl->preserve)
	 ret = drm_surface_copy_swap(nsurf);
      else
	 ret = drm_surface_swap_buffers(nsurf);
      break;
   default:
      ret = FALSE;
      break;
   }

   return ret;
}

static void
drm_surface_wait(struct native_surface *nsurf)
{
   struct drm_surface *drmsurf = drm_surface(nsurf);

   resource_surface_wait(drmsurf->rsurf);
}

static void
drm_surface_destroy(struct native_surface *nsurf)
{
   struct drm_surface *drmsurf = drm_surface(nsurf);

   resource_surface_wait(drmsurf->rsurf);
   if (drmsurf->current_crtc.crtc)
         drmModeFreeCrtc(drmsurf->current_crtc.crtc);

   if (drmsurf->front_fb.buffer_id)
      drmModeRmFB(drmsurf->drmdpy->fd, drmsurf->front_fb.buffer_id);
   pipe_resource_reference(&drmsurf->front_fb.texture, NULL);

   if (drmsurf->back_fb.buffer_id)
      drmModeRmFB(drmsurf->drmdpy->fd, drmsurf->back_fb.buffer_id);
   pipe_resource_reference(&drmsurf->back_fb.texture, NULL);

   resource_surface_destroy(drmsurf->rsurf);
   FREE(drmsurf);
}

static struct drm_surface *
drm_display_create_surface(struct native_display *ndpy,
                           const struct native_config *nconf,
                           uint width, uint height)
{
   struct drm_display *drmdpy = drm_display(ndpy);
   struct drm_config *drmconf = drm_config(nconf);
   struct drm_surface *drmsurf;

   drmsurf = CALLOC_STRUCT(drm_surface);
   if (!drmsurf)
      return NULL;

   drmsurf->drmdpy = drmdpy;
   drmsurf->color_format = drmconf->base.color_format;
   drmsurf->width = width;
   drmsurf->height = height;
   drmsurf->have_pageflip = TRUE;

   drmsurf->rsurf = resource_surface_create(drmdpy->base.screen,
         drmsurf->color_format,
         PIPE_BIND_RENDER_TARGET |
         PIPE_BIND_SAMPLER_VIEW |
         PIPE_BIND_DISPLAY_TARGET |
         PIPE_BIND_SCANOUT);
   if (!drmsurf->rsurf) {
      FREE(drmsurf);
      return NULL;
   }

   resource_surface_set_size(drmsurf->rsurf, drmsurf->width, drmsurf->height);

   drmsurf->base.destroy = drm_surface_destroy;
   drmsurf->base.present = drm_surface_present;
   drmsurf->base.validate = drm_surface_validate;
   drmsurf->base.wait = drm_surface_wait;

   return drmsurf;
}

struct native_surface *
drm_display_create_surface_from_resource(struct native_display *ndpy,
                                         struct pipe_resource *resource)
{
   struct drm_display *drmdpy = drm_display(ndpy);
   struct drm_surface *drmsurf;
   enum native_attachment natt = NATIVE_ATTACHMENT_FRONT_LEFT;

   drmsurf = CALLOC_STRUCT(drm_surface);
   if (!drmsurf)
      return NULL;

   drmsurf->drmdpy = drmdpy;
   drmsurf->color_format = resource->format;
   drmsurf->width = resource->width0;
   drmsurf->height = resource->height0;
   drmsurf->have_pageflip = FALSE;

   drmsurf->rsurf = resource_surface_create(drmdpy->base.screen,
         drmsurf->color_format,
         PIPE_BIND_RENDER_TARGET |
         PIPE_BIND_SAMPLER_VIEW |
         PIPE_BIND_DISPLAY_TARGET |
         PIPE_BIND_SCANOUT);
   
   resource_surface_import_resource(drmsurf->rsurf, natt, resource);

   drmsurf->base.destroy = drm_surface_destroy;
   drmsurf->base.present = drm_surface_present;
   drmsurf->base.validate = drm_surface_validate;
   drmsurf->base.wait = drm_surface_wait;

   return &drmsurf->base;
}
        

/**
 * Choose a CRTC that supports all given connectors.
 */
static uint32_t
drm_display_choose_crtc(struct native_display *ndpy,
                        uint32_t *connectors, int num_connectors)
{
   struct drm_display *drmdpy = drm_display(ndpy);
   int idx;

   for (idx = 0; idx < drmdpy->resources->count_crtcs; idx++) {
      boolean found_crtc = TRUE;
      int i, j;

      for (i = 0; i < num_connectors; i++) {
         drmModeConnectorPtr connector;
         int encoder_idx = -1;

         connector = drmModeGetConnector(drmdpy->fd, connectors[i]);
         if (!connector) {
            found_crtc = FALSE;
            break;
         }

         /* find an encoder the CRTC supports */
         for (j = 0; j < connector->count_encoders; j++) {
            drmModeEncoderPtr encoder =
               drmModeGetEncoder(drmdpy->fd, connector->encoders[j]);
            if (encoder->possible_crtcs & (1 << idx)) {
               encoder_idx = j;
               break;
            }
            drmModeFreeEncoder(encoder);
         }

         drmModeFreeConnector(connector);
         if (encoder_idx < 0) {
            found_crtc = FALSE;
            break;
         }
      }

      if (found_crtc)
         break;
   }

   if (idx >= drmdpy->resources->count_crtcs) {
      _eglLog(_EGL_WARNING,
            "failed to find a CRTC that supports the given %d connectors",
            num_connectors);
      return 0;
   }

   return drmdpy->resources->crtcs[idx];
}

/**
 * Remember the original CRTC status and set the CRTC
 */
static boolean
drm_display_set_crtc(struct native_display *ndpy, int crtc_idx,
                     uint32_t buffer_id, uint32_t x, uint32_t y,
                     uint32_t *connectors, int num_connectors,
                     drmModeModeInfoPtr mode)
{
   struct drm_display *drmdpy = drm_display(ndpy);
   struct drm_crtc *drmcrtc = &drmdpy->saved_crtcs[crtc_idx];
   uint32_t crtc_id;
   int err;

   if (drmcrtc->crtc) {
      crtc_id = drmcrtc->crtc->crtc_id;
   }
   else {
      int count = 0, i;

      /*
       * Choose the CRTC once.  It could be more dynamic, but let's keep it
       * simple for now.
       */
      crtc_id = drm_display_choose_crtc(&drmdpy->base,
            connectors, num_connectors);

      /* save the original CRTC status */
      drmcrtc->crtc = drmModeGetCrtc(drmdpy->fd, crtc_id);
      if (!drmcrtc->crtc)
         return FALSE;

      for (i = 0; i < drmdpy->num_connectors; i++) {
         struct drm_connector *drmconn = &drmdpy->connectors[i];
         drmModeConnectorPtr connector = drmconn->connector;
         drmModeEncoderPtr encoder;

         encoder = drmModeGetEncoder(drmdpy->fd, connector->encoder_id);
         if (encoder) {
            if (encoder->crtc_id == crtc_id) {
               drmcrtc->connectors[count++] = connector->connector_id;
               if (count >= Elements(drmcrtc->connectors))
                  break;
            }
            drmModeFreeEncoder(encoder);
         }
      }

      drmcrtc->num_connectors = count;
   }

   err = drmModeSetCrtc(drmdpy->fd, crtc_id, buffer_id, x, y,
         connectors, num_connectors, mode);
   if (err) {
      drmModeFreeCrtc(drmcrtc->crtc);
      drmcrtc->crtc = NULL;
      drmcrtc->num_connectors = 0;

      return FALSE;
   }

   return TRUE;
}

static boolean
drm_display_program(struct native_display *ndpy, int crtc_idx,
                    struct native_surface *nsurf, uint x, uint y,
                    const struct native_connector **nconns, int num_nconns,
                    const struct native_mode *nmode)
{
   struct drm_display *drmdpy = drm_display(ndpy);
   struct drm_surface *drmsurf = drm_surface(nsurf);
   const struct drm_mode *drmmode = drm_mode(nmode);
   uint32_t connector_ids[32];
   uint32_t buffer_id;
   drmModeModeInfo mode_tmp, *mode;
   int i;

   if (num_nconns > Elements(connector_ids)) {
      _eglLog(_EGL_WARNING, "too many connectors (%d)", num_nconns);
      num_nconns = Elements(connector_ids);
   }

   if (drmsurf) {
      if (!drm_surface_init_framebuffers(&drmsurf->base, FALSE))
         return FALSE;

      buffer_id = drmsurf->front_fb.buffer_id;
      /* the mode argument of drmModeSetCrtc is not constified */
      mode_tmp = drmmode->mode;
      mode = &mode_tmp;
   }
   else {
      /* disable the CRTC */
      buffer_id = 0;
      mode = NULL;
      num_nconns = 0;
   }

   for (i = 0; i < num_nconns; i++) {
      struct drm_connector *drmconn = drm_connector(nconns[i]);
      connector_ids[i] = drmconn->connector->connector_id;
   }

   if (!drm_display_set_crtc(&drmdpy->base, crtc_idx, buffer_id, x, y,
            connector_ids, num_nconns, mode)) {
      _eglLog(_EGL_WARNING, "failed to set CRTC %d", crtc_idx);

      return FALSE;
   }

   if (drmdpy->shown_surfaces[crtc_idx])
      drmdpy->shown_surfaces[crtc_idx]->is_shown = FALSE;
   drmdpy->shown_surfaces[crtc_idx] = drmsurf;

   /* remember the settings for buffer swapping */
   if (drmsurf) {
      uint32_t crtc_id = drmdpy->saved_crtcs[crtc_idx].crtc->crtc_id;
      struct drm_crtc *drmcrtc = &drmsurf->current_crtc;

      if (drmcrtc->crtc)
         drmModeFreeCrtc(drmcrtc->crtc);
      drmcrtc->crtc = drmModeGetCrtc(drmdpy->fd, crtc_id);

      assert(num_nconns < Elements(drmcrtc->connectors));
      memcpy(drmcrtc->connectors, connector_ids,
            sizeof(*connector_ids) * num_nconns);
      drmcrtc->num_connectors = num_nconns;

      drmsurf->is_shown = TRUE;
   }

   return TRUE;
}

static const struct native_mode **
drm_display_get_modes(struct native_display *ndpy,
                      const struct native_connector *nconn,
                      int *num_modes)
{
   struct drm_display *drmdpy = drm_display(ndpy);
   struct drm_connector *drmconn = drm_connector(nconn);
   const struct native_mode **nmodes_return;
   int count, i;

   /* delete old data */
   if (drmconn->connector) {
      drmModeFreeConnector(drmconn->connector);
      FREE(drmconn->drm_modes);

      drmconn->connector = NULL;
      drmconn->drm_modes = NULL;
      drmconn->num_modes = 0;
   }

   /* detect again */
   drmconn->connector = drmModeGetConnector(drmdpy->fd, drmconn->connector_id);
   if (!drmconn->connector)
      return NULL;

   count = drmconn->connector->count_modes;
   drmconn->drm_modes = CALLOC(count, sizeof(*drmconn->drm_modes));
   if (!drmconn->drm_modes) {
      drmModeFreeConnector(drmconn->connector);
      drmconn->connector = NULL;

      return NULL;
   }

   for (i = 0; i < count; i++) {
      struct drm_mode *drmmode = &drmconn->drm_modes[i];
      drmModeModeInfoPtr mode = &drmconn->connector->modes[i];

      drmmode->mode = *mode;

      drmmode->base.desc = drmmode->mode.name;
      drmmode->base.width = drmmode->mode.hdisplay;
      drmmode->base.height = drmmode->mode.vdisplay;
      drmmode->base.refresh_rate = drmmode->mode.vrefresh;
      /* not all kernels have vrefresh = refresh_rate * 1000 */
      if (drmmode->base.refresh_rate < 1000)
         drmmode->base.refresh_rate *= 1000;
   }

   nmodes_return = MALLOC(count * sizeof(*nmodes_return));
   if (nmodes_return) {
      for (i = 0; i < count; i++)
         nmodes_return[i] = &drmconn->drm_modes[i].base;
      if (num_modes)
         *num_modes = count;
   }

   return nmodes_return;
}

static const struct native_connector **
drm_display_get_connectors(struct native_display *ndpy, int *num_connectors,
                           int *num_crtc)
{
   struct drm_display *drmdpy = drm_display(ndpy);
   const struct native_connector **connectors;
   int i;

   if (!drmdpy->connectors) {
      drmdpy->connectors =
         CALLOC(drmdpy->resources->count_connectors, sizeof(*drmdpy->connectors));
      if (!drmdpy->connectors)
         return NULL;

      for (i = 0; i < drmdpy->resources->count_connectors; i++) {
         struct drm_connector *drmconn = &drmdpy->connectors[i];

         drmconn->connector_id = drmdpy->resources->connectors[i];
         /* drmconn->connector is allocated when the modes are asked */
      }

      drmdpy->num_connectors = drmdpy->resources->count_connectors;
   }

   connectors = MALLOC(drmdpy->num_connectors * sizeof(*connectors));
   if (connectors) {
      for (i = 0; i < drmdpy->num_connectors; i++)
         connectors[i] = &drmdpy->connectors[i].base;
      if (num_connectors)
         *num_connectors = drmdpy->num_connectors;
   }

   if (num_crtc)
      *num_crtc = drmdpy->resources->count_crtcs;

   return connectors;
}

static struct native_surface *
drm_display_create_scanout_surface(struct native_display *ndpy,
                                   const struct native_config *nconf,
                                   uint width, uint height)
{
   struct drm_surface *drmsurf;

   drmsurf = drm_display_create_surface(ndpy, nconf, width, height);
   return &drmsurf->base;
}

static struct native_display_modeset drm_display_modeset = {
   .get_connectors = drm_display_get_connectors,
   .get_modes = drm_display_get_modes,
   .create_scanout_surface = drm_display_create_scanout_surface,
   .program = drm_display_program
};

void
drm_display_fini_modeset(struct native_display *ndpy)
{
   struct drm_display *drmdpy = drm_display(ndpy);
   int i;

   if (drmdpy->connectors) {
      for (i = 0; i < drmdpy->num_connectors; i++) {
         struct drm_connector *drmconn = &drmdpy->connectors[i];
         if (drmconn->connector) {
            drmModeFreeConnector(drmconn->connector);
            FREE(drmconn->drm_modes);
         }
      }
      FREE(drmdpy->connectors);
   }

   if (drmdpy->shown_surfaces) {
      FREE(drmdpy->shown_surfaces);
      drmdpy->shown_surfaces = NULL;
   }

   if (drmdpy->saved_crtcs) {
      for (i = 0; i < drmdpy->resources->count_crtcs; i++) {
         struct drm_crtc *drmcrtc = &drmdpy->saved_crtcs[i];

         if (drmcrtc->crtc) {
            /* restore crtc */
            drmModeSetCrtc(drmdpy->fd, drmcrtc->crtc->crtc_id,
                  drmcrtc->crtc->buffer_id, drmcrtc->crtc->x, drmcrtc->crtc->y,
                  drmcrtc->connectors, drmcrtc->num_connectors,
                  &drmcrtc->crtc->mode);

            drmModeFreeCrtc(drmcrtc->crtc);
         }
      }
      FREE(drmdpy->saved_crtcs);
   }

   if (drmdpy->resources) {
      drmModeFreeResources(drmdpy->resources);
      drmdpy->resources = NULL;
   }

   drmdpy->base.modeset = NULL;
}

boolean
drm_display_init_modeset(struct native_display *ndpy)
{
   struct drm_display *drmdpy = drm_display(ndpy);

   /* resources are fixed, unlike crtc, connector, or encoder */
   drmdpy->resources = drmModeGetResources(drmdpy->fd);
   if (!drmdpy->resources) {
      _eglLog(_EGL_DEBUG, "Failed to get KMS resources.  Disable modeset.");
      return FALSE;
   }

   drmdpy->saved_crtcs =
      CALLOC(drmdpy->resources->count_crtcs, sizeof(*drmdpy->saved_crtcs));
   if (!drmdpy->saved_crtcs) {
      drm_display_fini_modeset(&drmdpy->base);
      return FALSE;
   }

   drmdpy->shown_surfaces =
      CALLOC(drmdpy->resources->count_crtcs, sizeof(*drmdpy->shown_surfaces));
   if (!drmdpy->shown_surfaces) {
      drm_display_fini_modeset(&drmdpy->base);
      return FALSE;
   }

   drmdpy->base.modeset = &drm_display_modeset;

   return TRUE;
}
