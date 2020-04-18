// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_icon_sizes_parser.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class WebIconSizesParserTest : public testing::Test {};

TEST(WebIconSizesParserTest, parseSizes) {
  WebString sizes_attribute = "32x33";
  WebVector<WebSize> sizes;
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(1U, sizes.size());
  EXPECT_EQ(32, sizes[0].width);
  EXPECT_EQ(33, sizes[0].height);

  sizes_attribute = "0x33";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(0U, sizes.size());

  UChar attribute[] = {'3', '2', 'x', '3', '3', 0};
  sizes_attribute = AtomicString(attribute);
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(1U, sizes.size());
  EXPECT_EQ(32, sizes[0].width);
  EXPECT_EQ(33, sizes[0].height);

  sizes_attribute = "   32x33   16X17    128x129   ";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(3U, sizes.size());
  EXPECT_EQ(32, sizes[0].width);
  EXPECT_EQ(33, sizes[0].height);
  EXPECT_EQ(16, sizes[1].width);
  EXPECT_EQ(17, sizes[1].height);
  EXPECT_EQ(128, sizes[2].width);
  EXPECT_EQ(129, sizes[2].height);

  sizes_attribute = "  \n 32x33 \r  16X17 \t   128x129 \f  ";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(3U, sizes.size());

  sizes_attribute = "any";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(1U, sizes.size());
  EXPECT_EQ(0, sizes[0].width);
  EXPECT_EQ(0, sizes[0].height);

  sizes_attribute = "ANY";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(1U, sizes.size());

  sizes_attribute = "AnY";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(1U, sizes.size());

  sizes_attribute = "an";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(0U, sizes.size());

  sizes_attribute = "32x33 32";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(1U, sizes.size());
  EXPECT_EQ(32, sizes[0].width);
  EXPECT_EQ(33, sizes[0].height);

  sizes_attribute = "32x33 32x";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(1U, sizes.size());
  EXPECT_EQ(32, sizes[0].width);
  EXPECT_EQ(33, sizes[0].height);

  sizes_attribute = "32x33 x32";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(1U, sizes.size());
  EXPECT_EQ(32, sizes[0].width);
  EXPECT_EQ(33, sizes[0].height);

  sizes_attribute = "32x33 any";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(2U, sizes.size());
  EXPECT_EQ(32, sizes[0].width);
  EXPECT_EQ(33, sizes[0].height);
  EXPECT_EQ(0, sizes[1].width);
  EXPECT_EQ(0, sizes[1].height);

  sizes_attribute = "32x33, 64x64";
  sizes = WebIconSizesParser::ParseIconSizes(sizes_attribute);
  ASSERT_EQ(1U, sizes.size());
  EXPECT_EQ(64, sizes[0].width);
  EXPECT_EQ(64, sizes[0].height);
}

}  // namespace blink
