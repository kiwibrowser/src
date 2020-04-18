// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MIRRORING_SERVICE_VIDEO_CAPTURE_CLIENT_H_
#define COMPONENTS_MIRRORING_SERVICE_VIDEO_CAPTURE_CLIENT_H_

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "media/capture/mojom/video_capture.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/buffer.h"

namespace media {
class VideoFrame;
class VideoFrameMetadata;
}  // namespace media

namespace mirroring {

// On Start(), this class connects to |host| through the
// media::mojom::VideoCaptureHost interface and requests to launch a video
// capture device. After the device is started, the captured video frames are
// received through the media::mojom::VideoCaptureObserver interface.
class VideoCaptureClient : public media::mojom::VideoCaptureObserver {
 public:
  VideoCaptureClient(const media::VideoCaptureParams& params,
                     media::mojom::VideoCaptureHostPtr host);
  ~VideoCaptureClient() override;

  using FrameDeliverCallback = base::RepeatingCallback<void(
      scoped_refptr<media::VideoFrame> video_frame)>;
  void Start(FrameDeliverCallback deliver_callback,
             base::OnceClosure error_callback);

  void Stop();

  // Will stop delivering frames on this call.
  void Pause();

  void Resume(FrameDeliverCallback deliver_callback);

  // Requests to receive a refreshed captured video frame. Do nothing if the
  // capturing device is not started or the capturing is paused.
  void RequestRefreshFrame();

  // media::mojom::VideoCaptureObserver implementations.
  void OnStateChanged(media::mojom::VideoCaptureState state) override;
  void OnNewBuffer(int32_t buffer_id,
                   media::mojom::VideoBufferHandlePtr buffer_handle) override;
  void OnBufferReady(int32_t buffer_id,
                     media::mojom::VideoFrameInfoPtr info) override;
  void OnBufferDestroyed(int32_t buffer_id) override;

 private:
  using BufferFinishedCallback =
      base::OnceCallback<void(double consumer_resource_utilization)>;
  // Called by the VideoFrame destructor.
  static void DidFinishConsumingFrame(const media::VideoFrameMetadata* metadata,
                                      BufferFinishedCallback callback);

  // Reports the utilization and returns the buffer.
  void OnClientBufferFinished(int buffer_id,
                              double consumer_resource_utilization);

  const media::VideoCaptureParams params_;
  const media::mojom::VideoCaptureHostPtr video_capture_host_;

  // Called when capturing failed to start.
  base::OnceClosure error_callback_;

  mojo::Binding<media::mojom::VideoCaptureObserver> binding_;

  using ClientBufferMap =
      base::flat_map<int32_t, mojo::ScopedSharedBufferHandle>;
  // Stores the buffer handler on OnBufferCreated(). |buffer_id| is the key.
  ClientBufferMap client_buffers_;

  using MappingAndSize = std::pair<mojo::ScopedSharedBufferMapping, uint32_t>;
  using MappingMap = base::flat_map<int32_t, MappingAndSize>;
  // Stores the mapped buffers and their size. Each buffer is added the first
  // time the mapping is done or a larger size is requested.
  // |buffer_id| is the key to this map.
  MappingMap mapped_buffers_;

  // The reference time for the first frame. Used to calculate the timestamp of
  // the captured frame if not provided in the frame info.
  base::TimeTicks first_frame_ref_time_;

  // The callback to deliver the received frame.
  FrameDeliverCallback frame_deliver_callback_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<VideoCaptureClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureClient);
};

}  // namespace mirroring

#endif  // COMPONENTS_MIRRORING_SERVICE_VIDEO_CAPTURE_CLIENT_H_
