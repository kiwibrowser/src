// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/hardware_display_plane_atomic.h"

#include "ui/ozone/platform/drm/gpu/drm_device.h"

namespace ui {
namespace {

const char* kCrtcPropName = "CRTC_ID";
const char* kFbPropName = "FB_ID";
const char* kCrtcXPropName = "CRTC_X";
const char* kCrtcYPropName = "CRTC_Y";
const char* kCrtcWPropName = "CRTC_W";
const char* kCrtcHPropName = "CRTC_H";
const char* kSrcXPropName = "SRC_X";
const char* kSrcYPropName = "SRC_Y";
const char* kSrcWPropName = "SRC_W";
const char* kSrcHPropName = "SRC_H";
const char* kRotationPropName = "rotation";
const char* kInFenceFdPropName = "IN_FENCE_FD";

// TODO(dcastagna): Remove the following defines once they're in libdrm headers.
#if !defined(DRM_ROTATE_0)
#define BIT(n) (1 << (n))
#define DRM_ROTATE_0 BIT(0)
#define DRM_ROTATE_90 BIT(1)
#define DRM_ROTATE_180 BIT(2)
#define DRM_ROTATE_270 BIT(3)
#define DRM_REFLECT_X BIT(4)
#define DRM_REFLECT_Y BIT(5)
#endif

uint32_t OverlayTransformToDrmRotationPropertyValue(
    gfx::OverlayTransform transform) {
  switch (transform) {
    case gfx::OVERLAY_TRANSFORM_NONE:
      return DRM_ROTATE_0;
    case gfx::OVERLAY_TRANSFORM_FLIP_HORIZONTAL:
      return DRM_REFLECT_X;
    case gfx::OVERLAY_TRANSFORM_FLIP_VERTICAL:
      return DRM_REFLECT_Y;
    case gfx::OVERLAY_TRANSFORM_ROTATE_90:
      return DRM_ROTATE_90;
    case gfx::OVERLAY_TRANSFORM_ROTATE_180:
      return DRM_ROTATE_180;
    case gfx::OVERLAY_TRANSFORM_ROTATE_270:
      return DRM_ROTATE_270;
    default:
      NOTREACHED();
  }
  return 0;
}

}  // namespace

HardwareDisplayPlaneAtomic::Property::Property() {
}

bool HardwareDisplayPlaneAtomic::Property::Initialize(
    DrmDevice* drm,
    const char* name,
    const ScopedDrmObjectPropertyPtr& plane_props) {
  for (uint32_t i = 0; i < plane_props->count_props; i++) {
    ScopedDrmPropertyPtr property(
        drmModeGetProperty(drm->get_fd(), plane_props->props[i]));
    if (property && !strcmp(property->name, name)) {
      id = property->prop_id;
      break;
    }
  }
  return !!id;
}

HardwareDisplayPlaneAtomic::HardwareDisplayPlaneAtomic(uint32_t plane_id,
                                                       uint32_t possible_crtcs)
    : HardwareDisplayPlane(plane_id, possible_crtcs) {
}
HardwareDisplayPlaneAtomic::~HardwareDisplayPlaneAtomic() {
}

bool HardwareDisplayPlaneAtomic::SetPlaneData(
    drmModeAtomicReq* property_set,
    uint32_t crtc_id,
    uint32_t framebuffer,
    const gfx::Rect& crtc_rect,
    const gfx::Rect& src_rect,
    const gfx::OverlayTransform transform,
    int in_fence_fd) {
  if (transform != gfx::OVERLAY_TRANSFORM_NONE && !rotation_prop_.id)
    return false;

  int plane_set_succeeded =
      drmModeAtomicAddProperty(property_set, plane_id_, crtc_prop_.id,
                               crtc_id) &&
      drmModeAtomicAddProperty(property_set, plane_id_, fb_prop_.id,
                               framebuffer) &&
      drmModeAtomicAddProperty(property_set, plane_id_, crtc_x_prop_.id,
                               crtc_rect.x()) &&
      drmModeAtomicAddProperty(property_set, plane_id_, crtc_y_prop_.id,
                               crtc_rect.y()) &&
      drmModeAtomicAddProperty(property_set, plane_id_, crtc_w_prop_.id,
                               crtc_rect.width()) &&
      drmModeAtomicAddProperty(property_set, plane_id_, crtc_h_prop_.id,
                               crtc_rect.height()) &&
      drmModeAtomicAddProperty(property_set, plane_id_, src_x_prop_.id,
                               src_rect.x()) &&
      drmModeAtomicAddProperty(property_set, plane_id_, src_y_prop_.id,
                               src_rect.y()) &&
      drmModeAtomicAddProperty(property_set, plane_id_, src_w_prop_.id,
                               src_rect.width()) &&
      drmModeAtomicAddProperty(property_set, plane_id_, src_h_prop_.id,
                               src_rect.height());

  if (rotation_prop_.id) {
    plane_set_succeeded =
        plane_set_succeeded &&
        drmModeAtomicAddProperty(
            property_set, plane_id_, rotation_prop_.id,
            OverlayTransformToDrmRotationPropertyValue(transform));
  }
  if (in_fence_fd_prop_.id && in_fence_fd >= 0) {
    plane_set_succeeded =
        plane_set_succeeded &&
        drmModeAtomicAddProperty(property_set, plane_id_, in_fence_fd_prop_.id,
                                 in_fence_fd);
  }
  if (!plane_set_succeeded) {
    PLOG(ERROR) << "Failed to set plane data";
    return false;
  }
  return true;
}

bool HardwareDisplayPlaneAtomic::InitializeProperties(
    DrmDevice* drm,
    const ScopedDrmObjectPropertyPtr& plane_props) {
  bool props_init = crtc_prop_.Initialize(drm, kCrtcPropName, plane_props) &&
                    fb_prop_.Initialize(drm, kFbPropName, plane_props) &&
                    crtc_x_prop_.Initialize(drm, kCrtcXPropName, plane_props) &&
                    crtc_y_prop_.Initialize(drm, kCrtcYPropName, plane_props) &&
                    crtc_w_prop_.Initialize(drm, kCrtcWPropName, plane_props) &&
                    crtc_h_prop_.Initialize(drm, kCrtcHPropName, plane_props) &&
                    src_x_prop_.Initialize(drm, kSrcXPropName, plane_props) &&
                    src_y_prop_.Initialize(drm, kSrcYPropName, plane_props) &&
                    src_w_prop_.Initialize(drm, kSrcWPropName, plane_props) &&
                    src_h_prop_.Initialize(drm, kSrcHPropName, plane_props);

  if (!props_init) {
    LOG(ERROR) << "Unable to get plane properties.";
    return false;
  }
  // The following properties are optional.
  rotation_prop_.Initialize(drm, kRotationPropName, plane_props);
  in_fence_fd_prop_.Initialize(drm, kInFenceFdPropName, plane_props);
  return true;
}

}  // namespace ui
