// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/frame_crypto.h"

#include <array>
#include <cstring>
#include <vector>

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace cast_streaming {
namespace {

TEST(FrameCryptoTest, EncryptsAndDecryptsFrames) {
  // Prepare two frames with different FrameIds, but having the same payload
  // bytes.
  EncodedFrame frame0;
  frame0.frame_id = FrameId::first();
  const char kPayload[] = "The quick brown fox jumps over the lazy dog.";
  frame0.data.assign(
      reinterpret_cast<const uint8_t*>(kPayload),
      reinterpret_cast<const uint8_t*>(kPayload) + sizeof(kPayload));
  EncodedFrame frame1;
  frame1.frame_id = frame0.frame_id + 1;
  frame1.data = frame0.data;

  const std::array<uint8_t, 16> key = FrameCrypto::GenerateRandomBytes();
  const std::array<uint8_t, 16> iv = FrameCrypto::GenerateRandomBytes();
  EXPECT_NE(0, memcmp(key.data(), iv.data(), sizeof(key)));
  const FrameCrypto crypto(key, iv);

  // Encrypt both frames, and confirm the encrypted data is something other than
  // the plaintext, and that both frames have different encrypted data.
  const EncryptedFrame encrypted_frame0 = crypto.Encrypt(frame0);
  EXPECT_EQ(frame0.frame_id, encrypted_frame0.frame_id);
  ASSERT_EQ(frame0.data.size(), encrypted_frame0.data.size());
  EXPECT_NE(0, memcmp(frame0.data.data(), encrypted_frame0.data.data(),
                      frame0.data.size()));
  const EncryptedFrame encrypted_frame1 = crypto.Encrypt(frame1);
  EXPECT_EQ(frame1.frame_id, encrypted_frame1.frame_id);
  ASSERT_EQ(frame1.data.size(), encrypted_frame1.data.size());
  EXPECT_NE(0, memcmp(frame1.data.data(), encrypted_frame1.data.data(),
                      frame1.data.size()));
  ASSERT_EQ(encrypted_frame0.data.size(), encrypted_frame1.data.size());
  EXPECT_NE(0,
            memcmp(encrypted_frame0.data.data(), encrypted_frame1.data.data(),
                   encrypted_frame0.data.size()));

  // Now, decrypt the encrypted frames, and confirm the original payload
  // plaintext is retrieved.
  const EncodedFrame decrypted_frame0 = crypto.Decrypt(encrypted_frame0);
  EXPECT_EQ(frame0.frame_id, decrypted_frame0.frame_id);
  ASSERT_EQ(frame0.data.size(), decrypted_frame0.data.size());
  EXPECT_EQ(0, memcmp(frame0.data.data(), decrypted_frame0.data.data(),
                      frame0.data.size()));
  const EncodedFrame decrypted_frame1 = crypto.Decrypt(encrypted_frame1);
  EXPECT_EQ(frame1.frame_id, decrypted_frame1.frame_id);
  ASSERT_EQ(frame1.data.size(), decrypted_frame1.data.size());
  EXPECT_EQ(0, memcmp(frame1.data.data(), decrypted_frame1.data.data(),
                      frame1.data.size()));
}

}  // namespace
}  // namespace cast_streaming
}  // namespace openscreen
