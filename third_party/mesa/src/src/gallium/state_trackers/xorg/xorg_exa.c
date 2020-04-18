/*
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 *
 * Author: Alan Hourihane <alanh@tungstengraphics.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 *
 */

#include "xorg_exa.h"
#include "xorg_tracker.h"
#include "xorg_composite.h"
#include "xorg_exa_tgsi.h"

#include <xorg-server.h>
#include <xf86.h>
#include <picturestr.h>
#include <picture.h>

#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "util/u_rect.h"
#include "util/u_math.h"
#include "util/u_debug.h"
#include "util/u_format.h"
#include "util/u_box.h"
#include "util/u_surface.h"

#define ROUND_UP_TEXTURES 1

static INLINE void
exa_debug_printf(const char *format, ...) _util_printf_format(1,2);

static INLINE void
exa_debug_printf(const char *format, ...)
{
#if 0
   va_list ap;
   va_start(ap, format);
   _debug_vprintf(format, ap);
   va_end(ap);
#else
   (void) format; /* silence warning */
#endif
}

/*
 * Helper functions
 */
struct render_format_str {
   int format;
   const char *name;
};
static const struct render_format_str formats_info[] =
{
   {PICT_a8r8g8b8, "PICT_a8r8g8b8"},
   {PICT_x8r8g8b8, "PICT_x8r8g8b8"},
   {PICT_a8b8g8r8, "PICT_a8b8g8r8"},
   {PICT_x8b8g8r8, "PICT_x8b8g8r8"},
#ifdef PICT_TYPE_BGRA
   {PICT_b8g8r8a8, "PICT_b8g8r8a8"},
   {PICT_b8g8r8x8, "PICT_b8g8r8x8"},
   {PICT_a2r10g10b10, "PICT_a2r10g10b10"},
   {PICT_x2r10g10b10, "PICT_x2r10g10b10"},
   {PICT_a2b10g10r10, "PICT_a2b10g10r10"},
   {PICT_x2b10g10r10, "PICT_x2b10g10r10"},
#endif
   {PICT_r8g8b8, "PICT_r8g8b8"},
   {PICT_b8g8r8, "PICT_b8g8r8"},
   {PICT_r5g6b5, "PICT_r5g6b5"},
   {PICT_b5g6r5, "PICT_b5g6r5"},
   {PICT_a1r5g5b5, "PICT_a1r5g5b5"},
   {PICT_x1r5g5b5, "PICT_x1r5g5b5"},
   {PICT_a1b5g5r5, "PICT_a1b5g5r5"},
   {PICT_x1b5g5r5, "PICT_x1b5g5r5"},
   {PICT_a4r4g4b4, "PICT_a4r4g4b4"},
   {PICT_x4r4g4b4, "PICT_x4r4g4b4"},
   {PICT_a4b4g4r4, "PICT_a4b4g4r4"},
   {PICT_x4b4g4r4, "PICT_x4b4g4r4"},
   {PICT_a8, "PICT_a8"},
   {PICT_r3g3b2, "PICT_r3g3b2"},
   {PICT_b2g3r3, "PICT_b2g3r3"},
   {PICT_a2r2g2b2, "PICT_a2r2g2b2"},
   {PICT_a2b2g2r2, "PICT_a2b2g2r2"},
   {PICT_c8, "PICT_c8"},
   {PICT_g8, "PICT_g8"},
   {PICT_x4a4, "PICT_x4a4"},
   {PICT_x4c4, "PICT_x4c4"},
   {PICT_x4g4, "PICT_x4g4"},
   {PICT_a4, "PICT_a4"},
   {PICT_r1g2b1, "PICT_r1g2b1"},
   {PICT_b1g2r1, "PICT_b1g2r1"},
   {PICT_a1r1g1b1, "PICT_a1r1g1b1"},
   {PICT_a1b1g1r1, "PICT_a1b1g1r1"},
   {PICT_c4, "PICT_c4"},
   {PICT_g4, "PICT_g4"},
   {PICT_a1, "PICT_a1"},
   {PICT_g1, "PICT_g1"}
};
static const char *render_format_name(int format)
{
   int i = 0;
   for (i = 0; i < sizeof(formats_info)/sizeof(formats_info[0]); ++i) {
      if (formats_info[i].format == format)
         return formats_info[i].name;
   }
   return NULL;
}

