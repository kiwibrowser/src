// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "content/common/mac/attributed_string_coder.h"

#include <AppKit/AppKit.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"

using mac::AttributedStringCoder;

class AttributedStringCoderTest : public testing::Test {
 public:
  NSMutableAttributedString* NewAttrString() {
    NSString* str = @"The quick brown fox jumped over the lazy dog.";
    return [[NSMutableAttributedString alloc] initWithString:str];
  }

  NSDictionary* FontAttribute(NSString* name, CGFloat size) {
    NSFont* font = [NSFont fontWithName:name size:size];
    return [NSDictionary dictionaryWithObject:font forKey:NSFontAttributeName];
  }

  NSAttributedString* EncodeAndDecode(NSAttributedString* str) {
    std::unique_ptr<const AttributedStringCoder::EncodedString> encoded_str(
        AttributedStringCoder::Encode(str));
    return AttributedStringCoder::Decode(encoded_str.get());
  }
};

TEST_F(AttributedStringCoderTest, SimpleString) {
  base::scoped_nsobject<NSMutableAttributedString> attr_str(NewAttrString());
  [attr_str addAttributes:FontAttribute(@"Helvetica", 12.5)
                    range:NSMakeRange(0, [attr_str length])];

  NSAttributedString* decoded = EncodeAndDecode(attr_str.get());
  EXPECT_NSEQ(attr_str.get(), decoded);
}

TEST_F(AttributedStringCoderTest, NoAttributes) {
  base::scoped_nsobject<NSAttributedString> attr_str(NewAttrString());
  NSAttributedString* decoded = EncodeAndDecode(attr_str.get());
  EXPECT_NSEQ(attr_str.get(), decoded);
}

TEST_F(AttributedStringCoderTest, StripColor) {
  base::scoped_nsobject<NSMutableAttributedString> attr_str(NewAttrString());
  const NSUInteger kStringLength = [attr_str length];
  [attr_str addAttribute:NSFontAttributeName
                   value:[NSFont systemFontOfSize:26]
                   range:NSMakeRange(0, kStringLength)];
  [attr_str addAttribute:NSForegroundColorAttributeName
                   value:[NSColor redColor]
                   range:NSMakeRange(0, kStringLength)];

  NSAttributedString* decoded = EncodeAndDecode(attr_str.get());

  NSRange range;
  NSDictionary* attrs = [decoded attributesAtIndex:0 effectiveRange:&range];
  EXPECT_TRUE(NSEqualRanges(NSMakeRange(0, kStringLength), range));
  EXPECT_NSEQ([NSFont systemFontOfSize:26],
              [attrs objectForKey:NSFontAttributeName]);
  EXPECT_FALSE([attrs objectForKey:NSForegroundColorAttributeName]);
}

TEST_F(AttributedStringCoderTest, MultipleFonts) {
  base::scoped_nsobject<NSMutableAttributedString> attr_str(NewAttrString());
  [attr_str setAttributes:FontAttribute(@"Courier", 12)
                    range:NSMakeRange(0, 10)];
  [attr_str addAttributes:FontAttribute(@"Helvetica", 16)
                    range:NSMakeRange(12, 6)];
  [attr_str addAttributes:FontAttribute(@"Helvetica", 14)
                    range:NSMakeRange(15, 5)];

  NSAttributedString* decoded = EncodeAndDecode(attr_str);

  EXPECT_NSEQ(attr_str.get(), decoded);
}

TEST_F(AttributedStringCoderTest, NoPertinentAttributes) {
  base::scoped_nsobject<NSMutableAttributedString> attr_str(NewAttrString());
  [attr_str addAttribute:NSForegroundColorAttributeName
                   value:[NSColor blueColor]
                   range:NSMakeRange(0, 10)];
  [attr_str addAttribute:NSBackgroundColorAttributeName
                   value:[NSColor blueColor]
                   range:NSMakeRange(15, 5)];
  [attr_str addAttribute:NSKernAttributeName
                   value:[NSNumber numberWithFloat:2.6]
                   range:NSMakeRange(11, 3)];

  NSAttributedString* decoded = EncodeAndDecode(attr_str.get());

  base::scoped_nsobject<NSAttributedString> expected(NewAttrString());
  EXPECT_NSEQ(expected.get(), decoded);
}

TEST_F(AttributedStringCoderTest, NilString) {
  NSAttributedString* decoded = EncodeAndDecode(nil);
  EXPECT_TRUE(decoded);
  EXPECT_EQ(0U, [decoded length]);
}

TEST_F(AttributedStringCoderTest, OutOfRange) {
  NSFont* system_font = [NSFont systemFontOfSize:10];
  base::string16 font_name = base::SysNSStringToUTF16([system_font fontName]);
  AttributedStringCoder::EncodedString encoded(
      base::ASCIIToUTF16("Hello World"));
  encoded.attributes()->push_back(
      AttributedStringCoder::FontAttribute(font_name, 12, gfx::Range(0, 5)));
  encoded.attributes()->push_back(
      AttributedStringCoder::FontAttribute(font_name, 14, gfx::Range(5, 100)));
  encoded.attributes()->push_back(
      AttributedStringCoder::FontAttribute(font_name, 16, gfx::Range(100, 5)));

  NSAttributedString* decoded = AttributedStringCoder::Decode(&encoded);
  EXPECT_TRUE(decoded);

  NSRange range;
  NSDictionary* attrs = [decoded attributesAtIndex:0 effectiveRange:&range];
  EXPECT_NSEQ([NSFont systemFontOfSize:12],
              [attrs objectForKey:NSFontAttributeName]);
  EXPECT_TRUE(NSEqualRanges(range, NSMakeRange(0, 5)));

  attrs = [decoded attributesAtIndex:5 effectiveRange:&range];
  EXPECT_FALSE([attrs objectForKey:NSFontAttributeName]);
  EXPECT_EQ(0U, [attrs count]);
}
