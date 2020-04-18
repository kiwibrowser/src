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


#include "xorg-server.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "mipointer.h"
#include "micmap.h"
#include <X11/extensions/randr.h>
#include "fb.h"
#include "edid.h"
#include "xf86i2c.h"
#include "xf86Crtc.h"
#include "miscstruct.h"
#include "dixstruct.h"
#include "xf86cmap.h"
#include "xf86xv.h"
#include "xorgVersion.h"
#ifndef XSERVER_LIBPCIACCESS
#error "libpciaccess needed"
#endif

#include <pciaccess.h>

#include "state_tracker/drm_driver.h"
#include "pipe/p_context.h"
#include "xorg_tracker.h"
#include "xorg_winsys.h"

#ifdef HAVE_LIBKMS
#include "libkms/libkms.h"
#endif

/*
 * Functions and symbols exported to Xorg via pointers.
 */

static Bool drv_pre_init(ScrnInfoPtr pScrn, int flags);
static Bool drv_screen_init(SCREEN_INIT_ARGS_DECL);
static Bool drv_switch_mode(SWITCH_MODE_ARGS_DECL);
static void drv_adjust_frame(ADJUST_FRAME_ARGS_DECL);
static Bool drv_enter_vt(VT_FUNC_ARGS_DECL);
static Bool drv_enter_vt_flags(ScrnInfoPtr pScrn, int flags);
static void drv_leave_vt(VT_FUNC_ARGS_DECL);
static void drv_free_screen(FREE_SCREEN_ARGS_DECL);
static ModeStatus drv_valid_mode(SCRN_ARG_TYPE arg, DisplayModePtr mode, Bool verbose,
			         int flags);

typedef enum
{
    OPTION_SW_CURSOR,
    OPTION_2D_ACCEL,
    OPTION_DEBUG_FALLBACK,
    OPTION_THROTTLE_SWAP,
    OPTION_THROTTLE_DIRTY,
    OPTION_3D_ACCEL
} drv_option_enums;

static const OptionInfoRec drv_options[] = {
    {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_2D_ACCEL, "2DAccel", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_DEBUG_FALLBACK, "DebugFallback", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_THROTTLE_SWAP, "SwapThrottling", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_THROTTLE_DIRTY, "DirtyThrottling", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_3D_ACCEL, "3DAccel", OPTV_BOOLEAN, {0}, FALSE},
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};


/*
 * Exported Xorg driver functions to winsys
 */

const OptionInfoRec *
xorg_tracker_available_options(int chipid, int busid)
{
    return drv_options;
}

void
xorg_tracker_set_functions(ScrnInfoPtr scrn)
{
    scrn->PreInit = drv_pre_init;
    scrn->ScreenInit = drv_screen_init;
    scrn->SwitchMode = drv_switch_mode;
    scrn->AdjustFrame = drv_adjust_frame;
    scrn->EnterVT = drv_enter_vt;
    scrn->LeaveVT = drv_leave_vt;
    scrn->FreeScreen = drv_free_screen;
    scrn->ValidMode = drv_valid_mode;
}

Bool
xorg_tracker_have_modesetting(ScrnInfoPtr pScrn, struct pci_device *device)
{
    char *BusID = malloc(64);
    sprintf(BusID, "pci:%04x:%02x:%02x.%d",
	    device->domain, device->bus,
	    device->dev, device->func);

    if (drmCheckModesettingSupported(BusID)) {
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 0,
		       "Drm modesetting not supported %s\n", BusID);
	free(BusID);
	return FALSE;
    }

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 0,
		   "Drm modesetting supported on %s\n", BusID);

    free(BusID);
    return TRUE;
}


/*
 * Internal function definitions
 */

static Bool drv_init_front_buffer_functions(ScrnInfoPtr pScrn);
static Bool drv_close_screen(CLOSE_SCREEN_ARGS_DECL);


/*
 * Internal functions
 */

static Bool
drv_get_rec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(1, sizeof(modesettingRec));

    return TRUE;
}

static void
drv_free_rec(ScrnInfoPtr pScrn)
{
    if (!pScrn)
	return;

    if (!pScrn->driverPrivate)
	return;

    free(pScrn->driverPrivate);

    pScrn->driverPrivate = NULL;
}

static void
drv_probe_ddc(ScrnInfoPtr pScrn, int index)
{
    ConfiguredMonitor = NULL;
}