static void
exa_get_pipe_format(int depth, enum pipe_format *format, int *bbp, int *picture_format)
{
    switch (depth) {
    case 32:
	*format = PIPE_FORMAT_B8G8R8A8_UNORM;
	*picture_format = PICT_a8r8g8b8;
	assert(*bbp == 32);
	break;
    case 24:
	*format = PIPE_FORMAT_B8G8R8X8_UNORM;
	*picture_format = PICT_x8r8g8b8;
	assert(*bbp == 32);
	break;
    case 16:
	*format = PIPE_FORMAT_B5G6R5_UNORM;
	*picture_format = PICT_r5g6b5;
	assert(*bbp == 16);
	break;
    case 15:
	*format = PIPE_FORMAT_B5G5R5A1_UNORM;
	*picture_format = PICT_x1r5g5b5;
	assert(*bbp == 16);
	break;
    case 8:
	*format = PIPE_FORMAT_L8_UNORM;
	*picture_format = PICT_a8;
	assert(*bbp == 8);
	break;
    case 4:
    case 1:
	*format = PIPE_FORMAT_B8G8R8A8_UNORM; /* bad bad bad */
	break;
    default:
	assert(0);
	break;
    }
}


/*
 * Static exported EXA functions
 */

static void
ExaWaitMarker(ScreenPtr pScreen, int marker)
{
   /* Nothing to do, handled in the PrepareAccess hook */
}

static int
ExaMarkSync(ScreenPtr pScreen)
{
   return 1;
}


/***********************************************************************
 * Screen upload/download
 */

static Bool
ExaDownloadFromScreen(PixmapPtr pPix, int x,  int y, int w,  int h, char *dst,
		      int dst_pitch)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPix);
    struct pipe_transfer *transfer;

    if (!priv || !priv->tex)
	return FALSE;

    transfer = pipe_get_transfer(exa->pipe, priv->tex, 0, 0,
                                 PIPE_TRANSFER_READ, x, y, w, h);
    if (!transfer)
	return FALSE;

    exa_debug_printf("------ ExaDownloadFromScreen(%d, %d, %d, %d, %d)\n",
                 x, y, w, h, dst_pitch);

    util_copy_rect((unsigned char*)dst, priv->tex->format, dst_pitch, 0, 0,
		   w, h, exa->pipe->transfer_map(exa->pipe, transfer),
		   transfer->stride, 0, 0);

    exa->pipe->transfer_unmap(exa->pipe, transfer);
    exa->pipe->transfer_destroy(exa->pipe, transfer);

    return TRUE;
}

static Bool
ExaUploadToScreen(PixmapPtr pPix, int x, int y, int w, int h, char *src,
		  int src_pitch)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPix);
    struct pipe_transfer *transfer;

    if (!priv || !priv->tex)
	return FALSE;

    transfer = pipe_get_transfer(exa->pipe, priv->tex, 0, 0,
                                 PIPE_TRANSFER_WRITE, x, y, w, h);
    if (!transfer)
	return FALSE;

    exa_debug_printf("++++++ ExaUploadToScreen(%d, %d, %d, %d, %d)\n",
                 x, y, w, h, src_pitch);

    util_copy_rect(exa->pipe->transfer_map(exa->pipe, transfer),
		   priv->tex->format, transfer->stride, 0, 0, w, h,
		   (unsigned char*)src, src_pitch, 0, 0);

    exa->pipe->transfer_unmap(exa->pipe, transfer);
    exa->pipe->transfer_destroy(exa->pipe, transfer);

    return TRUE;
}

