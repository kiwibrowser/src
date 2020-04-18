// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/mock_mojo_media_stream_dispatcher_host.h"

#include "base/strings/string_number_conversions.h"

namespace content {

MockMojoMediaStreamDispatcherHost::MockMojoMediaStreamDispatcherHost()
    : binding_(this) {}

MockMojoMediaStreamDispatcherHost::~MockMojoMediaStreamDispatcherHost() {}

mojom::MediaStreamDispatcherHostPtr
MockMojoMediaStreamDispatcherHost::CreateInterfacePtrAndBind() {
  mojom::MediaStreamDispatcherHostPtr dispatcher_host;
  binding_.Bind(mojo::MakeRequest(&dispatcher_host));
  return dispatcher_host;
}

void MockMojoMediaStreamDispatcherHost::GenerateStream(
    int32_t request_id,
    const StreamControls& controls,
    bool user_gesture,
    GenerateStreamCallback callback) {
  request_id_ = request_id;
  audio_devices_.clear();
  video_devices_.clear();
  ++request_stream_counter_;

  if (controls.audio.requested) {
    MediaStreamDevice audio_device;
    audio_device.id = controls.audio.device_id + base::IntToString(session_id_);
    audio_device.name = "microphone";
    audio_device.type = MEDIA_DEVICE_AUDIO_CAPTURE;
    audio_device.session_id = session_id_;
    audio_device.matched_output_device_id =
        "associated_output_device_id" + base::IntToString(request_id_);
    audio_devices_.push_back(audio_device);
  }

  if (controls.video.requested) {
    MediaStreamDevice video_device;
    video_device.id = controls.video.device_id + base::IntToString(session_id_);
    video_device.name = "usb video camera";
    video_device.type = MEDIA_DEVICE_VIDEO_CAPTURE;
    video_device.video_facing = media::MEDIA_VIDEO_FACING_USER;
    video_device.session_id = session_id_;
    video_devices_.push_back(video_device);
  }

  if (do_not_run_cb_) {
    generate_stream_cb_ = std::move(callback);
  } else {
    std::move(callback).Run(MEDIA_DEVICE_OK,
                            "dummy" + base::IntToString(request_id_),
                            audio_devices_, video_devices_);
  }
}

void MockMojoMediaStreamDispatcherHost::CancelRequest(int32_t request_id) {
  EXPECT_EQ(request_id, request_id_);
}

void MockMojoMediaStreamDispatcherHost::StopStreamDevice(
    const std::string& device_id,
    int32_t session_id) {
  for (const MediaStreamDevice& device : audio_devices_) {
    if (device.id == device_id && device.session_id == session_id) {
      ++stop_audio_device_counter_;
      return;
    }
  }
  for (const MediaStreamDevice& device : video_devices_) {
    if (device.id == device_id && device.session_id == session_id) {
      ++stop_video_device_counter_;
      return;
    }
  }
  NOTREACHED();
}

void MockMojoMediaStreamDispatcherHost::OpenDevice(
    int32_t request_id,
    const std::string& device_id,
    MediaStreamType type,
    OpenDeviceCallback callback) {
  MediaStreamDevice device;
  device.id = device_id;
  device.type = type;
  device.session_id = session_id_;
  std::move(callback).Run(true /* success */,
                          "dummy" + base::IntToString(request_id), device);
}

}  // namespace content
