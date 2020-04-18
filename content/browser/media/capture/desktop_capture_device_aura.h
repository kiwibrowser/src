// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_DESKTOP_CAPTURE_DEVICE_AURA_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_DESKTOP_CAPTURE_DEVICE_AURA_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/browser/desktop_media_id.h"
#include "media/capture/content/screen_capture_device_core.h"
#include "media/capture/video/video_capture_device.h"

namespace content {

// An implementation of VideoCaptureDevice that mirrors an Aura window.
class CONTENT_EXPORT DesktopCaptureDeviceAura
    : public media::VideoCaptureDevice {
 public:
  // Creates a VideoCaptureDevice for the Aura desktop.  If |source| does not
  // reference a registered aura window, returns nullptr instead.
  static std::unique_ptr<media::VideoCaptureDevice> Create(
      const DesktopMediaID& source);

  ~DesktopCaptureDeviceAura() override;

  // VideoCaptureDevice implementation.
  void AllocateAndStart(const media::VideoCaptureParams& params,
                        std::unique_ptr<Client> client) override;
  void RequestRefreshFrame() override;
  void StopAndDeAllocate() override;
  void OnUtilizationReport(int frame_feedback_id, double utilization) override;

 private:
  explicit DesktopCaptureDeviceAura(const DesktopMediaID& source);

  std::unique_ptr<media::ScreenCaptureDeviceCore> core_;

  DISALLOW_COPY_AND_ASSIGN(DesktopCaptureDeviceAura);
};


}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_DESKTOP_CAPTURE_DEVICE_AURA_H_
