// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_VIDEO_CAPTURE_IMPL_MANAGER_H_
#define CONTENT_RENDERER_MEDIA_VIDEO_CAPTURE_IMPL_MANAGER_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "content/common/media/video_capture.h"
#include "content/public/common/media_stream_request.h"
#include "content/public/renderer/media_stream_video_sink.h"
#include "media/capture/video_capture_types.h"

namespace content {

class VideoCaptureImpl;

// TODO(hclam): This class should be renamed to VideoCaptureService.

// This class provides access to a video capture device in the browser
// process through IPC. The main function is to deliver video frames
// to a client.
//
// THREADING
//
// VideoCaptureImplManager lives only on the Render Main thread. All methods
// must be called on this thread.
//
// VideoFrames are delivered on the IO thread. Callbacks provided by
// a client are also called on the IO thread.
class CONTENT_EXPORT VideoCaptureImplManager {
 public:
  VideoCaptureImplManager();
  virtual ~VideoCaptureImplManager();

  // Open a device associated with the session ID.
  // This method must be called before any methods with the same ID
  // is used.
  // Returns a callback that should be used to release the acquired
  // resources.
  base::Closure UseDevice(media::VideoCaptureSessionId id);

  // Start receiving video frames for the given session ID.
  //
  // |state_update_cb| will be called on the IO thread when capturing
  // state changes.
  // States will be one of the following four:
  // * VIDEO_CAPTURE_STATE_STARTED
  // * VIDEO_CAPTURE_STATE_STOPPED
  // * VIDEO_CAPTURE_STATE_PAUSED
  // * VIDEO_CAPTURE_STATE_ERROR
  //
  // |deliver_frame_cb| will be called on the IO thread when a video
  // frame is ready.
  //
  // Returns a callback that is used to stop capturing. Note that stopping
  // video capture is not synchronous. Client should handle the case where
  // callbacks are called after capturing is instructed to stop, typically
  // by binding the passed callbacks on a WeakPtr.
  base::Closure StartCapture(
      media::VideoCaptureSessionId id,
      const media::VideoCaptureParams& params,
      const VideoCaptureStateUpdateCB& state_update_cb,
      const VideoCaptureDeliverFrameCB& deliver_frame_cb);

  // Requests that the video capturer send a frame "soon" (e.g., to resolve
  // picture loss or quality issues).
  void RequestRefreshFrame(media::VideoCaptureSessionId id);

  // Requests frame delivery be suspended/resumed for a given capture session.
  void Suspend(media::VideoCaptureSessionId id);
  void Resume(media::VideoCaptureSessionId id);

  // Get supported formats supported by the device for the given session
  // ID. |callback| will be called on the IO thread.
  void GetDeviceSupportedFormats(media::VideoCaptureSessionId id,
                                 const VideoCaptureDeviceFormatsCB& callback);

  // Get supported formats currently in use for the given session ID.
  // |callback| will be called on the IO thread.
  void GetDeviceFormatsInUse(media::VideoCaptureSessionId id,
                             const VideoCaptureDeviceFormatsCB& callback);

  // Make all VideoCaptureImpl instances in the input |video_devices|
  // stop/resume delivering video frames to their clients, depends on flag
  // |suspend|. This is called in response to a RenderView-wide
  // PageHidden/Shown() event.
  // To suspend/resume an individual session, please call Suspend(id) or
  // Resume(id).
  void SuspendDevices(const MediaStreamDevices& video_devices, bool suspend);

  virtual std::unique_ptr<VideoCaptureImpl> CreateVideoCaptureImplForTesting(
      media::VideoCaptureSessionId session_id) const;

 private:
  // Holds bookkeeping info for each VideoCaptureImpl shared by clients.
  struct DeviceEntry;

  void StopCapture(int client_id, media::VideoCaptureSessionId id);
  void UnrefDevice(media::VideoCaptureSessionId id);

  // Devices currently in use.
  std::vector<DeviceEntry> devices_;

  // This is an internal ID for identifying clients of VideoCaptureImpl.
  // The ID is global for the render process.
  int next_client_id_;

  // Hold a pointer to the Render Main message loop to check we operate on the
  // right thread.
  const scoped_refptr<base::SingleThreadTaskRunner> render_main_task_runner_;

  // Set to true if SuspendDevices(true) was called. This, along with
  // DeviceEntry::is_individually_suspended, is used to determine whether to
  // take action when suspending/resuming each device.
  bool is_suspending_all_;

  // Bound to the render thread.
  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<VideoCaptureImplManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureImplManager);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_VIDEO_CAPTURE_IMPL_MANAGER_H_
