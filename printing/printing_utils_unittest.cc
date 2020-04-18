// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/strings/utf_string_conversions.h"
#include "printing/printing_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace printing {

namespace {

const size_t kTestLength = 8;

std::string Simplify(const std::string& title) {
  return base::UTF16ToUTF8(
      SimplifyDocumentTitleWithLength(base::UTF8ToUTF16(title), kTestLength));
}

std::string Format(const std::string& owner, const std::string& title) {
  return base::UTF16ToUTF8(FormatDocumentTitleWithOwnerAndLength(
      base::UTF8ToUTF16(owner), base::UTF8ToUTF16(title), kTestLength));
}

}  // namespace

TEST(PrintingUtilsTest, SimplifyDocumentTitle) {
  EXPECT_EQ("", Simplify(""));
  EXPECT_EQ("abcdefgh", Simplify("abcdefgh"));
  EXPECT_EQ("abc...ij", Simplify("abcdefghij"));
  EXPECT_EQ("Controls", Simplify("C\ron\nt\15rols"));
  EXPECT_EQ("C:_foo_", Simplify("C:\\foo\\"));
  EXPECT_EQ("", Simplify("\n\r\n\r\t\r"));
}

TEST(PrintingUtilsTest, FormatDocumentTitleWithOwner) {
  EXPECT_EQ(": ", Format("", ""));
  EXPECT_EQ("abc: ", Format("abc", ""));
  EXPECT_EQ(": 123", Format("", "123"));
  EXPECT_EQ("abc: 123", Format("abc", "123"));
  EXPECT_EQ("abc: 0.9", Format("abc", "0123456789"));
  EXPECT_EQ("ab...j: ", Format("abcdefghij", "123"));
  EXPECT_EQ("xyz: _.o", Format("xyz", "\\f\\oo"));
  EXPECT_EQ("ab...j: ", Format("abcdefghij", "0123456789"));
}

}  // namespace printing
