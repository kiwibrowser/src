// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/media_pipeline_backend_wrapper.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chromecast/media/cma/backend/audio_decoder_wrapper.h"
#include "chromecast/media/cma/backend/media_pipeline_backend_manager.h"
#include "chromecast/media/cma/backend/video_decoder_wrapper.h"
#include "chromecast/public/cast_media_shlib.h"
#include "chromecast/public/media/media_pipeline_backend.h"
#include "chromecast/public/volume_control.h"

namespace chromecast {
namespace media {

using DecoderType = MediaPipelineBackendManager::DecoderType;

MediaPipelineBackendWrapper::MediaPipelineBackendWrapper(
    const media::MediaPipelineDeviceParams& params,
    MediaPipelineBackendManager* backend_manager)
    : backend_(base::WrapUnique(
          media::CastMediaShlib::CreateMediaPipelineBackend(params))),
      backend_manager_(backend_manager),
      audio_stream_type_(params.audio_type),
      content_type_(params.content_type),
      playing_(false) {
  DCHECK(backend_);
  DCHECK(backend_manager_);
}

MediaPipelineBackendWrapper::~MediaPipelineBackendWrapper() {
  if (audio_decoder_) {
    backend_manager_->DecrementDecoderCount(
        IsSfx() ? DecoderType::SFX_DECODER : DecoderType::AUDIO_DECODER);
    if (playing_) {
      backend_manager_->UpdatePlayingAudioCount(IsSfx(), -1);
    }
  }
  if (video_decoder_) {
    backend_manager_->DecrementDecoderCount(DecoderType::VIDEO_DECODER);
  }
}

void MediaPipelineBackendWrapper::LogicalPause() {
  SetPlaying(false);
}

void MediaPipelineBackendWrapper::LogicalResume() {
  SetPlaying(true);
}

CmaBackend::AudioDecoder* MediaPipelineBackendWrapper::CreateAudioDecoder() {
  DCHECK(!audio_decoder_);

  if (!backend_manager_->IncrementDecoderCount(
          IsSfx() ? DecoderType::SFX_DECODER : DecoderType::AUDIO_DECODER))
    return nullptr;
  MediaPipelineBackend::AudioDecoder* real_decoder =
      backend_->CreateAudioDecoder();
  if (!real_decoder) {
    return nullptr;
  }

  MediaPipelineBackendManager::BufferDelegate* delegate = nullptr;
  // Only set delegate for the primary media stream.
  if (content_type_ == media::AudioContentType::kMedia &&
      audio_stream_type_ ==
          media::MediaPipelineDeviceParams::kAudioStreamNormal) {
    delegate = backend_manager_->buffer_delegate();
  }

  audio_decoder_ = std::make_unique<AudioDecoderWrapper>(
      backend_manager_, real_decoder, content_type_, delegate);
  return audio_decoder_.get();
}

CmaBackend::VideoDecoder* MediaPipelineBackendWrapper::CreateVideoDecoder() {
  DCHECK(!video_decoder_);

  if (!backend_manager_->IncrementDecoderCount(DecoderType::VIDEO_DECODER))
    return nullptr;
  MediaPipelineBackend::VideoDecoder* real_decoder =
      backend_->CreateVideoDecoder();
  if (!real_decoder) {
    return nullptr;
  }

  video_decoder_ = std::make_unique<VideoDecoderWrapper>(real_decoder);
  return video_decoder_.get();
}

bool MediaPipelineBackendWrapper::Initialize() {
  return backend_->Initialize();
}

bool MediaPipelineBackendWrapper::Start(int64_t start_pts) {
  if (!backend_->Start(start_pts)) {
    return false;
  }
  SetPlaying(true);
  return true;
}

void MediaPipelineBackendWrapper::Stop() {
  backend_->Stop();
  SetPlaying(false);
}

bool MediaPipelineBackendWrapper::Pause() {
  if (!backend_->Pause()) {
    return false;
  }
  SetPlaying(false);
  return true;
}

bool MediaPipelineBackendWrapper::Resume() {
  if (!backend_->Resume()) {
    return false;
  }
  SetPlaying(true);
  return true;
}

int64_t MediaPipelineBackendWrapper::GetCurrentPts() {
  return backend_->GetCurrentPts();
}

bool MediaPipelineBackendWrapper::SetPlaybackRate(float rate) {
  return backend_->SetPlaybackRate(rate);
}

void MediaPipelineBackendWrapper::SetPlaying(bool playing) {
  if (playing == playing_) {
    return;
  }
  playing_ = playing;
  if (audio_decoder_) {
    backend_manager_->UpdatePlayingAudioCount(IsSfx(), (playing_ ? 1 : -1));
  }
}

}  // namespace media
}  // namespace chromecast