static Bool
ExaPrepareAccess(PixmapPtr pPix, int index)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv;

    priv = exaGetPixmapDriverPrivate(pPix);

    if (!priv)
	return FALSE;

    if (!priv->tex)
	return FALSE;

    exa_debug_printf("ExaPrepareAccess %d\n", index);

    if (priv->map_count == 0)
    {
        assert(pPix->drawable.width <= priv->tex->width0);
        assert(pPix->drawable.height <= priv->tex->height0);

	priv->map_transfer =
	   pipe_get_transfer(exa->pipe, priv->tex, 0, 0,
#ifdef EXA_MIXED_PIXMAPS
					PIPE_TRANSFER_MAP_DIRECTLY |
#endif
					PIPE_TRANSFER_READ_WRITE,
					0, 0, 
                                        pPix->drawable.width,
                                        pPix->drawable.height );
	if (!priv->map_transfer)
#ifdef EXA_MIXED_PIXMAPS
	    return FALSE;
#else
	    FatalError("failed to create transfer\n");
#endif

	pPix->devPrivate.ptr =
	    exa->pipe->transfer_map(exa->pipe, priv->map_transfer);
	pPix->devKind = priv->map_transfer->stride;
    }

    priv->map_count++;

    exa_debug_printf("ExaPrepareAccess %d prepared\n", index);

    return TRUE;
}

static void
ExaFinishAccess(PixmapPtr pPix, int index)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv;
    priv = exaGetPixmapDriverPrivate(pPix);

    if (!priv)
	return;

    if (!priv->map_transfer)
	return;

    exa_debug_printf("ExaFinishAccess %d\n", index);

    if (--priv->map_count == 0) {
	assert(priv->map_transfer);
	exa->pipe->transfer_unmap(exa->pipe, priv->map_transfer);
	exa->pipe->transfer_destroy(exa->pipe, priv->map_transfer);
	priv->map_transfer = NULL;
	pPix->devPrivate.ptr = NULL;
    }

    exa_debug_printf("ExaFinishAccess %d finished\n", index);
}

/***********************************************************************
 * Solid Fills
 */

static Bool
ExaPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planeMask, Pixel fg)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pPixmap->drawable.pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_context *exa = ms->exa;

    exa_debug_printf("ExaPrepareSolid(0x%x)\n", fg);

    if (!exa->accel)
	return FALSE;

    if (!exa->pipe)
	XORG_FALLBACK("accel not enabled");

    if (!priv || !priv->tex)
	XORG_FALLBACK("%s", !priv ? "!priv" : "!priv->tex");

    if (!EXA_PM_IS_SOLID(&pPixmap->drawable, planeMask))
	XORG_FALLBACK("planeMask is not solid");

    if (alu != GXcopy)
	XORG_FALLBACK("not GXcopy");

    if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                        priv->tex->target, 0,
                                        PIPE_BIND_RENDER_TARGET)) {
	XORG_FALLBACK("format %s", util_format_name(priv->tex->format));
    }

    return xorg_solid_bind_state(exa, priv, fg);
}

static void
ExaSolid(PixmapPtr pPixmap, int x0, int y0, int x1, int y1)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pPixmap->drawable.pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);

    exa_debug_printf("\tExaSolid(%d, %d, %d, %d)\n", x0, y0, x1, y1);

    if (x0 == 0 && y0 == 0 &&
        x1 == pPixmap->drawable.width && y1 == pPixmap->drawable.height) {
       union pipe_color_union solid_color;
       solid_color.f[0] = exa->solid_color[0];
       solid_color.f[1] = exa->solid_color[1];
       solid_color.f[2] = exa->solid_color[2];
       solid_color.f[3] = exa->solid_color[3];
       exa->pipe->clear(exa->pipe, PIPE_CLEAR_COLOR, &solid_color, 0.0, 0);
       return;
    }

    xorg_solid(exa, priv, x0, y0, x1, y1) ;
}


static void
ExaDoneSolid(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pPixmap->drawable.pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_context *exa = ms->exa;

    if (!priv)
	return;

    exa_debug_printf("ExaDoneSolid\n");
    xorg_composite_done(exa);
    exa_debug_printf("ExaDoneSolid done\n");
}

/***********************************************************************
 * Copy Blits
 */

