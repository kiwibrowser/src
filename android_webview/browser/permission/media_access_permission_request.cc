// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/permission/media_access_permission_request.h"

#include <utility>

#include "android_webview/browser/permission/aw_permission_request.h"
#include "content/public/browser/media_capture_devices.h"

using content::MediaCaptureDevices;
using content::MediaStreamDevice;
using content::MediaStreamDevices;

namespace android_webview {

namespace {

// Return the device specified by |device_id| if exists, otherwise the first
// available device is returned.
const MediaStreamDevice* GetDeviceByIdOrFirstAvailable(
    const MediaStreamDevices& devices,
    const std::string& device_id) {
  if (devices.empty())
    return NULL;

  if (!device_id.empty()) {
    for (size_t i = 0; i < devices.size(); ++i) {
      if (devices[i].id == device_id)
        return &devices[i];
    }
  }

  return &devices[0];
}

}  // namespace

MediaAccessPermissionRequest::MediaAccessPermissionRequest(
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback)
    : request_(request), callback_(callback) {}

MediaAccessPermissionRequest::~MediaAccessPermissionRequest() {}

void MediaAccessPermissionRequest::NotifyRequestResult(bool allowed) {
  std::unique_ptr<content::MediaStreamUI> ui;
  MediaStreamDevices devices;
  if (!allowed) {
    callback_.Run(devices, content::MEDIA_DEVICE_PERMISSION_DENIED,
                  std::move(ui));
    return;
  }

  if (request_.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE) {
    const MediaStreamDevices& audio_devices =
        audio_test_devices_.empty()
            ? MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices()
            : audio_test_devices_;
    const MediaStreamDevice* device = GetDeviceByIdOrFirstAvailable(
        audio_devices, request_.requested_audio_device_id);
    if (device)
      devices.push_back(*device);
  }

  if (request_.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE) {
    const MediaStreamDevices& video_devices =
        video_test_devices_.empty()
            ? MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices()
            : video_test_devices_;
    const MediaStreamDevice* device = GetDeviceByIdOrFirstAvailable(
        video_devices, request_.requested_video_device_id);
    if (device)
      devices.push_back(*device);
  }
  callback_.Run(devices,
                devices.empty() ? content::MEDIA_DEVICE_NO_HARDWARE
                                : content::MEDIA_DEVICE_OK,
                std::move(ui));
}

const GURL& MediaAccessPermissionRequest::GetOrigin() {
  return request_.security_origin;
}

int64_t MediaAccessPermissionRequest::GetResources() {
  return (request_.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE
              ? AwPermissionRequest::AudioCapture
              : 0) |
         (request_.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE
              ? AwPermissionRequest::VideoCapture
              : 0);
}

}  // namespace android_webview
