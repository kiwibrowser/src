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

#include "../../state_trackers/xorg/xorg_winsys.h"
#include <nouveau.h>
#include <xorg/dri.h>
#include <xf86drmMode.h>

static void nouveau_xorg_identify(int flags);
static Bool nouveau_xorg_pci_probe(DriverPtr driver, int entity_num,
				   struct pci_device *device,
				   intptr_t match_data);

static const struct pci_id_match nouveau_xorg_device_match[] = {
    { 0x10de, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY,
      0x00030000, 0x00ffffff, 0 },
    {0, 0, 0},
};

static PciChipsets nouveau_xorg_pci_devices[] = {
    {PCI_MATCH_ANY, PCI_MATCH_ANY, NULL},
    {-1, -1, NULL}
};

static XF86ModuleVersionInfo nouveau_xorg_version = {
    "nouveau2",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    0, 1, 0, /* major, minor, patch */
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}
};

/*
 * Xorg driver exported structures
 */

_X_EXPORT DriverRec nouveau2 = {
    1,
    "nouveau2",
    nouveau_xorg_identify,
    NULL,
    xorg_tracker_available_options,
    NULL,
    0,
    NULL,
    nouveau_xorg_device_match,
    nouveau_xorg_pci_probe
};

static MODULESETUPPROTO(nouveau_xorg_setup);

_X_EXPORT XF86ModuleData nouveau2ModuleData = {
    &nouveau_xorg_version,
    nouveau_xorg_setup,
    NULL
};

/*
 * Xorg driver functions
 */

static pointer
nouveau_xorg_setup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = 0;

    /* This module should be loaded only once, but check to be sure.
     */
    if (!setupDone) {
	setupDone = 1;
	xf86AddDriver(&nouveau2, module, HaveDriverFuncs);

	/*
	 * The return value must be non-NULL on success even though there
	 * is no TearDownProc.
	 */
	return (pointer) 1;
    } else {
	if (errmaj)
	    *errmaj = LDR_ONCEONLY;
	return NULL;
    }
}

static void
nouveau_xorg_identify(int flags)
{
    xf86DrvMsg(0, X_INFO, "nouveau2: Gallium3D based 2D driver for NV30+ NVIDIA chipsets\n");
}

static Bool
nouveau_xorg_pci_probe(DriverPtr driver,
	  int entity_num, struct pci_device *device, intptr_t match_data)
{
    ScrnInfoPtr scrn = NULL;
    EntityInfoPtr entity;
    struct nouveau_device *dev = NULL;
    char *busid;
    int chipset, ret;

    if (device->vendor_id != 0x10DE)
	return FALSE;

    if (!xf86LoaderCheckSymbol("DRICreatePCIBusID")) {
	xf86DrvMsg(-1, X_ERROR, "[drm] No DRICreatePCIBusID symbol\n");
	return FALSE;
    }
    busid = DRICreatePCIBusID(device);

    ret = nouveau_device_open(busid, &dev);
    if (ret) {
	xf86DrvMsg(-1, X_ERROR, "[drm] failed to open device\n");
	free(busid);
	return FALSE;
    }

    chipset = dev->chipset;
    nouveau_device_del(&dev);

    ret = drmCheckModesettingSupported(busid);
    free(busid);
    if (ret) {
	xf86DrvMsg(-1, X_ERROR, "[drm] KMS not enabled\n");
	return FALSE;
    }

    switch (chipset & 0xf0) {
    case 0x00:
    case 0x10:
    case 0x20:
	xf86DrvMsg(-1, X_NOTICE, "Too old chipset: NV%02x\n", chipset);
	return FALSE;
    case 0x30:
    case 0x40:
    case 0x60:
    case 0x50:
    case 0x80:
    case 0x90:
    case 0xa0:
    case 0xc0:
	xf86DrvMsg(-1, X_INFO, "Detected chipset: NV%02x\n", chipset);
	break;
    default:
	xf86DrvMsg(-1, X_ERROR, "Unknown chipset: NV%02x\n", chipset);
	return FALSE;
    }

    scrn = xf86ConfigPciEntity(scrn, 0, entity_num, nouveau_xorg_pci_devices,
			       NULL, NULL, NULL, NULL, NULL);
    if (scrn != NULL) {
	scrn->driverVersion = 1;
	scrn->driverName = "nouveau";
	scrn->name = "nouveau2";
	scrn->Probe = NULL;

	entity = xf86GetEntityInfo(entity_num);

	/* Use all the functions from the xorg tracker */
	xorg_tracker_set_functions(scrn);
    }
    return scrn != NULL;
}
