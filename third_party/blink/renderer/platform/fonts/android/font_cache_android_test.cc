// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/font_cache.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/fonts/simple_font_data.h"

namespace blink {

TEST(FontCacheAndroid, fallbackFontForCharacter) {
  // A Latin character in the common locale system font, but not in the
  // Chinese locale-preferred font.
  const UChar32 kTestChar = 228;

  FontDescription font_description;
  font_description.SetLocale(LayoutLocale::Get("zh"));
  ASSERT_EQ(USCRIPT_SIMPLIFIED_HAN, font_description.GetScript());
  font_description.SetGenericFamily(FontDescription::kStandardFamily);

  FontCache* font_cache = FontCache::GetFontCache();
  ASSERT_TRUE(font_cache);
  scoped_refptr<SimpleFontData> font_data =
      font_cache->FallbackFontForCharacter(font_description, kTestChar, 0);
  EXPECT_TRUE(font_data);
}

TEST(FontCacheAndroid, genericFamilyNameForScript) {
  FontDescription english;
  english.SetLocale(LayoutLocale::Get("en"));
  FontDescription chinese;
  chinese.SetLocale(LayoutLocale::Get("zh"));

  if (FontFamilyNames::webkit_standard.IsEmpty())
    FontFamilyNames::init();

  // For non-CJK, getGenericFamilyNameForScript should return the given
  // familyName.
  EXPECT_EQ(FontFamilyNames::webkit_standard,
            FontCache::GetGenericFamilyNameForScript(
                FontFamilyNames::webkit_standard, english));
  EXPECT_EQ(FontFamilyNames::webkit_monospace,
            FontCache::GetGenericFamilyNameForScript(
                FontFamilyNames::webkit_monospace, english));

  // For CJK, getGenericFamilyNameForScript should return CJK fonts except
  // monospace.
  EXPECT_NE(FontFamilyNames::webkit_standard,
            FontCache::GetGenericFamilyNameForScript(
                FontFamilyNames::webkit_standard, chinese));
  EXPECT_EQ(FontFamilyNames::webkit_monospace,
            FontCache::GetGenericFamilyNameForScript(
                FontFamilyNames::webkit_monospace, chinese));
}

}  // namespace blink
