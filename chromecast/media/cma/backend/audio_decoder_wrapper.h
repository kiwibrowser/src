// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_DECODER_WRAPPER_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_DECODER_WRAPPER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromecast/media/cma/backend/audio_decoder_software_wrapper.h"
#include "chromecast/media/cma/backend/cma_backend.h"
#include "chromecast/media/cma/backend/media_pipeline_backend_manager.h"
#include "chromecast/public/media/media_pipeline_backend.h"

namespace chromecast {
namespace media {

enum class AudioContentType;

class AudioDecoderWrapper : public CmaBackend::AudioDecoder {
 public:
  AudioDecoderWrapper(
      MediaPipelineBackendManager* backend_manager,
      MediaPipelineBackend::AudioDecoder* backend_decoder,
      AudioContentType type,
      MediaPipelineBackendManager::BufferDelegate* buffer_delegate);
  ~AudioDecoderWrapper() override;

  void SetGlobalVolumeMultiplier(float multiplier);

  AudioContentType content_type() const { return content_type_; }

 private:
  // CmaBackend::AudioDecoder implementation:
  void SetDelegate(Delegate* delegate) override;
  BufferStatus PushBuffer(scoped_refptr<DecoderBufferBase> buffer) override;
  bool SetConfig(const AudioConfig& config) override;
  bool SetVolume(float multiplier) override;
  RenderingDelay GetRenderingDelay() override;
  void GetStatistics(Statistics* statistics) override;
  bool RequiresDecryption() override;

  MediaPipelineBackendManager* const backend_manager_;
  AudioDecoderSoftwareWrapper decoder_;
  const AudioContentType content_type_;

  MediaPipelineBackendManager::BufferDelegate* const buffer_delegate_;
  bool delegate_active_;

  float global_volume_multiplier_;
  float stream_volume_multiplier_;

  DISALLOW_COPY_AND_ASSIGN(AudioDecoderWrapper);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_DECODER_WRAPPER_H_