static Bool
drv_crtc_resize(ScrnInfoPtr pScrn, int width, int height)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    modesettingPtr ms = modesettingPTR(pScrn);
    CustomizerPtr cust = ms->cust;
    ScreenPtr pScreen = pScrn->pScreen;
    int old_width, old_height;
    PixmapPtr rootPixmap;
    int i;

    if (width == pScrn->virtualX && height == pScrn->virtualY)
	return TRUE;

    if (cust && cust->winsys_check_fb_size &&
	!cust->winsys_check_fb_size(cust, width*pScrn->bitsPerPixel / 8,
				    height)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Requested framebuffer size %dx%dx%d will not fit "
		   "in display memory.\n",
		   width, height, pScrn->bitsPerPixel);
	return FALSE;
    }

    old_width = pScrn->virtualX;
    old_height = pScrn->virtualY;
    pScrn->virtualX = width;
    pScrn->virtualY = height;

    /* ms->create_front_buffer will remove the old front buffer */

    rootPixmap = pScreen->GetScreenPixmap(pScreen);
    if (!pScreen->ModifyPixmapHeader(rootPixmap, width, height, -1, -1, -1, NULL))
	goto error_modify;

    pScrn->displayWidth = rootPixmap->devKind / (rootPixmap->drawable.bitsPerPixel / 8);

    if (!ms->create_front_buffer(pScrn) || !ms->bind_front_buffer(pScrn))
	goto error_create;

    /*
     * create && bind will turn off all crtc(s) in the kernel so we need to
     * re-enable all the crtcs again. For real HW we might want to do this
     * before destroying the old framebuffer.
     */
    for (i = 0; i < xf86_config->num_crtc; i++) {
	xf86CrtcPtr crtc = xf86_config->crtc[i];

	if (!crtc->enabled)
	    continue;

	crtc->funcs->set_mode_major(crtc, &crtc->mode, crtc->rotation, crtc->x, crtc->y);
    }

    return TRUE;

    /*
     * This is the error recovery path.
     */
error_create:
    if (!pScreen->ModifyPixmapHeader(rootPixmap, old_width, old_height, -1, -1, -1, NULL))
	FatalError("failed to resize rootPixmap error path\n");

    pScrn->displayWidth = rootPixmap->devKind / (rootPixmap->drawable.bitsPerPixel / 8);

error_modify:
    pScrn->virtualX = old_width;
    pScrn->virtualY = old_height;

    if (ms->create_front_buffer(pScrn) && ms->bind_front_buffer(pScrn))
	return FALSE;

    FatalError("failed to setup old framebuffer\n");
    return FALSE;
}

static const xf86CrtcConfigFuncsRec crtc_config_funcs = {
    .resize = drv_crtc_resize
};

static Bool
drv_init_drm(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);

    /* deal with server regeneration */
    if (ms->fd < 0) {
	char *BusID;

	BusID = malloc(64);
	sprintf(BusID, "PCI:%d:%d:%d",
		((ms->PciInfo->domain << 8) | ms->PciInfo->bus),
		ms->PciInfo->dev, ms->PciInfo->func
	    );


	ms->fd = drmOpen(driver_descriptor.driver_name, BusID);
	ms->isMaster = TRUE;
	free(BusID);

	if (ms->fd >= 0)
	    return TRUE;

	return FALSE;
    }

    return TRUE;
}

static Bool
drv_init_resource_management(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    /*
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    Bool fbAccessDisabled;
    CARD8 *fbstart;
     */

    if (ms->screen || ms->kms)
	return TRUE;

    if (!ms->no3D)
	ms->screen = driver_descriptor.create_screen(ms->fd);

    if (ms->screen)
	return TRUE;

#ifdef HAVE_LIBKMS
    if (!kms_create(ms->fd, &ms->kms))
	return TRUE;
#endif

    return FALSE;
}

static void
drv_cleanup_fences(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    int i;

    assert(ms->screen);

    for (i = 0; i < XORG_NR_FENCES; i++) {
	if (ms->fence[i]) {
            ms->screen->fence_finish(ms->screen, ms->fence[i],
                                     PIPE_TIMEOUT_INFINITE);
	    ms->screen->fence_reference(ms->screen, &ms->fence[i], NULL);
	}
    }
}