static Bool
ExaPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap, int xdir,
	       int ydir, int alu, Pixel planeMask)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pDstPixmap->drawable.pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pDstPixmap);
    struct exa_pixmap_priv *src_priv = exaGetPixmapDriverPrivate(pSrcPixmap);

    exa_debug_printf("ExaPrepareCopy\n");

    if (!exa->accel)
	return FALSE;

    if (!exa->pipe)
	XORG_FALLBACK("accel not enabled");

    if (!priv || !priv->tex)
	XORG_FALLBACK("pDst %s", !priv ? "!priv" : "!priv->tex");

    if (!src_priv || !src_priv->tex)
	XORG_FALLBACK("pSrc %s", !src_priv ? "!priv" : "!priv->tex");

    if (!EXA_PM_IS_SOLID(&pSrcPixmap->drawable, planeMask))
	XORG_FALLBACK("planeMask is not solid");

    if (alu != GXcopy)
	XORG_FALLBACK("alu not GXcopy");

    if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                        priv->tex->target, 0,
                                        PIPE_BIND_RENDER_TARGET))
	XORG_FALLBACK("pDst format %s", util_format_name(priv->tex->format));

    if (!exa->scrn->is_format_supported(exa->scrn, src_priv->tex->format,
                                        src_priv->tex->target, 0,
                                        PIPE_BIND_SAMPLER_VIEW))
	XORG_FALLBACK("pSrc format %s", util_format_name(src_priv->tex->format));

    exa->copy.src = src_priv;
    exa->copy.dst = priv;

    return TRUE;
}

static void
ExaCopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY,
	int width, int height)
{
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pDstPixmap->drawable.pScreen);
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;
   struct pipe_box src_box;

   exa_debug_printf("\tExaCopy(srcx=%d, srcy=%d, dstX=%d, dstY=%d, w=%d, h=%d)\n",
                srcX, srcY, dstX, dstY, width, height);

   debug_assert(exaGetPixmapDriverPrivate(pDstPixmap) == exa->copy.dst);

   u_box_2d(srcX, srcY, width, height, &src_box);

   /* If source and destination overlap, we have to copy to/from a scratch
    * pixmap.
    */
   if (exa->copy.dst == exa->copy.src &&
       !((dstX + width) < srcX || dstX > (srcX + width) ||
	 (dstY + height) < srcY || dstY > (srcY + height))) {
      struct exa_pixmap_priv *tmp_priv;

      if (!exa->copy.tmp_pix) {
         exa->copy.tmp_pix = pScrn->pScreen->CreatePixmap(pScrn->pScreen,
                                                         pDstPixmap->drawable.width,
                                                         pDstPixmap->drawable.height,
                                                         pDstPixmap->drawable.depth,
                                                         pDstPixmap->drawable.width);
         exaMoveInPixmap(exa->copy.tmp_pix);
      }

      tmp_priv = exaGetPixmapDriverPrivate(exa->copy.tmp_pix);

      exa->pipe->resource_copy_region( exa->pipe,
                                       tmp_priv->tex,
                                       0,
                                       srcX, srcY, 0,
                                       exa->copy.src->tex,
                                       0, &src_box);
      exa->pipe->resource_copy_region( exa->pipe,
                                       exa->copy.dst->tex,
                                       0,
                                       dstX, dstY, 0,
                                       tmp_priv->tex,
                                       0, &src_box);
   } else
      exa->pipe->resource_copy_region( exa->pipe,
                                       exa->copy.dst->tex,
                                       0,
                                       dstX, dstY, 0,
                                       exa->copy.src->tex,
                                       0, &src_box);
}

static void
ExaDoneCopy(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pPixmap->drawable.pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_context *exa = ms->exa;

    if (!priv)
	return;

   exa_debug_printf("ExaDoneCopy\n");

   if (exa->copy.tmp_pix) {
      pScrn->pScreen->DestroyPixmap(exa->copy.tmp_pix);
      exa->copy.tmp_pix = NULL;
   }
   exa->copy.src = NULL;
   exa->copy.dst = NULL;

   exa_debug_printf("ExaDoneCopy done\n");
}



