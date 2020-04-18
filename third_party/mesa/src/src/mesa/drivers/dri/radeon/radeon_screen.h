/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 */

#ifndef __RADEON_SCREEN_H__
#define __RADEON_SCREEN_H__

/*
 * IMPORTS: these headers contain all the DRI, X and kernel-related
 * definitions that we need.
 */
#include "dri_util.h"
#include "radeon_dri.h"
#include "radeon_chipset.h"
#include "radeon_reg.h"
#include "drm_sarea.h"
#include "xmlconfig.h"


typedef struct {
   drm_handle_t handle;			/* Handle to the DRM region */
   drmSize size;			/* Size of the DRM region */
   drmAddress map;			/* Mapping of the DRM region */
} radeonRegionRec, *radeonRegionPtr;

typedef struct radeon_screen {
   int chip_family;
   int chip_flags;
   int cpp;
   int card_type;
   int device_id; /* PCI ID */
   int AGPMode;
   unsigned int irq;			/* IRQ number (0 means none) */

   unsigned int fbLocation;
   unsigned int frontOffset;
   unsigned int frontPitch;
   unsigned int backOffset;
   unsigned int backPitch;

   unsigned int depthOffset;
   unsigned int depthPitch;

    /* Shared texture data */
   int numTexHeaps;
   int texOffset[RADEON_NR_TEX_HEAPS];
   int texSize[RADEON_NR_TEX_HEAPS];
   int logTexGranularity[RADEON_NR_TEX_HEAPS];

   radeonRegionRec mmio;
   radeonRegionRec status;
   radeonRegionRec gartTextures;

   drmBufMapPtr buffers;

   __volatile__ uint32_t *scratch;

   __DRIscreen *driScreen;
   unsigned int sarea_priv_offset;
   unsigned int gart_buffer_offset;	/* offset in card memory space */
   unsigned int gart_texture_offset;	/* offset in card memory space */
   unsigned int gart_base;

   GLboolean depthHasSurface;

   /* Configuration cache with default values for all contexts */
   driOptionCache optionCache;

   const __DRIextension *extensions[17];

   int num_gb_pipes;
   int num_z_pipes;
   drm_radeon_sarea_t *sarea;	/* Private SAREA data */
   struct radeon_bo_manager *bom;

} radeonScreenRec, *radeonScreenPtr;

struct __DRIimageRec {
   struct radeon_bo *bo;
   GLenum internal_format;
   uint32_t dri_format;
   GLuint format;
   GLenum data_type;
   int width, height;  /* in pixels */
   int pitch;          /* in pixels */
   int cpp;
   void *data;
};

extern void radeonDestroyBuffer(__DRIdrawable *driDrawPriv);
#endif /* __RADEON_SCREEN_H__ */