static Bool
drv_pre_init(ScrnInfoPtr pScrn, int flags)
{
    xf86CrtcConfigPtr xf86_config;
    modesettingPtr ms;
    rgb defaultWeight = { 0, 0, 0 };
    EntityInfoPtr pEnt;
    EntPtr msEnt = NULL;
    CustomizerPtr cust;
    Bool use3D;

    if (pScrn->numEntities != 1)
	return FALSE;

    pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

    if (flags & PROBE_DETECT) {
	drv_probe_ddc(pScrn, pEnt->index);
	return TRUE;
    }

    cust = (CustomizerPtr) pScrn->driverPrivate;
    pScrn->driverPrivate = NULL;

    /* Allocate driverPrivate */
    if (!drv_get_rec(pScrn))
	return FALSE;

    ms = modesettingPTR(pScrn);
    ms->pEnt = pEnt;
    ms->cust = cust;
    ms->fb_id = -1;

    pScrn->displayWidth = 640;	       /* default it */

    if (ms->pEnt->location.type != BUS_PCI)
	return FALSE;

    ms->PciInfo = xf86GetPciInfoForEntity(ms->pEnt->index);

    /* Allocate an entity private if necessary */
    if (xf86IsEntityShared(pScrn->entityList[0])) {
	FatalError("Entity");
#if 0
	msEnt = xf86GetEntityPrivate(pScrn->entityList[0],
				     modesettingEntityIndex)->ptr;
	ms->entityPrivate = msEnt;
#else
	(void)msEnt;
#endif
    } else
	ms->entityPrivate = NULL;

    if (xf86IsEntityShared(pScrn->entityList[0])) {
	if (xf86IsPrimInitDone(pScrn->entityList[0])) {
	    /* do something */
	} else {
	    xf86SetPrimInitDone(pScrn->entityList[0]);
	}
    }

    ms->fd = -1;
    if (!drv_init_drm(pScrn))
	return FALSE;

    pScrn->monitor = pScrn->confScreen->monitor;
    pScrn->progClock = TRUE;
    pScrn->rgbBits = 8;

    if (!xf86SetDepthBpp
	(pScrn, 0, 0, 0,
	 PreferConvert24to32 | SupportConvert24to32 | Support32bppFb))
	return FALSE;

    switch (pScrn->depth) {
    case 8:
    case 15:
    case 16:
    case 24:
	break;
    default:
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Given depth (%d) is not supported by the driver\n",
		   pScrn->depth);
	return FALSE;
    }
    xf86PrintDepthBpp(pScrn);

    if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight))
	return FALSE;
    if (!xf86SetDefaultVisual(pScrn, -1))
	return FALSE;

    /* Process the options */
    xf86CollectOptions(pScrn, NULL);
    if (!(ms->Options = malloc(sizeof(drv_options))))
	return FALSE;
    memcpy(ms->Options, drv_options, sizeof(drv_options));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, ms->Options);

    use3D = cust ? !cust->no_3d : TRUE;
    ms->from_3D = xf86GetOptValBool(ms->Options, OPTION_3D_ACCEL,
				    &use3D) ?
	X_CONFIG : X_PROBED;

    ms->no3D = !use3D;

    if (!drv_init_resource_management(pScrn)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Could not init "
					       "Gallium3D or libKMS.\n");
	return FALSE;
    }

    /* Allocate an xf86CrtcConfig */
    xf86CrtcConfigInit(pScrn, &crtc_config_funcs);
    xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);

    /* get max width and height */
    {
	drmModeResPtr res;
	int max_width, max_height;

	res = drmModeGetResources(ms->fd);
	max_width = res->max_width;
	max_height = res->max_height;

	if (ms->screen) {
	    int max;

	    max = ms->screen->get_param(ms->screen,
					PIPE_CAP_MAX_TEXTURE_2D_LEVELS);
	    max = 1 << (max - 1);
	    max_width = max < max_width ? max : max_width;
	    max_height = max < max_height ? max : max_height;
	}

	xf86CrtcSetSizeRange(pScrn, res->min_width,
			     res->min_height, max_width, max_height);
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Min width %d, Max Width %d.\n",
		   res->min_width, max_width);
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Min height %d, Max Height %d.\n",
		   res->min_height, max_height);
	drmModeFreeResources(res);
    }


    if (xf86ReturnOptValBool(ms->Options, OPTION_SW_CURSOR, FALSE)) {
	ms->SWCursor = TRUE;
    }

    xorg_crtc_init(pScrn);
    xorg_output_init(pScrn);

    if (cust && cust->winsys_pre_init && !cust->winsys_pre_init(cust, ms->fd))
	return FALSE;

    if (!xf86InitialConfiguration(pScrn, TRUE)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes.\n");
	return FALSE;
    }

    /*
     * If the driver can do gamma correction, it should call xf86SetGamma() here.
     */
    {
	Gamma zeros = { 0.0, 0.0, 0.0 };

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }

    if (pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No modes.\n");
	return FALSE;
    }

    pScrn->currentMode = pScrn->modes;

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /* Load the required sub modules */
    if (!xf86LoadSubModule(pScrn, "fb"))
	return FALSE;

    /* XXX: these aren't needed when we are using libkms */
    if (!xf86LoadSubModule(pScrn, "exa"))
	return FALSE;

