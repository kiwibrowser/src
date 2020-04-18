#ifndef XORG_EXA_TGSI_H
#define XORG_EXA_TGSI_H

#include "xorg_renderer.h"

enum xorg_vs_traits {
   VS_COMPOSITE        = 1 << 0,
   VS_MASK             = 1 << 1,
   VS_SOLID_FILL       = 1 << 2,
   VS_LINGRAD_FILL     = 1 << 3,
   VS_RADGRAD_FILL     = 1 << 4,
   VS_YUV              = 1 << 5,


   VS_FILL             = (VS_SOLID_FILL |
                          VS_LINGRAD_FILL |
                          VS_RADGRAD_FILL)
};

enum xorg_fs_traits {
   FS_COMPOSITE        = 1 << 0,
   FS_MASK             = 1 << 1,
   FS_SOLID_FILL       = 1 << 2,
   FS_LINGRAD_FILL     = 1 << 3,
   FS_RADGRAD_FILL     = 1 << 4,
   FS_CA_FULL          = 1 << 5, /* src.rgba * mask.rgba */
   FS_CA_SRCALPHA      = 1 << 6, /* src.aaaa * mask.rgba */
   FS_YUV              = 1 << 7,
   FS_SRC_REPEAT_NONE  = 1 << 8,
   FS_MASK_REPEAT_NONE = 1 << 9,
   FS_SRC_SWIZZLE_RGB  = 1 << 10,
   FS_MASK_SWIZZLE_RGB = 1 << 11,
   FS_SRC_SET_ALPHA    = 1 << 12,
   FS_MASK_SET_ALPHA   = 1 << 13,
   FS_SRC_LUMINANCE    = 1 << 14,
   FS_MASK_LUMINANCE   = 1 << 15,

   FS_FILL             = (FS_SOLID_FILL |
                          FS_LINGRAD_FILL |
                          FS_RADGRAD_FILL),
   FS_COMPONENT_ALPHA  = (FS_CA_FULL |
                          FS_CA_SRCALPHA)
};

struct xorg_shader {
   void *fs;
   void *vs;
};

struct xorg_shaders;

struct xorg_shaders *xorg_shaders_create(struct xorg_renderer *renderer);
void xorg_shaders_destroy(struct xorg_shaders *shaders);

struct xorg_shader xorg_shaders_get(struct xorg_shaders *shaders,
                                    unsigned vs_traits,
                                    unsigned fs_traits);

#endif
