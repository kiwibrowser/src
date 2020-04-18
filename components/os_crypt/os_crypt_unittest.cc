// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/os_crypt.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "components/os_crypt/os_crypt_mocker.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
#include "components/os_crypt/os_crypt_mocker_linux.h"
#endif

namespace {

class OSCryptTest : public testing::Test {
 public:
  OSCryptTest() { OSCryptMocker::SetUp(); }

  ~OSCryptTest() override { OSCryptMocker::TearDown(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(OSCryptTest);
};

TEST_F(OSCryptTest, String16EncryptionDecryption) {
  base::string16 plaintext;
  base::string16 result;
  std::string utf8_plaintext;
  std::string utf8_result;
  std::string ciphertext;

  // Test borderline cases (empty strings).
  EXPECT_TRUE(OSCrypt::EncryptString16(plaintext, &ciphertext));
  EXPECT_TRUE(OSCrypt::DecryptString16(ciphertext, &result));
  EXPECT_EQ(plaintext, result);

  // Test a simple string.
  plaintext = base::ASCIIToUTF16("hello");
  EXPECT_TRUE(OSCrypt::EncryptString16(plaintext, &ciphertext));
  EXPECT_TRUE(OSCrypt::DecryptString16(ciphertext, &result));
  EXPECT_EQ(plaintext, result);

  // Test a 16-byte aligned string.  This previously hit a boundary error in
  // base::OSCrypt::Crypt() on Mac.
  plaintext = base::ASCIIToUTF16("1234567890123456");
  EXPECT_TRUE(OSCrypt::EncryptString16(plaintext, &ciphertext));
  EXPECT_TRUE(OSCrypt::DecryptString16(ciphertext, &result));
  EXPECT_EQ(plaintext, result);

  // Test Unicode.
  base::char16 wchars[] = { 0xdbeb, 0xdf1b, 0x4e03, 0x6708, 0x8849,
                            0x661f, 0x671f, 0x56db, 0x597c, 0x4e03,
                            0x6708, 0x56db, 0x6708, 0xe407, 0xdbaf,
                            0xdeb5, 0x4ec5, 0x544b, 0x661f, 0x671f,
                            0x65e5, 0x661f, 0x671f, 0x4e94, 0xd8b1,
                            0xdce1, 0x7052, 0x5095, 0x7c0b, 0xe586, 0};
  plaintext = wchars;
  utf8_plaintext = base::UTF16ToUTF8(plaintext);
  EXPECT_EQ(plaintext, base::UTF8ToUTF16(utf8_plaintext));
  EXPECT_TRUE(OSCrypt::EncryptString16(plaintext, &ciphertext));
  EXPECT_TRUE(OSCrypt::DecryptString16(ciphertext, &result));
  EXPECT_EQ(plaintext, result);
  EXPECT_TRUE(OSCrypt::DecryptString(ciphertext, &utf8_result));
  EXPECT_EQ(utf8_plaintext, base::UTF16ToUTF8(result));

  EXPECT_TRUE(OSCrypt::EncryptString(utf8_plaintext, &ciphertext));
  EXPECT_TRUE(OSCrypt::DecryptString16(ciphertext, &result));
  EXPECT_EQ(plaintext, result);
  EXPECT_TRUE(OSCrypt::DecryptString(ciphertext, &utf8_result));
  EXPECT_EQ(utf8_plaintext, base::UTF16ToUTF8(result));
}

TEST_F(OSCryptTest, EncryptionDecryption) {
  std::string plaintext;
  std::string result;
  std::string ciphertext;

  // Test borderline cases (empty strings).
  ASSERT_TRUE(OSCrypt::EncryptString(plaintext, &ciphertext));
  ASSERT_TRUE(OSCrypt::DecryptString(ciphertext, &result));
  EXPECT_EQ(plaintext, result);

  // Test a simple string.
  plaintext = "hello";
  ASSERT_TRUE(OSCrypt::EncryptString(plaintext, &ciphertext));
  ASSERT_TRUE(OSCrypt::DecryptString(ciphertext, &result));
  EXPECT_EQ(plaintext, result);

  // Make sure it null terminates.
  plaintext.assign("hello", 3);
  ASSERT_TRUE(OSCrypt::EncryptString(plaintext, &ciphertext));
  ASSERT_TRUE(OSCrypt::DecryptString(ciphertext, &result));
  EXPECT_EQ(plaintext, "hel");
}

TEST_F(OSCryptTest, CypherTextDiffers) {
  std::string plaintext;
  std::string result;
  std::string ciphertext;

  // Test borderline cases (empty strings).
  ASSERT_TRUE(OSCrypt::EncryptString(plaintext, &ciphertext));
  ASSERT_TRUE(OSCrypt::DecryptString(ciphertext, &result));
  // |cyphertext| is empty on the Mac, different on Windows.
  EXPECT_TRUE(ciphertext.empty() || plaintext != ciphertext);
  EXPECT_EQ(plaintext, result);

  // Test a simple string.
  plaintext = "hello";
  ASSERT_TRUE(OSCrypt::EncryptString(plaintext, &ciphertext));
  ASSERT_TRUE(OSCrypt::DecryptString(ciphertext, &result));
  EXPECT_NE(plaintext, ciphertext);
  EXPECT_EQ(plaintext, result);

  // Make sure it null terminates.
  plaintext.assign("hello", 3);
  ASSERT_TRUE(OSCrypt::EncryptString(plaintext, &ciphertext));
  ASSERT_TRUE(OSCrypt::DecryptString(ciphertext, &result));
  EXPECT_NE(plaintext, ciphertext);
  EXPECT_EQ(result, "hel");
}

TEST_F(OSCryptTest, DecryptError) {
  std::string plaintext;
  std::string result;
  std::string ciphertext;

  // Test a simple string, messing with ciphertext prior to decrypting.
  plaintext = "hello";
  ASSERT_TRUE(OSCrypt::EncryptString(plaintext, &ciphertext));
  EXPECT_NE(plaintext, ciphertext);
  ASSERT_LT(4UL, ciphertext.size());
  ciphertext[3] = ciphertext[3] + 1;
  EXPECT_FALSE(OSCrypt::DecryptString(ciphertext, &result));
  EXPECT_NE(plaintext, result);
  EXPECT_TRUE(result.empty());
}

class OSCryptConcurrencyTest : public testing::Test {
 public:
  OSCryptConcurrencyTest() { OSCryptMocker::SetUp(); }

  ~OSCryptConcurrencyTest() override { OSCryptMocker::TearDown(); };

 private:
  DISALLOW_COPY_AND_ASSIGN(OSCryptConcurrencyTest);
};

TEST_F(OSCryptConcurrencyTest, ConcurrentInitialization) {
  // Launch multiple threads
  base::Thread thread1("thread1");
  base::Thread thread2("thread2");
  std::vector<base::Thread*> threads = {&thread1, &thread2};
  for (base::Thread* thread : threads) {
    ASSERT_TRUE(thread->Start());
  }

  // Make calls.
  for (base::Thread* thread : threads) {
    ASSERT_TRUE(thread->task_runner()->PostTask(
        FROM_HERE, base::Bind([]() -> void {
          std::string plaintext = "secrets";
          std::string encrypted;
          std::string decrypted;
          ASSERT_TRUE(OSCrypt::EncryptString(plaintext, &encrypted));
          ASSERT_TRUE(OSCrypt::DecryptString(encrypted, &decrypted));
          ASSERT_EQ(plaintext, decrypted);
        })));
  }

  // Cleanup
  for (base::Thread* thread : threads) {
    thread->Stop();
  }
}

}  // namespace
