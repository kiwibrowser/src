#include "xorg_tracker.h"

#include <xf86.h>
#include <xf86xv.h>
#include <xf86xvmc.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/XvMC.h>
#include <fourcc.h>

#define FOURCC_RGB 0x0000003
#define XVIMAGE_RGB								\
{										\
	FOURCC_RGB,								\
	XvRGB,									\
	LSBFirst,								\
	{									\
		'R', 'G', 'B', 0x00,						\
		0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71	\
	},									\
	32,									\
	XvPacked,								\
	1,									\
	24, 0x00FF0000, 0x0000FF00, 0x000000FF,					\
	0, 0, 0,								\
	0, 0, 0,								\
	0, 0, 0,								\
	{									\
		'B','G','R','X',						\
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0		\
	},									\
	XvTopToBottom								\
}

static int subpicture_index_list[] = {
   FOURCC_RGB,
   FOURCC_IA44,
   FOURCC_AI44
};

static XF86MCImageIDList subpicture_list =
{
   sizeof(subpicture_index_list)/sizeof(*subpicture_index_list),
   subpicture_index_list
};

static XF86MCSurfaceInfoRec yv12_mpeg2_surface =
{
   FOURCC_I420,
   XVMC_CHROMA_FORMAT_420,
   0,
   2048, 2048, 2048, 2048,
   XVMC_IDCT | XVMC_MPEG_2,
   XVMC_SUBPICTURE_INDEPENDENT_SCALING | XVMC_BACKEND_SUBPICTURE,
   &subpicture_list
};

static const XF86MCSurfaceInfoRec uyvy_mpeg2_surface =
{
   FOURCC_UYVY,
   XVMC_CHROMA_FORMAT_422,
   0,
   2048, 2048, 2048, 2048,
   XVMC_IDCT | XVMC_MPEG_2,
   XVMC_SUBPICTURE_INDEPENDENT_SCALING | XVMC_BACKEND_SUBPICTURE,
   &subpicture_list
};

static XF86MCSurfaceInfoPtr surfaces[] =
{
   (XF86MCSurfaceInfoPtr)&yv12_mpeg2_surface,
   (XF86MCSurfaceInfoPtr)&uyvy_mpeg2_surface
};

static const XF86ImageRec rgb_subpicture = XVIMAGE_RGB;
static const XF86ImageRec ia44_subpicture = XVIMAGE_IA44;
static const XF86ImageRec ai44_subpicture = XVIMAGE_AI44;

static XF86ImagePtr subpictures[] =
{
   (XF86ImagePtr)&rgb_subpicture,
   (XF86ImagePtr)&ia44_subpicture,
   (XF86ImagePtr)&ai44_subpicture
};

static const XF86MCAdaptorRec adaptor_template =
{
   "",
   sizeof(surfaces)/sizeof(*surfaces),
   surfaces,
   sizeof(subpictures)/sizeof(*subpictures),
   subpictures,
   (xf86XvMCCreateContextProcPtr)NULL,
   (xf86XvMCDestroyContextProcPtr)NULL,
   (xf86XvMCCreateSurfaceProcPtr)NULL,
   (xf86XvMCDestroySurfaceProcPtr)NULL,
   (xf86XvMCCreateSubpictureProcPtr)NULL,
   (xf86XvMCDestroySubpictureProcPtr)NULL
};

void
xorg_xvmc_init(ScreenPtr pScreen, char *name)
{
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
   XF86MCAdaptorPtr adaptorXvMC = xf86XvMCCreateAdaptorRec();
   if (!adaptorXvMC)
      return;

   *adaptorXvMC = adaptor_template;
   adaptorXvMC->name = name;
   xf86DrvMsg(pScrn->scrnIndex, X_INFO,
              "[XvMC] Associated with %s.\n", name);
   if (!xf86XvMCScreenInit(pScreen, 1, &adaptorXvMC))
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                 "[XvMC] Failed to initialize extension.\n");
   else
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                 "[XvMC] Extension initialized.\n");
   xf86XvMCDestroyAdaptorRec(adaptorXvMC);
}
