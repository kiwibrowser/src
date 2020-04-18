#include "xorg_composite.h"

#include "xorg_renderer.h"
#include "xorg_exa_tgsi.h"

#include "cso_cache/cso_context.h"
#include "util/u_format.h"
#include "util/u_sampler.h"


/*XXX also in Xrender.h but the including it here breaks compilition */
#define XFixedToDouble(f)    (((double) (f)) / 65536.)

struct xorg_composite_blend {
   int op : 8;

   unsigned alpha_dst : 4;
   unsigned alpha_src : 4;

   unsigned rgb_src : 8;    /**< PIPE_BLENDFACTOR_x */
   unsigned rgb_dst : 8;    /**< PIPE_BLENDFACTOR_x */
};

#define BLEND_OP_OVER 3
static const struct xorg_composite_blend xorg_blends[] = {
   { PictOpClear,
     0, 0, PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_ZERO},
   { PictOpSrc,
     0, 0, PIPE_BLENDFACTOR_ONE, PIPE_BLENDFACTOR_ZERO},
   { PictOpDst,
     0, 0, PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_ONE},
   { PictOpOver,
     0, 1, PIPE_BLENDFACTOR_ONE, PIPE_BLENDFACTOR_INV_SRC_ALPHA},
   { PictOpOverReverse,
     1, 0, PIPE_BLENDFACTOR_INV_DST_ALPHA, PIPE_BLENDFACTOR_ONE},
   { PictOpIn,
     1, 0, PIPE_BLENDFACTOR_DST_ALPHA, PIPE_BLENDFACTOR_ZERO},
   { PictOpInReverse,
     0, 1, PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_SRC_ALPHA},
   { PictOpOut,
     1, 0, PIPE_BLENDFACTOR_INV_DST_ALPHA, PIPE_BLENDFACTOR_ZERO},
   { PictOpOutReverse,
     0, 1, PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_INV_SRC_ALPHA},
   { PictOpAtop,
     1, 1, PIPE_BLENDFACTOR_DST_ALPHA, PIPE_BLENDFACTOR_INV_SRC_ALPHA},
   { PictOpAtopReverse,
     1, 1, PIPE_BLENDFACTOR_INV_DST_ALPHA, PIPE_BLENDFACTOR_SRC_ALPHA},
   { PictOpXor,
     1, 1, PIPE_BLENDFACTOR_INV_DST_ALPHA, PIPE_BLENDFACTOR_INV_SRC_ALPHA},
   { PictOpAdd,
     0, 0, PIPE_BLENDFACTOR_ONE, PIPE_BLENDFACTOR_ONE},
};


static INLINE void
pixel_to_float4(Pixel pixel, float *color, enum pipe_format format)
{
   const struct util_format_description *format_desc;
   uint8_t packed[4];

   format_desc = util_format_description(format);
   packed[0] = pixel;
   packed[1] = pixel >> 8;
   packed[2] = pixel >> 16;
   packed[3] = pixel >> 24;
   format_desc->unpack_rgba_float(color, 0, packed, 0, 1, 1);
}

static boolean
blend_for_op(struct xorg_composite_blend *blend,
             int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
             PicturePtr pDstPicture)
{
   const int num_blends =
      sizeof(xorg_blends)/sizeof(struct xorg_composite_blend);
   int i;
   boolean supported = FALSE;

   /* our default in case something goes wrong */
   *blend = xorg_blends[BLEND_OP_OVER];

   for (i = 0; i < num_blends; ++i) {
      if (xorg_blends[i].op == op) {
         *blend = xorg_blends[i];
         supported = TRUE;
      }
   }

   /* If there's no dst alpha channel, adjust the blend op so that we'll treat
    * it as always 1. */
   if (pDstPicture &&
       PICT_FORMAT_A(pDstPicture->format) == 0 && blend->alpha_dst) {
      if (blend->rgb_src == PIPE_BLENDFACTOR_DST_ALPHA)
         blend->rgb_src = PIPE_BLENDFACTOR_ONE;
      else if (blend->rgb_src == PIPE_BLENDFACTOR_INV_DST_ALPHA)
         blend->rgb_src = PIPE_BLENDFACTOR_ZERO;
   }

   /* If the source alpha is being used, then we should only be in a case where
    * the source blend factor is 0, and the source blend value is the mask
    * channels multiplied by the source picture's alpha. */
   if (pMaskPicture && pMaskPicture->componentAlpha &&
       PICT_FORMAT_RGB(pMaskPicture->format) && blend->alpha_src) {
      if (blend->rgb_dst == PIPE_BLENDFACTOR_SRC_ALPHA) {
         blend->rgb_dst = PIPE_BLENDFACTOR_SRC_COLOR;
      } else if (blend->rgb_dst == PIPE_BLENDFACTOR_INV_SRC_ALPHA) {
         blend->rgb_dst = PIPE_BLENDFACTOR_INV_SRC_COLOR;
      }
   }

   return supported;
}

