// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/audio_decoder_wrapper.h"

#include <algorithm>

#include "base/logging.h"
#include "chromecast/media/cma/backend/media_pipeline_backend_manager.h"
#include "chromecast/media/cma/base/decoder_buffer_base.h"
#include "chromecast/public/media/cast_decoder_buffer.h"

namespace chromecast {
namespace media {

AudioDecoderWrapper::AudioDecoderWrapper(
    MediaPipelineBackendManager* backend_manager,
    MediaPipelineBackend::AudioDecoder* backend_decoder,
    AudioContentType type,
    MediaPipelineBackendManager::BufferDelegate* buffer_delegate)
    : backend_manager_(backend_manager),
      decoder_(backend_decoder),
      content_type_(type),
      buffer_delegate_(buffer_delegate),
      delegate_active_(false),
      global_volume_multiplier_(1.0f),
      stream_volume_multiplier_(1.0f) {
  DCHECK(backend_manager_);

  backend_manager_->AddAudioDecoder(this);
  if (buffer_delegate_) {
    buffer_delegate_->OnStreamStarted();
  }
}

AudioDecoderWrapper::~AudioDecoderWrapper() {
  if (buffer_delegate_) {
    buffer_delegate_->OnStreamStopped();
  }
  backend_manager_->RemoveAudioDecoder(this);
}

void AudioDecoderWrapper::SetGlobalVolumeMultiplier(float multiplier) {
  global_volume_multiplier_ = multiplier;
  if (!delegate_active_) {
    float volume = stream_volume_multiplier_ * global_volume_multiplier_;
    decoder_.SetVolume(volume);
    if (buffer_delegate_) {
      buffer_delegate_->OnSetVolume(volume);
    }
  }
}

void AudioDecoderWrapper::SetDelegate(Delegate* delegate) {
  decoder_.SetDelegate(delegate);
}

CmaBackend::BufferStatus AudioDecoderWrapper::PushBuffer(
    scoped_refptr<DecoderBufferBase> buffer) {
  if (buffer_delegate_ && buffer_delegate_->IsActive()) {
    // Mute the decoder, we are sending audio to delegate.
    if (!delegate_active_) {
      delegate_active_ = true;
      decoder_.SetVolume(0.0);
    }
    buffer_delegate_->OnPushBuffer(buffer.get());
  } else {
    // Restore original volume.
    if (delegate_active_) {
      delegate_active_ = false;
      if (!decoder_.SetVolume(stream_volume_multiplier_ *
                              global_volume_multiplier_)) {
        LOG(ERROR) << "SetVolume failed";
      }
    }
  }
  return decoder_.PushBuffer(buffer.get());
}

bool AudioDecoderWrapper::SetConfig(const AudioConfig& config) {
  if (buffer_delegate_) {
    buffer_delegate_->OnSetConfig(config);
  }
  return decoder_.SetConfig(config);
}

bool AudioDecoderWrapper::SetVolume(float multiplier) {
  stream_volume_multiplier_ = std::max(0.0f, std::min(multiplier, 1.0f));
  float volume = stream_volume_multiplier_ * global_volume_multiplier_;
  if (buffer_delegate_) {
    buffer_delegate_->OnSetVolume(volume);
  }

  if (delegate_active_) {
    return true;
  }
  return decoder_.SetVolume(volume);
}

AudioDecoderWrapper::RenderingDelay AudioDecoderWrapper::GetRenderingDelay() {
  return decoder_.GetRenderingDelay();
}

void AudioDecoderWrapper::GetStatistics(Statistics* statistics) {
  decoder_.GetStatistics(statistics);
}

bool AudioDecoderWrapper::RequiresDecryption() {
  return (MediaPipelineBackend::AudioDecoder::RequiresDecryption &&
          MediaPipelineBackend::AudioDecoder::RequiresDecryption()) ||
         decoder_.IsUsingSoftwareDecoder();
}

}  // namespace media
}  // namespace chromecast
