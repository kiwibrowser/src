// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "components/base32/base32.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base32 {
namespace {

TEST(Base32Test, EncodesRfcTestVectorsCorrectlyWithoutPadding) {
  // Tests from http://tools.ietf.org/html/rfc4648#section-10.
  constexpr char test_str[] = "foobar";

  constexpr const char* expected[] = {
      "", "MY", "MZXQ", "MZXW6", "MZXW6YQ", "MZXW6YTB", "MZXW6YTBOI"};

  // Run the tests, with one more letter in the input every pass.
  for (size_t i = 0; i < arraysize(expected); ++i) {
    std::string output = Base32Encode(base::StringPiece(test_str, i),
                                      Base32EncodePolicy::OMIT_PADDING);
    EXPECT_EQ(expected[i], output);
  }
}

TEST(Base32Test, EncodesRfcTestVectorsCorrectlyWithPadding) {
  // Tests from http://tools.ietf.org/html/rfc4648#section-10.
  constexpr char test_str[] = "foobar";

  constexpr const char* expected[] = {
      "",         "MY======", "MZXQ====",        "MZXW6===",
      "MZXW6YQ=", "MZXW6YTB", "MZXW6YTBOI======"};

  // Run the tests, with one more letter in the input every pass.
  for (size_t i = 0; i < arraysize(expected); ++i) {
    std::string output = Base32Encode(base::StringPiece(test_str, i));
    EXPECT_EQ(expected[i], output);
  }
}

TEST(Base32Test, EncodesSha256HashCorrectly) {
  // Useful to test with longer input than the RFC test vectors, and encoding
  // SHA-256 hashes is one of the use cases for this component.
  constexpr char hash[] =
      "\x1f\x25\xe1\xca\xba\x4f\xf9\xb8\x27\x24\x83\x0f\xca\x60\xe4\xc2\xbe\xa8"
      "\xc3\xa9\x44\x1c\x27\xb0\xb4\x3e\x6a\x96\x94\xc7\xb8\x04";
  std::string output = Base32Encode(base::StringPiece(hash, 32),
                                    Base32EncodePolicy::OMIT_PADDING);
  EXPECT_EQ("D4S6DSV2J743QJZEQMH4UYHEYK7KRQ5JIQOCPMFUHZVJNFGHXACA", output);
}

}  // namespace
}  // namespace base32
