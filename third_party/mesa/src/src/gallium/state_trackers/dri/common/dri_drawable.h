/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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

#ifndef DRI_DRAWABLE_H
#define DRI_DRAWABLE_H

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "state_tracker/st_api.h"

struct pipe_surface;
struct st_framebuffer;
struct dri_context;

#define DRI_SWAP_FENCES_MAX 4
#define DRI_SWAP_FENCES_MASK 3
#define DRI_SWAP_FENCES_DEFAULT 1

struct dri_drawable
{
   struct st_framebuffer_iface base;
   struct st_visual stvis;

   struct dri_screen *screen;

   /* dri */
   __DRIdrawable *dPriv;
   __DRIscreen *sPriv;

   __DRIbuffer old[8];
   unsigned old_num;
   unsigned old_w;
   unsigned old_h;

   struct pipe_resource *textures[ST_ATTACHMENT_COUNT];
   unsigned int texture_mask, texture_stamp;

   struct pipe_fence_handle *swap_fences[DRI_SWAP_FENCES_MAX];
   unsigned int cur_fences;
   unsigned int head;
   unsigned int tail;
   unsigned int desired_fences;

   /* used only by DRISW */
   struct pipe_surface *drisw_surface;

   /* hooks filled in by dri2 & drisw */
   void (*allocate_textures)(struct dri_drawable *drawable,
                             const enum st_attachment_type *statts,
                             unsigned count);

   void (*update_drawable_info)(struct dri_drawable *drawable);

   void (*flush_frontbuffer)(struct dri_drawable *drawable,
                             enum st_attachment_type statt);

   void (*update_tex_buffer)(struct dri_drawable *drawable,
                             struct dri_context *ctx,
                             struct pipe_resource *res);
};

static INLINE struct dri_drawable *
dri_drawable(__DRIdrawable * driDrawPriv)
{
   return (struct dri_drawable *) (driDrawPriv)
      ? driDrawPriv->driverPrivate : NULL;
}

/***********************************************************************
 * dri_drawable.c
 */
boolean
dri_create_buffer(__DRIscreen * sPriv,
		  __DRIdrawable * dPriv,
		  const struct gl_config * visual, boolean isPixmap);

void dri_destroy_buffer(__DRIdrawable * dPriv);

void
dri_drawable_get_format(struct dri_drawable *drawable,
                        enum st_attachment_type statt,
                        enum pipe_format *format,
                        unsigned *bind);

extern const __DRItexBufferExtension driTexBufferExtension;
extern const __DRI2throttleExtension dri2ThrottleExtension;
#endif

/* vim: set sw=3 ts=8 sts=3 expandtab: */
