// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resources/chromeos/zip_archiver/cpp/char_coding.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ZipArchiverTest, TestEncodeShortWords) {
  EXPECT_EQ(std::string("Hello, world!"),
            Cp437ToUtf8(std::string("Hello, world!")));
  EXPECT_EQ(std::string("á.txt"), Cp437ToUtf8(std::string("\xa0.txt")));
}

TEST(ZipArchiverTest, TestEncodePrintableAscii) {
  for (int c = 0; c <= 0x7f; c++) {
    if (!isprint(c))
      continue;
    std::string input(1, static_cast<char>(c));
    EXPECT_EQ(input, Cp437ToUtf8(input));
  }
}

TEST(ZipArchiverTest, TestEncodeNonAscii) {
  std::string expected(
      "ÇüéâäàåçêëèïîìÄÅ"
      "ÉæÆôöòûùÿÖÜ¢£¥₧ƒ"
      "áíóúñÑªº¿⌐¬½¼¡«»"
      "░▒▓│┤╡╢╖╕╣║╗╝╜╛┐"
      "└┴┬├─┼╞╟╚╔╩╦╠═╬╧"
      "╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀"
      "αßΓπΣσµτΦΘΩδ∞φε∩"
      "≡±≥≤⌠⌡÷≈°∙·√ⁿ²■\u00a0");  // \u00a0: NO-BREAK SPACE
  std::string input;
  for (int i = 0; i < 128; i++) {
    input.append(1, static_cast<char>(0x80 + i));
  }
  EXPECT_EQ(expected, Cp437ToUtf8(input));
}