#ifdef DRI2
    if (!xf86LoadSubModule(pScrn, "dri2"))
	return FALSE;
#endif

    return TRUE;
}

void xorg_flush(ScreenPtr pScreen)
{
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(pScreen));

    if (ms->ctx) {
	int j;

	ms->ctx->flush(ms->ctx,
		       ms->dirtyThrottling ?
		       &ms->fence[XORG_NR_FENCES-1] :
		       NULL);
       
	if (ms->dirtyThrottling) {
	    if (ms->fence[0])
		ms->ctx->screen->fence_finish(ms->ctx->screen,
                                              ms->fence[0],
                                              PIPE_TIMEOUT_INFINITE);
  
	    /* The amount of rendering generated by a block handler can be
	     * quite small.  Let us get a fair way ahead of hardware before
	     * throttling.
	     */
	    for (j = 0; j < XORG_NR_FENCES - 1; j++)
		ms->screen->fence_reference(ms->screen,
					    &ms->fence[j],
					    ms->fence[j+1]);

	    ms->screen->fence_reference(ms->screen,
					&ms->fence[XORG_NR_FENCES-1],
					NULL);
	}
    }

#ifdef DRM_MODE_FEATURE_DIRTYFB
    {
	RegionPtr dirty = DamageRegion(ms->damage);
	unsigned num_cliprects = REGION_NUM_RECTS(dirty);

	if (num_cliprects) {
	    drmModeClip *clip = alloca(num_cliprects * sizeof(drmModeClip));
	    BoxPtr rect = REGION_RECTS(dirty);
	    int i, ret;

	    /* XXX no need for copy? */
	    for (i = 0; i < num_cliprects; i++, rect++) {
		clip[i].x1 = rect->x1;
		clip[i].y1 = rect->y1;
		clip[i].x2 = rect->x2;
		clip[i].y2 = rect->y2;
	    }

	    /* TODO query connector property to see if this is needed */
	    ret = drmModeDirtyFB(ms->fd, ms->fb_id, clip, num_cliprects);
	    if (ret) {
		debug_printf("%s: failed to send dirty (%i, %s)\n",
			     __func__, ret, strerror(-ret));
	    }

	    DamageEmpty(ms->damage);
	}
    }
#endif
}

static void drv_block_handler(BLOCKHANDLER_ARGS_DECL)
{
    SCREEN_PTR(arg);
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(pScreen));

    pScreen->BlockHandler = ms->blockHandler;
    pScreen->BlockHandler(BLOCKHANDLER_ARGS);
    pScreen->BlockHandler = drv_block_handler;

    xorg_flush(pScreen);
}

static Bool
drv_create_screen_resources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap;
    Bool ret;

    ms->noEvict = TRUE;

    pScreen->CreateScreenResources = ms->createScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = drv_create_screen_resources;

    ms->bind_front_buffer(pScrn);

    ms->noEvict = FALSE;

    drv_adjust_frame(ADJUST_FRAME_ARGS(pScrn, pScrn->frameX0, pScrn->frameY0));

#ifdef DRM_MODE_FEATURE_DIRTYFB
    rootPixmap = pScreen->GetScreenPixmap(pScreen);
    ms->damage = DamageCreate(NULL, NULL, DamageReportNone, TRUE,
                              pScreen, rootPixmap);

    if (ms->damage) {
       DamageRegister(&rootPixmap->drawable, ms->damage);

       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Damage tracking initialized\n");
    } else {
       xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                  "Failed to create screen damage record\n");
       return FALSE;
    }
#else
    (void)rootPixmap;
#endif

    return ret;
}

static Bool
drv_set_master(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);

    if (!ms->isMaster && drmSetMaster(ms->fd) != 0) {
	if (errno == EINVAL) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "drmSetMaster failed: 2.6.29 or newer kernel required for "
		       "multi-server DRI\n");
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "drmSetMaster failed: %s\n", strerror(errno));
	}
	return FALSE;
    }

    ms->isMaster = TRUE;
    return TRUE;
}