static Bool
picture_check_formats(struct exa_pixmap_priv *pSrc, PicturePtr pSrcPicture)
{
   if (pSrc->picture_format == pSrcPicture->format)
      return TRUE;

   if (pSrc->picture_format != PICT_a8r8g8b8)
      return FALSE;

   /* pSrc->picture_format == PICT_a8r8g8b8 */
   switch (pSrcPicture->format) {
   case PICT_a8r8g8b8:
   case PICT_x8r8g8b8:
   case PICT_a8b8g8r8:
   case PICT_x8b8g8r8:
   /* just treat these two as x8... */
   case PICT_r8g8b8:
   case PICT_b8g8r8:
      return TRUE;
#ifdef PICT_TYPE_BGRA
   case PICT_b8g8r8a8:
   case PICT_b8g8r8x8:
      return FALSE; /* does not support swizzleing the alpha channel yet */
   case PICT_a2r10g10b10:
   case PICT_x2r10g10b10:
   case PICT_a2b10g10r10:
   case PICT_x2b10g10r10:
      return FALSE;
#endif
   default:
      return FALSE;
   }
   return FALSE;
}

/***********************************************************************
 * Composite entrypoints
 */

static Bool
ExaCheckComposite(int op,
		  PicturePtr pSrcPicture, PicturePtr pMaskPicture,
		  PicturePtr pDstPicture)
{
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pDstPicture->pDrawable->pScreen);
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;
   Bool accelerated = exa->accel && xorg_composite_accelerated(op,
				     pSrcPicture,
				     pMaskPicture,
				     pDstPicture);

   exa_debug_printf("ExaCheckComposite(%d, %p, %p, %p) = %d\n",
                op, pSrcPicture, pMaskPicture, pDstPicture, accelerated);

   return accelerated;
}


static Bool
ExaPrepareComposite(int op, PicturePtr pSrcPicture,
		    PicturePtr pMaskPicture, PicturePtr pDstPicture,
		    PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pDst->drawable.pScreen);
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;
   struct exa_pixmap_priv *priv;

   if (!exa->accel)
       return FALSE;

   exa_debug_printf("ExaPrepareComposite(%d, src=0x%p, mask=0x%p, dst=0x%p)\n",
                op, pSrcPicture, pMaskPicture, pDstPicture);
   exa_debug_printf("\tFormats: src(%s), mask(%s), dst(%s)\n",
                pSrcPicture ? render_format_name(pSrcPicture->format) : "none",
                pMaskPicture ? render_format_name(pMaskPicture->format) : "none",
                pDstPicture ? render_format_name(pDstPicture->format) : "none");

   if (!exa->pipe)
      XORG_FALLBACK("accel not enabled");

   priv = exaGetPixmapDriverPrivate(pDst);
   if (!priv || !priv->tex)
      XORG_FALLBACK("pDst %s", !priv ? "!priv" : "!priv->tex");

   if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                       priv->tex->target, 0,
                                       PIPE_BIND_RENDER_TARGET))
      XORG_FALLBACK("pDst format: %s", util_format_name(priv->tex->format));

   if (priv->picture_format != pDstPicture->format)
      XORG_FALLBACK("pDst pic_format: %s != %s",
                    render_format_name(priv->picture_format),
                    render_format_name(pDstPicture->format));

   if (pSrc) {
      priv = exaGetPixmapDriverPrivate(pSrc);
      if (!priv || !priv->tex)
         XORG_FALLBACK("pSrc %s", !priv ? "!priv" : "!priv->tex");

      if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                          priv->tex->target, 0,
                                          PIPE_BIND_SAMPLER_VIEW))
         XORG_FALLBACK("pSrc format: %s", util_format_name(priv->tex->format));

      if (!picture_check_formats(priv, pSrcPicture))
         XORG_FALLBACK("pSrc pic_format: %s != %s",
                       render_format_name(priv->picture_format),
                       render_format_name(pSrcPicture->format));

   }

   if (pMask) {
      priv = exaGetPixmapDriverPrivate(pMask);
      if (!priv || !priv->tex)
         XORG_FALLBACK("pMask %s", !priv ? "!priv" : "!priv->tex");

      if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                          priv->tex->target, 0,
                                          PIPE_BIND_SAMPLER_VIEW))
         XORG_FALLBACK("pMask format: %s", util_format_name(priv->tex->format));

      if (!picture_check_formats(priv, pMaskPicture))
         XORG_FALLBACK("pMask pic_format: %s != %s",
                       render_format_name(priv->picture_format),
                       render_format_name(pMaskPicture->format));
   }

   return xorg_composite_bind_state(exa, op, pSrcPicture, pMaskPicture,
                                    pDstPicture,
                                    pSrc ? exaGetPixmapDriverPrivate(pSrc) : NULL,
                                    pMask ? exaGetPixmapDriverPrivate(pMask) : NULL,
                                    exaGetPixmapDriverPrivate(pDst));
}

