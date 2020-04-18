#ifndef _RADEON_CHIPSET_H
#define _RADEON_CHIPSET_H

/* General chip classes:
 * r100 includes R100, RV100, RV200, RS100, RS200, RS250.
 * r200 includes R200, RV250, RV280, RS300.
 * (RS* denotes IGP)
 */

enum {
#define CHIPSET(id, name, family) PCI_CHIP_##name = id,
#if defined(RADEON_R100)
#include "pci_ids/radeon_pci_ids.h"
#elif defined(RADEON_R200)
#include "pci_ids/r200_pci_ids.h"
#endif
#undef CHIPSET
};

enum {
#if defined(RADEON_R100)
   CHIP_FAMILY_R100,
   CHIP_FAMILY_RV100,
   CHIP_FAMILY_RS100,
   CHIP_FAMILY_RV200,
   CHIP_FAMILY_RS200,
#elif defined(RADEON_R200)
   CHIP_FAMILY_R200,
   CHIP_FAMILY_RV250,
   CHIP_FAMILY_RS300,
   CHIP_FAMILY_RV280,
#endif
   CHIP_FAMILY_LAST
};

#define RADEON_CHIPSET_TCL		(1 << 0)	/* tcl support - any radeon */
#define RADEON_CHIPSET_BROKEN_STENCIL	(1 << 1)	/* r100 stencil bug */
#define R200_CHIPSET_YCBCR_BROKEN	(1 << 2)	/* r200 ycbcr bug */
#define RADEON_CHIPSET_DEPTH_ALWAYS_TILED (1 << 3)      /* M7 and R200s */

#endif /* _RADEON_CHIPSET_H */