static INLINE int
render_repeat_to_gallium(int mode)
{
   switch(mode) {
   case RepeatNone:
      return PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   case RepeatNormal:
      return PIPE_TEX_WRAP_REPEAT;
   case RepeatReflect:
      return PIPE_TEX_WRAP_MIRROR_REPEAT;
   case RepeatPad:
      return PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   default:
      debug_printf("Unsupported repeat mode\n");
   }
   return PIPE_TEX_WRAP_REPEAT;
}

static INLINE boolean
render_filter_to_gallium(int xrender_filter, int *out_filter)
{

   switch (xrender_filter) {
   case PictFilterNearest:
      *out_filter = PIPE_TEX_FILTER_NEAREST;
      break;
   case PictFilterBilinear:
      *out_filter = PIPE_TEX_FILTER_LINEAR;
      break;
   case PictFilterFast:
      *out_filter = PIPE_TEX_FILTER_NEAREST;
      break;
   case PictFilterGood:
      *out_filter = PIPE_TEX_FILTER_LINEAR;
      break;
   case PictFilterBest:
      *out_filter = PIPE_TEX_FILTER_LINEAR;
      break;
   case PictFilterConvolution:
      *out_filter = PIPE_TEX_FILTER_NEAREST;
      return FALSE;
   default:
      debug_printf("Unknown xrender filter\n");
      *out_filter = PIPE_TEX_FILTER_NEAREST;
      return FALSE;
   }

   return TRUE;
}

static boolean is_filter_accelerated(PicturePtr pic)
{
   int filter;
   if (pic && !render_filter_to_gallium(pic->filter, &filter))
       return FALSE;
   return TRUE;
}

boolean xorg_composite_accelerated(int op,
                                   PicturePtr pSrcPicture,
                                   PicturePtr pMaskPicture,
                                   PicturePtr pDstPicture)
{
   ScreenPtr pScreen = pDstPicture->pDrawable->pScreen;
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
   modesettingPtr ms = modesettingPTR(pScrn);
   struct xorg_composite_blend blend;

   if (!is_filter_accelerated(pSrcPicture) ||
       !is_filter_accelerated(pMaskPicture)) {
      XORG_FALLBACK("Unsupported Xrender filter");
   }

   if (pSrcPicture->pSourcePict) {
      if (pSrcPicture->pSourcePict->type != SourcePictTypeSolidFill)
         XORG_FALLBACK("Gradients not enabled (haven't been well tested)");
   }

   if (blend_for_op(&blend, op,
                    pSrcPicture, pMaskPicture, pDstPicture)) {
      /* Check for component alpha */
      if (pMaskPicture && pMaskPicture->componentAlpha &&
          PICT_FORMAT_RGB(pMaskPicture->format)) {
         if (blend.alpha_src && blend.rgb_src != PIPE_BLENDFACTOR_ZERO) {
            XORG_FALLBACK("Component alpha not supported with source "
                          "alpha and source value blending. (op=%d)",
                          op);
         }
      }

      return TRUE;
   }
   XORG_FALLBACK("Unsupported composition operation = %d", op);
}

static void
bind_blend_state(struct exa_context *exa, int op,
                 PicturePtr pSrcPicture,
                 PicturePtr pMaskPicture,
                 PicturePtr pDstPicture)
{
   struct xorg_composite_blend blend_opt;
   struct pipe_blend_state blend;

   blend_for_op(&blend_opt, op, pSrcPicture, pMaskPicture, pDstPicture);

   memset(&blend, 0, sizeof(struct pipe_blend_state));
   blend.rt[0].blend_enable = 1;
   blend.rt[0].colormask = PIPE_MASK_RGBA;

   blend.rt[0].rgb_src_factor   = blend_opt.rgb_src;
   blend.rt[0].alpha_src_factor = blend_opt.rgb_src;
   blend.rt[0].rgb_dst_factor   = blend_opt.rgb_dst;
   blend.rt[0].alpha_dst_factor = blend_opt.rgb_dst;

   cso_set_blend(exa->renderer->cso, &blend);
}