static void drv_load_palette(ScrnInfoPtr pScrn, int numColors,
			     int *indices, LOCO *colors, VisualPtr pVisual)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    modesettingPtr ms = modesettingPTR(pScrn);
    int index, j, i;
    int c;

    switch(pScrn->depth) {
    case 15:
	for (i = 0; i < numColors; i++) {
	    index = indices[i];
	    for (j = 0; j < 8; j++) {
		ms->lut_r[index * 8 + j] = colors[index].red << 8;
		ms->lut_g[index * 8 + j] = colors[index].green << 8;
		ms->lut_b[index * 8 + j] = colors[index].blue << 8;
	    }
	}
	break;
    case 16:
	for (i = 0; i < numColors; i++) {
	    index = indices[i];

	    if (index < 32) {
		for (j = 0; j < 8; j++) {
		    ms->lut_r[index * 8 + j] = colors[index].red << 8;
		    ms->lut_b[index * 8 + j] = colors[index].blue << 8;
		}
	    }

	    for (j = 0; j < 4; j++) {
		ms->lut_g[index * 4 + j] = colors[index].green << 8;
	    }
	}
	break;
    default:
	for (i = 0; i < numColors; i++) {
	    index = indices[i];
	    ms->lut_r[index] = colors[index].red << 8;
	    ms->lut_g[index] = colors[index].green << 8;
	    ms->lut_b[index] = colors[index].blue << 8;
	}
	break;
    }

    for (c = 0; c < xf86_config->num_crtc; c++) {
	xf86CrtcPtr crtc = xf86_config->crtc[c];

	/* Make the change through RandR */
#ifdef RANDR_12_INTERFACE
	if (crtc->randr_crtc)
	    RRCrtcGammaSet(crtc->randr_crtc, ms->lut_r, ms->lut_g, ms->lut_b);
	else
#endif
	    crtc->funcs->gamma_set(crtc, ms->lut_r, ms->lut_g, ms->lut_b, 256);
    }
}


static Bool
drv_screen_init(SCREEN_INIT_ARGS_DECL)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    VisualPtr visual;
    CustomizerPtr cust = ms->cust;
    MessageType from_st;
    MessageType from_dt;

    if (!drv_set_master(pScrn))
	return FALSE;

    if (!drv_init_front_buffer_functions(pScrn)) {
	FatalError("Could not init front buffer manager");
	return FALSE;
    }

    pScrn->pScreen = pScreen;

    /* HW dependent - FIXME */
    pScrn->displayWidth = pScrn->virtualX;

    miClearVisualTypes();

    if (!miSetVisualTypes(pScrn->depth,
			  miGetDefaultVisualMask(pScrn->depth),
			  pScrn->rgbBits, pScrn->defaultVisual))
	return FALSE;

    if (!miSetPixmapDepths())
	return FALSE;

    pScrn->memPhysBase = 0;
    pScrn->fbOffset = 0;

    if (!fbScreenInit(pScreen, NULL,
		      pScrn->virtualX, pScrn->virtualY,
		      pScrn->xDpi, pScrn->yDpi,
		      pScrn->displayWidth, pScrn->bitsPerPixel))
	return FALSE;

    if (pScrn->bitsPerPixel > 8) {
	/* Fixup RGB ordering */
	visual = pScreen->visuals + pScreen->numVisuals;
	while (--visual >= pScreen->visuals) {
	    if ((visual->class | DynamicClass) == DirectColor) {
		visual->offsetRed = pScrn->offset.red;
		visual->offsetGreen = pScrn->offset.green;
		visual->offsetBlue = pScrn->offset.blue;
		visual->redMask = pScrn->mask.red;
		visual->greenMask = pScrn->mask.green;
		visual->blueMask = pScrn->mask.blue;
	    }
	}
    }

    fbPictureInit(pScreen, NULL, 0);

    ms->blockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = drv_block_handler;
    ms->createScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = drv_create_screen_resources;

    xf86SetBlackWhitePixels(pScreen);

    ms->accelerate_2d = xf86ReturnOptValBool(ms->Options, OPTION_2D_ACCEL, FALSE);
    ms->debug_fallback = xf86ReturnOptValBool(ms->Options, OPTION_DEBUG_FALLBACK, ms->accelerate_2d);

    if (cust && cust->winsys_screen_init)
	cust->winsys_screen_init(cust);

    ms->swapThrottling = cust ?  cust->swap_throttling : TRUE;
    from_st = xf86GetOptValBool(ms->Options, OPTION_THROTTLE_SWAP,
				&ms->swapThrottling) ?
	X_CONFIG : X_DEFAULT;

    ms->dirtyThrottling = cust ?  cust->dirty_throttling : FALSE;
    from_dt = xf86GetOptValBool(ms->Options, OPTION_THROTTLE_DIRTY,
				&ms->dirtyThrottling) ?
	X_CONFIG : X_DEFAULT;

    if (ms->screen) {
	ms->exa = xorg_exa_init(pScrn, ms->accelerate_2d);

	xorg_xv_init(pScreen);
#ifdef DRI2
	xorg_dri2_init(pScreen);
#endif
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "#################################\n");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "# Useful debugging info follows #\n");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "#################################\n");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Using %s backend\n",
	       ms->screen ? "Gallium3D" : "libkms");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "2D Acceleration is %s\n",
	       ms->screen && ms->accelerate_2d ? "enabled" : "disabled");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Fallback debugging is %s\n",
	       ms->debug_fallback ? "enabled" : "disabled");