static void
ExaComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
	     int dstX, int dstY, int width, int height)
{
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pDst->drawable.pScreen);
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;
   struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pDst);

   exa_debug_printf("\tExaComposite(src[%d,%d], mask=[%d, %d], dst=[%d, %d], dim=[%d, %d])\n",
                srcX, srcY, maskX, maskY, dstX, dstY, width, height);
   exa_debug_printf("\t   Num bound samplers = %d\n",
                exa->num_bound_samplers);

   xorg_composite(exa, priv, srcX, srcY, maskX, maskY,
                  dstX, dstY, width, height);
}



static void
ExaDoneComposite(PixmapPtr pPixmap)
{
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pPixmap->drawable.pScreen);
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;

   xorg_composite_done(exa);
}


/***********************************************************************
 * Pixmaps
 */

static void *
ExaCreatePixmap(ScreenPtr pScreen, int size, int align)
{
    struct exa_pixmap_priv *priv;

    priv = calloc(1, sizeof(struct exa_pixmap_priv));
    if (!priv)
	return NULL;

    return priv;
}

static void
ExaDestroyPixmap(ScreenPtr pScreen, void *dPriv)
{
    struct exa_pixmap_priv *priv = (struct exa_pixmap_priv *)dPriv;

    if (!priv)
	return;

    pipe_resource_reference(&priv->tex, NULL);

    free(priv);
}

static Bool
ExaPixmapIsOffscreen(PixmapPtr pPixmap)
{
    struct exa_pixmap_priv *priv;

    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv)
	return FALSE;

    if (priv->tex)
	return TRUE;

    return FALSE;
}

int
xorg_exa_set_displayed_usage(PixmapPtr pPixmap)
{
    struct exa_pixmap_priv *priv;
    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv) {
	FatalError("NO PIXMAP PRIVATE\n");
	return 0;
    }

    priv->flags |= PIPE_BIND_SCANOUT;

    return 0;
}

int
xorg_exa_set_shared_usage(PixmapPtr pPixmap)
{
    struct exa_pixmap_priv *priv;
    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv) {
	FatalError("NO PIXMAP PRIVATE\n");
	return 0;
    }

    priv->flags |= PIPE_BIND_SHARED;

    return 0;
}



static Bool
size_match( int width, int tex_width )
{
#if ROUND_UP_TEXTURES
   if (width > tex_width)
      return FALSE;

   if (width * 2 < tex_width)
      return FALSE;

   return TRUE;
#else
   return width == tex_width;
#endif
}