static unsigned
picture_format_fixups(struct exa_pixmap_priv *pSrc, PicturePtr pSrcPicture, boolean mask,
                      PicturePtr pDstPicture)
{
   boolean set_alpha = FALSE;
   boolean swizzle = FALSE;
   unsigned ret = 0;

   if (pSrc && pSrc->picture_format == pSrcPicture->format) {
      if (pSrc->picture_format == PICT_a8) {
         if (mask)
            return FS_MASK_LUMINANCE;
         else if (pDstPicture->format != PICT_a8) {
            /* if both dst and src are luminance then
             * we don't want to swizzle the alpha (X) of the
             * source into W component of the dst because
             * it will break our destination */
            return FS_SRC_LUMINANCE;
         }
      }
      return 0;
   }

   if (pSrc && pSrc->picture_format != PICT_a8r8g8b8) {
      assert(!"can not handle formats");
      return 0;
   }

   /* pSrc->picture_format == PICT_a8r8g8b8 */
   switch (pSrcPicture->format) {
   case PICT_x8b8g8r8:
   case PICT_b8g8r8:
      set_alpha = TRUE; /* fall trough */
   case PICT_a8b8g8r8:
      swizzle = TRUE;
      break;
   case PICT_x8r8g8b8:
   case PICT_r8g8b8:
      set_alpha = TRUE; /* fall through */
   case PICT_a8r8g8b8:
      break;
#ifdef PICT_TYPE_BGRA
   case PICT_b8g8r8a8:
   case PICT_b8g8r8x8:
   case PICT_a2r10g10b10:
   case PICT_x2r10g10b10:
   case PICT_a2b10g10r10:
   case PICT_x2b10g10r10:
#endif
   default:
      assert(!"can not handle formats");
      return 0;
   }

   if (set_alpha)
      ret |= mask ? FS_MASK_SET_ALPHA : FS_SRC_SET_ALPHA;
   if (swizzle)
      ret |= mask ? FS_MASK_SWIZZLE_RGB : FS_SRC_SWIZZLE_RGB;

   return ret;
}

static void
bind_shaders(struct exa_context *exa, int op,
             PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture,
             struct exa_pixmap_priv *pSrc, struct exa_pixmap_priv *pMask)
{
   unsigned vs_traits = 0, fs_traits = 0;
   struct xorg_shader shader;

   exa->has_solid_color = FALSE;

   if (pSrcPicture) {
      if (pSrcPicture->repeatType == RepeatNone && pSrcPicture->transform)
         fs_traits |= FS_SRC_REPEAT_NONE;

      if (pSrcPicture->pSourcePict) {
         if (pSrcPicture->pSourcePict->type == SourcePictTypeSolidFill) {
            fs_traits |= FS_SOLID_FILL;
            vs_traits |= VS_SOLID_FILL;
            debug_assert(pSrcPicture->format == PICT_a8r8g8b8);
            pixel_to_float4(pSrcPicture->pSourcePict->solidFill.color,
                            exa->solid_color, PIPE_FORMAT_B8G8R8A8_UNORM);
            exa->has_solid_color = TRUE;
         } else {
            debug_assert("!gradients not supported");
         }
      } else {
         fs_traits |= FS_COMPOSITE;
         vs_traits |= VS_COMPOSITE;
      }

      fs_traits |= picture_format_fixups(pSrc, pSrcPicture, FALSE, pDstPicture);
   }

   if (pMaskPicture) {
      vs_traits |= VS_MASK;
      fs_traits |= FS_MASK;
      if (pMaskPicture->repeatType == RepeatNone && pMaskPicture->transform)
         fs_traits |= FS_MASK_REPEAT_NONE;
      if (pMaskPicture->componentAlpha) {
         struct xorg_composite_blend blend;
         blend_for_op(&blend, op,
                      pSrcPicture, pMaskPicture, NULL);
         if (blend.alpha_src) {
            fs_traits |= FS_CA_SRCALPHA;
         } else
            fs_traits |= FS_CA_FULL;
      }

      fs_traits |= picture_format_fixups(pMask, pMaskPicture, TRUE, pDstPicture);
   }

   shader = xorg_shaders_get(exa->renderer->shaders, vs_traits, fs_traits);
   cso_set_vertex_shader_handle(exa->renderer->cso, shader.vs);
   cso_set_fragment_shader_handle(exa->renderer->cso, shader.fs);
}