#ifdef DRI2
    xf86DrvMsg(pScrn->scrnIndex, ms->from_3D, "3D Acceleration is %s\n",
	       ms->screen ? "enabled" : "disabled");
#else
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "3D Acceleration is disabled\n");
#endif
    xf86DrvMsg(pScrn->scrnIndex, from_st, "Swap Throttling is %s.\n",
	       ms->swapThrottling ? "enabled" : "disabled");
    xf86DrvMsg(pScrn->scrnIndex, from_dt, "Dirty Throttling is %s.\n",
	       ms->dirtyThrottling ? "enabled" : "disabled");

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "##################################\n");

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* Need to extend HWcursor support to handle mask interleave */
    if (!ms->SWCursor)
	xf86_cursors_init(pScreen, 64, 64,
			  HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 |
			  HARDWARE_CURSOR_ARGB |
			  ((cust && cust->unhidden_hw_cursor_update) ?
			   HARDWARE_CURSOR_UPDATE_UNHIDDEN : 0));

    /* Must force it before EnterVT, so we are in control of VT and
     * later memory should be bound when allocating, e.g rotate_mem */
    pScrn->vtSema = TRUE;

    pScreen->SaveScreen = xf86SaveScreen;
    ms->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = drv_close_screen;

    if (!xf86CrtcScreenInit(pScreen))
	return FALSE;

    if (!miCreateDefColormap(pScreen))
	return FALSE;
    if (!xf86HandleColormaps(pScreen, 256, 8, drv_load_palette, NULL,
			     CMAP_PALETTED_TRUECOLOR |
			     CMAP_RELOAD_ON_MODE_SWITCH))
	return FALSE;

    xf86DPMSInit(pScreen, xf86DPMSSet, 0);

    if (serverGeneration == 1)
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

    return drv_enter_vt_flags(pScrn, 1);
}

static void
drv_adjust_frame(ADJUST_FRAME_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    xf86OutputPtr output = config->output[config->compat_output];
    xf86CrtcPtr crtc = output->crtc;

    if (crtc && crtc->enabled) {
	crtc->funcs->set_mode_major(crtc, pScrn->currentMode,
				    RR_Rotate_0, x, y);
	crtc->x = output->initial_x + x;
	crtc->y = output->initial_y + y;
    }
}

static void
drv_free_screen(FREE_SCREEN_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    drv_free_rec(pScrn);
}

static void
drv_leave_vt(VT_FUNC_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    modesettingPtr ms = modesettingPTR(pScrn);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    CustomizerPtr cust = ms->cust;
    int o;

    if (cust && cust->winsys_leave_vt)
	cust->winsys_leave_vt(cust);

    for (o = 0; o < config->num_crtc; o++) {
	xf86CrtcPtr crtc = config->crtc[o];

	xorg_crtc_cursor_destroy(crtc);

	if (crtc->rotatedPixmap || crtc->rotatedData) {
	    crtc->funcs->shadow_destroy(crtc, crtc->rotatedPixmap,
					crtc->rotatedData);
	    crtc->rotatedPixmap = NULL;
	    crtc->rotatedData = NULL;
	}
    }

    if (ms->fb_id != -1) {
	drmModeRmFB(ms->fd, ms->fb_id);
	ms->fb_id = -1;
    }

    /* idle hardware */
    if (!ms->kms)
	drv_cleanup_fences(pScrn);

    if (drmDropMaster(ms->fd))
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "drmDropMaster failed: %s\n", strerror(errno));

    ms->isMaster = FALSE;
    pScrn->vtSema = FALSE;
}

/*
 * This gets called when gaining control of the VT, and from ScreenInit().
 */
static Bool
drv_enter_vt_flags(ScrnInfoPtr pScrn, int flags)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    CustomizerPtr cust = ms->cust;

    if (!drv_set_master(pScrn))
	return FALSE;

    if (!ms->create_front_buffer(pScrn))
	return FALSE;

    if (!flags && !ms->bind_front_buffer(pScrn))
	return FALSE;

    if (!xf86SetDesiredModes(pScrn))
	return FALSE;

    if (cust && cust->winsys_enter_vt)
	cust->winsys_enter_vt(cust);

    return TRUE;
}

static Bool
drv_enter_vt(VT_FUNC_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    return drv_enter_vt_flags(pScrn, 0);
}

