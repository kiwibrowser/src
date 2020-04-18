/*
 * Copyright © 2009 Corbin Simpson
 * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 *      Joakim Sindholt <opensource@zhasha.com>
 *      Marek Olšák <maraeo@gmail.com>
 */

#include "radeon_drm_bo.h"
#include "radeon_drm_cs.h"
#include "radeon_drm_public.h"

#include "pipebuffer/pb_bufmgr.h"
#include "util/u_memory.h"

#include <xf86drm.h>
#include <stdio.h>

/*
 * this are copy from radeon_drm, once an updated libdrm is released
 * we should bump configure.ac requirement for it and remove the following
 * field
 */
#ifndef RADEON_INFO_TILING_CONFIG
#define RADEON_INFO_TILING_CONFIG 6
#endif

#ifndef RADEON_INFO_WANT_HYPERZ
#define RADEON_INFO_WANT_HYPERZ 7
#endif

#ifndef RADEON_INFO_WANT_CMASK
#define RADEON_INFO_WANT_CMASK 8
#endif

#ifndef RADEON_INFO_CLOCK_CRYSTAL_FREQ
#define RADEON_INFO_CLOCK_CRYSTAL_FREQ 9
#endif

#ifndef RADEON_INFO_NUM_BACKENDS
#define RADEON_INFO_NUM_BACKENDS 0xa
#endif

#ifndef RADEON_INFO_NUM_TILE_PIPES
#define RADEON_INFO_NUM_TILE_PIPES 0xb
#endif

#ifndef RADEON_INFO_BACKEND_MAP
#define RADEON_INFO_BACKEND_MAP 0xd
#endif

#ifndef RADEON_INFO_VA_START
/* virtual address start, va < start are reserved by the kernel */
#define RADEON_INFO_VA_START        0x0e
/* maximum size of ib using the virtual memory cs */
#define RADEON_INFO_IB_VM_MAX_SIZE  0x0f
#endif

#ifndef RADEON_INFO_MAX_PIPES
#define RADEON_INFO_MAX_PIPES 0x10
#endif

#ifndef RADEON_INFO_TIMESTAMP
#define RADEON_INFO_TIMESTAMP 0x11
#endif


/* Enable/disable feature access for one command stream.
 * If enable == TRUE, return TRUE on success.
 * Otherwise, return FALSE.
 *
 * We basically do the same thing kernel does, because we have to deal
 * with multiple contexts (here command streams) backed by one winsys. */
static boolean radeon_set_fd_access(struct radeon_drm_cs *applier,
                                    struct radeon_drm_cs **owner,
                                    pipe_mutex *mutex,
                                    unsigned request, boolean enable)
{
    struct drm_radeon_info info;
    unsigned value = enable ? 1 : 0;

    memset(&info, 0, sizeof(info));

    pipe_mutex_lock(*mutex);

    /* Early exit if we are sure the request will fail. */
    if (enable) {
        if (*owner) {
            pipe_mutex_unlock(*mutex);
            return FALSE;
        }
    } else {
        if (*owner != applier) {
            pipe_mutex_unlock(*mutex);
            return FALSE;
        }
    }

    /* Pass through the request to the kernel. */
    info.value = (unsigned long)&value;
    info.request = request;
    if (drmCommandWriteRead(applier->ws->fd, DRM_RADEON_INFO,
                            &info, sizeof(info)) != 0) {
        pipe_mutex_unlock(*mutex);
        return FALSE;
    }

    /* Update the rights in the winsys. */
    if (enable) {
        if (value) {
            *owner = applier;
            fprintf(stderr, "radeon: Acquired Hyper-Z.\n");
            pipe_mutex_unlock(*mutex);
            return TRUE;
        }
    } else {
        *owner = NULL;
        fprintf(stderr, "radeon: Released Hyper-Z.\n");
    }

    pipe_mutex_unlock(*mutex);
    return FALSE;
}

static boolean radeon_get_drm_value(int fd, unsigned request,
                                    const char *errname, uint32_t *out)
{
    struct drm_radeon_info info;
    int retval;

    memset(&info, 0, sizeof(info));

    info.value = (unsigned long)out;
    info.request = request;

    retval = drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info));
    if (retval) {
        if (errname) {
            fprintf(stderr, "radeon: Failed to get %s, error number %d\n",
                    errname, retval);
        }
        return FALSE;
    }
    return TRUE;
}

