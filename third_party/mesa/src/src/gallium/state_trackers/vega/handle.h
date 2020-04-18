/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.  All Rights Reserved.
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


/**
 * Convert opaque VG object handles into pointers and vice versa.
 * XXX This is not yet 64-bit safe!  All VG handles are 32 bits in size.
 */


#ifndef HANDLE_H
#define HANDLE_H

#include "pipe/p_compiler.h"
#include "util/u_hash_table.h"
#include "util/u_pointer.h"

#include "VG/openvg.h"
#include "vg_context.h"


extern struct util_hash_table *handle_hash;


struct vg_mask_layer;
struct vg_font;
struct vg_image;
struct vg_paint;
struct path;


extern void
init_handles(void);


extern void
free_handles(void);


extern VGHandle
create_handle(void *object);


extern void
destroy_handle(VGHandle h);


static INLINE VGHandle
object_to_handle(struct vg_object *obj)
{
   return obj ? obj->handle : VG_INVALID_HANDLE;
}


static INLINE VGHandle
image_to_handle(struct vg_image *img)
{
   /* vg_image is derived from vg_object */
   return object_to_handle((struct vg_object *) img);
}


static INLINE VGHandle
masklayer_to_handle(struct vg_mask_layer *mask)
{
   /* vg_object is derived from vg_object */
   return object_to_handle((struct vg_object *) mask);
}


static INLINE VGHandle
font_to_handle(struct vg_font *font)
{
   return object_to_handle((struct vg_object *) font);
}


static INLINE VGHandle
paint_to_handle(struct vg_paint *paint)
{
   return object_to_handle((struct vg_object *) paint);
}


static INLINE VGHandle
path_to_handle(struct path *path)
{
   return object_to_handle((struct vg_object *) path);
}


static INLINE void *
handle_to_pointer(VGHandle h)
{
   void *v = util_hash_table_get(handle_hash, intptr_to_pointer(h));
#ifdef DEBUG
   if (v) {
      struct vg_object *obj = (struct vg_object *) v;
      assert(obj->handle == h);
   }
#endif
   return v;
}


static INLINE struct vg_font *
handle_to_font(VGHandle h)
{
   return (struct vg_font *) handle_to_pointer(h);
}


static INLINE struct vg_image *
handle_to_image(VGHandle h)
{
   return (struct vg_image *) handle_to_pointer(h);
}


static INLINE struct vg_mask_layer *
handle_to_masklayer(VGHandle h)
{
   return (struct vg_mask_layer *) handle_to_pointer(h);
}


static INLINE struct vg_object *
handle_to_object(VGHandle h)
{
   return (struct vg_object *) handle_to_pointer(h);
}


static INLINE struct vg_paint *
handle_to_paint(VGHandle h)
{
   return (struct vg_paint *) handle_to_pointer(h);
}


static INLINE struct path *
handle_to_path(VGHandle h)
{
   return (struct path *) handle_to_pointer(h);
}


#endif /* HANDLE_H */
