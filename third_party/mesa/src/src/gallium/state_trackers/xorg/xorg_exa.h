#ifndef XORG_EXA_H
#define XORG_EXA_H

#include "xorg_tracker.h"

#include "pipe/p_state.h"

struct cso_context;
struct xorg_shaders;

/* src + mask + dst */
#define MAX_EXA_SAMPLERS 3

struct exa_context
{
   ExaDriverPtr pExa;
   struct pipe_context *pipe;
   struct pipe_screen *scrn;
   struct xorg_renderer *renderer;

   struct pipe_sampler_view *bound_sampler_views[MAX_EXA_SAMPLERS];
   int num_bound_samplers;

   float solid_color[4];
   boolean has_solid_color;

   boolean accel;

   /* float[9] projective matrix bound to pictures */
   struct {
      float    src[9];
      float   mask[9];
      boolean has_src;
      boolean has_mask;
   } transform;

   struct {
      struct exa_pixmap_priv *src;
      struct exa_pixmap_priv *dst;
      PixmapPtr tmp_pix;
   } copy;
};

struct exa_pixmap_priv
{
   int width, height;

   int flags;
   int tex_flags;

   int picture_format;

   struct pipe_resource *tex;
   struct pipe_resource *depth_stencil_tex;

   struct pipe_transfer *map_transfer;
   unsigned map_count;
};

#define XORG_FALLBACK(s, arg...)                              \
do {                                                          \
   if (ms->debug_fallback) {                                  \
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,                    \
                 "%s fallback " s "\n", __FUNCTION__, ##arg); \
   }                                                          \
   return FALSE;                                              \
} while(0)

struct pipe_surface *
xorg_gpu_surface(struct pipe_context *pipe, struct exa_pixmap_priv *priv);

void xorg_exa_flush(struct exa_context *exa,
                    struct pipe_fence_handle **fence);
void xorg_exa_finish(struct exa_context *exa);

#endif
