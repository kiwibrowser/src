// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_FRAME_CRYPTO_H_
#define STREAMING_CAST_FRAME_CRYPTO_H_

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <vector>

#include "osp_base/macros.h"
#include "streaming/cast/encoded_frame.h"
#include "third_party/boringssl/src/include/openssl/aes.h"

namespace openscreen {
namespace cast_streaming {

// A subclass of EncodedFrame that represents an EncodedFrame with encrypted
// payload data. It can only be value-constructed by FrameCrypto, but can be
// moved freely thereafter. Use FrameCrypto (below) to explicitly convert
// between EncryptedFrames and EncodedFrames.
struct EncryptedFrame : public EncodedFrame {
  ~EncryptedFrame();
  EncryptedFrame(EncryptedFrame&&) MAYBE_NOEXCEPT;
  EncryptedFrame& operator=(EncryptedFrame&&) MAYBE_NOEXCEPT;

 private:
  friend class FrameCrypto;
  EncryptedFrame();

  OSP_DISALLOW_COPY_AND_ASSIGN(EncryptedFrame);
};

// Encrypts EncodedFrames before sending, or decrypts EncryptedFrames that have
// been received.
class FrameCrypto {
 public:
  // Construct with the given 16-bytes AES key and IV mask. Both arguments
  // should be randomly-generated for each new streaming session.
  // GenerateRandomBytes() can be used to create them.
  FrameCrypto(const std::array<uint8_t, 16>& aes_key,
              const std::array<uint8_t, 16>& cast_iv_mask);

  ~FrameCrypto();

  EncryptedFrame Encrypt(const EncodedFrame& encoded_frame) const;
  EncodedFrame Decrypt(const EncryptedFrame& encrypted_frame) const;

  // Returns random bytes from a cryptographically-secure RNG source.
  static std::array<uint8_t, 16> GenerateRandomBytes();

 private:
  // The 244-byte AES_KEY struct, derived from the |aes_key| passed to the ctor,
  // and initialized by boringssl's AES_set_encrypt_key() function.
  const AES_KEY aes_key_;

  // Random bytes used in the custom heuristic to generate a different
  // initialization vector for each frame.
  const std::array<uint8_t, 16> cast_iv_mask_;

  // AES-CTR is symmetric. Thus, the "meat" of both Encrypt() and Decrypt() is
  // the same.
  std::vector<uint8_t> EncryptCommon(FrameId frame_id,
                                     const std::vector<uint8_t>& in) const;
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_FRAME_CRYPTO_H_
