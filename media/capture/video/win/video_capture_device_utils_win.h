// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_UTILS_WIN_H_
#define MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_UTILS_WIN_H_

#include <windows.h>

#include "media/base/video_facing.h"

namespace media {

// Returns the rotation of the camera. Returns 0 if it's not a built-in camera,
// or auto-rotation is not enabled, or only displays on external monitors.
int GetCameraRotation(VideoFacingMode facing);

bool IsAutoRotationEnabled();
bool IsInternalCamera(VideoFacingMode facing);

// Returns true if target device has active internal display panel, e.g. the
// screen attached to tablets or laptops, and stores its device info in
// |internal_display_device|.
bool HasActiveInternalDisplayDevice(DISPLAY_DEVICE* internal_display_device);

// Returns S_OK if the path info of the target display device with input
// |device_name| shows it is an internal display panel.
HRESULT CheckPathInfoForInternal(const PCWSTR device_name);

// Returns true if this is an integrated display panel.
bool IsInternalVideoOutput(
    const DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY video_output_tech_type);
}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_UTILS_WIN_H_