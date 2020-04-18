#ifndef __NV30_RESOURCE_H__
#define __NV30_RESOURCE_H__

#include "nouveau/nouveau_buffer.h"

void nv30_resource_screen_init(struct pipe_screen *);
void nv30_resource_init(struct pipe_context *);

struct nv30_surface {
   struct pipe_surface base;
   uint32_t offset;
   uint32_t pitch;
   uint32_t width;
   uint16_t height;
   uint16_t depth;
};

static INLINE struct nv30_surface *
nv30_surface(struct pipe_surface *ps)
{
   return (struct nv30_surface *)ps;
}

struct nv30_miptree_level {
   uint32_t offset;
   uint32_t pitch;
   uint32_t zslice_size;
};

struct nv30_miptree {
   struct nv04_resource base;
   struct nv30_miptree_level level[13];
   uint32_t uniform_pitch;
   uint32_t layer_size;
   boolean swizzled;
   unsigned ms_mode;
   unsigned ms_x:1;
   unsigned ms_y:1;
};

static INLINE struct nv30_miptree *
nv30_miptree(struct pipe_resource *pt)
{
   return (struct nv30_miptree *)pt;
}

struct pipe_resource *
nv30_miptree_create(struct pipe_screen *, const struct pipe_resource *);

struct pipe_resource *
nv30_miptree_from_handle(struct pipe_screen *, const struct pipe_resource *,
                         struct winsys_handle *);

struct pipe_surface *
nv30_miptree_surface_new(struct pipe_context *, struct pipe_resource *,
                         const struct pipe_surface *);

void
nv30_miptree_surface_del(struct pipe_context *, struct pipe_surface *);

void
nv30_resource_copy_region(struct pipe_context *pipe,
                          struct pipe_resource *dst, unsigned dst_level,
                          unsigned dstx, unsigned dsty, unsigned dstz,
                          struct pipe_resource *src, unsigned src_level,
                          const struct pipe_box *src_box);

void
nv30_resource_resolve(struct pipe_context *, const struct pipe_resolve_info *);

#endif
