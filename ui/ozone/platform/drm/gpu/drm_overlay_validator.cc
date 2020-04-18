// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/drm_overlay_validator.h"

#include <drm_fourcc.h>

#include "base/files/platform_file.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/ozone/platform/drm/common/drm_util.h"
#include "ui/ozone/platform/drm/gpu/drm_device.h"
#include "ui/ozone/platform/drm/gpu/drm_window.h"
#include "ui/ozone/platform/drm/gpu/hardware_display_controller.h"
#include "ui/ozone/platform/drm/gpu/scanout_buffer.h"

namespace ui {

namespace {

scoped_refptr<ScanoutBuffer> GetBufferForPageFlipTest(
    const scoped_refptr<DrmDevice>& drm_device,
    const gfx::Size& size,
    uint32_t format,
    ScanoutBufferGenerator* buffer_generator,
    std::vector<scoped_refptr<ScanoutBuffer>>* reusable_buffers) {
  // Check if we can re-use existing buffers.
  for (const auto& buffer : *reusable_buffers) {
    if (buffer->GetFramebufferPixelFormat() == format &&
        buffer->GetSize() == size) {
      return buffer;
    }
  }

  const std::vector<uint64_t>
      modifiers;  // TODO(dcastagna): use the right modifiers.
  scoped_refptr<ScanoutBuffer> scanout_buffer =
      buffer_generator->Create(drm_device, format, modifiers, size);
  if (scanout_buffer)
    reusable_buffers->push_back(scanout_buffer);

  return scanout_buffer;
}

}  // namespace

DrmOverlayValidator::DrmOverlayValidator(
    DrmWindow* window,
    ScanoutBufferGenerator* buffer_generator)
    : window_(window), buffer_generator_(buffer_generator) {}

DrmOverlayValidator::~DrmOverlayValidator() {}

std::vector<OverlayCheckReturn_Params> DrmOverlayValidator::TestPageFlip(
    const std::vector<OverlayCheck_Params>& params,
    const OverlayPlaneList& last_used_planes) {
  std::vector<OverlayCheckReturn_Params> returns(params.size());
  HardwareDisplayController* controller = window_->GetController();
  if (!controller) {
    // The controller is not yet installed.
    for (auto& param : returns)
      param.status = OVERLAY_STATUS_NOT;

    return returns;
  }

  OverlayPlaneList test_list;
  std::vector<scoped_refptr<ScanoutBuffer>> reusable_buffers;
  scoped_refptr<DrmDevice> drm = controller->GetAllocationDrmDevice();

  for (const auto& plane : last_used_planes)
    reusable_buffers.push_back(plane.buffer);

  for (size_t i = 0; i < params.size(); ++i) {
    if (!params[i].is_overlay_candidate) {
      returns[i].status = OVERLAY_STATUS_NOT;
      continue;
    }

    scoped_refptr<ScanoutBuffer> buffer =
        GetBufferForPageFlipTest(drm, params[i].buffer_size,
                                 GetFourCCFormatFromBufferFormat(params[i].format),
                                 buffer_generator_, &reusable_buffers);

    OverlayPlane plane(buffer, params[i].plane_z_order, params[i].transform,
                       params[i].display_rect, params[i].crop_rect,
                       /* enable_blend */ true, /* gpu_fence */ nullptr);
    test_list.push_back(plane);

    if (buffer && controller->TestPageFlip(test_list)) {
      returns[i].status = OVERLAY_STATUS_ABLE;
    } else {
      // If test failed here, platform cannot support this configuration
      // with current combination of layers. This is usually the case when this
      // plane has requested post processing capability which needs additional
      // hardware resources and they might be already in use by other planes.
      // For example this plane has requested scaling capabilities and all
      // available scalars are already in use by other planes.
      returns[i].status = OVERLAY_STATUS_NOT;
      test_list.pop_back();
    }
  }

  return returns;
}

}  // namespace ui
