// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_VIDEO_CAPTURE_IMPL_H_
#define CONTENT_RENDERER_MEDIA_VIDEO_CAPTURE_IMPL_H_

#include <stdint.h>

#include <list>
#include <map>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "content/common/media/video_capture.h"
#include "media/base/video_frame.h"
#include "media/capture/mojom/video_capture.mojom.h"
#include "media/capture/video_capture_types.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

// VideoCaptureImpl represents a capture device in renderer process. It provides
// an interface for clients to command the capture (Start, Stop, etc), and
// communicates back to these clients e.g. the capture state or incoming
// captured VideoFrames. VideoCaptureImpl is created in the main Renderer thread
// but otherwise operates on |io_task_runner_|, which is usually the IO thread.
class CONTENT_EXPORT VideoCaptureImpl
    : public media::mojom::VideoCaptureObserver {
 public:
  explicit VideoCaptureImpl(media::VideoCaptureSessionId session_id);
  ~VideoCaptureImpl() override;

  // Stop/resume delivering video frames to clients, based on flag |suspend|.
  void SuspendCapture(bool suspend);

  // Start capturing using the provided parameters.
  // |client_id| must be unique to this object in the render process. It is
  // used later to stop receiving video frames.
  // |state_update_cb| will be called when state changes.
  // |deliver_frame_cb| will be called when a frame is ready.
  void StartCapture(int client_id,
                    const media::VideoCaptureParams& params,
                    const VideoCaptureStateUpdateCB& state_update_cb,
                    const VideoCaptureDeliverFrameCB& deliver_frame_cb);

  // Stop capturing. |client_id| is the identifier used to call StartCapture.
  void StopCapture(int client_id);

  // Requests that the video capturer send a frame "soon" (e.g., to resolve
  // picture loss or quality issues).
  void RequestRefreshFrame();

  // Get capturing formats supported by this device.
  // |callback| will be invoked with the results.
  void GetDeviceSupportedFormats(const VideoCaptureDeviceFormatsCB& callback);

  // Get capturing formats currently in use by this device.
  // |callback| will be invoked with the results.
  void GetDeviceFormatsInUse(const VideoCaptureDeviceFormatsCB& callback);

  media::VideoCaptureSessionId session_id() const { return session_id_; }

  void SetVideoCaptureHostForTesting(media::mojom::VideoCaptureHost* service) {
    video_capture_host_for_testing_ = service;
  }

  // media::mojom::VideoCaptureObserver implementation.
  void OnStateChanged(media::mojom::VideoCaptureState state) override;
  void OnNewBuffer(int32_t buffer_id,
                   media::mojom::VideoBufferHandlePtr buffer_handle) override;
  void OnBufferReady(int32_t buffer_id,
                     media::mojom::VideoFrameInfoPtr info) override;
  void OnBufferDestroyed(int32_t buffer_id) override;

 private:
  friend class VideoCaptureImplTest;
  friend class MockVideoCaptureImpl;

  struct BufferContext;

  // Contains information about a video capture client, including capture
  //  parameters callbacks to the client.
  struct ClientInfo;
  using ClientInfoMap = std::map<int, ClientInfo>;

  using BufferFinishedCallback =
      base::OnceCallback<void(double consumer_resource_utilization)>;

  void OnAllClientsFinishedConsumingFrame(
      int buffer_id,
      scoped_refptr<BufferContext> buffer_context,
      double consumer_resource_utilization);

  void StopDevice();
  void RestartCapture();
  void StartCaptureInternal();

  void OnDeviceSupportedFormats(
      const VideoCaptureDeviceFormatsCB& callback,
      const media::VideoCaptureFormats& supported_formats);
  void OnDeviceFormatsInUse(
      const VideoCaptureDeviceFormatsCB& callback,
      const media::VideoCaptureFormats& formats_in_use);

  // Tries to remove |client_id| from |clients|, returning false if not found.
  bool RemoveClient(int client_id, ClientInfoMap* clients);

  media::mojom::VideoCaptureHost* GetVideoCaptureHost();

  // Called (by an unknown thread) when all consumers are done with a VideoFrame
  // and its ref-count has gone to zero.  This helper function grabs the
  // RESOURCE_UTILIZATION value from the |metadata| and then runs the given
  // callback, to trampoline back to the IO thread with the values.
  static void DidFinishConsumingFrame(
      const media::VideoFrameMetadata* metadata,
      BufferFinishedCallback callback_to_io_thread);

  // |device_id_| and |session_id_| are different concepts, but we reuse the
  // same numerical value, passed on construction.
  const int device_id_;
  const int session_id_;

  // |video_capture_host_| is an IO-thread InterfacePtr to a remote service
  // implementation and is created by binding |video_capture_host_info_|,
  // unless a |video_capture_host_for_testing_| has been injected.
  media::mojom::VideoCaptureHostPtrInfo video_capture_host_info_;
  media::mojom::VideoCaptureHostPtr video_capture_host_;
  media::mojom::VideoCaptureHost* video_capture_host_for_testing_;

  mojo::Binding<media::mojom::VideoCaptureObserver> observer_binding_;

  // Buffers available for sending to the client.
  using ClientBufferMap = std::map<int32_t, scoped_refptr<BufferContext>>;
  ClientBufferMap client_buffers_;

  ClientInfoMap clients_;
  ClientInfoMap clients_pending_on_restart_;

  // Video format requested by the client to this class via StartCapture().
  media::VideoCaptureParams params_;

  // First captured frame reference time sent from browser process side.
  base::TimeTicks first_frame_ref_time_;

  VideoCaptureState state_;

  base::ThreadChecker io_thread_checker_;

  // WeakPtrFactory pointing back to |this| object, for use with
  // media::VideoFrames constructed in OnBufferReceived() from buffers cached
  // in |client_buffers_|.
  base::WeakPtrFactory<VideoCaptureImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_VIDEO_CAPTURE_IMPL_H_
