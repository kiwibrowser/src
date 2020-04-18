// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_DECODER_WRAPPER_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_DECODER_WRAPPER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromecast/media/cma/backend/cma_backend.h"
#include "chromecast/public/media/decoder_config.h"
#include "chromecast/public/media/media_pipeline_backend.h"

namespace chromecast {
namespace media {
class DecoderBufferBase;

class VideoDecoderWrapper : public CmaBackend::VideoDecoder {
 public:
  explicit VideoDecoderWrapper(MediaPipelineBackend::VideoDecoder* decoder);
  ~VideoDecoderWrapper() override;

 private:
  // CmaBackend::VideoDecoder implementation:
  void SetDelegate(Delegate* delegate) override;
  BufferStatus PushBuffer(scoped_refptr<DecoderBufferBase> buffer) override;
  bool SetConfig(const VideoConfig& config) override;
  void GetStatistics(Statistics* statistics) override;

  MediaPipelineBackend::VideoDecoder* const decoder_;

  DISALLOW_COPY_AND_ASSIGN(VideoDecoderWrapper);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_DECODER_WRAPPER_H_
