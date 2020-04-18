// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/interfaces/encryption_scheme_struct_traits.h"

#include <utility>

#include "media/base/encryption_scheme.h"
#include "media/base/media_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

TEST(EncryptionSchemeStructTraitsTest,
     ConvertEncryptionSchemeAesCbcWithPattern) {
  EncryptionScheme input(EncryptionScheme::CIPHER_MODE_AES_CBC,
                         EncryptionPattern(1, 9));
  std::vector<uint8_t> data = media::mojom::EncryptionScheme::Serialize(&input);

  EncryptionScheme output;
  EXPECT_TRUE(
      media::mojom::EncryptionScheme::Deserialize(std::move(data), &output));
  EXPECT_TRUE(output.Matches(input));

  // Verify a couple of negative cases.
  EXPECT_FALSE(output.Matches(Unencrypted()));
  EXPECT_FALSE(output.Matches(AesCtrEncryptionScheme()));
}

}  // namespace media
