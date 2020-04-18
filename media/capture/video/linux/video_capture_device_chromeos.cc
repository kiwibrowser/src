// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/linux/video_capture_device_chromeos.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/display/display.h"
#include "ui/display/display_observer.h"
#include "ui/display/screen.h"

namespace media {

static CameraConfigChromeOS* GetCameraConfig() {
  static CameraConfigChromeOS* config = new CameraConfigChromeOS();
  return config;
}

VideoCaptureDeviceChromeOS::VideoCaptureDeviceChromeOS(
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    const VideoCaptureDeviceDescriptor& device_descriptor)
    : VideoCaptureDeviceLinux(device_descriptor),
      screen_observer_delegate_(
          new ScreenObserverDelegate(this, ui_task_runner)),
      lens_facing_(
          GetCameraConfig()->GetCameraFacing(device_descriptor.device_id,
                                             device_descriptor.model_id)),
      camera_orientation_(
          GetCameraConfig()->GetOrientation(device_descriptor.device_id,
                                            device_descriptor.model_id)),
      // External cameras have lens_facing as MEDIA_VIDEO_FACING_NONE.
      // We don't want to rotate the frame even if the device rotates.
      rotates_with_device_(lens_facing_ !=
                           VideoFacingMode::MEDIA_VIDEO_FACING_NONE) {}

VideoCaptureDeviceChromeOS::~VideoCaptureDeviceChromeOS() {
  screen_observer_delegate_->RemoveObserver();
}

void VideoCaptureDeviceChromeOS::SetRotation(int rotation) {
  if (!rotates_with_device_) {
    rotation = 0;
  } else if (lens_facing_ == VideoFacingMode::MEDIA_VIDEO_FACING_ENVIRONMENT) {
    // Original frame when |rotation| = 0
    // -----------------------
    // |          *          |
    // |         * *         |
    // |        *   *        |
    // |       *******       |
    // |      *       *      |
    // |     *         *     |
    // -----------------------
    //
    // |rotation| = 90, this is what back camera sees
    // -----------------------
    // |    ********         |
    // |       *   ****      |
    // |       *      ***    |
    // |       *      ***    |
    // |       *   ****      |
    // |    ********         |
    // -----------------------
    //
    // |rotation| = 90, this is what front camera sees
    // -----------------------
    // |         ********    |
    // |      ****   *       |
    // |    ***      *       |
    // |    ***      *       |
    // |      ****   *       |
    // |         ********    |
    // -----------------------
    //
    // Therefore, for back camera, we need to rotate (360 - |rotation|).
    rotation = (360 - rotation) % 360;
  }
  // Take into account camera orientation w.r.t. the display. External cameras
  // would have camera_orientation_ as 0.
  rotation = (rotation + camera_orientation_) % 360;
  VideoCaptureDeviceLinux::SetRotation(rotation);
}

void VideoCaptureDeviceChromeOS::SetDisplayRotation(
    const display::Display& display) {
  if (display.IsInternal())
    SetRotation(display.rotation() * 90);
}

}  // namespace media
