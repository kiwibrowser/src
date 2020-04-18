// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_DECODER_CAST_AUDIO_DECODER_H_
#define CHROMECAST_MEDIA_CMA_DECODER_CAST_AUDIO_DECODER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace chromecast {
namespace media {
struct AudioConfig;
class DecoderBufferBase;

// Audio decoder interface.
class CastAudioDecoder {
 public:
  enum Status {
    kDecodeOk,
    kDecodeError,
  };

  enum OutputFormat {
    kOutputSigned16,     // Output signed 16-bit interleaved samples.
    kOutputPlanarFloat,  // Output planar float samples.
  };

  // The callback that is called when the decoder initialization is complete.
  // |success| is true if initialization was successful; if |success| is false
  // then the CastAudioDecoder instance is unusable and should be destroyed.
  typedef base::Callback<void(bool success)> InitializedCallback;
  typedef base::Callback<void(
      Status status,
      const scoped_refptr<media::DecoderBufferBase>& decoded)> DecodeCallback;

  // Creates a CastAudioDecoder instance for the given |config|. Decoding must
  // occur on the same thread as |task_runner|. Returns an empty scoped_ptr if
  // the decoder could not be created.
  static std::unique_ptr<CastAudioDecoder> Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      const media::AudioConfig& config,
      OutputFormat output_format,
      const InitializedCallback& initialized_callback);

  // Given a CastAudioDecoder::OutputFormat, return the size of each sample in
  // that OutputFormat in bytes.
  static int OutputFormatSizeInBytes(CastAudioDecoder::OutputFormat format);

  CastAudioDecoder() {}
  virtual ~CastAudioDecoder() {}

  // Converts encoded data to the |output_format|. Must be called on the same
  // thread as |task_runner|. Decoded data will be passed to |decode_callback|.
  // It is OK to call Decode before the |initialized_callback| has been called;
  // those buffers will be queued until initialization completes, at which point
  // they will be decoded in order (if initialization was successful), or
  // ignored if initialization failed.
  // It is OK to pass an end-of-stream DecoderBuffer as |data|.
  // Returns |false| if the decoder is not ready yet.
  virtual bool Decode(const scoped_refptr<media::DecoderBufferBase>& data,
                      const DecodeCallback& decode_callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastAudioDecoder);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_DECODER_CAST_AUDIO_DECODER_H_
