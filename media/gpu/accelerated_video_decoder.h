// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ACCELERATED_VIDEO_DECODER_H_
#define MEDIA_GPU_ACCELERATED_VIDEO_DECODER_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "media/base/decrypt_config.h"
#include "media/gpu/media_gpu_export.h"
#include "ui/gfx/geometry/size.h"

namespace media {

// An AcceleratedVideoDecoder is a video decoder that requires support from an
// external accelerator (typically a hardware accelerator) to partially offload
// the decode process after parsing stream headers, and performing reference
// frame and state management.
class MEDIA_GPU_EXPORT AcceleratedVideoDecoder {
 public:
  AcceleratedVideoDecoder() {}
  virtual ~AcceleratedVideoDecoder() {}

  // Set the buffer at |ptr| of |size| bytes as the current source of encoded
  // stream data. Pictures produced as a result of this call should be assigned
  // the passed stream |id|. |decrypt_config| may specify the decryption
  // configuration of the specified buffer, and in that case, Decode() may
  // return kNoKey.
  virtual void SetStream(int32_t id,
                         const uint8_t* ptr,
                         size_t size,
                         const DecryptConfig* decrypt_config = nullptr) = 0;

  // Have the decoder flush its state and trigger output of all previously
  // decoded surfaces. Return false on failure.
  virtual bool Flush() WARN_UNUSED_RESULT = 0;

  // Stop (pause) decoding, discarding all remaining inputs and outputs,
  // but do not flush decoder state, so that playback can be resumed later,
  // possibly from a different location.
  // To be called during decoding.
  virtual void Reset() = 0;

  enum DecodeResult {
    kDecodeError,  // Error while decoding.
    // TODO(posciak): unsupported streams are currently treated as error
    // in decoding; in future it could perhaps be possible to fall back
    // to software decoding instead.
    // kStreamError,  // Error in stream.
    kAllocateNewSurfaces,  // Need a new set of surfaces to be allocated.
    kRanOutOfStreamData,   // Need more stream data to proceed.
    kRanOutOfSurfaces,     // Waiting for the client to free up output surfaces.
    kNeedContextUpdate,    // Waiting for the client to update decoding context
                           // with data acquired from the accelerator.
    kNoKey,  // The buffer is encrypted and could not be processed because the
             // key for decryption is missing.
  };

  // Try to decode more of the stream, returning decoded frames asynchronously.
  // Return when more stream is needed, when we run out of free surfaces, when
  // we need a new set of them, or when an error occurs.
  virtual DecodeResult Decode() WARN_UNUSED_RESULT = 0;

  // Return dimensions/required number of output surfaces that client should
  // be ready to provide for the decoder to function properly.
  // To be used after Decode() returns kAllocateNewSurfaces.
  virtual gfx::Size GetPicSize() const = 0;
  virtual size_t GetRequiredNumOfPictures() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AcceleratedVideoDecoder);
};

}  //  namespace media

#endif  // MEDIA_GPU_ACCELERATED_VIDEO_DECODER_H_
