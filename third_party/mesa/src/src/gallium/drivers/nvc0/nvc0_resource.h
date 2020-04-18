
#ifndef __NVC0_RESOURCE_H__
#define __NVC0_RESOURCE_H__

#include "nv50/nv50_resource.h"

#define NVC0_RESOURCE_FLAG_VIDEO (NOUVEAU_RESOURCE_FLAG_DRV_PRIV << 0)


#define NVC0_TILE_SHIFT_X(m) ((((m) >> 0) & 0xf) + 6)
#define NVC0_TILE_SHIFT_Y(m) ((((m) >> 4) & 0xf) + 3)
#define NVC0_TILE_SHIFT_Z(m) ((((m) >> 8) & 0xf) + 0)

#define NVC0_TILE_SIZE_X(m) (64 << (((m) >> 0) & 0xf))
#define NVC0_TILE_SIZE_Y(m) ( 8 << (((m) >> 4) & 0xf))
#define NVC0_TILE_SIZE_Z(m) ( 1 << (((m) >> 8) & 0xf))

/* it's ok to mask only in the end because max value is 3 * 5 */

#define NVC0_TILE_SIZE_2D(m) ((64 * 8) << (((m) + ((m) >> 4)) & 0xf))

#define NVC0_TILE_SIZE(m) ((64 * 8) << (((m) + ((m) >> 4) + ((m) >> 8)) & 0xf))


void
nvc0_init_resource_functions(struct pipe_context *pcontext);

void
nvc0_screen_init_resource_functions(struct pipe_screen *pscreen);

/* Internal functions:
 */
struct pipe_resource *
nvc0_miptree_create(struct pipe_screen *pscreen,
                    const struct pipe_resource *tmp);

const struct u_resource_vtbl nvc0_miptree_vtbl;

struct pipe_surface *
nvc0_miptree_surface_new(struct pipe_context *,
                         struct pipe_resource *,
                         const struct pipe_surface *templ);

unsigned
nvc0_mt_zslice_offset(const struct nv50_miptree *, unsigned l, unsigned z);

struct pipe_transfer *
nvc0_miptree_transfer_new(struct pipe_context *pcontext,
                          struct pipe_resource *pt,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box);
void
nvc0_miptree_transfer_del(struct pipe_context *pcontext,
                          struct pipe_transfer *ptx);
void *
nvc0_miptree_transfer_map(struct pipe_context *pcontext,
                          struct pipe_transfer *ptx);
void
nvc0_miptree_transfer_unmap(struct pipe_context *pcontext,
                            struct pipe_transfer *ptx);

#endif
