/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

#ifndef VG_CONTEXT_H
#define VG_CONTEXT_H

#include "vg_state.h"

#include "pipe/p_format.h"
#include "pipe/p_state.h"
#include "util/u_pointer.h"
#include "util/u_math.h"
#include "state_tracker/st_api.h"

#include "cso_cache/cso_hash.h"
#include "cso_cache/cso_context.h"

struct renderer;
struct shaders_cache;
struct shader;
struct vg_shader;
struct mapi_table;

struct st_renderbuffer {
   enum pipe_format   format;
   struct pipe_surface *surface;
   struct pipe_resource *texture;
   VGint width, height;
};

struct st_framebuffer {
   VGint width, height;
   struct st_renderbuffer *strb;
   struct st_renderbuffer *dsrb;

   struct pipe_sampler_view *surface_mask_view;

   struct pipe_sampler_view *blend_texture_view;


   struct st_framebuffer_iface *iface;
   enum st_attachment_type strb_att;

   void *privateData;
   int32_t stamp;
   int32_t iface_stamp;
};

enum vg_object_type {
   VG_OBJECT_UNKNOWN = 0,
   VG_OBJECT_PAINT,
   VG_OBJECT_IMAGE,
   VG_OBJECT_MASK,
   VG_OBJECT_FONT,
   VG_OBJECT_PATH,

   VG_OBJECT_LAST
};
enum dirty_state {
   BLEND_DIRTY         = 1 << 0,
   FRAMEBUFFER_DIRTY   = 1 << 1,
   DEPTH_STENCIL_DIRTY = 1 << 2,
   PAINT_DIRTY         = 1 << 3,

   ALL_DIRTY           = BLEND_DIRTY |
                         FRAMEBUFFER_DIRTY |
                         DEPTH_STENCIL_DIRTY |
                         PAINT_DIRTY
};

struct vg_context
{
   struct st_context_iface iface;
   struct mapi_table *dispatch;

   struct pipe_context *pipe;
   enum pipe_format ds_format;

   struct {
      struct vg_state vg;
      VGbitfield dirty;
   } state;

   VGErrorCode _error;

   struct st_framebuffer *draw_buffer;

   struct cso_hash *owned_objects[VG_OBJECT_LAST];

   struct {
      struct pipe_resource *cbuf;
      struct pipe_sampler_state sampler;

      struct vg_shader *union_fs;
      struct vg_shader *intersect_fs;
      struct vg_shader *subtract_fs;
      struct vg_shader *set_fs;
   } mask;

   struct cso_context *cso_context;

   struct renderer *renderer;
   struct shaders_cache *sc;
   struct shader *shader;

   struct pipe_sampler_state blend_sampler;
   struct vg_paint *default_paint;

   struct blit_state *blit;

   int32_t draw_stamp;
};


/**
 *  Base class for VG objects like paths, images, fonts.
 */
struct vg_object {
   enum vg_object_type type;
   VGHandle handle;
   struct vg_context *ctx;
};


void vg_init_object(struct vg_object *obj, struct vg_context *ctx, enum vg_object_type type);
void vg_free_object(struct vg_object *obj);

VGboolean vg_object_is_valid(VGHandle object, enum vg_object_type type);

struct vg_context *vg_create_context(struct pipe_context *pipe,
                                     const void *visual,
                                     struct vg_context *share);
void vg_destroy_context(struct vg_context *ctx);
struct vg_context *vg_current_context(void);
void vg_set_current_context(struct vg_context *ctx);

VGboolean vg_context_is_object_valid(struct vg_context *ctx,
                                     enum vg_object_type type,
                                     VGHandle object);
void vg_context_add_object(struct vg_context *ctx,
                           struct vg_object *obj);
void vg_context_remove_object(struct vg_context *ctx,
                              struct vg_object *obj);

void vg_validate_state(struct vg_context *ctx);

