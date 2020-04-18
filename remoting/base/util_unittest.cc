// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <algorithm>

#include "remoting/base/util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_geometry.h"

namespace remoting {

TEST(ReplaceLfByCrLfTest, Basic) {
  EXPECT_EQ("ab", ReplaceLfByCrLf("ab"));
  EXPECT_EQ("\r\nab", ReplaceLfByCrLf("\nab"));
  EXPECT_EQ("\r\nab\r\n", ReplaceLfByCrLf("\nab\n"));
  EXPECT_EQ("\r\nab\r\ncd", ReplaceLfByCrLf("\nab\ncd"));
  EXPECT_EQ("\r\nab\r\ncd\r\n", ReplaceLfByCrLf("\nab\ncd\n"));
  EXPECT_EQ("\r\n\r\nab\r\n\r\ncd\r\n\r\n",
      ReplaceLfByCrLf("\n\nab\n\ncd\n\n"));
}

TEST(ReplaceLfByCrLfTest, Speed) {
  int kLineSize = 128;
  std::string line(kLineSize, 'a');
  line[kLineSize - 1] = '\n';
  // Make a 10M string.
  int kLineNum = 10 * 1024 * 1024 / kLineSize;
  std::string buffer;
  buffer.resize(kLineNum * kLineSize);
  for (int i = 0; i < kLineNum; ++i) {
    memcpy(&buffer[i * kLineSize], &line[0], kLineSize);
  }
  // Convert the string.
  buffer = ReplaceLfByCrLf(buffer);
  // Check the converted string.
  EXPECT_EQ(static_cast<size_t>((kLineSize + 1) * kLineNum), buffer.size());
  const char* p = &buffer[0];
  for (int i = 0; i < kLineNum; ++i) {
    EXPECT_EQ(0, memcmp(&line[0], p, kLineSize - 1));
    p += kLineSize - 1;
    EXPECT_EQ('\r', *p++);
    EXPECT_EQ('\n', *p++);
  }
}

TEST(ReplaceCrLfByLfTest, Basic) {
  EXPECT_EQ("ab", ReplaceCrLfByLf("ab"));
  EXPECT_EQ("\nab", ReplaceCrLfByLf("\r\nab"));
  EXPECT_EQ("\nab\n", ReplaceCrLfByLf("\r\nab\r\n"));
  EXPECT_EQ("\nab\ncd", ReplaceCrLfByLf("\r\nab\r\ncd"));
  EXPECT_EQ("\nab\ncd\n", ReplaceCrLfByLf("\r\nab\r\ncd\n"));
  EXPECT_EQ("\n\nab\n\ncd\n\n",
      ReplaceCrLfByLf("\r\n\r\nab\r\n\r\ncd\r\n\r\n"));
  EXPECT_EQ("\rab\rcd\r", ReplaceCrLfByLf("\rab\rcd\r"));
}

TEST(ReplaceCrLfByLfTest, Speed) {
  int kLineSize = 128;
  std::string line(kLineSize, 'a');
  line[kLineSize - 2] = '\r';
  line[kLineSize - 1] = '\n';
  // Make a 10M string.
  int kLineNum = 10 * 1024 * 1024 / kLineSize;
  std::string buffer;
  buffer.resize(kLineNum * kLineSize);
  for (int i = 0; i < kLineNum; ++i) {
    memcpy(&buffer[i * kLineSize], &line[0], kLineSize);
  }
  // Convert the string.
  buffer = ReplaceCrLfByLf(buffer);
  // Check the converted string.
  EXPECT_EQ(static_cast<size_t>((kLineSize - 1) * kLineNum), buffer.size());
  const char* p = &buffer[0];
  for (int i = 0; i < kLineNum; ++i) {
    EXPECT_EQ(0, memcmp(&line[0], p, kLineSize - 2));
    p += kLineSize - 2;
    EXPECT_EQ('\n', *p++);
  }
}

TEST(StringIsUtf8Test, Basic) {
  EXPECT_TRUE(StringIsUtf8("", 0));
  EXPECT_TRUE(StringIsUtf8("\0", 1));
  EXPECT_TRUE(StringIsUtf8("abc", 3));
  EXPECT_TRUE(StringIsUtf8("\xc0\x80", 2));
  EXPECT_TRUE(StringIsUtf8("\xe0\x80\x80", 3));
  EXPECT_TRUE(StringIsUtf8("\xf0\x80\x80\x80", 4));
  EXPECT_TRUE(StringIsUtf8("\xf8\x80\x80\x80\x80", 5));
  EXPECT_TRUE(StringIsUtf8("\xfc\x80\x80\x80\x80\x80", 6));

  // Not enough continuation characters
  EXPECT_FALSE(StringIsUtf8("\xc0", 1));
  EXPECT_FALSE(StringIsUtf8("\xe0\x80", 2));
  EXPECT_FALSE(StringIsUtf8("\xf0\x80\x80", 3));
  EXPECT_FALSE(StringIsUtf8("\xf8\x80\x80\x80", 4));
  EXPECT_FALSE(StringIsUtf8("\xfc\x80\x80\x80\x80", 5));

  // One more continuation character than needed
  EXPECT_FALSE(StringIsUtf8("\xc0\x80\x80", 3));
  EXPECT_FALSE(StringIsUtf8("\xe0\x80\x80\x80", 4));
  EXPECT_FALSE(StringIsUtf8("\xf0\x80\x80\x80\x80", 5));
  EXPECT_FALSE(StringIsUtf8("\xf8\x80\x80\x80\x80\x80", 6));
  EXPECT_FALSE(StringIsUtf8("\xfc\x80\x80\x80\x80\x80\x80", 7));

  // Invalid first byte
  EXPECT_FALSE(StringIsUtf8("\xfe\x80\x80\x80\x80\x80\x80", 7));
  EXPECT_FALSE(StringIsUtf8("\xff\x80\x80\x80\x80\x80\x80", 7));

  // Invalid continuation byte
  EXPECT_FALSE(StringIsUtf8("\xc0\x00", 2));
  EXPECT_FALSE(StringIsUtf8("\xc0\x40", 2));
  EXPECT_FALSE(StringIsUtf8("\xc0\xc0", 2));
}

}  // namespace remoting
