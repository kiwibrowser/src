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
#ifndef _UAPI_VC4_DRM_H_
#define _UAPI_VC4_DRM_H_
#include "drm.h"
#ifdef __cplusplus
#endif
#define DRM_VC4_SUBMIT_CL 0x00
#define DRM_VC4_WAIT_SEQNO 0x01
#define DRM_VC4_WAIT_BO 0x02
#define DRM_VC4_CREATE_BO 0x03
#define DRM_VC4_MMAP_BO 0x04
#define DRM_VC4_CREATE_SHADER_BO 0x05
#define DRM_VC4_GET_HANG_STATE 0x06
#define DRM_VC4_GET_PARAM 0x07
#define DRM_IOCTL_VC4_SUBMIT_CL DRM_IOWR(DRM_COMMAND_BASE + DRM_VC4_SUBMIT_CL, struct drm_vc4_submit_cl)
#define DRM_IOCTL_VC4_WAIT_SEQNO DRM_IOWR(DRM_COMMAND_BASE + DRM_VC4_WAIT_SEQNO, struct drm_vc4_wait_seqno)
#define DRM_IOCTL_VC4_WAIT_BO DRM_IOWR(DRM_COMMAND_BASE + DRM_VC4_WAIT_BO, struct drm_vc4_wait_bo)
#define DRM_IOCTL_VC4_CREATE_BO DRM_IOWR(DRM_COMMAND_BASE + DRM_VC4_CREATE_BO, struct drm_vc4_create_bo)
#define DRM_IOCTL_VC4_MMAP_BO DRM_IOWR(DRM_COMMAND_BASE + DRM_VC4_MMAP_BO, struct drm_vc4_mmap_bo)
#define DRM_IOCTL_VC4_CREATE_SHADER_BO DRM_IOWR(DRM_COMMAND_BASE + DRM_VC4_CREATE_SHADER_BO, struct drm_vc4_create_shader_bo)
#define DRM_IOCTL_VC4_GET_HANG_STATE DRM_IOWR(DRM_COMMAND_BASE + DRM_VC4_GET_HANG_STATE, struct drm_vc4_get_hang_state)
#define DRM_IOCTL_VC4_GET_PARAM DRM_IOWR(DRM_COMMAND_BASE + DRM_VC4_GET_PARAM, struct drm_vc4_get_param)
struct drm_vc4_submit_rcl_surface {
  __u32 hindex;
  __u32 offset;
  __u16 bits;
#define VC4_SUBMIT_RCL_SURFACE_READ_IS_FULL_RES (1 << 0)
  __u16 flags;
};
struct drm_vc4_submit_cl {
  __u64 bin_cl;
  __u64 shader_rec;
  __u64 uniforms;
  __u64 bo_handles;
  __u32 bin_cl_size;
  __u32 shader_rec_size;
  __u32 shader_rec_count;
  __u32 uniforms_size;
  __u32 bo_handle_count;
  __u16 width;
  __u16 height;
  __u8 min_x_tile;
  __u8 min_y_tile;
  __u8 max_x_tile;
  __u8 max_y_tile;
  struct drm_vc4_submit_rcl_surface color_read;
  struct drm_vc4_submit_rcl_surface color_write;
  struct drm_vc4_submit_rcl_surface zs_read;
  struct drm_vc4_submit_rcl_surface zs_write;
  struct drm_vc4_submit_rcl_surface msaa_color_write;
  struct drm_vc4_submit_rcl_surface msaa_zs_write;
  __u32 clear_color[2];
  __u32 clear_z;
  __u8 clear_s;
  __u32 pad : 24;
#define VC4_SUBMIT_CL_USE_CLEAR_COLOR (1 << 0)
  __u32 flags;
  __u64 seqno;
};
struct drm_vc4_wait_seqno {
  __u64 seqno;
  __u64 timeout_ns;
};
struct drm_vc4_wait_bo {
  __u32 handle;
  __u32 pad;
  __u64 timeout_ns;
};
struct drm_vc4_create_bo {
  __u32 size;
  __u32 flags;
  __u32 handle;
  __u32 pad;
};
struct drm_vc4_mmap_bo {
  __u32 handle;
  __u32 flags;
  __u64 offset;
};
struct drm_vc4_create_shader_bo {
  __u32 size;
  __u32 flags;
  __u64 data;
  __u32 handle;
  __u32 pad;
};
struct drm_vc4_get_hang_state_bo {
  __u32 handle;
  __u32 paddr;
  __u32 size;
  __u32 pad;
};
struct drm_vc4_get_hang_state {
  __u64 bo;
  __u32 bo_count;
  __u32 start_bin, start_render;
  __u32 ct0ca, ct0ea;
  __u32 ct1ca, ct1ea;
  __u32 ct0cs, ct1cs;
  __u32 ct0ra0, ct1ra0;
  __u32 bpca, bpcs;
  __u32 bpoa, bpos;
  __u32 vpmbase;
  __u32 dbge;
  __u32 fdbgo;
  __u32 fdbgb;
  __u32 fdbgr;
  __u32 fdbgs;
  __u32 errstat;
  __u32 pad[16];
};
#define DRM_VC4_PARAM_V3D_IDENT0 0
#define DRM_VC4_PARAM_V3D_IDENT1 1
#define DRM_VC4_PARAM_V3D_IDENT2 2
#define DRM_VC4_PARAM_SUPPORTS_BRANCHES 3
#define DRM_VC4_PARAM_SUPPORTS_ETC1 4
#define DRM_VC4_PARAM_SUPPORTS_THREADED_FS 5
struct drm_vc4_get_param {
  __u32 param;
  __u32 pad;
  __u64 value;
};
#ifdef __cplusplus
#endif
#endif
