// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSING_PIPELINE_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSING_PIPELINE_H_

#include <memory>
#include <vector>

#include "chromecast/public/volume_control.h"

namespace base {
class ListValue;
}  // namespace base

namespace chromecast {
namespace media {

class PostProcessingPipeline {
 public:
  virtual ~PostProcessingPipeline() = default;
  virtual int ProcessFrames(float* data,
                            int num_frames,
                            float current_multiplier,
                            bool is_silence) = 0;
  virtual float* GetOutputBuffer() = 0;
  virtual int NumOutputChannels() = 0;

  virtual bool SetSampleRate(int sample_rate) = 0;
  virtual bool IsRinging() = 0;
  virtual void SetPostProcessorConfig(const std::string& name,
                                      const std::string& config) = 0;
  virtual void SetContentType(AudioContentType content_type) = 0;
  virtual void UpdatePlayoutChannel(int channel) = 0;
};

class PostProcessingPipelineFactory {
 public:
  virtual ~PostProcessingPipelineFactory() = default;

  virtual std::unique_ptr<PostProcessingPipeline> CreatePipeline(
      const std::string& name,
      const base::ListValue* filter_description_list,
      int num_channels) = 0;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSING_PIPELINE_H_
