// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/wtf/text/text_codec_icu.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/text/character_names.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace WTF {

TEST(TextCodecICUTest, IgnorableCodePoint) {
  TextEncoding iso2022jp("iso-2022-jp");
  std::unique_ptr<TextCodec> codec = TextCodecICU::Create(iso2022jp, nullptr);
  Vector<UChar> source;
  source.push_back('a');
  source.push_back(kZeroWidthJoinerCharacter);
  CString encoded =
      codec->Encode(source.data(), source.size(), kEntitiesForUnencodables);
  EXPECT_STREQ("a&#8205;", encoded.data());
  const String source2(u"ABC~¤•★星🌟星★•¤~XYZ");
  const CString encoded2(codec->Encode(source2.GetCharacters<UChar>(),
                                       source2.length(),
                                       kEntitiesForUnencodables));
  const String source3(u"ABC~&#164;&#8226;★星&#127775;星★&#8226;&#164;~XYZ");
  const CString encoded3(codec->Encode(source3.GetCharacters<UChar>(),
                                       source3.length(),
                                       kEntitiesForUnencodables));
  EXPECT_STREQ(encoded3.data(), encoded2.data());
  EXPECT_STREQ(
      "ABC~&#164;&#8226;\x1B$B!z@1\x1B(B&#127775;\x1B$B@1!z\x1B(B&#8226;&#164;~"
      "XYZ",
      encoded2.data());
}
}