void vg_set_error(struct vg_context *ctx,
                  VGErrorCode code);

struct pipe_sampler_view *vg_prepare_blend_surface(struct vg_context *ctx);
struct pipe_sampler_view *vg_prepare_blend_surface_from_mask(struct vg_context *ctx);

struct pipe_sampler_view *vg_get_surface_mask(struct vg_context *ctx);

VGboolean vg_get_paint_matrix(struct vg_context *ctx,
                              const struct matrix *paint_to_user,
                              const struct matrix *user_to_surface,
                              struct matrix *mat);

static INLINE VGboolean is_aligned_to(const void *ptr, VGbyte alignment)
{
   void *aligned = align_pointer(ptr, alignment);
   return (ptr == aligned) ? VG_TRUE : VG_FALSE;
}

static INLINE VGboolean is_aligned(const void *ptr)
{
   return is_aligned_to(ptr, 4);
}

static INLINE void vg_shift_rectx(VGfloat coords[4],
                                 const VGfloat *bounds,
                                 const VGfloat shift)
{
   coords[0] += shift;
   coords[2] -= shift;
   if (bounds) {
      coords[2] = MIN2(coords[2], bounds[2]);
      /* bound x/y + width/height */
      if ((coords[0] + coords[2]) > (bounds[0] + bounds[2])) {
         coords[2] = (bounds[0] + bounds[2]) - coords[0];
      }
   }
}

static INLINE void vg_shift_recty(VGfloat coords[4],
                                 const VGfloat *bounds,
                                 const VGfloat shift)
{
   coords[1] += shift;
   coords[3] -= shift;
   if (bounds) {
      coords[3] = MIN2(coords[3], bounds[3]);
      if ((coords[1] + coords[3]) > (bounds[1] + bounds[3])) {
         coords[3] = (bounds[1] + bounds[3]) - coords[1];
      }
   }
}

static INLINE void vg_bound_rect(VGfloat coords[4],
                                 const VGfloat bounds[4],
                                 VGfloat shift[4])
{
   /* if outside the bounds */
   if (coords[0] > (bounds[0] + bounds[2]) ||
       coords[1] > (bounds[1] + bounds[3]) ||
       (coords[0] + coords[2]) < bounds[0] ||
       (coords[1] + coords[3]) < bounds[1]) {
      coords[0] = 0.f;
      coords[1] = 0.f;
      coords[2] = 0.f;
      coords[3] = 0.f;
      shift[0] = 0.f;
      shift[1] = 0.f;
      return;
   }

   /* bound x */
   if (coords[0] < bounds[0]) {
      shift[0] = bounds[0] - coords[0];
      coords[2] -= shift[0];
      coords[0] = bounds[0];
   } else
      shift[0] = 0.f;

   /* bound y */
   if (coords[1] < bounds[1]) {
      shift[1] = bounds[1] - coords[1];
      coords[3] -= shift[1];
      coords[1] = bounds[1];
   } else
      shift[1] = 0.f;

   shift[2] = bounds[2] - coords[2];
   shift[3] = bounds[3] - coords[3];
   /* bound width/height */
   coords[2] = MIN2(coords[2], bounds[2]);
   coords[3] = MIN2(coords[3], bounds[3]);

   /* bound x/y + width/height */
   if ((coords[0] + coords[2]) > (bounds[0] + bounds[2])) {
      coords[2] = (bounds[0] + bounds[2]) - coords[0];
   }
   if ((coords[1] + coords[3]) > (bounds[1] + bounds[3])) {
      coords[3] = (bounds[1] + bounds[3]) - coords[1];
   }

   /* if outside the bounds */
   if ((coords[0] + coords[2]) < bounds[0] ||
       (coords[1] + coords[3]) < bounds[1]) {
      coords[0] = 0.f;
      coords[1] = 0.f;
      coords[2] = 0.f;
      coords[3] = 0.f;
      return;
   }
}

#endif
