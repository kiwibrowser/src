// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/frame_crypto.h"

#include <random>

#include "osp_base/big_endian.h"
#include "osp_base/boringssl_util.h"
#include "third_party/boringssl/src/include/openssl/crypto.h"
#include "third_party/boringssl/src/include/openssl/err.h"
#include "third_party/boringssl/src/include/openssl/rand.h"

namespace openscreen {
namespace cast_streaming {

EncryptedFrame::EncryptedFrame() = default;
EncryptedFrame::~EncryptedFrame() = default;

EncryptedFrame::EncryptedFrame(EncryptedFrame&&) MAYBE_NOEXCEPT = default;
EncryptedFrame& EncryptedFrame::operator=(EncryptedFrame&&)
    MAYBE_NOEXCEPT = default;

FrameCrypto::FrameCrypto(const std::array<uint8_t, 16>& aes_key,
                         const std::array<uint8_t, 16>& cast_iv_mask)
    : aes_key_{}, cast_iv_mask_(cast_iv_mask) {
  // Ensure that the library has been initialized. CRYPTO_library_init() may be
  // safely called multiple times during the life of a process.
  CRYPTO_library_init();

  // Initialize the 244-byte AES_KEY struct once, here at construction time. The
  // const_cast<> is reasonable as this is a one-time-ctor-initialized value
  // that will remain constant from here onward.
  const int return_code = AES_set_encrypt_key(
      aes_key.data(), aes_key.size() * 8, const_cast<AES_KEY*>(&aes_key_));
  if (return_code != 0) {
    LogAndClearBoringSslErrors();
    OSP_LOG_FATAL << "Failure when setting encryption key; unsafe to continue.";
    OSP_NOTREACHED();
  }
}

FrameCrypto::~FrameCrypto() = default;

EncryptedFrame FrameCrypto::Encrypt(const EncodedFrame& encoded_frame) const {
  EncryptedFrame result;
  encoded_frame.CopyMetadataTo(&result);
  result.data = EncryptCommon(encoded_frame.frame_id, encoded_frame.data);
  return result;
}

EncodedFrame FrameCrypto::Decrypt(const EncryptedFrame& encrypted_frame) const {
  EncodedFrame result;
  encrypted_frame.CopyMetadataTo(&result);
  // AES-CTC is symmetric. Thus, decryption back to the plaintext is the same as
  // encrypting the ciphertext.
  result.data = EncryptCommon(encrypted_frame.frame_id, encrypted_frame.data);
  return result;
}

std::vector<uint8_t> FrameCrypto::EncryptCommon(
    FrameId frame_id,
    const std::vector<uint8_t>& in) const {
  OSP_DCHECK(!frame_id.is_null());

  // Compute the AES nonce for Cast Streaming payload encryption, which is based
  // on the |frame_id|.
  std::array<uint8_t, 16> aes_nonce{/* zero initialized */};
  static_assert(AES_BLOCK_SIZE == sizeof(aes_nonce),
                "AES_BLOCK_SIZE is not 16 bytes.");
  WriteBigEndian<uint32_t>(frame_id.lower_32_bits(), aes_nonce.data() + 8);
  for (size_t i = 0; i < aes_nonce.size(); ++i) {
    aes_nonce[i] ^= cast_iv_mask_[i];
  }

  std::array<uint8_t, 16> ecount_buf{/* zero initialized */};
  unsigned int block_offset = 0;
  std::vector<uint8_t> out(in.size());
  AES_ctr128_encrypt(in.data(), out.data(), in.size(), &aes_key_,
                     aes_nonce.data(), ecount_buf.data(), &block_offset);
  return out;
}

// static
std::array<uint8_t, 16> FrameCrypto::GenerateRandomBytes() {
  std::array<uint8_t, 16> result;
  const int return_code = RAND_bytes(result.data(), sizeof(result));
  if (return_code != 1) {
    LogAndClearBoringSslErrors();
    OSP_LOG_FATAL
        << "Failure when generating random bytes; unsafe to continue.";
    OSP_NOTREACHED();
  }
  return result;
}

}  // namespace cast_streaming
}  // namespace openscreen
