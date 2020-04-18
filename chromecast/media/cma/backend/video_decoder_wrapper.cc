// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/video_decoder_wrapper.h"

#include "chromecast/media/cma/base/decoder_buffer_base.h"

namespace chromecast {
namespace media {

VideoDecoderWrapper::VideoDecoderWrapper(
    MediaPipelineBackend::VideoDecoder* decoder)
    : decoder_(decoder) {
  DCHECK(decoder_);
}

VideoDecoderWrapper::~VideoDecoderWrapper() = default;

void VideoDecoderWrapper::SetDelegate(
    media::CmaBackend::VideoDecoder::Delegate* delegate) {
  decoder_->SetDelegate(delegate);
}

media::CmaBackend::BufferStatus VideoDecoderWrapper::PushBuffer(
    scoped_refptr<media::DecoderBufferBase> buffer) {
  return decoder_->PushBuffer(buffer.get());
}

bool VideoDecoderWrapper::SetConfig(const media::VideoConfig& config) {
  return decoder_->SetConfig(config);
}

void VideoDecoderWrapper::GetStatistics(Statistics* statistics) {
  decoder_->GetStatistics(statistics);
}

}  // namespace media
}  // namespace chromecast
