// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MOCK_MOJO_MEDIA_STREAM_DISPATCHER_HOST_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MOCK_MOJO_MEDIA_STREAM_DISPATCHER_HOST_H_

#include <string>

#include "base/macros.h"
#include "content/common/media/media_stream.mojom.h"
#include "content/common/media/media_stream_controls.h"
#include "content/public/common/media_stream_request.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace content {

class MockMojoMediaStreamDispatcherHost
    : public mojom::MediaStreamDispatcherHost {
 public:
  MockMojoMediaStreamDispatcherHost();
  ~MockMojoMediaStreamDispatcherHost() override;

  mojom::MediaStreamDispatcherHostPtr CreateInterfacePtrAndBind();

  void GenerateStream(int32_t request_id,
                      const StreamControls& controls,
                      bool user_gesture,
                      GenerateStreamCallback callback) override;
  void CancelRequest(int32_t request_id) override;
  void StopStreamDevice(const std::string& device_id,
                        int32_t session_id) override;
  void OpenDevice(int32_t request_id,
                  const std::string& device_id,
                  MediaStreamType type,
                  OpenDeviceCallback callback) override;

  MOCK_METHOD1(CloseDevice, void(const std::string&));
  MOCK_METHOD3(SetCapturingLinkSecured, void(int32_t, MediaStreamType, bool));
  MOCK_METHOD1(OnStreamStarted, void(const std::string&));

  void IncrementSessionId() { ++session_id_; }
  void DoNotRunCallback() { do_not_run_cb_ = true; }

  int request_stream_counter() const { return request_stream_counter_; }
  int stop_audio_device_counter() const { return stop_audio_device_counter_; }
  int stop_video_device_counter() const { return stop_video_device_counter_; }

  const MediaStreamDevices& audio_devices() const { return audio_devices_; }
  const MediaStreamDevices& video_devices() const { return video_devices_; }

 private:
  int request_id_ = -1;
  int request_stream_counter_ = 0;
  int stop_audio_device_counter_ = 0;
  int stop_video_device_counter_ = 0;
  int session_id_ = 0;
  bool do_not_run_cb_ = false;
  MediaStreamDevices audio_devices_;
  MediaStreamDevices video_devices_;
  GenerateStreamCallback generate_stream_cb_;
  mojo::Binding<mojom::MediaStreamDispatcherHost> binding_;

  DISALLOW_COPY_AND_ASSIGN(MockMojoMediaStreamDispatcherHost);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MOCK_MOJO_MEDIA_STREAM_DISPATCHER_HOST_H_
