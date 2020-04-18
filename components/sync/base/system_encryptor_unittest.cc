// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/system_encryptor.h"

#include "build/build_config.h"
#include "components/os_crypt/os_crypt_mocker.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

const char kPlaintext[] = "The Magic Words are Squeamish Ossifrage";

class SystemEncryptorTest : public testing::Test {
 protected:
  SystemEncryptor encryptor_;
};

TEST_F(SystemEncryptorTest, EncryptDecrypt) {
  // SystemEncryptor ends up needing access to the keychain on OS X,
  // so use the mock keychain to prevent prompts.
  ::OSCryptMocker::SetUp();
  std::string ciphertext;
  EXPECT_TRUE(encryptor_.EncryptString(kPlaintext, &ciphertext));
  EXPECT_NE(kPlaintext, ciphertext);
  std::string plaintext;
  EXPECT_TRUE(encryptor_.DecryptString(ciphertext, &plaintext));
  EXPECT_EQ(kPlaintext, plaintext);
  ::OSCryptMocker::TearDown();
}

}  // namespace

}  // namespace syncer
