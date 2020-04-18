// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/layout_locale.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/fonts/font_global_context.h"

namespace blink {

TEST(LayoutLocaleTest, Get) {
  FontGlobalContext::ClearForTesting();

  EXPECT_EQ(nullptr, LayoutLocale::Get(g_null_atom));

  EXPECT_EQ(g_empty_atom, LayoutLocale::Get(g_empty_atom)->LocaleString());

  EXPECT_STRCASEEQ("en-us",
                   LayoutLocale::Get("en-us")->LocaleString().Ascii().data());
  EXPECT_STRCASEEQ("ja-jp",
                   LayoutLocale::Get("ja-jp")->LocaleString().Ascii().data());

  FontGlobalContext::ClearForTesting();
}

TEST(LayoutLocaleTest, GetCaseInsensitive) {
  const LayoutLocale* en_us = LayoutLocale::Get("en-us");
  EXPECT_EQ(en_us, LayoutLocale::Get("en-US"));
}

TEST(LayoutLocaleTest, ScriptTest) {
  // Test combinations of BCP 47 locales.
  // https://tools.ietf.org/html/bcp47
  struct {
    const char* locale;
    UScriptCode script;
    bool has_script_for_han;
    UScriptCode script_for_han;
  } tests[] = {
      {"en-US", USCRIPT_LATIN},

      // Common lang-script.
      {"en-Latn", USCRIPT_LATIN},
      {"ar-Arab", USCRIPT_ARABIC},

      // Common lang-region in East Asia.
      {"ja-JP", USCRIPT_KATAKANA_OR_HIRAGANA, true},
      {"ko-KR", USCRIPT_HANGUL, true},
      {"zh", USCRIPT_SIMPLIFIED_HAN, true},
      {"zh-CN", USCRIPT_SIMPLIFIED_HAN, true},
      {"zh-HK", USCRIPT_TRADITIONAL_HAN, true},
      {"zh-MO", USCRIPT_TRADITIONAL_HAN, true},
      {"zh-SG", USCRIPT_SIMPLIFIED_HAN, true},
      {"zh-TW", USCRIPT_TRADITIONAL_HAN, true},

      // Encompassed languages within the Chinese macrolanguage.
      // Both "lang" and "lang-extlang" should work.
      {"nan", USCRIPT_TRADITIONAL_HAN, true},
      {"wuu", USCRIPT_SIMPLIFIED_HAN, true},
      {"yue", USCRIPT_TRADITIONAL_HAN, true},
      {"zh-nan", USCRIPT_TRADITIONAL_HAN, true},
      {"zh-wuu", USCRIPT_SIMPLIFIED_HAN, true},
      {"zh-yue", USCRIPT_TRADITIONAL_HAN, true},

      // Script has priority over other subtags.
      {"zh-Hant", USCRIPT_TRADITIONAL_HAN, true},
      {"en-Hans", USCRIPT_SIMPLIFIED_HAN, true},
      {"en-Hant", USCRIPT_TRADITIONAL_HAN, true},
      {"en-Hans-TW", USCRIPT_SIMPLIFIED_HAN, true},
      {"en-Hant-CN", USCRIPT_TRADITIONAL_HAN, true},
      {"wuu-Hant", USCRIPT_TRADITIONAL_HAN, true},
      {"yue-Hans", USCRIPT_SIMPLIFIED_HAN, true},
      {"zh-wuu-Hant", USCRIPT_TRADITIONAL_HAN, true},
      {"zh-yue-Hans", USCRIPT_SIMPLIFIED_HAN, true},

      // Lang has priority over region.
      // icu::Locale::getDefault() returns other combinations if, for instnace,
      // English Windows with the display language set to Japanese.
      {"ja", USCRIPT_KATAKANA_OR_HIRAGANA, true},
      {"ja-US", USCRIPT_KATAKANA_OR_HIRAGANA, true},
      {"ko", USCRIPT_HANGUL, true},
      {"ko-US", USCRIPT_HANGUL, true},
      {"wuu-TW", USCRIPT_SIMPLIFIED_HAN, true},
      {"yue-CN", USCRIPT_TRADITIONAL_HAN, true},
      {"zh-wuu-TW", USCRIPT_SIMPLIFIED_HAN, true},
      {"zh-yue-CN", USCRIPT_TRADITIONAL_HAN, true},

      // Region should not affect script, but it can influence scriptForHan.
      {"en-CN", USCRIPT_LATIN, false},
      {"en-HK", USCRIPT_LATIN, true, USCRIPT_TRADITIONAL_HAN},
      {"en-MO", USCRIPT_LATIN, true, USCRIPT_TRADITIONAL_HAN},
      {"en-SG", USCRIPT_LATIN, false},
      {"en-TW", USCRIPT_LATIN, true, USCRIPT_TRADITIONAL_HAN},
      {"en-JP", USCRIPT_LATIN, true, USCRIPT_KATAKANA_OR_HIRAGANA},
      {"en-KR", USCRIPT_LATIN, true, USCRIPT_HANGUL},

      // Multiple regions are invalid, but it can still give hints for the font
      // selection.
      {"en-US-JP", USCRIPT_LATIN, true, USCRIPT_KATAKANA_OR_HIRAGANA},
  };

  for (const auto& test : tests) {
    scoped_refptr<LayoutLocale> locale =
        LayoutLocale::CreateForTesting(test.locale);
    EXPECT_EQ(test.script, locale->GetScript()) << test.locale;
    EXPECT_EQ(test.has_script_for_han, locale->HasScriptForHan())
        << test.locale;
    if (!test.has_script_for_han) {
      EXPECT_EQ(USCRIPT_SIMPLIFIED_HAN, locale->GetScriptForHan())
          << test.locale;
    } else if (test.script_for_han) {
      EXPECT_EQ(test.script_for_han, locale->GetScriptForHan()) << test.locale;
    } else {
      EXPECT_EQ(test.script, locale->GetScriptForHan()) << test.locale;
    }
  }
}

TEST(LayoutLocaleTest, BreakKeyword) {
  struct {
    const char* expected;
    const char* locale;
    LineBreakIteratorMode mode;
  } tests[] = {
      {nullptr, nullptr, LineBreakIteratorMode::kDefault},
      {"", "", LineBreakIteratorMode::kDefault},
      {nullptr, nullptr, LineBreakIteratorMode::kStrict},
      {"", "", LineBreakIteratorMode::kStrict},
      {"ja", "ja", LineBreakIteratorMode::kDefault},
      {"ja@lb=normal", "ja", LineBreakIteratorMode::kNormal},
      {"ja@lb=strict", "ja", LineBreakIteratorMode::kStrict},
      {"ja@lb=loose", "ja", LineBreakIteratorMode::kLoose},
  };
  for (const auto& test : tests) {
    scoped_refptr<LayoutLocale> locale =
        LayoutLocale::CreateForTesting(test.locale);
    EXPECT_EQ(test.expected, locale->LocaleWithBreakKeyword(test.mode))
        << String::Format("'%s' with line-break %d should be '%s'", test.locale,
                          static_cast<int>(test.mode), test.expected);
  }
}

TEST(LayoutLocaleTest, ExistingKeywordName) {
  const char* tests[] = {
      "en@x=", "en@lb=xyz", "en@ =",
  };
  for (auto* const test : tests) {
    scoped_refptr<LayoutLocale> locale = LayoutLocale::CreateForTesting(test);
    EXPECT_EQ(test,
              locale->LocaleWithBreakKeyword(LineBreakIteratorMode::kNormal));
  }
}

}  // namespace blink