static void
bind_samplers(struct exa_context *exa, int op,
              PicturePtr pSrcPicture, PicturePtr pMaskPicture,
              PicturePtr pDstPicture,
              struct exa_pixmap_priv *pSrc,
              struct exa_pixmap_priv *pMask,
              struct exa_pixmap_priv *pDst)
{
   struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS] = {0};
   struct pipe_sampler_state src_sampler, mask_sampler;
   struct pipe_sampler_view view_templ;
   struct pipe_sampler_view *src_view;
   struct pipe_context *pipe = exa->pipe;

   exa->num_bound_samplers = 0;

   memset(&src_sampler, 0, sizeof(struct pipe_sampler_state));
   memset(&mask_sampler, 0, sizeof(struct pipe_sampler_state));

   if (pSrcPicture && pSrc) {
      if (exa->has_solid_color) {
         debug_assert(!"solid color with textures");
         samplers[0] = NULL;
         pipe_sampler_view_reference(&exa->bound_sampler_views[0], NULL);
      } else {
         unsigned src_wrap = render_repeat_to_gallium(
            pSrcPicture->repeatType);
         int filter;

         render_filter_to_gallium(pSrcPicture->filter, &filter);

         src_sampler.wrap_s = src_wrap;
         src_sampler.wrap_t = src_wrap;
         src_sampler.min_img_filter = filter;
         src_sampler.mag_img_filter = filter;
         src_sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
         src_sampler.normalized_coords = 1;
         samplers[0] = &src_sampler;
         exa->num_bound_samplers = 1;
         u_sampler_view_default_template(&view_templ,
                                         pSrc->tex,
                                         pSrc->tex->format);
         src_view = pipe->create_sampler_view(pipe, pSrc->tex, &view_templ);
         pipe_sampler_view_reference(&exa->bound_sampler_views[0], NULL);
         exa->bound_sampler_views[0] = src_view;
      }
   }

   if (pMaskPicture && pMask) {
      unsigned mask_wrap = render_repeat_to_gallium(
         pMaskPicture->repeatType);
      int filter;

      render_filter_to_gallium(pMaskPicture->filter, &filter);

      mask_sampler.wrap_s = mask_wrap;
      mask_sampler.wrap_t = mask_wrap;
      mask_sampler.min_img_filter = filter;
      mask_sampler.mag_img_filter = filter;
      src_sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
      mask_sampler.normalized_coords = 1;
      samplers[1] = &mask_sampler;
      exa->num_bound_samplers = 2;
      u_sampler_view_default_template(&view_templ,
                                      pMask->tex,
                                      pMask->tex->format);
      src_view = pipe->create_sampler_view(pipe, pMask->tex, &view_templ);
      pipe_sampler_view_reference(&exa->bound_sampler_views[1], NULL);
      exa->bound_sampler_views[1] = src_view;
   }

   cso_set_samplers(exa->renderer->cso, PIPE_SHADER_FRAGMENT,
                    exa->num_bound_samplers,
                    (const struct pipe_sampler_state **)samplers);
   cso_set_sampler_views(exa->renderer->cso, PIPE_SHADER_FRAGMENT,
                         exa->num_bound_samplers,
                         exa->bound_sampler_views);
}



static INLINE boolean matrix_from_pict_transform(PictTransform *trans, float *matrix)
{
   if (!trans)
      return FALSE;

   matrix[0] = XFixedToDouble(trans->matrix[0][0]);
   matrix[3] = XFixedToDouble(trans->matrix[0][1]);
   matrix[6] = XFixedToDouble(trans->matrix[0][2]);

   matrix[1] = XFixedToDouble(trans->matrix[1][0]);
   matrix[4] = XFixedToDouble(trans->matrix[1][1]);
   matrix[7] = XFixedToDouble(trans->matrix[1][2]);

   matrix[2] = XFixedToDouble(trans->matrix[2][0]);
   matrix[5] = XFixedToDouble(trans->matrix[2][1]);
   matrix[8] = XFixedToDouble(trans->matrix[2][2]);

   return TRUE;
}

static void
setup_transforms(struct  exa_context *exa,
                 PicturePtr pSrcPicture, PicturePtr pMaskPicture)
{
   PictTransform *src_t = NULL;
   PictTransform *mask_t = NULL;

   if (pSrcPicture)
      src_t = pSrcPicture->transform;
   if (pMaskPicture)
      mask_t = pMaskPicture->transform;

   exa->transform.has_src  =
      matrix_from_pict_transform(src_t, exa->transform.src);
   exa->transform.has_mask =
      matrix_from_pict_transform(mask_t, exa->transform.mask);
}

