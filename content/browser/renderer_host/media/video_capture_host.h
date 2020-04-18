// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_HOST_H_

#include <map>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner_helpers.h"
#include "content/browser/renderer_host/media/video_capture_controller.h"
#include "content/browser/renderer_host/media/video_capture_controller_event_handler.h"
#include "content/common/content_export.h"
#include "media/capture/mojom/video_capture.mojom.h"

namespace content {
class MediaStreamManager;

// VideoCaptureHost is the IO thread browser process communication endpoint
// between a renderer process (which can initiate and receive a video capture
// stream) and a VideoCaptureController in the browser process (which provides
// the stream from a video device). Every remote client is identified via a
// unique |device_id|, and is paired with a single VideoCaptureController.
class CONTENT_EXPORT VideoCaptureHost
    : public VideoCaptureControllerEventHandler,
      public media::mojom::VideoCaptureHost {
 public:
  VideoCaptureHost(uint32_t render_process_id,
                   MediaStreamManager* media_stream_manager);
  class RenderProcessHostDelegate;
  VideoCaptureHost(std::unique_ptr<RenderProcessHostDelegate> delegate,
                   MediaStreamManager* media_stream_manager);
  ~VideoCaptureHost() override;

  static void Create(uint32_t render_process_id,
                     MediaStreamManager* media_stream_manager,
                     media::mojom::VideoCaptureHostRequest request);

  // Interface for notifying RenderProcessHost instance about active video
  // capture stream changes.
  class CONTENT_EXPORT RenderProcessHostDelegate {
   public:
    virtual ~RenderProcessHostDelegate();
    virtual void NotifyStreamAdded() = 0;
    virtual void NotifyStreamRemoved() = 0;
  };

 private:
  friend class VideoCaptureTest;
  FRIEND_TEST_ALL_PREFIXES(VideoCaptureTest, IncrementMatchesDecrementCalls);

  // VideoCaptureControllerEventHandler implementation.
  void OnError(VideoCaptureControllerID id) override;
  void OnNewBuffer(VideoCaptureControllerID id,
                   media::mojom::VideoBufferHandlePtr buffer_handle,
                   int length,
                   int buffer_id) override;
  void OnBufferDestroyed(VideoCaptureControllerID id,
                         int buffer_id) override;
  void OnBufferReady(
      VideoCaptureControllerID id,
      int buffer_id,
      const media::mojom::VideoFrameInfoPtr& frame_info) override;
  void OnEnded(VideoCaptureControllerID id) override;
  void OnStarted(VideoCaptureControllerID id) override;
  void OnStartedUsingGpuDecode(VideoCaptureControllerID id) override;

  // media::mojom::VideoCaptureHost implementation
  void Start(int32_t device_id,
             int32_t session_id,
             const media::VideoCaptureParams& params,
             media::mojom::VideoCaptureObserverPtr observer) override;
  void Stop(int32_t device_id) override;
  void Pause(int32_t device_id) override;
  void Resume(int32_t device_id,
              int32_t session_id,
              const media::VideoCaptureParams& params) override;
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

  void DoError(VideoCaptureControllerID id);
  void DoEnded(VideoCaptureControllerID id);

  // Bound as callback for VideoCaptureManager::StartCaptureForClient().
  void OnControllerAdded(
      int device_id,
      const base::WeakPtr<VideoCaptureController>& controller);

  // Helper function that deletes the controller and tells VideoCaptureManager
  // to StopCaptureForClient(). |on_error| is true if this is triggered by
  // VideoCaptureControllerEventHandler::OnError.
  void DeleteVideoCaptureController(VideoCaptureControllerID controller_id,
                                    bool on_error);

  void NotifyStreamAdded();
  void NotifyStreamRemoved();
  void NotifyAllStreamsRemoved();

  class RenderProcessHostDelegateImpl;
  std::unique_ptr<RenderProcessHostDelegate> render_process_host_delegate_;
  uint32_t number_of_active_streams_ = 0;

  MediaStreamManager* const media_stream_manager_;

  // A map of VideoCaptureControllerID to the VideoCaptureController to which it
  // is connected. An entry in this map holds a null controller while it is in
  // the process of starting.
  std::map<VideoCaptureControllerID, base::WeakPtr<VideoCaptureController>>
      controllers_;

  // VideoCaptureObservers map, each one is used and should be valid between
  // Start() and the corresponding Stop().
  std::map<int32_t, media::mojom::VideoCaptureObserverPtr>
      device_id_to_observer_map_;

  base::WeakPtrFactory<VideoCaptureHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_HOST_H_
