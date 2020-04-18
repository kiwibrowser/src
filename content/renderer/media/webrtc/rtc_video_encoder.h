// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_RTC_VIDEO_ENCODER_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_RTC_VIDEO_ENCODER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "media/base/video_decoder_config.h"
#include "third_party/webrtc/api/video/video_bitrate_allocation.h"
#include "third_party/webrtc/modules/video_coding/include/video_codec_interface.h"
#include "ui/gfx/geometry/size.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {
class GpuVideoAcceleratorFactories;
}  // namespace media

namespace content {

// RTCVideoEncoder uses a media::VideoEncodeAccelerator to implement a
// webrtc::VideoEncoder class for WebRTC.  Internally, VEA methods are
// trampolined to a private RTCVideoEncoder::Impl instance.  The Impl class runs
// on the worker thread queried from the |gpu_factories_|, which is presently
// the media thread.  RTCVideoEncoder is sychronized by webrtc::VideoSender.
// webrtc::VideoEncoder methods do not run concurrently. RtcVideoEncoder needs
// to synchronize RegisterEncodeCompleteCallback and encode complete callback.
class CONTENT_EXPORT RTCVideoEncoder : public webrtc::VideoEncoder {
 public:
  RTCVideoEncoder(media::VideoCodecProfile profile,
                  media::GpuVideoAcceleratorFactories* gpu_factories);
  ~RTCVideoEncoder() override;

  // webrtc::VideoEncoder implementation.  Tasks are posted to |impl_| using the
  // appropriate VEA methods.
  int32_t InitEncode(const webrtc::VideoCodec* codec_settings,
                     int32_t number_of_cores,
                     size_t max_payload_size) override;
  int32_t Encode(
      const webrtc::VideoFrame& input_image,
      const webrtc::CodecSpecificInfo* codec_specific_info,
      const std::vector<webrtc::FrameType>* frame_types) override;
  int32_t RegisterEncodeCompleteCallback(
      webrtc::EncodedImageCallback* callback) override;
  int32_t Release() override;
  int32_t SetChannelParameters(uint32_t packet_loss, int64_t rtt) override;
  int32_t SetRateAllocation(const webrtc::VideoBitrateAllocation& allocation,
                            uint32_t framerate) override;
  bool SupportsNativeHandle() const override;
  const char* ImplementationName() const override;

 private:
  class Impl;
  friend class RTCVideoEncoder::Impl;

  const media::VideoCodecProfile profile_;

  // Factory for creating VEAs, shared memory buffers, etc.
  media::GpuVideoAcceleratorFactories* gpu_factories_;

  // Task runner that the video accelerator runs on.
  const scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;

  // The RTCVideoEncoder::Impl that does all the work.
  scoped_refptr<Impl> impl_;

  DISALLOW_COPY_AND_ASSIGN(RTCVideoEncoder);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_RTC_VIDEO_ENCODER_H_
