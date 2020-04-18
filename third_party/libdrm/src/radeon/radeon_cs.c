#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libdrm_macros.h"
#include <stdio.h>
#include "radeon_cs.h"
#include "radeon_cs_int.h"

struct radeon_cs *
radeon_cs_create(struct radeon_cs_manager *csm, uint32_t ndw)
{
    struct radeon_cs_int *csi = csm->funcs->cs_create(csm, ndw);
    return (struct radeon_cs *)csi;
}

int
radeon_cs_write_reloc(struct radeon_cs *cs, struct radeon_bo *bo,
                      uint32_t read_domain, uint32_t write_domain,
                      uint32_t flags)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;

    return csi->csm->funcs->cs_write_reloc(csi,
                                           bo,
                                           read_domain,
                                           write_domain,
                                           flags);
}

int
radeon_cs_begin(struct radeon_cs *cs, uint32_t ndw,
                const char *file, const char *func, int line)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;
    return csi->csm->funcs->cs_begin(csi, ndw, file, func, line);
}

int
radeon_cs_end(struct radeon_cs *cs,
              const char *file, const char *func, int line)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;
    return csi->csm->funcs->cs_end(csi, file, func, line);
}

int radeon_cs_emit(struct radeon_cs *cs)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;
    return csi->csm->funcs->cs_emit(csi);
}

int radeon_cs_destroy(struct radeon_cs *cs)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;
    return csi->csm->funcs->cs_destroy(csi);
}

int radeon_cs_erase(struct radeon_cs *cs)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;
    return csi->csm->funcs->cs_erase(csi);
}

int radeon_cs_need_flush(struct radeon_cs *cs)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;
    return csi->csm->funcs->cs_need_flush(csi);
}

void radeon_cs_print(struct radeon_cs *cs, FILE *file)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;
    csi->csm->funcs->cs_print(csi, file);
}

void
radeon_cs_set_limit(struct radeon_cs *cs, uint32_t domain, uint32_t limit)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;
    if (domain == RADEON_GEM_DOMAIN_VRAM)
        csi->csm->vram_limit = limit;
    else
        csi->csm->gart_limit = limit;
}

void radeon_cs_space_set_flush(struct radeon_cs *cs, 
                                          void (*fn)(void *), void *data)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;
    csi->space_flush_fn = fn;
    csi->space_flush_data = data;
}

uint32_t radeon_cs_get_id(struct radeon_cs *cs)
{
    struct radeon_cs_int *csi = (struct radeon_cs_int *)cs;
    return csi->id;
}
