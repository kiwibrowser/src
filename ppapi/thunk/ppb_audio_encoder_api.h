// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_AUDIO_ENCODER_API_H_
#define PPAPI_THUNK_AUDIO_ENCODER_API_H_

#include <stdint.h>

#include "ppapi/c/ppb_audio_encoder.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {
namespace thunk {

class PPAPI_THUNK_EXPORT PPB_AudioEncoder_API {
 public:
  virtual ~PPB_AudioEncoder_API() {}

  virtual int32_t GetSupportedProfiles(
      const PP_ArrayOutput& output,
      const scoped_refptr<TrackedCallback>& callback) = 0;
  virtual int32_t Initialize(
      uint32_t channels,
      PP_AudioBuffer_SampleRate input_sample_rate,
      PP_AudioBuffer_SampleSize input_sample_size,
      PP_AudioProfile output_profile,
      uint32_t initial_bitrate,
      PP_HardwareAcceleration acceleration,
      const scoped_refptr<TrackedCallback>& callback) = 0;
  virtual int32_t GetNumberOfSamples() = 0;
  virtual int32_t GetBuffer(PP_Resource* audio_buffer,
                            const scoped_refptr<TrackedCallback>& callback) = 0;
  virtual int32_t Encode(PP_Resource audio_buffer,
                         const scoped_refptr<TrackedCallback>& callback) = 0;
  virtual int32_t GetBitstreamBuffer(
      PP_AudioBitstreamBuffer* bitstream_buffer,
      const scoped_refptr<TrackedCallback>& callback) = 0;
  virtual void RecycleBitstreamBuffer(
      const PP_AudioBitstreamBuffer* bitstream_buffer) = 0;
  virtual void RequestBitrateChange(uint32_t bitrate) = 0;
  virtual void Close() = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_AUDIO_ENCODER_API_H_