boolean xorg_composite_bind_state(struct exa_context *exa,
                                  int op,
                                  PicturePtr pSrcPicture,
                                  PicturePtr pMaskPicture,
                                  PicturePtr pDstPicture,
                                  struct exa_pixmap_priv *pSrc,
                                  struct exa_pixmap_priv *pMask,
                                  struct exa_pixmap_priv *pDst)
{
   struct pipe_surface *dst_surf = xorg_gpu_surface(exa->pipe, pDst);

   renderer_bind_destination(exa->renderer, dst_surf,
                             pDst->width,
                             pDst->height);

   bind_blend_state(exa, op, pSrcPicture, pMaskPicture, pDstPicture);
   bind_shaders(exa, op, pSrcPicture, pMaskPicture, pDstPicture, pSrc, pMask);
   bind_samplers(exa, op, pSrcPicture, pMaskPicture,
                 pDstPicture, pSrc, pMask, pDst);

   setup_transforms(exa, pSrcPicture, pMaskPicture);

   if (exa->num_bound_samplers == 0 ) { /* solid fill */
      renderer_begin_solid(exa->renderer);
   } else {
      renderer_begin_textures(exa->renderer,
                              exa->num_bound_samplers);
   }


   pipe_surface_reference(&dst_surf, NULL);
   return TRUE;
}

void xorg_composite(struct exa_context *exa,
                    struct exa_pixmap_priv *dst,
                    int srcX, int srcY, int maskX, int maskY,
                    int dstX, int dstY, int width, int height)
{
   if (exa->num_bound_samplers == 0 ) { /* solid fill */
      renderer_solid(exa->renderer,
                     dstX, dstY, dstX + width, dstY + height,
                     exa->solid_color);
   } else {
      int pos[6] = {srcX, srcY, maskX, maskY, dstX, dstY};
      float *src_matrix = NULL;
      float *mask_matrix = NULL;

      if (exa->transform.has_src)
         src_matrix = exa->transform.src;
      if (exa->transform.has_mask)
         mask_matrix = exa->transform.mask;

      renderer_texture(exa->renderer,
                       pos, width, height,
                       exa->bound_sampler_views,
                       exa->num_bound_samplers,
                       src_matrix, mask_matrix);
   }
}

boolean xorg_solid_bind_state(struct exa_context *exa,
                              struct exa_pixmap_priv *pixmap,
                              Pixel fg)
{
   struct pipe_surface *dst_surf = xorg_gpu_surface(exa->pipe, pixmap);
   unsigned vs_traits, fs_traits;
   struct xorg_shader shader;

   pixel_to_float4(fg, exa->solid_color, pixmap->tex->format);
   exa->has_solid_color = TRUE;

#if 0
   debug_printf("Color Pixel=(%d, %d, %d, %d), RGBA=(%f, %f, %f, %f)\n",
                (fg >> 24) & 0xff, (fg >> 16) & 0xff,
                (fg >> 8) & 0xff,  (fg >> 0) & 0xff,
                exa->solid_color[0], exa->solid_color[1],
                exa->solid_color[2], exa->solid_color[3]);
#endif

   vs_traits = VS_SOLID_FILL;
   fs_traits = FS_SOLID_FILL;

   renderer_bind_destination(exa->renderer, dst_surf, 
                             pixmap->width, pixmap->height);
   bind_blend_state(exa, PictOpSrc, NULL, NULL, NULL);
   cso_set_samplers(exa->renderer->cso, PIPE_SHADER_FRAGMENT, 0, NULL);
   cso_set_sampler_views(exa->renderer->cso, PIPE_SHADER_FRAGMENT, 0, NULL);

   shader = xorg_shaders_get(exa->renderer->shaders, vs_traits, fs_traits);
   cso_set_vertex_shader_handle(exa->renderer->cso, shader.vs);
   cso_set_fragment_shader_handle(exa->renderer->cso, shader.fs);

   renderer_begin_solid(exa->renderer);

   pipe_surface_reference(&dst_surf, NULL);
   return TRUE;
}

void xorg_solid(struct exa_context *exa,
                struct exa_pixmap_priv *pixmap,
                int x0, int y0, int x1, int y1)
{
   renderer_solid(exa->renderer,
                  x0, y0, x1, y1, exa->solid_color);
}

void
xorg_composite_done(struct exa_context *exa)
{
   renderer_draw_flush(exa->renderer);

   exa->transform.has_src = FALSE;
   exa->transform.has_mask = FALSE;
   exa->has_solid_color = FALSE;
   exa->num_bound_samplers = 0;
}
