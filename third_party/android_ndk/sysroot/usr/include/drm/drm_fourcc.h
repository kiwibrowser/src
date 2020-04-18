/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef DRM_FOURCC_H
#define DRM_FOURCC_H
#include "drm.h"
#ifdef __cplusplus
#endif
#define fourcc_code(a,b,c,d) ((__u32) (a) | ((__u32) (b) << 8) | ((__u32) (c) << 16) | ((__u32) (d) << 24))
#define DRM_FORMAT_BIG_ENDIAN (1 << 31)
#define DRM_FORMAT_C8 fourcc_code('C', '8', ' ', ' ')
#define DRM_FORMAT_R8 fourcc_code('R', '8', ' ', ' ')
#define DRM_FORMAT_R16 fourcc_code('R', '1', '6', ' ')
#define DRM_FORMAT_RG88 fourcc_code('R', 'G', '8', '8')
#define DRM_FORMAT_GR88 fourcc_code('G', 'R', '8', '8')
#define DRM_FORMAT_RG1616 fourcc_code('R', 'G', '3', '2')
#define DRM_FORMAT_GR1616 fourcc_code('G', 'R', '3', '2')
#define DRM_FORMAT_RGB332 fourcc_code('R', 'G', 'B', '8')
#define DRM_FORMAT_BGR233 fourcc_code('B', 'G', 'R', '8')
#define DRM_FORMAT_XRGB4444 fourcc_code('X', 'R', '1', '2')
#define DRM_FORMAT_XBGR4444 fourcc_code('X', 'B', '1', '2')
#define DRM_FORMAT_RGBX4444 fourcc_code('R', 'X', '1', '2')
#define DRM_FORMAT_BGRX4444 fourcc_code('B', 'X', '1', '2')
#define DRM_FORMAT_ARGB4444 fourcc_code('A', 'R', '1', '2')
#define DRM_FORMAT_ABGR4444 fourcc_code('A', 'B', '1', '2')
#define DRM_FORMAT_RGBA4444 fourcc_code('R', 'A', '1', '2')
#define DRM_FORMAT_BGRA4444 fourcc_code('B', 'A', '1', '2')
#define DRM_FORMAT_XRGB1555 fourcc_code('X', 'R', '1', '5')
#define DRM_FORMAT_XBGR1555 fourcc_code('X', 'B', '1', '5')
#define DRM_FORMAT_RGBX5551 fourcc_code('R', 'X', '1', '5')
#define DRM_FORMAT_BGRX5551 fourcc_code('B', 'X', '1', '5')
#define DRM_FORMAT_ARGB1555 fourcc_code('A', 'R', '1', '5')
#define DRM_FORMAT_ABGR1555 fourcc_code('A', 'B', '1', '5')
#define DRM_FORMAT_RGBA5551 fourcc_code('R', 'A', '1', '5')
#define DRM_FORMAT_BGRA5551 fourcc_code('B', 'A', '1', '5')
#define DRM_FORMAT_RGB565 fourcc_code('R', 'G', '1', '6')
#define DRM_FORMAT_BGR565 fourcc_code('B', 'G', '1', '6')
#define DRM_FORMAT_RGB888 fourcc_code('R', 'G', '2', '4')
#define DRM_FORMAT_BGR888 fourcc_code('B', 'G', '2', '4')
#define DRM_FORMAT_XRGB8888 fourcc_code('X', 'R', '2', '4')
#define DRM_FORMAT_XBGR8888 fourcc_code('X', 'B', '2', '4')
#define DRM_FORMAT_RGBX8888 fourcc_code('R', 'X', '2', '4')
#define DRM_FORMAT_BGRX8888 fourcc_code('B', 'X', '2', '4')
#define DRM_FORMAT_ARGB8888 fourcc_code('A', 'R', '2', '4')
#define DRM_FORMAT_ABGR8888 fourcc_code('A', 'B', '2', '4')
#define DRM_FORMAT_RGBA8888 fourcc_code('R', 'A', '2', '4')
#define DRM_FORMAT_BGRA8888 fourcc_code('B', 'A', '2', '4')
#define DRM_FORMAT_XRGB2101010 fourcc_code('X', 'R', '3', '0')
#define DRM_FORMAT_XBGR2101010 fourcc_code('X', 'B', '3', '0')
#define DRM_FORMAT_RGBX1010102 fourcc_code('R', 'X', '3', '0')
#define DRM_FORMAT_BGRX1010102 fourcc_code('B', 'X', '3', '0')
#define DRM_FORMAT_ARGB2101010 fourcc_code('A', 'R', '3', '0')
#define DRM_FORMAT_ABGR2101010 fourcc_code('A', 'B', '3', '0')
#define DRM_FORMAT_RGBA1010102 fourcc_code('R', 'A', '3', '0')
#define DRM_FORMAT_BGRA1010102 fourcc_code('B', 'A', '3', '0')
#define DRM_FORMAT_YUYV fourcc_code('Y', 'U', 'Y', 'V')
#define DRM_FORMAT_YVYU fourcc_code('Y', 'V', 'Y', 'U')
#define DRM_FORMAT_UYVY fourcc_code('U', 'Y', 'V', 'Y')
#define DRM_FORMAT_VYUY fourcc_code('V', 'Y', 'U', 'Y')
#define DRM_FORMAT_AYUV fourcc_code('A', 'Y', 'U', 'V')
#define DRM_FORMAT_XRGB8888_A8 fourcc_code('X', 'R', 'A', '8')
#define DRM_FORMAT_XBGR8888_A8 fourcc_code('X', 'B', 'A', '8')
#define DRM_FORMAT_RGBX8888_A8 fourcc_code('R', 'X', 'A', '8')
#define DRM_FORMAT_BGRX8888_A8 fourcc_code('B', 'X', 'A', '8')
#define DRM_FORMAT_RGB888_A8 fourcc_code('R', '8', 'A', '8')
#define DRM_FORMAT_BGR888_A8 fourcc_code('B', '8', 'A', '8')
#define DRM_FORMAT_RGB565_A8 fourcc_code('R', '5', 'A', '8')
#define DRM_FORMAT_BGR565_A8 fourcc_code('B', '5', 'A', '8')
#define DRM_FORMAT_NV12 fourcc_code('N', 'V', '1', '2')
#define DRM_FORMAT_NV21 fourcc_code('N', 'V', '2', '1')
#define DRM_FORMAT_NV16 fourcc_code('N', 'V', '1', '6')
#define DRM_FORMAT_NV61 fourcc_code('N', 'V', '6', '1')
#define DRM_FORMAT_NV24 fourcc_code('N', 'V', '2', '4')
#define DRM_FORMAT_NV42 fourcc_code('N', 'V', '4', '2')
#define DRM_FORMAT_YUV410 fourcc_code('Y', 'U', 'V', '9')
#define DRM_FORMAT_YVU410 fourcc_code('Y', 'V', 'U', '9')
#define DRM_FORMAT_YUV411 fourcc_code('Y', 'U', '1', '1')
#define DRM_FORMAT_YVU411 fourcc_code('Y', 'V', '1', '1')
#define DRM_FORMAT_YUV420 fourcc_code('Y', 'U', '1', '2')
#define DRM_FORMAT_YVU420 fourcc_code('Y', 'V', '1', '2')
#define DRM_FORMAT_YUV422 fourcc_code('Y', 'U', '1', '6')
#define DRM_FORMAT_YVU422 fourcc_code('Y', 'V', '1', '6')
#define DRM_FORMAT_YUV444 fourcc_code('Y', 'U', '2', '4')
#define DRM_FORMAT_YVU444 fourcc_code('Y', 'V', '2', '4')
#define DRM_FORMAT_MOD_NONE 0
#define DRM_FORMAT_MOD_VENDOR_NONE 0
#define DRM_FORMAT_MOD_VENDOR_INTEL 0x01
#define DRM_FORMAT_MOD_VENDOR_AMD 0x02
#define DRM_FORMAT_MOD_VENDOR_NV 0x03
#define DRM_FORMAT_MOD_VENDOR_SAMSUNG 0x04
#define DRM_FORMAT_MOD_VENDOR_QCOM 0x05
#define DRM_FORMAT_MOD_VENDOR_VIVANTE 0x06
#define fourcc_mod_code(vendor,val) ((((__u64) DRM_FORMAT_MOD_VENDOR_ ##vendor) << 56) | (val & 0x00ffffffffffffffULL))
#define DRM_FORMAT_MOD_LINEAR fourcc_mod_code(NONE, 0)
#define I915_FORMAT_MOD_X_TILED fourcc_mod_code(INTEL, 1)
#define I915_FORMAT_MOD_Y_TILED fourcc_mod_code(INTEL, 2)
#define I915_FORMAT_MOD_Yf_TILED fourcc_mod_code(INTEL, 3)
#define DRM_FORMAT_MOD_SAMSUNG_64_32_TILE fourcc_mod_code(SAMSUNG, 1)
#define DRM_FORMAT_MOD_VIVANTE_TILED fourcc_mod_code(VIVANTE, 1)
#define DRM_FORMAT_MOD_VIVANTE_SUPER_TILED fourcc_mod_code(VIVANTE, 2)
#define DRM_FORMAT_MOD_VIVANTE_SPLIT_TILED fourcc_mod_code(VIVANTE, 3)
#define DRM_FORMAT_MOD_VIVANTE_SPLIT_SUPER_TILED fourcc_mod_code(VIVANTE, 4)
#define __fourcc_mod_tegra_mode_shift 32
#define fourcc_mod_tegra_code(val,params) fourcc_mod_code(NV, ((((__u64) val) << __fourcc_mod_tegra_mode_shift) | params))
#define fourcc_mod_tegra_mod(m) (m & ~((1ULL << __fourcc_mod_tegra_mode_shift) - 1))
#define fourcc_mod_tegra_param(m) (m & ((1ULL << __fourcc_mod_tegra_mode_shift) - 1))
#define NV_FORMAT_MOD_TEGRA_TILED fourcc_mod_tegra_code(1, 0)
#define NV_FORMAT_MOD_TEGRA_16BX2_BLOCK(v) fourcc_mod_tegra_code(2, v)
#ifdef __cplusplus
#endif
#endif