static Bool
drv_switch_mode(SWITCH_MODE_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);

    return xf86SetSingleMode(pScrn, mode, RR_Rotate_0);
}

static Bool
drv_close_screen(CLOSE_SCREEN_ARGS_DECL)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    CustomizerPtr cust = ms->cust;

    if (ms->cursor) {
       FreeCursor(ms->cursor, None);
       ms->cursor = NULL;
    }

    if (cust && cust->winsys_screen_close)
	cust->winsys_screen_close(cust);

#ifdef DRI2
    if (ms->screen)
	xorg_dri2_close(pScreen);
#endif

    pScreen->BlockHandler = ms->blockHandler;
    pScreen->CreateScreenResources = ms->createScreenResources;

#ifdef DRM_MODE_FEATURE_DIRTYFB
    if (ms->damage) {
	DamageUnregister(&pScreen->GetScreenPixmap(pScreen)->drawable, ms->damage);
	DamageDestroy(ms->damage);
	ms->damage = NULL;
    }
#endif

    ms->destroy_front_buffer(pScrn);

    if (ms->exa)
	xorg_exa_close(pScrn);
    ms->exa = NULL;

    /* calls drop master make sure we don't talk to 3D HW after that */
    if (pScrn->vtSema) {
	drv_leave_vt(VT_FUNC_ARGS);
    }

    pScrn->vtSema = FALSE;
    pScreen->CloseScreen = ms->CloseScreen;

    return (*pScreen->CloseScreen) (CLOSE_SCREEN_ARGS);
}

static ModeStatus
drv_valid_mode(SCRN_ARG_TYPE arg, DisplayModePtr mode, Bool verbose, int flags)
{
    return MODE_OK;
}


/*
 * Front buffer backing store functions.
 */

static Bool
drv_destroy_front_buffer_ga3d(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);

    if (!ms->root_texture)
	return TRUE;

    if (ms->fb_id != -1) {
	drmModeRmFB(ms->fd, ms->fb_id);
	ms->fb_id = -1;
    }

    pipe_resource_reference(&ms->root_texture, NULL);
    return TRUE;
}

static Bool
drv_create_front_buffer_ga3d(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    struct pipe_resource *tex;
    struct winsys_handle whandle;
    unsigned fb_id;
    int ret;

    ms->noEvict = TRUE;

    tex = xorg_exa_create_root_texture(pScrn, pScrn->virtualX, pScrn->virtualY,
				       pScrn->depth, pScrn->bitsPerPixel);

    if (!tex)
	return FALSE;

    memset(&whandle, 0, sizeof(whandle));
    whandle.type = DRM_API_HANDLE_TYPE_KMS;

    if (!ms->screen->resource_get_handle(ms->screen, tex, &whandle))
	goto err_destroy;

    ret = drmModeAddFB(ms->fd,
		       pScrn->virtualX,
		       pScrn->virtualY,
		       pScrn->depth,
		       pScrn->bitsPerPixel,
		       whandle.stride,
		       whandle.handle,
		       &fb_id);
    if (ret) {
	debug_printf("%s: failed to create framebuffer (%i, %s)\n",
		     __func__, ret, strerror(-ret));
	goto err_destroy;
    }

    if (!drv_destroy_front_buffer_ga3d(pScrn))
	FatalError("%s: failed to take down old framebuffer\n", __func__);

    pScrn->frameX0 = 0;
    pScrn->frameY0 = 0;
    drv_adjust_frame(ADJUST_FRAME_ARGS(pScrn, pScrn->frameX0, pScrn->frameY0));

    pipe_resource_reference(&ms->root_texture, tex);
    pipe_resource_reference(&tex, NULL);
    ms->fb_id = fb_id;

    return TRUE;

err_destroy:
    pipe_resource_reference(&tex, NULL);
    return FALSE;
}

static Bool
drv_bind_front_buffer_ga3d(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    struct pipe_resource *check;

    xorg_exa_set_displayed_usage(rootPixmap);
    xorg_exa_set_shared_usage(rootPixmap);
    xorg_exa_set_texture(rootPixmap, ms->root_texture);
    if (!pScreen->ModifyPixmapHeader(rootPixmap, -1, -1, -1, -1, -1, NULL))
	FatalError("Couldn't adjust screen pixmap\n");

    check = xorg_exa_get_texture(rootPixmap);
    if (ms->root_texture != check)
	FatalError("Created new root texture\n");

    pipe_resource_reference(&check, NULL);
    return TRUE;
}

