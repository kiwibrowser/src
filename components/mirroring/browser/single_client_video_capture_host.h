// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MIRRORING_BROWSER_SINGLE_CLIENT_VIDEO_CAPTURE_HOST_H_
#define COMPONENTS_MIRRORING_BROWSER_SINGLE_CLIENT_VIDEO_CAPTURE_HOST_H_

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequence_checker.h"
#include "content/public/browser/video_capture_device_launcher.h"
#include "media/capture/mojom/video_capture.mojom.h"
#include "media/capture/video/video_frame_receiver.h"

using media::VideoCaptureParams;
using media::VideoCaptureDevice;

namespace mirroring {

// Implements a subset of mojom::VideoCaptureHost to proxy between a
// content::LaunchedVideoCaptureDevice and a single client. On Start(), uses the
// provided DeviceLauncherCreateCallback to launch a video capture device that
// outputs frames to |this|. The frames are received through the
// VideoFrameReceiver interface and are forwarded to |observer| through the
// mojom::VideoCaptureObserver interface.
// Instances of this class must be operated from the same thread that is reqired
// by the DeviceLauncherCreateCallback.
class SingleClientVideoCaptureHost final
    : public media::mojom::VideoCaptureHost,
      public media::VideoFrameReceiver {
 public:
  using DeviceLauncherCreateCallback = base::RepeatingCallback<
      std::unique_ptr<content::VideoCaptureDeviceLauncher>()>;
  SingleClientVideoCaptureHost(const std::string& device_id,
                               content::MediaStreamType type,
                               DeviceLauncherCreateCallback callback);
  ~SingleClientVideoCaptureHost() override;

  // media::mojom::VideoCaptureHost implementations
  // |device_id| and |session_id| are ignored since there will be only one
  // device and one client.
  void Start(int32_t device_id,
             int32_t session_id,
             const VideoCaptureParams& params,
             media::mojom::VideoCaptureObserverPtr observer) override;
  void Stop(int32_t device_id) override;
  void Pause(int32_t device_id) override;
  void Resume(int32_t device_id,
              int32_t session_id,
              const VideoCaptureParams& params) override;
  void RequestRefreshFrame(int32_t device_id) override;
  void ReleaseBuffer(int32_t device_id,
                     int32_t buffer_id,
                     double consumer_resource_utilization) override;
  void GetDeviceSupportedFormats(
      int32_t device_id,
      int32_t session_id,
      GetDeviceSupportedFormatsCallback callback) override;
  void GetDeviceFormatsInUse(int32_t device_id,
                             int32_t session_id,
                             GetDeviceFormatsInUseCallback callback) override;

  // media::VideoFrameReceiver implementations
  using Buffer = VideoCaptureDevice::Client::Buffer;
  void OnNewBuffer(int buffer_id,
                   media::mojom::VideoBufferHandlePtr buffer_handle) override;
  void OnFrameReadyInBuffer(
      int buffer_id,
      int frame_feedback_id,
      std::unique_ptr<
          VideoCaptureDevice::Client::Buffer::ScopedAccessPermission>
          buffer_read_permission,
      media::mojom::VideoFrameInfoPtr frame_info) override;
  void OnBufferRetired(int buffer_id) override;
  void OnError() override;
  void OnLog(const std::string& message) override;
  void OnStarted() override;
  void OnStartedUsingGpuDecode() override;

  void OnDeviceLaunched(
      std::unique_ptr<content::LaunchedVideoCaptureDevice> device);
  void OnDeviceLaunchFailed();
  void OnDeviceLaunchAborted();

 private:
  // Reports the |consumer_resource_utilization| and removes the buffer context.
  void OnFinishedConsumingBuffer(int buffer_context_id,
                                 double consumer_resource_utilization);

  const std::string device_id_;
  const content::MediaStreamType type_;
  const DeviceLauncherCreateCallback device_launcher_callback_;

  media::mojom::VideoCaptureObserverPtr observer_;
  std::unique_ptr<content::LaunchedVideoCaptureDevice> launched_device_;

  // Unique ID assigned for the next buffer provided by OnNewBufferHandle().
  int next_buffer_context_id_ = 0;

  // Records the assigned buffer_context_id for buffers that are not retired.
  // The |buffer_id| provided by OnNewBufferHandle() is used as the key.
  base::flat_map<int, int> id_map_;

  // Tracks the the retired buffers that are still held by |observer_|.
  base::flat_set<int> retired_buffers_;

  // Records the |frame_feedback_id| and |buffer_read_permission| provided by
  // OnFrameReadyInBuffer(). The key is the assigned buffer context id. Each
  // entry is removed after the |observer_| finishes consuming the buffer by
  // calling ReleaseBuffer(). When Stop() is called, all the buffers are cleared
  // immediately.
  using BufferContext = std::pair<
      int,
      std::unique_ptr<
          VideoCaptureDevice::Client::Buffer::ScopedAccessPermission>>;
  base::flat_map<int, BufferContext> buffer_context_map_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<SingleClientVideoCaptureHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SingleClientVideoCaptureHost);
};

}  // namespace mirroring

#endif  // COMPONENTS_MIRRORING_BROWSER_SINGLE_CLIENT_VIDEO_CAPTURE_HOST_H_
