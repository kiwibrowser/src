// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/session_keys.h"

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cryptauth {
namespace {

// Values generated using the Android implementation.
const char kMasterKeyHex[] =
    "f611126a04302551ac1e8ed512952ee287a1d2561e2a2c72e7bf1ebe4bdc74ce";
const char kInitiatorKeyHex[] =
    "787ec48783f0a1f9fb9c5bc0230c2e7f45b8783acf8c9bd1c63242df9da31999";
const char kResponderKeyHex[] =
    "a366ec1f9cf327b69c341211216545cc302379078229eae78b43d60c110a6fba";

}  // namespace

class CryptAuthSessionKeysTest : public testing::Test {
 protected:
  CryptAuthSessionKeysTest() {}

  DISALLOW_COPY_AND_ASSIGN(CryptAuthSessionKeysTest);
};

TEST_F(CryptAuthSessionKeysTest, GenerateKeys) {
  std::vector<uint8_t> data;

  std::string master_key(kMasterKeyHex);
  ASSERT_TRUE(base::HexStringToBytes(master_key, &data));
  master_key.assign(reinterpret_cast<char*>(&data[0]), data.size());

  data.clear();
  std::string initiator_key(kInitiatorKeyHex);
  ASSERT_TRUE(base::HexStringToBytes(initiator_key, &data));
  initiator_key.assign(reinterpret_cast<char*>(&data[0]), data.size());

  data.clear();
  std::string responder_key(kResponderKeyHex);
  ASSERT_TRUE(base::HexStringToBytes(responder_key, &data));
  responder_key.assign(reinterpret_cast<char*>(&data[0]), data.size());

  SessionKeys session_keys(master_key);
  EXPECT_EQ(initiator_key, session_keys.initiator_encode_key());
  EXPECT_EQ(responder_key, session_keys.responder_encode_key());
}

}  // namespace cryptauth