static Bool
ExaModifyPixmapHeader(PixmapPtr pPixmap, int width, int height,
		      int depth, int bitsPerPixel, int devKind,
		      pointer pPixData)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;

    if (!priv || pPixData)
	return FALSE;

    if (0) {
       debug_printf("%s pixmap %p sz %dx%dx%d devKind %d\n",
                    __FUNCTION__, pPixmap, width, height, bitsPerPixel, devKind);
       
       if (priv->tex)
          debug_printf("  ==> old texture %dx%d\n",
                       priv->tex->width0, 
                       priv->tex->height0);
    }


    if (depth <= 0)
	depth = pPixmap->drawable.depth;

    if (bitsPerPixel <= 0)
	bitsPerPixel = pPixmap->drawable.bitsPerPixel;

    if (width <= 0)
	width = pPixmap->drawable.width;

    if (height <= 0)
	height = pPixmap->drawable.height;

    if (width <= 0 || height <= 0 || depth <= 0)
	return FALSE;

    miModifyPixmapHeader(pPixmap, width, height, depth,
			     bitsPerPixel, devKind, NULL);

    priv->width = width;
    priv->height = height;

    /* Deal with screen resize */
    if ((exa->accel || priv->flags) &&
        (!priv->tex ||
         !size_match(width, priv->tex->width0) ||
         !size_match(height, priv->tex->height0) ||
         priv->tex_flags != priv->flags)) {
	struct pipe_resource *texture = NULL;
	struct pipe_resource template;

	memset(&template, 0, sizeof(template));
	template.target = PIPE_TEXTURE_2D;
	exa_get_pipe_format(depth, &template.format, &bitsPerPixel, &priv->picture_format);
        if (ROUND_UP_TEXTURES && priv->flags == 0) {
           template.width0 = util_next_power_of_two(width);
           template.height0 = util_next_power_of_two(height);
        }
        else {
           template.width0 = width;
           template.height0 = height;
        }

	template.depth0 = 1;
	template.array_size = 1;
	template.last_level = 0;
	template.bind = PIPE_BIND_RENDER_TARGET | priv->flags;
	priv->tex_flags = priv->flags;
	texture = exa->scrn->resource_create(exa->scrn, &template);

	if (priv->tex) {
            struct pipe_box src_box;
            u_box_origin_2d(min(width, texture->width0),
                            min(height, texture->height0),
                            &src_box);
            exa->pipe->resource_copy_region(exa->pipe, texture,
                                            0, 0, 0, 0,
                                            priv->tex,
                                            0, &src_box);
	}

	pipe_resource_reference(&priv->tex, texture);
	/* the texture we create has one reference */
	pipe_resource_reference(&texture, NULL);
    }

    return TRUE;
}

struct pipe_resource *
xorg_exa_get_texture(PixmapPtr pPixmap)
{
   struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
   struct pipe_resource *tex = NULL;
   pipe_resource_reference(&tex, priv->tex);
   return tex;
}

Bool
xorg_exa_set_texture(PixmapPtr pPixmap, struct  pipe_resource *tex)
{
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);

    int mask = PIPE_BIND_SHARED | PIPE_BIND_SCANOUT;

    if (!priv)
	return FALSE;

    if (pPixmap->drawable.width != tex->width0 ||
	pPixmap->drawable.height != tex->height0)
	return FALSE;

    pipe_resource_reference(&priv->tex, tex);
    priv->tex_flags = tex->bind & mask;

    return TRUE;
}

struct pipe_resource *
xorg_exa_create_root_texture(ScrnInfoPtr pScrn,
			     int width, int height,
			     int depth, int bitsPerPixel)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct pipe_resource template;
    int dummy;

    memset(&template, 0, sizeof(template));
    template.target = PIPE_TEXTURE_2D;
    exa_get_pipe_format(depth, &template.format, &bitsPerPixel, &dummy);
    template.width0 = width;
    template.height0 = height;
    template.depth0 = 1;
    template.array_size = 1;
    template.last_level = 0;
    template.bind |= PIPE_BIND_RENDER_TARGET;
    template.bind |= PIPE_BIND_SCANOUT;
    template.bind |= PIPE_BIND_SHARED;

    return exa->scrn->resource_create(exa->scrn, &template);
}

void
xorg_exa_close(ScrnInfoPtr pScrn)
{
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;

   pipe_sampler_view_reference(&exa->bound_sampler_views[0], NULL);
   pipe_sampler_view_reference(&exa->bound_sampler_views[1], NULL);

   renderer_destroy(exa->renderer);

   xorg_exa_finish(exa);

   if (exa->pipe)
      exa->pipe->destroy(exa->pipe);
   exa->pipe = NULL;
   /* Since this was shared be proper with the pointer */
   ms->ctx = NULL;

   exaDriverFini(pScrn->pScreen);
   free(exa);
   ms->exa = NULL;
}

