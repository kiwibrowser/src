// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/keychain_password_mac.h"

#include "crypto/mock_apple_keychain.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using crypto::MockAppleKeychain;

// Test that if we have an existing password in the Keychain and we are
// authorized by the user to read it then we get it back correctly.
TEST(KeychainPasswordTest, FindPasswordSuccess) {
  MockAppleKeychain keychain;
  keychain.set_find_generic_result(noErr);
  KeychainPassword password(keychain);
  EXPECT_FALSE(password.GetPassword().empty());
  EXPECT_FALSE(keychain.called_add_generic());
  EXPECT_EQ(0, keychain.password_data_count());
}

// Test that if we do not have an existing password in the Keychain then it
// gets added successfully and returned.
TEST(KeychainPasswordTest, FindPasswordNotFound) {
  MockAppleKeychain keychain;
  keychain.set_find_generic_result(errSecItemNotFound);
  KeychainPassword password(keychain);
  EXPECT_EQ(24U, password.GetPassword().length());
  EXPECT_TRUE(keychain.called_add_generic());
  EXPECT_EQ(0, keychain.password_data_count());
}

// Test that if get denied access by the user then we return an empty password.
// And we should not try to add one.
TEST(KeychainPasswordTest, FindPasswordNotAuthorized) {
  MockAppleKeychain keychain;
  keychain.set_find_generic_result(errSecAuthFailed);
  KeychainPassword password(keychain);
  EXPECT_TRUE(password.GetPassword().empty());
  EXPECT_FALSE(keychain.called_add_generic());
  EXPECT_EQ(0, keychain.password_data_count());
}

// Test that if some random other error happens then we return an empty
// password, and we should not try to add one.
TEST(KeychainPasswordTest, FindPasswordOtherError) {
  MockAppleKeychain keychain;
  keychain.set_find_generic_result(errSecNotAvailable);
  KeychainPassword password(keychain);
  EXPECT_TRUE(password.GetPassword().empty());
  EXPECT_FALSE(keychain.called_add_generic());
  EXPECT_EQ(0, keychain.password_data_count());
}

// Test that subsequent additions to the keychain give different passwords.
TEST(KeychainPasswordTest, PasswordsDiffer) {
  MockAppleKeychain keychain1;
  keychain1.set_find_generic_result(errSecItemNotFound);
  KeychainPassword encryptor_password1(keychain1);
  std::string password1 = encryptor_password1.GetPassword();
  EXPECT_FALSE(password1.empty());
  EXPECT_TRUE(keychain1.called_add_generic());
  EXPECT_EQ(0, keychain1.password_data_count());

  MockAppleKeychain keychain2;
  keychain2.set_find_generic_result(errSecItemNotFound);
  KeychainPassword encryptor_password2(keychain2);
  std::string password2 = encryptor_password2.GetPassword();
  EXPECT_FALSE(password2.empty());
  EXPECT_TRUE(keychain2.called_add_generic());
  EXPECT_EQ(0, keychain2.password_data_count());

  // And finally check that the passwords are different.
  EXPECT_NE(password1, password2);
}

}  // namespace