#ifdef HAVE_LIBKMS
static Bool
drv_destroy_front_buffer_kms(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);

    /* XXX Do something with the rootPixmap.
     * This currently works fine but if we are getting crashes in
     * the fb functions after VT switches maybe look more into it.
     */
    (void)rootPixmap;

    if (!ms->root_bo)
	return TRUE;

    if (ms->fb_id != -1) {
	drmModeRmFB(ms->fd, ms->fb_id);
	ms->fb_id = -1;
    }

    kms_bo_unmap(ms->root_bo);
    kms_bo_destroy(&ms->root_bo);
    return TRUE;
}

static Bool
drv_create_front_buffer_kms(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    unsigned handle, stride;
    struct kms_bo *bo;
    unsigned attr[8];
    unsigned fb_id;
    int ret;

    attr[0] = KMS_BO_TYPE;
#ifdef KMS_BO_TYPE_SCANOUT_X8R8G8B8
    attr[1] = KMS_BO_TYPE_SCANOUT_X8R8G8B8;
#else
    attr[1] = KMS_BO_TYPE_SCANOUT;
#endif
    attr[2] = KMS_WIDTH;
    attr[3] = pScrn->virtualX;
    attr[4] = KMS_HEIGHT;
    attr[5] = pScrn->virtualY;
    attr[6] = 0;

    if (kms_bo_create(ms->kms, attr, &bo))
	return FALSE;

    if (kms_bo_get_prop(bo, KMS_PITCH, &stride))
	goto err_destroy;

    if (kms_bo_get_prop(bo, KMS_HANDLE, &handle))
	goto err_destroy;

    ret = drmModeAddFB(ms->fd,
		       pScrn->virtualX,
		       pScrn->virtualY,
		       pScrn->depth,
		       pScrn->bitsPerPixel,
		       stride,
		       handle,
		       &fb_id);
    if (ret) {
	debug_printf("%s: failed to create framebuffer (%i, %s)",
		     __func__, ret, strerror(-ret));
	goto err_destroy;
    }

    if (!drv_destroy_front_buffer_kms(pScrn))
	FatalError("%s: could not takedown old bo", __func__);

    pScrn->frameX0 = 0;
    pScrn->frameY0 = 0;
    drv_adjust_frame(ADJUST_FRAME_ARGS(pScrn, pScrn->frameX0, pScrn->frameY0));
    ms->root_bo = bo;
    ms->fb_id = fb_id;

    return TRUE;

err_destroy:
    kms_bo_destroy(&bo);
    return FALSE;
}

static Bool
drv_bind_front_buffer_kms(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    unsigned stride;
    void *ptr;

    if (kms_bo_get_prop(ms->root_bo, KMS_PITCH, &stride))
	return FALSE;

    if (kms_bo_map(ms->root_bo, &ptr))
	goto err_destroy;

    pScreen->ModifyPixmapHeader(rootPixmap,
				pScrn->virtualX,
				pScrn->virtualY,
				pScreen->rootDepth,
				pScrn->bitsPerPixel,
				stride,
				ptr);

#if (XORG_VERSION_CURRENT < XORG_VERSION_NUMERIC(1, 9, 99, 1, 0))

    /* This a hack to work around EnableDisableFBAccess setting the pointer
     * the real fix would be to replace pScrn->EnableDisableFBAccess hook
     * and set the rootPixmap->devPrivate.ptr to something valid before that.
     *
     * But in its infinit visdome something uses either this some times before
     * that, so our hook doesn't get called before the crash happens.
     */
    pScrn->pixmapPrivate.ptr = ptr;

#endif

    return TRUE;

err_destroy:
    kms_bo_destroy(&ms->root_bo);
    return FALSE;
}
#endif /* HAVE_LIBKMS */

static Bool drv_init_front_buffer_functions(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    if (ms->screen) {
	ms->destroy_front_buffer = drv_destroy_front_buffer_ga3d;
	ms->create_front_buffer = drv_create_front_buffer_ga3d;
	ms->bind_front_buffer = drv_bind_front_buffer_ga3d;
#ifdef HAVE_LIBKMS
    } else if (ms->kms) {
	ms->destroy_front_buffer = drv_destroy_front_buffer_kms;
	ms->create_front_buffer = drv_create_front_buffer_kms;
	ms->bind_front_buffer = drv_bind_front_buffer_kms;
#endif
    } else
	return FALSE;

    return TRUE;
}

CustomizerPtr xorg_customizer(ScrnInfoPtr pScrn)
{
    return modesettingPTR(pScrn)->cust;
}

Bool xorg_has_gallium(ScrnInfoPtr pScrn)
{
    return modesettingPTR(pScrn)->screen != NULL;
}

/* vim: set sw=4 ts=8 sts=4: */