void *
xorg_exa_init(ScrnInfoPtr pScrn, Bool accel)
{
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa;
   ExaDriverPtr pExa;
   CustomizerPtr cust = ms->cust;

   exa = calloc(1, sizeof(struct exa_context));
   if (!exa)
      return NULL;

   exa->scrn = ms->screen;
   exa->pipe = exa->scrn->context_create(exa->scrn, NULL);
   if (exa->pipe == NULL)
      goto out_err;

   pExa = exaDriverAlloc();
   if (!pExa) {
      goto out_err;
   }

   pExa->exa_major         = 2;
   pExa->exa_minor         = 2;
   pExa->memoryBase        = 0;
   pExa->memorySize        = 0;
   pExa->offScreenBase     = 0;
   pExa->pixmapOffsetAlign = 0;
   pExa->pixmapPitchAlign  = 1;
   pExa->flags             = EXA_OFFSCREEN_PIXMAPS | EXA_HANDLES_PIXMAPS;
#ifdef EXA_SUPPORTS_PREPARE_AUX
   pExa->flags            |= EXA_SUPPORTS_PREPARE_AUX;
#endif
#ifdef EXA_MIXED_PIXMAPS
   pExa->flags            |= EXA_MIXED_PIXMAPS;
#endif

   pExa->maxX = pExa->maxY =
   1 << (exa->scrn->get_param(exa->scrn, PIPE_CAP_MAX_TEXTURE_2D_LEVELS) - 1);

   pExa->WaitMarker         = ExaWaitMarker;
   pExa->MarkSync           = ExaMarkSync;
   pExa->PrepareSolid       = ExaPrepareSolid;
   pExa->Solid              = ExaSolid;
   pExa->DoneSolid          = ExaDoneSolid;
   pExa->PrepareCopy        = ExaPrepareCopy;
   pExa->Copy               = ExaCopy;
   pExa->DoneCopy           = ExaDoneCopy;
   pExa->CheckComposite     = ExaCheckComposite;
   pExa->PrepareComposite   = ExaPrepareComposite;
   pExa->Composite          = ExaComposite;
   pExa->DoneComposite      = ExaDoneComposite;
   pExa->PixmapIsOffscreen  = ExaPixmapIsOffscreen;
   pExa->DownloadFromScreen = ExaDownloadFromScreen;
   pExa->UploadToScreen     = ExaUploadToScreen;
   pExa->PrepareAccess      = ExaPrepareAccess;
   pExa->FinishAccess       = ExaFinishAccess;
   pExa->CreatePixmap       = ExaCreatePixmap;
   pExa->DestroyPixmap      = ExaDestroyPixmap;
   pExa->ModifyPixmapHeader = ExaModifyPixmapHeader;

   if (!exaDriverInit(pScrn->pScreen, pExa)) {
      goto out_err;
   }

   /* Share context with DRI */
   ms->ctx = exa->pipe;
   if (cust && cust->winsys_context_throttle)
       cust->winsys_context_throttle(cust, ms->ctx, THROTTLE_RENDER);

   exa->renderer = renderer_create(exa->pipe);
   exa->accel = accel;

   return (void *)exa;

out_err:
   xorg_exa_close(pScrn);
   free(exa);

   return NULL;
}

struct pipe_surface *
xorg_gpu_surface(struct pipe_context *pipe, struct exa_pixmap_priv *priv)
{
   struct pipe_surface surf_tmpl;
   memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   u_surface_default_template(&surf_tmpl, priv->tex,
                              PIPE_BIND_RENDER_TARGET);

   return pipe->create_surface(pipe, priv->tex, &surf_tmpl);

}

void xorg_exa_flush(struct exa_context *exa,
                    struct pipe_fence_handle **fence)
{
   exa->pipe->flush(exa->pipe, fence);
}

void xorg_exa_finish(struct exa_context *exa)
{
   struct pipe_fence_handle *fence = NULL;

   xorg_exa_flush(exa, &fence);

   exa->pipe->screen->fence_finish(exa->pipe->screen, fence,
                                   PIPE_TIMEOUT_INFINITE);
   exa->pipe->screen->fence_reference(exa->pipe->screen, &fence, NULL);
}

