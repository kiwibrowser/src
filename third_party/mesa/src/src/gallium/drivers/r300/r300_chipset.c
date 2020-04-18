/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2011 Marek Olšák <maraeo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "r300_chipset.h"

#include "util/u_debug.h"

#include <stdio.h>

/* r300_chipset: A file all to itself for deducing the various properties of
 * Radeons. */

/* Parse a PCI ID and fill an r300_capabilities struct with information. */
void r300_parse_chipset(uint32_t pci_id, struct r300_capabilities* caps)
{
    switch (pci_id) {
#define CHIPSET(pci_id, name, chipfamily) \
        case pci_id: \
            caps->family = CHIP_FAMILY_##chipfamily; \
            break;
#include "pci_ids/r300_pci_ids.h"
#undef CHIPSET

    default:
        fprintf(stderr, "r300: Warning: Unknown chipset 0x%x\nAborting...",
                pci_id);
        abort();
    }

    /* Defaults. */
    caps->high_second_pipe = FALSE;
    caps->num_vert_fpus = 0;
    caps->hiz_ram = 0;
    caps->zmask_ram = 0;


    switch (caps->family) {
    case CHIP_FAMILY_R300:
    case CHIP_FAMILY_R350:
        caps->high_second_pipe = TRUE;
        caps->num_vert_fpus = 4;
        caps->hiz_ram = R300_HIZ_LIMIT;
        caps->zmask_ram = PIPE_ZMASK_SIZE;
        break;

    case CHIP_FAMILY_RV350:
    case CHIP_FAMILY_RV370:
        caps->high_second_pipe = TRUE;
        caps->num_vert_fpus = 2;
        caps->zmask_ram = RV3xx_ZMASK_SIZE;
        break;

    case CHIP_FAMILY_RV380:
        caps->high_second_pipe = TRUE;
        caps->num_vert_fpus = 2;
        caps->hiz_ram = R300_HIZ_LIMIT;
        caps->zmask_ram = RV3xx_ZMASK_SIZE;
        break;

    case CHIP_FAMILY_RS400:
    case CHIP_FAMILY_RS600:
    case CHIP_FAMILY_RS690:
    case CHIP_FAMILY_RS740:
        break;

    case CHIP_FAMILY_RC410:
    case CHIP_FAMILY_RS480:
        caps->zmask_ram = RV3xx_ZMASK_SIZE;
        break;

    case CHIP_FAMILY_R420:
    case CHIP_FAMILY_R423:
    case CHIP_FAMILY_R430:
    case CHIP_FAMILY_R480:
    case CHIP_FAMILY_R481:
    case CHIP_FAMILY_RV410:
        caps->num_vert_fpus = 6;
        caps->hiz_ram = R300_HIZ_LIMIT;
        caps->zmask_ram = PIPE_ZMASK_SIZE;
        break;

    case CHIP_FAMILY_R520:
        caps->num_vert_fpus = 8;
        caps->hiz_ram = R300_HIZ_LIMIT;
        caps->zmask_ram = PIPE_ZMASK_SIZE;
        break;

    case CHIP_FAMILY_RV515:
        caps->num_vert_fpus = 2;
        caps->hiz_ram = R300_HIZ_LIMIT;
        caps->zmask_ram = PIPE_ZMASK_SIZE;
        break;

    case CHIP_FAMILY_RV530:
        caps->num_vert_fpus = 5;
        caps->hiz_ram = RV530_HIZ_LIMIT;
        caps->zmask_ram = PIPE_ZMASK_SIZE;
        break;

    case CHIP_FAMILY_R580:
    case CHIP_FAMILY_RV560:
    case CHIP_FAMILY_RV570:
        caps->num_vert_fpus = 8;
        caps->hiz_ram = RV530_HIZ_LIMIT;
        caps->zmask_ram = PIPE_ZMASK_SIZE;
        break;
    }

    caps->num_tex_units = 16;
    caps->is_r400 = caps->family >= CHIP_FAMILY_R420 && caps->family < CHIP_FAMILY_RV515;
    caps->is_r500 = caps->family >= CHIP_FAMILY_RV515;
    caps->is_rv350 = caps->family >= CHIP_FAMILY_RV350;
    caps->z_compress = caps->is_rv350 ? R300_ZCOMP_8X8 : R300_ZCOMP_4X4;
    caps->dxtc_swizzle = caps->is_r400 || caps->is_r500;
    caps->has_us_format = caps->family == CHIP_FAMILY_R520;
    caps->has_tcl = caps->num_vert_fpus > 0;

    if (caps->has_tcl) {
        caps->has_tcl = debug_get_bool_option("RADEON_NO_TCL", FALSE) ? FALSE : TRUE;
    }
}
