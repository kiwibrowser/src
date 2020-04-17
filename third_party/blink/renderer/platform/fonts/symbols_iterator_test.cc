// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/symbols_iterator.h"

#include "testing/gtest/include/gtest/gtest.h"
#include <string>

namespace blink {

struct FallbackTestRun {
  std::string text;
  FontFallbackPriority font_fallback_priority;
};

struct FallbackExpectedRun {
  unsigned limit;
  FontFallbackPriority font_fallback_priority;

  FallbackExpectedRun(unsigned the_limit,
                      FontFallbackPriority the_font_fallback_priority)
      : limit(the_limit), font_fallback_priority(the_font_fallback_priority) {}
};

class SymbolsIteratorTest : public testing::Test {
 protected:
  void CheckRuns(const Vector<FallbackTestRun>& runs) {
    String text(g_empty_string16_bit);
    Vector<FallbackExpectedRun> expect;
    for (auto& run : runs) {
      text.append(String::FromUTF8(run.text.c_str()));
      expect.push_back(
          FallbackExpectedRun(text.length(), run.font_fallback_priority));
    }
    SymbolsIterator symbols_iterator(text.Characters16(), text.length());
    VerifyRuns(&symbols_iterator, expect);
  }

  void VerifyRuns(SymbolsIterator* symbols_iterator,
                  const Vector<FallbackExpectedRun>& expect) {
    unsigned limit;
    FontFallbackPriority font_fallback_priority;
    unsigned long run_count = 0;
    while (symbols_iterator->Consume(&limit, &font_fallback_priority)) {
      ASSERT_LT(run_count, expect.size());
      ASSERT_EQ(expect[run_count].limit, limit);
      ASSERT_EQ(expect[run_count].font_fallback_priority,
                font_fallback_priority);
      ++run_count;
    }
    ASSERT_EQ(expect.size(), run_count);
  }
};

// Some of our compilers cannot initialize a vector from an array yet.
#define DECLARE_FALLBACK_RUNSVECTOR(...)                   \
  static const FallbackTestRun kRunsArray[] = __VA_ARGS__; \
  Vector<FallbackTestRun> runs;                            \
  runs.Append(kRunsArray, sizeof(kRunsArray) / sizeof(*kRunsArray));

#define CHECK_RUNS(...)                     \
  DECLARE_FALLBACK_RUNSVECTOR(__VA_ARGS__); \
  CheckRuns(runs);

TEST_F(SymbolsIteratorTest, Empty) {
  String empty(g_empty_string16_bit);
  SymbolsIterator symbols_iterator(empty.Characters16(), empty.length());
  unsigned limit = 0;
  FontFallbackPriority symbols_font = FontFallbackPriority::kInvalid;
  DCHECK(!symbols_iterator.Consume(&limit, &symbols_font));
  ASSERT_EQ(limit, 0u);
  ASSERT_EQ(symbols_font, FontFallbackPriority::kInvalid);
}

TEST_F(SymbolsIteratorTest, Space) {
  CHECK_RUNS({{" ", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, Latin) {
  CHECK_RUNS({{"Aa", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, LatinColorEmojiTextEmoji) {
  CHECK_RUNS({{"a", FontFallbackPriority::kText},
              {"⌚", FontFallbackPriority::kEmojiEmoji},
              {"☎", FontFallbackPriority::kEmojiText}});
}

TEST_F(SymbolsIteratorTest, IgnoreVSInMath) {
  CHECK_RUNS({{"⊆⊇⊈\xEF\xB8\x8E⊙⊚⊚", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, IgnoreVS15InText) {
  CHECK_RUNS({{"abcdef\xEF\xB8\x8Eghji", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, IgnoreVS16InText) {
  CHECK_RUNS({{"abcdef\xEF\xB8\x8Fghji", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, AllHexValuesText) {
  // Helps with detecting incorrect emoji pattern definitions which are
  // missing a \U000... prefix for example.
  CHECK_RUNS({{"abcdef0123456789ABCDEF", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, NumbersAndHashNormalAndEmoji) {
  CHECK_RUNS({{"0123456789#*", FontFallbackPriority::kText},
              {"0⃣1⃣2⃣3⃣4⃣5⃣6⃣7⃣8⃣9⃣*⃣", FontFallbackPriority::kEmojiEmoji},
              {"0123456789#*", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, VS16onDigits) {
  CHECK_RUNS({{"#", FontFallbackPriority::kText},
              {"#\uFE0F#\uFE0F\u20E3", FontFallbackPriority::kEmojiEmoji},
              {"#", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, SingleFlag) {
  CHECK_RUNS({{"🇺", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, CombiningCircle) {
  CHECK_RUNS({{"◌́◌̀◌̈◌̂◌̄◌̊", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, CombiningEnclosingCircleBackslash) {
  CHECK_RUNS({{"A⃠B⃠C⃠", FontFallbackPriority::kText},
              {"🚷🚯🚱🔞📵🚭🚫", FontFallbackPriority::kEmojiEmoji},
              {"🎙⃠", FontFallbackPriority::kEmojiText},
              {"📸⃠🔫⃠", FontFallbackPriority::kEmojiEmoji},
              {"a⃠b⃠c⃠", FontFallbackPriority::kText}});
}

// TODO: Perhaps check for invalid country indicator combinations?

TEST_F(SymbolsIteratorTest, FlagsVsNonFlags) {
  CHECK_RUNS({{"🇺🇸🇸", FontFallbackPriority::kEmojiEmoji},  // "US"
              {"abc", FontFallbackPriority::kText},
              {"🇺🇸", FontFallbackPriority::kEmojiEmoji},
              {"a🇿", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, EmojiVS15) {
  // A VS15 after the anchor must trigger text display.
  CHECK_RUNS({{"⚓\xEF\xB8\x8E", FontFallbackPriority::kEmojiText},
              {"⛵", FontFallbackPriority::kEmojiEmoji}});
}

TEST_F(SymbolsIteratorTest, EmojiZWSSequences) {
  CHECK_RUNS({{"👩‍👩‍👧‍👦👩‍❤️‍💋‍👨",
               FontFallbackPriority::kEmojiEmoji},
              {"abcd", FontFallbackPriority::kText},
              {"👩‍👩‍", FontFallbackPriority::kEmojiEmoji},
              {"efgh", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, AllEmojiZWSSequences) {
  // clang-format gets confused by Emojis, http://llvm.org/PR30530
  // clang-format off
  CHECK_RUNS(
      {{"💏👩‍❤️‍💋‍👨👨‍❤️‍💋‍👨👩‍❤️‍💋‍👩💑👩‍❤️‍👨👨‍❤"
        "️"
        "‍👨👩‍❤️"
        "‍👩👪👨‍👩‍👦👨‍👩‍👧👨‍👩‍👧‍👦👨‍👩‍👦‍👦👨‍👩‍👧‍👧👨‍👨"
        "‍"
        "👦👨‍👨‍👧👨‍👨‍👧‍👦👨‍👨‍👦‍👦👨‍👨‍👧"
        "‍"
        "👧"
        "👩‍👩‍👦👩‍👩‍👧👩‍👩‍👧‍👦👩‍👩‍👦‍👦👩‍👩‍👧‍👧👁"
        "‍"
        "🗨",
        FontFallbackPriority::kEmojiEmoji}});
  // clang-format on
}

TEST_F(SymbolsIteratorTest, ModifierPlusGender) {
  CHECK_RUNS({{"⛹🏻‍♂", FontFallbackPriority::kEmojiEmoji}});
}

TEST_F(SymbolsIteratorTest, TextMemberZwjSequence) {
  CHECK_RUNS({{"👨‍⚕", FontFallbackPriority::kEmojiEmoji}});
}

TEST_F(SymbolsIteratorTest, FacepalmCartwheelShrugModifierFemale) {
  CHECK_RUNS({{"🤦‍♀🤸‍♀🤷‍♀🤷🏾‍♀",
               FontFallbackPriority::kEmojiEmoji}});
}

TEST_F(SymbolsIteratorTest, AesculapiusMaleFemalEmoji) {
  // Emoji Data 4 has upgraded those three characters to Emoji.
  CHECK_RUNS({{"a", FontFallbackPriority::kText},
              {"⚕♀♂", FontFallbackPriority::kEmojiText}});
}

TEST_F(SymbolsIteratorTest, EyeSpeechBubble) {
  CHECK_RUNS({{"👁‍🗨", FontFallbackPriority::kEmojiEmoji}});
}

TEST_F(SymbolsIteratorTest, Modifier) {
  CHECK_RUNS({{"👶🏿", FontFallbackPriority::kEmojiEmoji}});
}

TEST_F(SymbolsIteratorTest, DingbatsMiscSymbolsModifier) {
  CHECK_RUNS({{"⛹🏻✍🏻✊🏼", FontFallbackPriority::kEmojiEmoji}});
}

TEST_F(SymbolsIteratorTest, ExtraZWJPrefix) {
  CHECK_RUNS({{"\xE2\x80\x8D", FontFallbackPriority::kText},
              {"\xF0\x9F\x91\xA9\xE2\x80\x8D\xE2"
               "\x9D\xA4\xEF\xB8\x8F\xE2\x80\x8D"
               "\xF0\x9F\x92\x8B\xE2\x80\x8D\xF0\x9F\x91\xA8",
               FontFallbackPriority::kEmojiEmoji}});
}

TEST_F(SymbolsIteratorTest, Arrows) {
  CHECK_RUNS({{"x→←x←↑↓→", FontFallbackPriority::kText}});
}

TEST_F(SymbolsIteratorTest, JudgePilot) {
  CHECK_RUNS({{"👨‍⚖️👩‍⚖️👨🏼‍⚖️👩🏼‍⚖️",
               FontFallbackPriority::kEmojiEmoji}});
}

// Extracted from http://unicode.org/emoji/charts/emoji-released.html for Emoji
// v5.0, except for the subdivision-flag section.
// Before ICU 59 new emoji sequences and new single emoji are not detected as
// emoji type text and sequences get split up in the middle so that shaping
// cannot form the right glyph from the emoji font. Running this as one run in
// one test ensures that the new emoji form an unbroken emoji-type sequence.
TEST_F(SymbolsIteratorTest, Emoji5AdditionsExceptFlags) {
  CHECK_RUNS(
      {{"\xF0\x9F\xA7\x94\xF0\x9F\x8F\xBB\xF0\x9F\xA7\x94\xF0\x9F\x8F\xBC\xF0"
        "\x9F\xA7\x94\xF0\x9F\x8F\xBD\xF0\x9F\xA7\x94\xF0\x9F\x8F\xBE\xF0\x9F"
        "\xA7\x94\xF0\x9F\x8F\xBF\xF0\x9F\xA4\xB1\xF0\x9F\xA4\xB1\xF0\x9F\x8F"
        "\xBB\xF0\x9F\xA4\xB1\xF0\x9F\x8F\xBC\xF0\x9F\xA4\xB1\xF0\x9F\x8F\xBD"
        "\xF0\x9F\xA4\xB1\xF0\x9F\x8F\xBE\xF0\x9F\xA4\xB1\xF0\x9F\x8F\xBF\xF0"
        "\x9F\xA7\x99\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBB\xF0\x9F\xA7\x99\xF0\x9F"
        "\x8F\xBC\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBD\xF0\x9F\xA7\x99\xF0\x9F\x8F"
        "\xBE\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBF\xF0\x9F\xA7\x99\xE2\x80\x8D\xE2"
        "\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBB\xE2\x80\x8D\xE2"
        "\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBC\xE2\x80\x8D\xE2"
        "\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBD\xE2\x80\x8D\xE2"
        "\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBE\xE2\x80\x8D\xE2"
        "\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBF\xE2\x80\x8D\xE2"
        "\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x99\xE2\x80\x8D\xE2\x99\x82\xEF\xB8"
        "\x8F\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBB\xE2\x80\x8D\xE2\x99\x82\xEF\xB8"
        "\x8F\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBC\xE2\x80\x8D\xE2\x99\x82\xEF\xB8"
        "\x8F\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBD\xE2\x80\x8D\xE2\x99\x82\xEF\xB8"
        "\x8F\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBE\xE2\x80\x8D\xE2\x99\x82\xEF\xB8"
        "\x8F\xF0\x9F\xA7\x99\xF0\x9F\x8F\xBF\xE2\x80\x8D\xE2\x99\x82\xEF\xB8"
        "\x8F\xF0\x9F\xA7\x9A\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBB\xF0\x9F\xA7\x9A"
        "\xF0\x9F\x8F\xBC\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBD\xF0\x9F\xA7\x9A\xF0"
        "\x9F\x8F\xBE\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBF\xF0\x9F\xA7\x9A\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBB\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBC\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBD\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBE\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBF\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBB\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBC\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBD\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBE\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x9A\xF0\x9F\x8F\xBF\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBB\xF0\x9F"
        "\xA7\x9B\xF0\x9F\x8F\xBC\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBD\xF0\x9F\xA7"
        "\x9B\xF0\x9F\x8F\xBE\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBF\xF0\x9F\xA7\x9B"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBB"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBC"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBD"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBE"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBF"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBB\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBC\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBD\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBE\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9B\xF0\x9F\x8F\xBF\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F\xA7\x9C\xF0\x9F\x8F\xBB"
        "\xF0\x9F\xA7\x9C\xF0\x9F\x8F\xBC\xF0\x9F\xA7\x9C\xF0\x9F\x8F\xBD\xF0"
        "\x9F\xA7\x9C\xF0\x9F\x8F\xBE\xF0\x9F\xA7\x9C\xF0\x9F\x8F\xBF\xF0\x9F"
        "\xA7\x9C\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F"
        "\x8F\xBB\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F"
        "\x8F\xBC\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F"
        "\x8F\xBD\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F"
        "\x8F\xBE\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F"
        "\x8F\xBF\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F\x8F\xBB\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F\x8F\xBC\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F\x8F\xBD\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F\x8F\xBE\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9C\xF0\x9F\x8F\xBF\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9D\xF0\x9F\xA7\x9D\xF0\x9F"
        "\x8F\xBB\xF0\x9F\xA7\x9D\xF0\x9F\x8F\xBC\xF0\x9F\xA7\x9D\xF0\x9F\x8F"
        "\xBD\xF0\x9F\xA7\x9D\xF0\x9F\x8F\xBE\xF0\x9F\xA7\x9D\xF0\x9F\x8F\xBF"
        "\xF0\x9F\xA7\x9D\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9D"
        "\xF0\x9F\x8F\xBB\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9D"
        "\xF0\x9F\x8F\xBC\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9D"
        "\xF0\x9F\x8F\xBD\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9D"
        "\xF0\x9F\x8F\xBE\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9D"
        "\xF0\x9F\x8F\xBF\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9D"
        "\xE2\x80\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9D\xF0\x9F\x8F\xBB"
        "\xE2\x80\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9D\xF0\x9F\x8F\xBC"
        "\xE2\x80\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9D\xF0\x9F\x8F\xBD"
        "\xE2\x80\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9D\xF0\x9F\x8F\xBE"
        "\xE2\x80\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9D\xF0\x9F\x8F\xBF"
        "\xE2\x80\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9E\xF0\x9F\xA7\x9E"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9E\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x9F\xF0\x9F\xA7\x9F\xE2\x80\x8D\xE2"
        "\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x9F\xE2\x80\x8D\xE2\x99\x82\xEF\xB8"
        "\x8F\xF0\x9F\xA7\x96\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBB\xF0\x9F\xA7\x96"
        "\xF0\x9F\x8F\xBC\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBD\xF0\x9F\xA7\x96\xF0"
        "\x9F\x8F\xBE\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBF\xF0\x9F\xA7\x96\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBB\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBC\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBD\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBE\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBF\xE2\x80"
        "\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x96\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBB\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBC\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBD\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBE\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x96\xF0\x9F\x8F\xBF\xE2\x80\x8D\xE2\x99\x82"
        "\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBB\xF0\x9F"
        "\xA7\x97\xF0\x9F\x8F\xBC\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBD\xF0\x9F\xA7"
        "\x97\xF0\x9F\x8F\xBE\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBF\xF0\x9F\xA7\x97"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBB"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBC"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBD"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBE"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBF"
        "\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x97\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBB\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBC\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBD\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBE\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x97\xF0\x9F\x8F\xBF\xE2\x80\x8D\xE2"
        "\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F\xA7\x98\xF0\x9F\x8F\xBB"
        "\xF0\x9F\xA7\x98\xF0\x9F\x8F\xBC\xF0\x9F\xA7\x98\xF0\x9F\x8F\xBD\xF0"
        "\x9F\xA7\x98\xF0\x9F\x8F\xBE\xF0\x9F\xA7\x98\xF0\x9F\x8F\xBF\xF0\x9F"
        "\xA7\x98\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F"
        "\x8F\xBB\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F"
        "\x8F\xBC\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F"
        "\x8F\xBD\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F"
        "\x8F\xBE\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F"
        "\x8F\xBF\xE2\x80\x8D\xE2\x99\x80\xEF\xB8\x8F\xF0\x9F\xA7\x98\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F\x8F\xBB\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F\x8F\xBC\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F\x8F\xBD\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F\x8F\xBE\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA7\x98\xF0\x9F\x8F\xBF\xE2\x80"
        "\x8D\xE2\x99\x82\xEF\xB8\x8F\xF0\x9F\xA4\x9F\xF0\x9F\xA4\x9F\xF0\x9F"
        "\x8F\xBB\xF0\x9F\xA4\x9F\xF0\x9F\x8F\xBC\xF0\x9F\xA4\x9F\xF0\x9F\x8F"
        "\xBD\xF0\x9F\xA4\x9F\xF0\x9F\x8F\xBE\xF0\x9F\xA4\x9F\xF0\x9F\x8F\xBF"
        "\xF0\x9F\xA4\xB2\xF0\x9F\xA4\xB2\xF0\x9F\x8F\xBB\xF0\x9F\xA4\xB2\xF0"
        "\x9F\x8F\xBC\xF0\x9F\xA4\xB2\xF0\x9F\x8F\xBD\xF0\x9F\xA4\xB2\xF0\x9F"
        "\x8F\xBE\xF0\x9F\xA4\xB2\xF0\x9F\x8F\xBF\xF0\x9F\xA7\xA0\xF0\x9F\xA7"
        "\xA1\xF0\x9F\xA7\xA3\xF0\x9F\xA7\xA4\xF0\x9F\xA7\xA5\xF0\x9F\xA7\xA6"
        "\xF0\x9F\xA7\xA2\xF0\x9F\xA6\x93\xF0\x9F\xA6\x92\xF0\x9F\xA6\x94\xF0"
        "\x9F\xA6\x95\xF0\x9F\xA6\x96\xF0\x9F\xA6\x97\xF0\x9F\xA5\xA5\xF0\x9F"
        "\xA5\xA6\xF0\x9F\xA5\xA8\xF0\x9F\xA5\xA9\xF0\x9F\xA5\xAA\xF0\x9F\xA5"
        "\xA3\xF0\x9F\xA5\xAB\xF0\x9F\xA5\x9F\xF0\x9F\xA5\xA0\xF0\x9F\xA5\xA1"
        "\xF0\x9F\xA5\xA7\xF0\x9F\xA5\xA4\xF0\x9F\xA5\xA2\xF0\x9F\x9B\xB8\xF0"
        "\x9F\x9B\xB7\xF0\x9F\xA5\x8C",
        FontFallbackPriority::kEmojiEmoji}});
}

TEST_F(SymbolsIteratorTest, EmojiSubdivisionFlags) {
  CHECK_RUNS(
      {{"🏴󠁧󠁢󠁷󠁬󠁳󠁿🏴󠁧󠁢󠁳󠁣󠁴󠁿🏴󠁧󠁢",
        FontFallbackPriority::kEmojiEmoji}});
}

// Extracted from http://unicode.org/emoji/charts/emoji-released.html for Emoji
// v11, removed U+265F Chess Pawn and U+267E as they do not have default emoji
// presentation.
TEST_F(SymbolsIteratorTest, Emoji11Additions) {
  CheckRuns(
      {{u8"\U0001F970\U0001F975\U0001F976\U0001F973\U0001F974\U0001F97A"
        u8"\U0001F468\U0000200D\U0001F9B0\U0001F468\U0001F3FB\U0000200D"
        u8"\U0001F9B0\U0001F468\U0001F3FC\U0000200D\U0001F9B0\U0001F468"
        u8"\U0001F3FD\U0000200D\U0001F9B0\U0001F468\U0001F3FE\U0000200D"
        u8"\U0001F9B0\U0001F468\U0001F3FF\U0000200D\U0001F9B0\U0001F468"
        u8"\U0000200D\U0001F9B1\U0001F468\U0001F3FB\U0000200D\U0001F9B1"
        u8"\U0001F468\U0001F3FC\U0000200D\U0001F9B1\U0001F468\U0001F3FD"
        u8"\U0000200D\U0001F9B1\U0001F468\U0001F3FE\U0000200D\U0001F9B1"
        u8"\U0001F468\U0001F3FF\U0000200D\U0001F9B1\U0001F468\U0000200D"
        u8"\U0001F9B3\U0001F468\U0001F3FB\U0000200D\U0001F9B3\U0001F468"
        u8"\U0001F3FC\U0000200D\U0001F9B3\U0001F468\U0001F3FD\U0000200D"
        u8"\U0001F9B3\U0001F468\U0001F3FE\U0000200D\U0001F9B3\U0001F468"
        u8"\U0001F3FF\U0000200D\U0001F9B3\U0001F468\U0000200D\U0001F9B2"
        u8"\U0001F468\U0001F3FB\U0000200D\U0001F9B2\U0001F468\U0001F3FC"
        u8"\U0000200D\U0001F9B2\U0001F468\U0001F3FD\U0000200D\U0001F9B2"
        u8"\U0001F468\U0001F3FE\U0000200D\U0001F9B2\U0001F468\U0001F3FF"
        u8"\U0000200D\U0001F9B2\U0001F469\U0000200D\U0001F9B0\U0001F469"
        u8"\U0001F3FB\U0000200D\U0001F9B0\U0001F469\U0001F3FC\U0000200D"
        u8"\U0001F9B0\U0001F469\U0001F3FD\U0000200D\U0001F9B0\U0001F469"
        u8"\U0001F3FE\U0000200D\U0001F9B0\U0001F469\U0001F3FF\U0000200D"
        u8"\U0001F9B0\U0001F469\U0000200D\U0001F9B1\U0001F469\U0001F3FB"
        u8"\U0000200D\U0001F9B1\U0001F469\U0001F3FC\U0000200D\U0001F9B1"
        u8"\U0001F469\U0001F3FD\U0000200D\U0001F9B1\U0001F469\U0001F3FE"
        u8"\U0000200D\U0001F9B1\U0001F469\U0001F3FF\U0000200D\U0001F9B1"
        u8"\U0001F469\U0000200D\U0001F9B3\U0001F469\U0001F3FB\U0000200D"
        u8"\U0001F9B3\U0001F469\U0001F3FC\U0000200D\U0001F9B3\U0001F469"
        u8"\U0001F3FD\U0000200D\U0001F9B3\U0001F469\U0001F3FE\U0000200D"
        u8"\U0001F9B3\U0001F469\U0001F3FF\U0000200D\U0001F9B3\U0001F469"
        u8"\U0000200D\U0001F9B2\U0001F469\U0001F3FB\U0000200D\U0001F9B2"
        u8"\U0001F469\U0001F3FC\U0000200D\U0001F9B2\U0001F469\U0001F3FD"
        u8"\U0000200D\U0001F9B2\U0001F469\U0001F3FE\U0000200D\U0001F9B2"
        u8"\U0001F469\U0001F3FF\U0000200D\U0001F9B2\U0001F9B8\U0001F9B8"
        u8"\U0001F3FB\U0001F9B8\U0001F3FC\U0001F9B8\U0001F3FD\U0001F9B8"
        u8"\U0001F3FE\U0001F9B8\U0001F3FF\U0001F9B8\U0000200D\U00002640"
        u8"\U0000FE0F\U0001F9B8\U0001F3FB\U0000200D\U00002640\U0000FE0F"
        u8"\U0001F9B8\U0001F3FC\U0000200D\U00002640\U0000FE0F\U0001F9B8"
        u8"\U0001F3FD\U0000200D\U00002640\U0000FE0F\U0001F9B8\U0001F3FE"
        u8"\U0000200D\U00002640\U0000FE0F\U0001F9B8\U0001F3FF\U0000200D"
        u8"\U00002640\U0000FE0F\U0001F9B8\U0000200D\U00002642\U0000FE0F"
        u8"\U0001F9B8\U0001F3FB\U0000200D\U00002642\U0000FE0F\U0001F9B8"
        u8"\U0001F3FC\U0000200D\U00002642\U0000FE0F\U0001F9B8\U0001F3FD"
        u8"\U0000200D\U00002642\U0000FE0F\U0001F9B8\U0001F3FE\U0000200D"
        u8"\U00002642\U0000FE0F\U0001F9B8\U0001F3FF\U0000200D\U00002642"
        u8"\U0000FE0F\U0001F9B9\U0001F9B9\U0001F3FB\U0001F9B9\U0001F3FC"
        u8"\U0001F9B9\U0001F3FD\U0001F9B9\U0001F3FE\U0001F9B9\U0001F3FF"
        u8"\U0001F9B9\U0000200D\U00002640\U0000FE0F\U0001F9B9\U0001F3FB"
        u8"\U0000200D\U00002640\U0000FE0F\U0001F9B9\U0001F3FC\U0000200D"
        u8"\U00002640\U0000FE0F\U0001F9B9\U0001F3FD\U0000200D\U00002640"
        u8"\U0000FE0F\U0001F9B9\U0001F3FE\U0000200D\U00002640\U0000FE0F"
        u8"\U0001F9B9\U0001F3FF\U0000200D\U00002640\U0000FE0F\U0001F9B9"
        u8"\U0000200D\U00002642\U0000FE0F\U0001F9B9\U0001F3FB\U0000200D"
        u8"\U00002642\U0000FE0F\U0001F9B9\U0001F3FC\U0000200D\U00002642"
        u8"\U0000FE0F\U0001F9B9\U0001F3FD\U0000200D\U00002642\U0000FE0F"
        u8"\U0001F9B9\U0001F3FE\U0000200D\U00002642\U0000FE0F\U0001F9B9"
        u8"\U0001F3FF\U0000200D\U00002642\U0000FE0F\U0001F9B5\U0001F9B5"
        u8"\U0001F3FB\U0001F9B5\U0001F3FC\U0001F9B5\U0001F3FD\U0001F9B5"
        u8"\U0001F3FE\U0001F9B5\U0001F3FF\U0001F9B6\U0001F9B6\U0001F3FB"
        u8"\U0001F9B6\U0001F3FC\U0001F9B6\U0001F3FD\U0001F9B6\U0001F3FE"
        u8"\U0001F9B6\U0001F3FF\U0001F9B4\U0001F9B7\U0001F9B0\U0001F9B1"
        u8"\U0001F9B3\U0001F9B2\U0001F97D\U0001F97C\U0001F97E\U0001F97F"
        u8"\U0001F99D\U0001F999\U0001F99B\U0001F998\U0001F9A1\U0001F9A2"
        u8"\U0001F99A\U0001F99C\U0001F99E\U0001F99F\U0001F9A0\U0001F96D"
        u8"\U0001F96C\U0001F96F\U0001F9C2\U0001F96E\U0001F9C1\U0001F9ED"
        u8"\U0001F9F1\U0001F6F9\U0001F9F3\U0001F9E8\U0001F9E7\U0001F94E"
        u8"\U0001F94F\U0001F94D\U0001F9FF\U0001F9E9\U0001F9F8\U0001F9F5"
        u8"\U0001F9F6\U0001F9EE\U0001F9FE\U0001F9F0\U0001F9F2\U0001F9EA"
        u8"\U0001F9EB\U0001F9EC\U0001F9F4\U0001F9F7\U0001F9F9\U0001F9FA"
        u8"\U0001F9FB\U0001F9FC\U0001F9FD\U0001F9EF\U0001F3F4\U0000200D"
        u8"\U00002620\U0000FE0F",
        FontFallbackPriority::kEmojiEmoji}});
}

}  // namespace blink