/* Helper function to do the ioctls needed for setup and init. */
static boolean do_winsys_init(struct radeon_drm_winsys *ws)
{
    struct drm_radeon_gem_info gem_info;
    int retval;
    drmVersionPtr version;

    memset(&gem_info, 0, sizeof(gem_info));

    /* We do things in a specific order here.
     *
     * DRM version first. We need to be sure we're running on a KMS chipset.
     * This is also for some features.
     *
     * Then, the PCI ID. This is essential and should return usable numbers
     * for all Radeons. If this fails, we probably got handed an FD for some
     * non-Radeon card.
     *
     * The GEM info is actually bogus on the kernel side, as well as our side
     * (see radeon_gem_info_ioctl in radeon_gem.c) but that's alright because
     * we don't actually use the info for anything yet.
     *
     * The GB and Z pipe requests should always succeed, but they might not
     * return sensical values for all chipsets, but that's alright because
     * the pipe drivers already know that.
     */

    /* Get DRM version. */
    version = drmGetVersion(ws->fd);
    if (version->version_major != 2 ||
        version->version_minor < 3) {
        fprintf(stderr, "%s: DRM version is %d.%d.%d but this driver is "
                "only compatible with 2.3.x (kernel 2.6.34) or later.\n",
                __FUNCTION__,
                version->version_major,
                version->version_minor,
                version->version_patchlevel);
        drmFreeVersion(version);
        return FALSE;
    }

    ws->info.drm_major = version->version_major;
    ws->info.drm_minor = version->version_minor;
    ws->info.drm_patchlevel = version->version_patchlevel;
    drmFreeVersion(version);

    /* Get PCI ID. */
    if (!radeon_get_drm_value(ws->fd, RADEON_INFO_DEVICE_ID, "PCI ID",
                              &ws->info.pci_id))
        return FALSE;

    /* Check PCI ID. */
    switch (ws->info.pci_id) {
#define CHIPSET(pci_id, name, family) case pci_id:
#include "pci_ids/r300_pci_ids.h"
#undef CHIPSET
        ws->gen = R300;
        break;

#define CHIPSET(pci_id, name, family) case pci_id:
#include "pci_ids/r600_pci_ids.h"
#undef CHIPSET
        ws->gen = R600;
        break;

#define CHIPSET(pci_id, name, family) case pci_id:
#include "pci_ids/radeonsi_pci_ids.h"
#undef CHIPSET
        ws->gen = SI;
        break;

    default:
        fprintf(stderr, "radeon: Invalid PCI ID.\n");
        return FALSE;
    }

    /* Get GEM info. */
    retval = drmCommandWriteRead(ws->fd, DRM_RADEON_GEM_INFO,
            &gem_info, sizeof(gem_info));
    if (retval) {
        fprintf(stderr, "radeon: Failed to get MM info, error number %d\n",
                retval);
        return FALSE;
    }
    ws->info.gart_size = gem_info.gart_size;
    ws->info.vram_size = gem_info.vram_size;

    ws->num_cpus = sysconf(_SC_NPROCESSORS_ONLN);

    /* Generation-specific queries. */
    if (ws->gen == R300) {
        if (!radeon_get_drm_value(ws->fd, RADEON_INFO_NUM_GB_PIPES,
                                  "GB pipe count",
                                  &ws->info.r300_num_gb_pipes))
            return FALSE;

        if (!radeon_get_drm_value(ws->fd, RADEON_INFO_NUM_Z_PIPES,
                                  "Z pipe count",
                                  &ws->info.r300_num_z_pipes))
            return FALSE;
    }
    else if (ws->gen >= R600) {
        if (ws->info.drm_minor >= 9 &&
            !radeon_get_drm_value(ws->fd, RADEON_INFO_NUM_BACKENDS,
                                  "num backends",
                                  &ws->info.r600_num_backends))
            return FALSE;

        /* get the GPU counter frequency, failure is not fatal */
        radeon_get_drm_value(ws->fd, RADEON_INFO_CLOCK_CRYSTAL_FREQ, NULL,
                             &ws->info.r600_clock_crystal_freq);

        radeon_get_drm_value(ws->fd, RADEON_INFO_TILING_CONFIG, NULL,
                             &ws->info.r600_tiling_config);

        if (ws->info.drm_minor >= 11) {
            radeon_get_drm_value(ws->fd, RADEON_INFO_NUM_TILE_PIPES, NULL,
                                 &ws->info.r600_num_tile_pipes);

            if (radeon_get_drm_value(ws->fd, RADEON_INFO_BACKEND_MAP, NULL,
                                      &ws->info.r600_backend_map))
                ws->info.r600_backend_map_valid = TRUE;
        }

        ws->info.r600_virtual_address = FALSE;
        if (ws->info.drm_minor >= 13) {
            ws->info.r600_virtual_address = TRUE;
            if (!radeon_get_drm_value(ws->fd, RADEON_INFO_VA_START, NULL,
                                      &ws->info.r600_va_start))
                ws->info.r600_virtual_address = FALSE;
            if (!radeon_get_drm_value(ws->fd, RADEON_INFO_IB_VM_MAX_SIZE, NULL,
                                      &ws->info.r600_ib_vm_max_size))
                ws->info.r600_virtual_address = FALSE;
        }
        /* Remove this line once the virtual address space feature is fixed. */
        if (ws->gen == R600 && !debug_get_bool_option("RADEON_VA", FALSE))
            ws->info.r600_virtual_address = FALSE;
    }

    /* Get max pipes, this is only needed for compute shaders.  All evergreen+
     * chips have at least 2 pipes, so we use 2 as a default. */
    ws->info.r600_max_pipes = 2;
    radeon_get_drm_value(ws->fd, RADEON_INFO_MAX_PIPES, NULL,
                         &ws->info.r600_max_pipes);

    return TRUE;
}

