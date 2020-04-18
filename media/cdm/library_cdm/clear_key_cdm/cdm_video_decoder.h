// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_CDM_VIDEO_DECODER_H_
#define MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_CDM_VIDEO_DECODER_H_

#include <stdint.h>

#include <memory>

#include "media/cdm/api/content_decryption_module.h"

namespace media {

class CdmHostProxy;

class CdmVideoDecoder {
 public:
  virtual ~CdmVideoDecoder() {}
  virtual bool Initialize(const cdm::VideoDecoderConfig_2& config) = 0;
  virtual void Deinitialize() = 0;
  virtual void Reset() = 0;
  virtual bool is_initialized() const = 0;

  // Decodes |compressed_frame|. Stores output frame in |decoded_frame| and
  // returns |cdm::kSuccess| when an output frame is available. Returns
  // |cdm::kNeedMoreData| when |compressed_frame| does not produce an output
  // frame. Returns |cdm::kDecodeError| when decoding fails.
  //
  // A null |compressed_frame| will attempt to flush the decoder of any
  // remaining frames. |compressed_frame_size| and |timestamp| are ignored.
  virtual cdm::Status DecodeFrame(const uint8_t* compressed_frame,
                                  int32_t compressed_frame_size,
                                  int64_t timestamp,
                                  cdm::VideoFrame* decoded_frame) = 0;
};

// Initializes the appropriate video decoder based on build flags and the value
// of |config.codec|. Returns a scoped_ptr containing a non-null initialized
// CdmVideoDecoder pointer upon success.
std::unique_ptr<CdmVideoDecoder> CreateVideoDecoder(
    CdmHostProxy* cdm_host_proxy,
    const cdm::VideoDecoderConfig_2& config);

}  // namespace media

#endif  // MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_CDM_VIDEO_DECODER_H_