static void radeon_winsys_destroy(struct radeon_winsys *rws)
{
    struct radeon_drm_winsys *ws = (struct radeon_drm_winsys*)rws;

    pipe_mutex_destroy(ws->hyperz_owner_mutex);
    pipe_mutex_destroy(ws->cmask_owner_mutex);

    ws->cman->destroy(ws->cman);
    ws->kman->destroy(ws->kman);
    if (ws->gen >= R600) {
        radeon_surface_manager_free(ws->surf_man);
    }
    FREE(rws);
}

static void radeon_query_info(struct radeon_winsys *rws,
                              struct radeon_info *info)
{
    *info = ((struct radeon_drm_winsys *)rws)->info;
}

static boolean radeon_cs_request_feature(struct radeon_winsys_cs *rcs,
                                         enum radeon_feature_id fid,
                                         boolean enable)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);

    switch (fid) {
    case RADEON_FID_R300_HYPERZ_ACCESS:
        if (debug_get_bool_option("RADEON_HYPERZ", FALSE)) {
            return radeon_set_fd_access(cs, &cs->ws->hyperz_owner,
                                        &cs->ws->hyperz_owner_mutex,
                                        RADEON_INFO_WANT_HYPERZ, enable);
        } else {
            return FALSE;
        }

    case RADEON_FID_R300_CMASK_ACCESS:
        if (debug_get_bool_option("RADEON_CMASK", FALSE)) {
            return radeon_set_fd_access(cs, &cs->ws->cmask_owner,
                                        &cs->ws->cmask_owner_mutex,
                                        RADEON_INFO_WANT_CMASK, enable);
        } else {
            return FALSE;
        }
    }
    return FALSE;
}

static int radeon_drm_winsys_surface_init(struct radeon_winsys *rws,
                                          struct radeon_surface *surf)
{
    struct radeon_drm_winsys *ws = (struct radeon_drm_winsys*)rws;

    return radeon_surface_init(ws->surf_man, surf);
}

static int radeon_drm_winsys_surface_best(struct radeon_winsys *rws,
                                          struct radeon_surface *surf)
{
    struct radeon_drm_winsys *ws = (struct radeon_drm_winsys*)rws;

    return radeon_surface_best(ws->surf_man, surf);
}

static uint64_t radeon_query_timestamp(struct radeon_winsys *rws)
{
    struct radeon_drm_winsys *ws = (struct radeon_drm_winsys*)rws;
    uint64_t ts = 0;

    if (ws->info.drm_minor < 20 ||
        ws->gen < R600) {
        assert(0);
        return 0;
    }

    radeon_get_drm_value(ws->fd, RADEON_INFO_TIMESTAMP, "timestamp",
                         (uint32_t*)&ts);
    return ts;
}

struct radeon_winsys *radeon_drm_winsys_create(int fd)
{
    struct radeon_drm_winsys *ws = CALLOC_STRUCT(radeon_drm_winsys);
    if (!ws) {
        return NULL;
    }

    ws->fd = fd;

    if (!do_winsys_init(ws))
        goto fail;

    /* Create managers. */
    ws->kman = radeon_bomgr_create(ws);
    if (!ws->kman)
        goto fail;
    ws->cman = pb_cache_manager_create(ws->kman, 1000000);
    if (!ws->cman)
        goto fail;

    if (ws->gen >= R600) {
        ws->surf_man = radeon_surface_manager_new(fd);
        if (!ws->surf_man)
            goto fail;
    }

    /* Set functions. */
    ws->base.destroy = radeon_winsys_destroy;
    ws->base.query_info = radeon_query_info;
    ws->base.cs_request_feature = radeon_cs_request_feature;
    ws->base.surface_init = radeon_drm_winsys_surface_init;
    ws->base.surface_best = radeon_drm_winsys_surface_best;
    ws->base.query_timestamp = radeon_query_timestamp;

    radeon_bomgr_init_functions(ws);
    radeon_drm_cs_init_functions(ws);

    pipe_mutex_init(ws->hyperz_owner_mutex);
    pipe_mutex_init(ws->cmask_owner_mutex);

    return &ws->base;

fail:
    if (ws->cman)
        ws->cman->destroy(ws->cman);
    if (ws->kman)
        ws->kman->destroy(ws->kman);
    if (ws->surf_man)
        radeon_surface_manager_free(ws->surf_man);
    FREE(ws);
    return NULL;
}
