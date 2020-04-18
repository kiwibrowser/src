// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/shaping/harf_buzz_shaper.h"

#include <unicode/uscript.h>

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/fonts/font.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/fonts/font_test_utilities.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_inline_headers.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_spacing.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_test_info.h"
#include "third_party/blink/renderer/platform/layout_test_support.h"
#include "third_party/blink/renderer/platform/testing/font_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/text/text_break_iterator.h"
#include "third_party/blink/renderer/platform/text/text_run.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class HarfBuzzShaperTest : public testing::Test {
 protected:
  void SetUp() override {
    font_description.SetComputedSize(12.0);
    font = Font(font_description);
    font.Update(nullptr);
  }

  void TearDown() override {}

  Font CreateAhem(float size) {
    FontDescription::VariantLigatures ligatures;
    return blink::test::CreateTestFont(
        "Ahem", blink::test::PlatformTestDataPath("Ahem.woff"), size,
        &ligatures);
  }

  scoped_refptr<ShapeResult> SplitRun(scoped_refptr<ShapeResult> shape_result,
                                      unsigned offset) {
    unsigned length = shape_result->NumCharacters();
    scoped_refptr<ShapeResult> run2 = shape_result->SubRange(offset, length);
    shape_result = shape_result->SubRange(0, offset);
    run2->CopyRange(offset, length, shape_result.get());
    return shape_result;
  }

  FontCachePurgePreventer font_cache_purge_preventer;
  FontDescription font_description;
  Font font;
  unsigned start_index = 0;
  unsigned num_characters = 0;
  unsigned num_glyphs = 0;
  hb_script_t script = HB_SCRIPT_INVALID;
};

class ScopedSubpixelOverride {
 public:
  ScopedSubpixelOverride(bool b) {
    prev_layout_test_ = LayoutTestSupport::IsRunningLayoutTest();
    prev_subpixel_allowed_ =
        LayoutTestSupport::IsTextSubpixelPositioningAllowedForTest();
    prev_antialias_ = LayoutTestSupport::IsFontAntialiasingEnabledForTest();
    prev_fd_subpixel_ = FontDescription::SubpixelPositioning();

    // This is required for all LayoutTestSupport settings to have effects.
    LayoutTestSupport::SetIsRunningLayoutTest(true);

    if (b) {
      // Allow subpixel positioning.
      LayoutTestSupport::SetTextSubpixelPositioningAllowedForTest(true);

      // Now, enable subpixel positioning in platform-specific ways.

      // Mac always enables subpixel positioning.

      // On Windows, subpixel positioning also requires antialiasing.
      LayoutTestSupport::SetFontAntialiasingEnabledForTest(true);

      // On platforms other than Windows and Mac this needs to be set as
      // well.
      FontDescription::SetSubpixelPositioning(true);
    } else {
      // Explicitly disallow all subpixel positioning.
      LayoutTestSupport::SetTextSubpixelPositioningAllowedForTest(false);
    }
  }
  ~ScopedSubpixelOverride() {
    FontDescription::SetSubpixelPositioning(prev_fd_subpixel_);
    LayoutTestSupport::SetFontAntialiasingEnabledForTest(prev_antialias_);
    LayoutTestSupport::SetTextSubpixelPositioningAllowedForTest(
        prev_subpixel_allowed_);
    LayoutTestSupport::SetIsRunningLayoutTest(prev_layout_test_);

    // Fonts cached with a different subpixel positioning state are not
    // automatically invalidated and need to be cleared between test
    // runs.
    FontCache::GetFontCache()->Invalidate();
  }

 private:
  bool prev_layout_test_;
  bool prev_subpixel_allowed_;
  bool prev_antialias_;
  bool prev_fd_subpixel_;
};

class ShapeParameterTest : public HarfBuzzShaperTest,
                           public testing::WithParamInterface<TextDirection> {
 protected:
  scoped_refptr<ShapeResult> ShapeWithParameter(HarfBuzzShaper* shaper) {
    TextDirection direction = GetParam();
    return shaper->Shape(&font, direction);
  }
};

INSTANTIATE_TEST_CASE_P(HarfBuzzShaperTest,
                        ShapeParameterTest,
                        testing::Values(TextDirection::kLtr,
                                        TextDirection::kRtl));

static inline ShapeResultTestInfo* TestInfo(scoped_refptr<ShapeResult>& result) {
  return static_cast<ShapeResultTestInfo*>(result.get());
}

TEST_F(HarfBuzzShaperTest, MutableUnique) {
  scoped_refptr<ShapeResult> result =
      ShapeResult::Create(&font, 0, TextDirection::kLtr);
  EXPECT_TRUE(result->HasOneRef());

  // At this point, |result| has only one ref count.
  scoped_refptr<ShapeResult> result2 = result->MutableUnique();
  EXPECT_EQ(result.get(), result2.get());
  EXPECT_FALSE(result2->HasOneRef());

  // Since |result| has 2 ref counts, it should return a clone.
  scoped_refptr<ShapeResult> result3 = result->MutableUnique();
  EXPECT_NE(result.get(), result3.get());
  EXPECT_TRUE(result3->HasOneRef());
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsLatin) {
  String latin_common = To16Bit("ABC DEF.", 8);
  HarfBuzzShaper shaper(latin_common.Characters16(), 8);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  EXPECT_EQ(1u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(8u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_LATIN, script);
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsLeadingCommon) {
  String leading_common = To16Bit("... test", 8);
  HarfBuzzShaper shaper(leading_common.Characters16(), 8);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  EXPECT_EQ(1u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(8u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_LATIN, script);
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsUnicodeVariants) {
  struct {
    const char* name;
    UChar string[4];
    unsigned length;
    hb_script_t script;
  } testlist[] = {
      {"Standard Variants text style", {0x30, 0xFE0E}, 2, HB_SCRIPT_COMMON},
      {"Standard Variants emoji style", {0x203C, 0xFE0F}, 2, HB_SCRIPT_COMMON},
      {"Standard Variants of Ideograph", {0x4FAE, 0xFE00}, 2, HB_SCRIPT_HAN},
      {"Ideographic Variants", {0x3402, 0xDB40, 0xDD00}, 3, HB_SCRIPT_HAN},
      {"Not-defined Variants", {0x41, 0xDB40, 0xDDEF}, 3, HB_SCRIPT_LATIN},
  };
  for (auto& test : testlist) {
    HarfBuzzShaper shaper(test.string, test.length);
    scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

    EXPECT_EQ(1u, TestInfo(result)->NumberOfRunsForTesting()) << test.name;
    ASSERT_TRUE(
        TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script))
        << test.name;
    EXPECT_EQ(0u, start_index) << test.name;
    if (num_glyphs == 2) {
// If the specified VS is not in the font, it's mapped to .notdef.
// then hb_ot_hide_default_ignorables() swaps it to a space with zero-advance.
// http://lists.freedesktop.org/archives/harfbuzz/2015-May/004888.html
#if !defined(OS_MACOSX)
      EXPECT_EQ(TestInfo(result)->FontDataForTesting(0)->SpaceGlyph(),
                TestInfo(result)->GlyphForTesting(0, 1))
          << test.name;
#endif
      EXPECT_EQ(0.f, TestInfo(result)->AdvanceForTesting(0, 1)) << test.name;
    } else {
      EXPECT_EQ(1u, num_glyphs) << test.name;
    }
    EXPECT_EQ(test.script, script) << test.name;
  }
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsDevanagariCommon) {
  UChar devanagari_common_string[] = {0x915, 0x94d, 0x930, 0x28, 0x20, 0x29};
  String devanagari_common_latin(devanagari_common_string, 6);
  HarfBuzzShaper shaper(devanagari_common_latin.Characters16(), 6);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  EXPECT_EQ(2u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_DEVANAGARI, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(1, start_index, num_glyphs, script));
  EXPECT_EQ(3u, start_index);
  EXPECT_EQ(3u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_DEVANAGARI, script);
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsDevanagariCommonLatinCommon) {
  UChar devanagari_common_latin_string[] = {0x915, 0x94d, 0x930, 0x20,
                                            0x61,  0x62,  0x2E};
  HarfBuzzShaper shaper(devanagari_common_latin_string, 7);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  EXPECT_EQ(3u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_DEVANAGARI, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(1, start_index, num_glyphs, script));
  EXPECT_EQ(3u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_DEVANAGARI, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(2, start_index, num_glyphs, script));
  EXPECT_EQ(4u, start_index);
  EXPECT_EQ(3u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_LATIN, script);
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsArabicThaiHanLatin) {
  UChar mixed_string[] = {0x628, 0x64A, 0x629, 0xE20, 0x65E5, 0x62};
  HarfBuzzShaper shaper(mixed_string, 6);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  EXPECT_EQ(4u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(3u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_ARABIC, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(1, start_index, num_glyphs, script));
  EXPECT_EQ(3u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_THAI, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(2, start_index, num_glyphs, script));
  EXPECT_EQ(4u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_HAN, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(3, start_index, num_glyphs, script));
  EXPECT_EQ(5u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_LATIN, script);
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsArabicThaiHanLatinTwice) {
  UChar mixed_string[] = {0x628, 0x64A, 0x629, 0xE20, 0x65E5, 0x62};
  HarfBuzzShaper shaper(mixed_string, 6);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);
  EXPECT_EQ(4u, TestInfo(result)->NumberOfRunsForTesting());

  // Shape again on the same shape object and check the number of runs.
  // Should be equal if no state was retained between shape calls.
  scoped_refptr<ShapeResult> result2 = shaper.Shape(&font, TextDirection::kLtr);
  EXPECT_EQ(4u, TestInfo(result2)->NumberOfRunsForTesting());
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsArabic) {
  UChar arabic_string[] = {0x628, 0x64A, 0x629};
  HarfBuzzShaper shaper(arabic_string, 3);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kRtl);

  EXPECT_EQ(1u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(3u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_ARABIC, script);
}

// This is a simplified test and doesn't accuratly reflect how the shape range
// is to be used. If you instead of the string you imagine the following HTML:
// <div>Hello <span>World</span>!</div>
// It better reflects the intended use where the range given to each shape call
// corresponds to the text content of a TextNode.
TEST_F(HarfBuzzShaperTest, ShapeLatinSegment) {
  String string = To16Bit("Hello World!", 12);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 12);
  scoped_refptr<ShapeResult> combined = shaper.Shape(&font, direction);
  scoped_refptr<ShapeResult> first = shaper.Shape(&font, direction, 0, 6);
  scoped_refptr<ShapeResult> second = shaper.Shape(&font, direction, 6, 11);
  scoped_refptr<ShapeResult> third = shaper.Shape(&font, direction, 11, 12);

  ASSERT_TRUE(TestInfo(first)->RunInfoForTesting(0, start_index, num_characters,
                                                 num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(6u, num_characters);
  ASSERT_TRUE(TestInfo(second)->RunInfoForTesting(
      0, start_index, num_characters, num_glyphs, script));
  EXPECT_EQ(6u, start_index);
  EXPECT_EQ(5u, num_characters);
  ASSERT_TRUE(TestInfo(third)->RunInfoForTesting(0, start_index, num_characters,
                                                 num_glyphs, script));
  EXPECT_EQ(11u, start_index);
  EXPECT_EQ(1u, num_characters);

  HarfBuzzShaper shaper2(string.Characters16(), 6);
  scoped_refptr<ShapeResult> first_reference = shaper2.Shape(&font, direction);

  HarfBuzzShaper shaper3(string.Characters16() + 6, 5);
  scoped_refptr<ShapeResult> second_reference = shaper3.Shape(&font, direction);

  HarfBuzzShaper shaper4(string.Characters16() + 11, 1);
  scoped_refptr<ShapeResult> third_reference = shaper4.Shape(&font, direction);

  // Width of each segment should be the same when shaped using start and end
  // offset as it is when shaping the three segments using separate shaper
  // instances.
  // A full pixel is needed for tolerance to account for kerning on some
  // platforms.
  ASSERT_NEAR(first_reference->Width(), first->Width(), 1);
  ASSERT_NEAR(second_reference->Width(), second->Width(), 1);
  ASSERT_NEAR(third_reference->Width(), third->Width(), 1);

  // Width of shape results for the entire string should match the combined
  // shape results from the three segments.
  float total_width = first->Width() + second->Width() + third->Width();
  ASSERT_NEAR(combined->Width(), total_width, 1);
}

// Represents the case where a part of a cluster has a different color.
// <div>0x647<span style="color: red;">0x64A</span></div>
// This test requires context-aware shaping which hasn't been implemented yet.
// See crbug.com/689155
TEST_F(HarfBuzzShaperTest, DISABLED_ShapeArabicWithContext) {
  UChar arabic_string[] = {0x647, 0x64A};
  HarfBuzzShaper shaper(arabic_string, 2);

  scoped_refptr<ShapeResult> combined = shaper.Shape(&font, TextDirection::kRtl);

  scoped_refptr<ShapeResult> first = shaper.Shape(&font, TextDirection::kRtl, 0, 1);
  scoped_refptr<ShapeResult> second = shaper.Shape(&font, TextDirection::kRtl, 1, 2);

  // Combined width should be the same when shaping the two characters
  // separately as when shaping them combined.
  ASSERT_NEAR(combined->Width(), first->Width() + second->Width(), 0.1);
}

TEST_F(HarfBuzzShaperTest, ShapeVerticalUpright) {
  font_description.SetOrientation(FontOrientation::kVerticalUpright);
  font = Font(font_description);
  font.Update(nullptr);

  // This string should create 2 runs, ideographic and Latin, both in upright.
  String string(u"\u65E5\u65E5\u65E5lllll");
  TextDirection direction = TextDirection::kLtr;
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  // Check width and bounds are not too much different. ".1" is heuristic.
  EXPECT_NEAR(result->Width(), result->Bounds().Width(), result->Width() * .1);

  // Shape each run and merge them using CopyRange. Bounds() should match.
  scoped_refptr<ShapeResult> result1 = shaper.Shape(&font, direction, 0, 3);
  scoped_refptr<ShapeResult> result2 =
      shaper.Shape(&font, direction, 3, string.length());

  scoped_refptr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result1->CopyRange(0, 3, composite_result.get());
  result2->CopyRange(3, string.length(), composite_result.get());

  EXPECT_EQ(result->Bounds(), composite_result->Bounds());
}

TEST_F(HarfBuzzShaperTest, RangeShapeSmallCaps) {
  // Test passes if no assertion is hit of the ones below, but also the newly
  // introduced one in HarfBuzzShaper::ShapeSegment: DCHECK_GT(shape_end,
  // shape_start) is not hit.
  FontDescription font_description;
  font_description.SetVariantCaps(FontDescription::kSmallCaps);
  font_description.SetComputedSize(12.0);
  Font font(font_description);
  font.Update(nullptr);

  // Shaping index 2 to 3 means that case splitting for small caps splits before
  // character index 2 since the initial 'a' needs to be uppercased, but the
  // space character does not need to be uppercased. This triggered
  // crbug.com/817271.
  String string(u"a aa");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result =
      shaper.Shape(&font, TextDirection::kLtr, 2, 3);
  EXPECT_EQ(1u, result->NumCharacters());

  string = u"aa a";
  HarfBuzzShaper shaper_two(string.Characters16(), string.length());
  result = shaper_two.Shape(&font, TextDirection::kLtr, 3, 4);
  EXPECT_EQ(1u, result->NumCharacters());

  string = u"a aa";
  HarfBuzzShaper shaper_three(string.Characters16(), string.length());
  result = shaper_three.Shape(&font, TextDirection::kLtr, 1, 2);
  EXPECT_EQ(1u, result->NumCharacters());

  string = u"aa aa aa aa aa aa aa aa aa aa";
  HarfBuzzShaper shaper_four(string.Characters16(), string.length());
  result = shaper_four.Shape(&font, TextDirection::kLtr, 21, 23);
  EXPECT_EQ(2u, result->NumCharacters());

  string = u"aa aa aa aa aa aa aa aa aa aa";
  HarfBuzzShaper shaper_five(string.Characters16(), string.length());
  result = shaper_five.Shape(&font, TextDirection::kLtr, 27, 29);
  EXPECT_EQ(2u, result->NumCharacters());
}

TEST_F(HarfBuzzShaperTest, ShapeVerticalMixed) {
  font_description.SetOrientation(FontOrientation::kVerticalMixed);
  font = Font(font_description);
  font.Update(nullptr);

  // This string should create 2 runs, ideographic in upright and Latin in
  // rotated horizontal.
  String string(u"\u65E5\u65E5\u65E5lllll");
  TextDirection direction = TextDirection::kLtr;
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  // Check width and bounds are not too much different. ".1" is heuristic.
  EXPECT_NEAR(result->Width(), result->Bounds().Width(), result->Width() * .1);

  // Shape each run and merge them using CopyRange. Bounds() should match.
  scoped_refptr<ShapeResult> result1 = shaper.Shape(&font, direction, 0, 3);
  scoped_refptr<ShapeResult> result2 =
      shaper.Shape(&font, direction, 3, string.length());

  scoped_refptr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result1->CopyRange(0, 3, composite_result.get());
  result2->CopyRange(3, string.length(), composite_result.get());

  EXPECT_EQ(result->Bounds(), composite_result->Bounds());
}

TEST_P(ShapeParameterTest, MissingGlyph) {
  // U+FFF0 is not assigned as of Unicode 10.0.
  String string(
      u"\uFFF0"
      u"Hello");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = ShapeWithParameter(&shaper);
  EXPECT_EQ(0u, result->StartIndexForResult());
  EXPECT_EQ(string.length(), result->EndIndexForResult());
}

TEST_P(ShapeParameterTest, ZeroWidthSpace) {
  UChar string[] = {kZeroWidthSpaceCharacter,
                    kZeroWidthSpaceCharacter,
                    0x0627,
                    0x0631,
                    0x062F,
                    0x0648,
                    kZeroWidthSpaceCharacter,
                    kZeroWidthSpaceCharacter};
  const unsigned length = arraysize(string);
  HarfBuzzShaper shaper(string, length);
  scoped_refptr<ShapeResult> result = ShapeWithParameter(&shaper);
  EXPECT_EQ(0u, result->StartIndexForResult());
  EXPECT_EQ(length, result->EndIndexForResult());
#if DCHECK_IS_ON()
  result->CheckConsistency();
#endif
}

TEST_F(HarfBuzzShaperTest, NegativeLetterSpacing) {
  String string(u"Hello");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);
  float width = result->Width();
  FloatRect bounds = result->Bounds();

  ShapeResultSpacing<String> spacing(string);
  FontDescription font_description;
  font_description.SetLetterSpacing(-5);
  spacing.SetSpacing(font_description);
  result->ApplySpacing(spacing);

  EXPECT_EQ(5 * 5, width - result->Width());
  EXPECT_EQ(5 * 4 - 1, bounds.Width() - result->Bounds().Width());
}

TEST_F(HarfBuzzShaperTest, NegativeLetterSpacingTo0) {
  String string(u"00000");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);
  float char_width = result->Width() / string.length();

  ShapeResultSpacing<String> spacing(string);
  FontDescription font_description;
  font_description.SetLetterSpacing(-char_width);
  spacing.SetSpacing(font_description);
  result->ApplySpacing(spacing);

  // EXPECT_EQ(0.0f, result->Width());
  EXPECT_NEAR(0.0f, result->Bounds().X(), 1);
  // Because all characters are at 0, the glyph bounds must be the char_width.
  // Allow being larger because accurate width requires re-measuring each glyph.
  EXPECT_GE(result->Bounds().MaxX(), char_width);
  EXPECT_LE(result->Bounds().MaxX(), char_width * 1.2);
}

TEST_F(HarfBuzzShaperTest, NegativeLetterSpacingToNegative) {
  String string(u"00000");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);
  float char_width = result->Width() / string.length();

  ShapeResultSpacing<String> spacing(string);
  FontDescription font_description;
  font_description.SetLetterSpacing(-2 * char_width);
  spacing.SetSpacing(font_description);
  result->ApplySpacing(spacing);

  // CSS does not allow negative width, it should be clampled to 0.
  // EXPECT_EQ(0.0f, result->Width());
  // Glyph bounding box should overflow to the left.
  EXPECT_EQ(-char_width * string.length(), result->Bounds().X());
  // MaxX() should be char_width. Allow being larger.
  EXPECT_GE(result->Bounds().MaxX(), char_width);
}

static struct OffsetForPositionTestData {
  float position;
  unsigned offset_ltr;
  unsigned offset_rtl;
  unsigned hit_test_ltr;
  unsigned hit_test_rtl;
  unsigned fit_ltr_ltr;
  unsigned fit_ltr_rtl;
  unsigned fit_rtl_ltr;
  unsigned fit_rtl_rtl;
} offset_for_position_fixed_pitch_test_data[] = {
    // The left edge.
    {-1, 0, 5, 0, 5, 0, 0, 5, 5},
    {0, 0, 5, 0, 5, 0, 0, 5, 5},
    // Hit test should round to the nearest glyph at the middle of a glyph.
    {4, 0, 4, 0, 5, 0, 1, 5, 4},
    {6, 0, 4, 1, 4, 0, 1, 5, 4},
    // Glyph boundary between the 1st and the 2nd glyph.
    // Avoid testing "10.0" to avoid rounding differences on Windows.
    {9.9, 0, 4, 1, 4, 0, 1, 5, 4},
    {10.1, 1, 3, 1, 4, 1, 2, 4, 3},
    // Run boundary is at position 20. The 1st run has 2 characters.
    {14, 1, 3, 1, 4, 1, 2, 4, 3},
    {16, 1, 3, 2, 3, 1, 2, 4, 3},
    {20.1, 2, 2, 2, 3, 2, 3, 3, 2},
    {24, 2, 2, 2, 3, 2, 3, 3, 2},
    {26, 2, 2, 3, 2, 2, 3, 3, 2},
    // The end of the ShapeResult. The result has 5 characters.
    {44, 4, 0, 4, 1, 4, 5, 1, 0},
    {46, 4, 0, 5, 0, 4, 5, 1, 0},
    {50, 5, 0, 5, 0, 5, 5, 0, 0},
    // Beyond the right edge of the ShapeResult.
    {51, 5, 0, 5, 0, 5, 5, 0, 0},
};

std::ostream& operator<<(std::ostream& ostream,
                         const OffsetForPositionTestData& data) {
  return ostream << data.position;
}

class OffsetForPositionTest
    : public HarfBuzzShaperTest,
      public testing::WithParamInterface<OffsetForPositionTestData> {};

INSTANTIATE_TEST_CASE_P(
    HarfBuzzShaperTest,
    OffsetForPositionTest,
    testing::ValuesIn(offset_for_position_fixed_pitch_test_data));

TEST_P(OffsetForPositionTest, Data) {
  auto data = GetParam();
  String string(u"01234");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  Font ahem = CreateAhem(10);
  scoped_refptr<ShapeResult> result =
      SplitRun(shaper.Shape(&ahem, TextDirection::kLtr), 2);
  EXPECT_EQ(data.offset_ltr, result->OffsetForPosition(data.position, false));
  EXPECT_EQ(data.hit_test_ltr, result->OffsetForPosition(data.position, true));
  EXPECT_EQ(data.fit_ltr_ltr,
            result->OffsetToFit(data.position, TextDirection::kLtr));
  EXPECT_EQ(data.fit_ltr_rtl,
            result->OffsetToFit(data.position, TextDirection::kRtl));

  result = SplitRun(shaper.Shape(&ahem, TextDirection::kRtl), 3);
  EXPECT_EQ(data.offset_rtl, result->OffsetForPosition(data.position, false));
  EXPECT_EQ(data.hit_test_rtl, result->OffsetForPosition(data.position, true));
  EXPECT_EQ(data.fit_rtl_ltr,
            result->OffsetToFit(data.position, TextDirection::kLtr));
  EXPECT_EQ(data.fit_rtl_rtl,
            result->OffsetToFit(data.position, TextDirection::kRtl));
}

TEST_F(HarfBuzzShaperTest, PositionForOffsetLatin) {
  String string = To16Bit("Hello World!", 12);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 12);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);
  scoped_refptr<ShapeResult> first = shaper.Shape(&font, direction, 0, 5);    // Hello
  scoped_refptr<ShapeResult> second = shaper.Shape(&font, direction, 6, 11);  // World

  EXPECT_EQ(0.0f, result->PositionForOffset(0));
  ASSERT_NEAR(first->Width(), result->PositionForOffset(5), 1);
  ASSERT_NEAR(second->Width(),
              result->PositionForOffset(11) - result->PositionForOffset(6), 1);
  ASSERT_NEAR(result->Width(), result->PositionForOffset(12), 0.1);
}

TEST_F(HarfBuzzShaperTest, PositionForOffsetArabic) {
  UChar arabic_string[] = {0x628, 0x64A, 0x629};
  TextDirection direction = TextDirection::kRtl;

  HarfBuzzShaper shaper(arabic_string, 3);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  EXPECT_EQ(0.0f, result->PositionForOffset(3));
  ASSERT_NEAR(result->Width(), result->PositionForOffset(0), 0.1);
}

TEST_F(HarfBuzzShaperTest, EmojiZWJSequence) {
  UChar emoji_zwj_sequence[] = {0x270C, 0x200D, 0xD83C, 0xDFFF,
                                0x270C, 0x200D, 0xD83C, 0xDFFC};
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(emoji_zwj_sequence, arraysize(emoji_zwj_sequence));
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);
}

// A Value-Parameterized Test class to test OffsetForPosition() with
// |include_partial_glyphs| parameter.
class IncludePartialGlyphs : public HarfBuzzShaperTest,
                             public testing::WithParamInterface<bool> {};

INSTANTIATE_TEST_CASE_P(OffsetForPositionTest,
                        IncludePartialGlyphs,
                        testing::Bool());

TEST_P(IncludePartialGlyphs, OffsetForPositionMatchesPositionForOffsetLatin) {
  String string = To16Bit("Hello World!", 12);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 12);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  bool include_partial_glyphs = GetParam();
  EXPECT_EQ(0u, result->OffsetForPosition(result->PositionForOffset(0),
                                          include_partial_glyphs));
  EXPECT_EQ(1u, result->OffsetForPosition(result->PositionForOffset(1),
                                          include_partial_glyphs));
  EXPECT_EQ(2u, result->OffsetForPosition(result->PositionForOffset(2),
                                          include_partial_glyphs));
  EXPECT_EQ(3u, result->OffsetForPosition(result->PositionForOffset(3),
                                          include_partial_glyphs));
  EXPECT_EQ(4u, result->OffsetForPosition(result->PositionForOffset(4),
                                          include_partial_glyphs));
  EXPECT_EQ(5u, result->OffsetForPosition(result->PositionForOffset(5),
                                          include_partial_glyphs));
  EXPECT_EQ(6u, result->OffsetForPosition(result->PositionForOffset(6),
                                          include_partial_glyphs));
  EXPECT_EQ(7u, result->OffsetForPosition(result->PositionForOffset(7),
                                          include_partial_glyphs));
  EXPECT_EQ(8u, result->OffsetForPosition(result->PositionForOffset(8),
                                          include_partial_glyphs));
  EXPECT_EQ(9u, result->OffsetForPosition(result->PositionForOffset(9),
                                          include_partial_glyphs));
  EXPECT_EQ(10u, result->OffsetForPosition(result->PositionForOffset(10),
                                           include_partial_glyphs));
  EXPECT_EQ(11u, result->OffsetForPosition(result->PositionForOffset(11),
                                           include_partial_glyphs));
  EXPECT_EQ(12u, result->OffsetForPosition(result->PositionForOffset(12),
                                           include_partial_glyphs));
}

TEST_P(IncludePartialGlyphs, OffsetForPositionMatchesPositionForOffsetArabic) {
  UChar arabic_string[] = {0x628, 0x64A, 0x629};
  TextDirection direction = TextDirection::kRtl;

  HarfBuzzShaper shaper(arabic_string, 3);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  bool include_partial_glyphs = GetParam();
  EXPECT_EQ(0u, result->OffsetForPosition(result->PositionForOffset(0),
                                          include_partial_glyphs));
  EXPECT_EQ(1u, result->OffsetForPosition(result->PositionForOffset(1),
                                          include_partial_glyphs));
  EXPECT_EQ(2u, result->OffsetForPosition(result->PositionForOffset(2),
                                          include_partial_glyphs));
  EXPECT_EQ(3u, result->OffsetForPosition(result->PositionForOffset(3),
                                          include_partial_glyphs));
}

TEST_P(IncludePartialGlyphs, OffsetForPositionMatchesPositionForOffsetMixed) {
  UChar mixed_string[] = {0x628, 0x64A, 0x629, 0xE20, 0x65E5, 0x62};
  HarfBuzzShaper shaper(mixed_string, 6);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  bool include_partial_glyphs = GetParam();
  EXPECT_EQ(0u, result->OffsetForPosition(result->PositionForOffset(0),
                                          include_partial_glyphs));
  EXPECT_EQ(1u, result->OffsetForPosition(result->PositionForOffset(1),
                                          include_partial_glyphs));
  EXPECT_EQ(2u, result->OffsetForPosition(result->PositionForOffset(2),
                                          include_partial_glyphs));
  EXPECT_EQ(3u, result->OffsetForPosition(result->PositionForOffset(3),
                                          include_partial_glyphs));
  EXPECT_EQ(4u, result->OffsetForPosition(result->PositionForOffset(4),
                                          include_partial_glyphs));
  EXPECT_EQ(5u, result->OffsetForPosition(result->PositionForOffset(5),
                                          include_partial_glyphs));
  EXPECT_EQ(6u, result->OffsetForPosition(result->PositionForOffset(6),
                                          include_partial_glyphs));
}

TEST_F(HarfBuzzShaperTest, PositionForOffsetMissingGlyph) {
  String string(u"\u0633\u0644\u0627\u0645");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kRtl);
  // Because the offset 1 and 2 should form a ligature, SubRange(2, 4) creates a
  // ShapeResult that does not have its first glyph.
  result = result->SubRange(2, 4);
  result->PositionForOffset(0);
  // Pass if |PositionForOffset| does not crash.
}

static struct ShapeResultCopyRangeTestData {
  const char16_t* string;
  TextDirection direction;
  unsigned break_point;
} shape_result_copy_range_test_data[] = {
    {u"ABC", TextDirection::kLtr, 1},
    {u"\u0648\u0644\u064A", TextDirection::kRtl, 1},
    // These strings creates 3 runs. Split it in the middle of 2nd run.
    {u"\u65E5Hello\u65E5\u65E5", TextDirection::kLtr, 3},
    {u"\u0648\u0644\u064A AB \u0628\u062A", TextDirection::kRtl, 5}};

std::ostream& operator<<(std::ostream& ostream,
                         const ShapeResultCopyRangeTestData& data) {
  return ostream << String(data.string) << " @ " << data.break_point << ", "
                 << data.direction;
}

class ShapeResultCopyRangeTest
    : public HarfBuzzShaperTest,
      public testing::WithParamInterface<ShapeResultCopyRangeTestData> {};

INSTANTIATE_TEST_CASE_P(HarfBuzzShaperTest,
                        ShapeResultCopyRangeTest,
                        testing::ValuesIn(shape_result_copy_range_test_data));

// Split a ShapeResult and combine them should match to the original result.
TEST_P(ShapeResultCopyRangeTest, Split) {
  const auto& test_data = GetParam();
  String string(test_data.string);
  TextDirection direction = test_data.direction;

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  // Split the result.
  scoped_refptr<ShapeResult> result1 = ShapeResult::Create(&font, 0, direction);
  result->CopyRange(0, test_data.break_point, result1.get());
  EXPECT_EQ(test_data.break_point, result1->NumCharacters());
  EXPECT_EQ(0u, result1->StartIndexForResult());
  EXPECT_EQ(test_data.break_point, result1->EndIndexForResult());

  scoped_refptr<ShapeResult> result2 = ShapeResult::Create(&font, 0, direction);
  result->CopyRange(test_data.break_point, string.length(), result2.get());
  EXPECT_EQ(string.length() - test_data.break_point, result2->NumCharacters());
  EXPECT_EQ(test_data.break_point, result2->StartIndexForResult());
  EXPECT_EQ(string.length(), result2->EndIndexForResult());

  // Combine them.
  scoped_refptr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result1->CopyRange(0, test_data.break_point, composite_result.get());
  result2->CopyRange(0, string.length(), composite_result.get());
  EXPECT_EQ(string.length(), composite_result->NumCharacters());

  // Test character indexes match.
  Vector<unsigned> expected_character_indexes =
      TestInfo(result)->CharacterIndexesForTesting();
  Vector<unsigned> composite_character_indexes =
      TestInfo(result)->CharacterIndexesForTesting();
  EXPECT_EQ(expected_character_indexes, composite_character_indexes);
}

// Shape ranges and combine them shold match to the result of shaping the whole
// string.
TEST_P(ShapeResultCopyRangeTest, ShapeRange) {
  const auto& test_data = GetParam();
  String string(test_data.string);
  TextDirection direction = test_data.direction;

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  // Shape each range.
  scoped_refptr<ShapeResult> result1 =
      shaper.Shape(&font, direction, 0, test_data.break_point);
  EXPECT_EQ(test_data.break_point, result1->NumCharacters());
  scoped_refptr<ShapeResult> result2 =
      shaper.Shape(&font, direction, test_data.break_point, string.length());
  EXPECT_EQ(string.length() - test_data.break_point, result2->NumCharacters());

  // Combine them.
  scoped_refptr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result1->CopyRange(0, test_data.break_point, composite_result.get());
  result2->CopyRange(0, string.length(), composite_result.get());
  EXPECT_EQ(string.length(), composite_result->NumCharacters());

  // Test character indexes match.
  Vector<unsigned> expected_character_indexes =
      TestInfo(result)->CharacterIndexesForTesting();
  Vector<unsigned> composite_character_indexes =
      TestInfo(result)->CharacterIndexesForTesting();
  EXPECT_EQ(expected_character_indexes, composite_character_indexes);
}

TEST_F(HarfBuzzShaperTest, ShapeResultCopyRangeIntoLatin) {
  String string = To16Bit("Testing ShapeResult::createSubRun", 33);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 33);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  scoped_refptr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result->CopyRange(0, 10, composite_result.get());
  result->CopyRange(10, 20, composite_result.get());
  result->CopyRange(20, 30, composite_result.get());
  result->CopyRange(30, 33, composite_result.get());

  EXPECT_EQ(result->NumCharacters(), composite_result->NumCharacters());
  EXPECT_EQ(result->SnappedWidth(), composite_result->SnappedWidth());
  EXPECT_EQ(result->Bounds(), composite_result->Bounds());
  EXPECT_EQ(result->SnappedStartPositionForOffset(0),
            composite_result->SnappedStartPositionForOffset(0));
  EXPECT_EQ(result->SnappedStartPositionForOffset(15),
            composite_result->SnappedStartPositionForOffset(15));
  EXPECT_EQ(result->SnappedStartPositionForOffset(30),
            composite_result->SnappedStartPositionForOffset(30));
  EXPECT_EQ(result->SnappedStartPositionForOffset(33),
            composite_result->SnappedStartPositionForOffset(33));
}

TEST_F(HarfBuzzShaperTest, ShapeResultCopyRangeIntoArabicThaiHanLatin) {
  UChar mixed_string[] = {0x628, 0x20, 0x64A, 0x629, 0x20, 0xE20, 0x65E5, 0x62};
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(mixed_string, 8);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  // Check width and bounds are not too much different. ".2" is heuristic.
  EXPECT_NEAR(result->Width(), result->Bounds().Width(), result->Width() * .2);

  scoped_refptr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result->CopyRange(0, 4, composite_result.get());
  result->CopyRange(4, 6, composite_result.get());
  result->CopyRange(6, 8, composite_result.get());

  EXPECT_EQ(result->NumCharacters(), composite_result->NumCharacters());
  EXPECT_EQ(result->SnappedWidth(), composite_result->SnappedWidth());
  EXPECT_TRUE(composite_result->Bounds().Contains(result->Bounds()))
      << composite_result->Bounds() << "/" << result->Bounds();
  EXPECT_EQ(result->SnappedStartPositionForOffset(0),
            composite_result->SnappedStartPositionForOffset(0));
  EXPECT_EQ(result->SnappedStartPositionForOffset(1),
            composite_result->SnappedStartPositionForOffset(1));
  EXPECT_EQ(result->SnappedStartPositionForOffset(2),
            composite_result->SnappedStartPositionForOffset(2));
  EXPECT_EQ(result->SnappedStartPositionForOffset(3),
            composite_result->SnappedStartPositionForOffset(3));
  EXPECT_EQ(result->SnappedStartPositionForOffset(4),
            composite_result->SnappedStartPositionForOffset(4));
  EXPECT_EQ(result->SnappedStartPositionForOffset(5),
            composite_result->SnappedStartPositionForOffset(5));
  EXPECT_EQ(result->SnappedStartPositionForOffset(6),
            composite_result->SnappedStartPositionForOffset(6));
  EXPECT_EQ(result->SnappedStartPositionForOffset(7),
            composite_result->SnappedStartPositionForOffset(7));
  EXPECT_EQ(result->SnappedStartPositionForOffset(8),
            composite_result->SnappedStartPositionForOffset(8));
}

TEST_F(HarfBuzzShaperTest, ShapeResultCopyRangeAcrossRuns) {
  // Create 3 runs:
  // [0]: 1 character.
  // [1]: 5 characters.
  // [2]: 2 character.
  String mixed_string(u"\u65E5Hello\u65E5\u65E5");
  TextDirection direction = TextDirection::kLtr;
  HarfBuzzShaper shaper(mixed_string.Characters16(), mixed_string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  // Check width and bounds are not too much different. ".1" is heuristic.
  EXPECT_NEAR(result->Width(), result->Bounds().Width(), result->Width() * .1);

  // CopyRange(5, 7) should copy 1 character from [1] and 1 from [2].
  scoped_refptr<ShapeResult> target = ShapeResult::Create(&font, 0, direction);
  result->CopyRange(5, 7, target.get());
  EXPECT_EQ(2u, target->NumCharacters());
}

TEST_F(HarfBuzzShaperTest, ShapeResultCopyRangeSegmentGlyphBoundingBox) {
  String string(u"THello worldL");
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result1 = shaper.Shape(&font, direction, 0, 6);
  scoped_refptr<ShapeResult> result2 =
      shaper.Shape(&font, direction, 6, string.length());

  scoped_refptr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result1->CopyRange(0, 6, composite_result.get());
  result2->CopyRange(6, string.length(), composite_result.get());

  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);
  EXPECT_EQ(result->Bounds(), composite_result->Bounds());

  // Check width and bounds are not too much different. ".1" is heuristic.
  EXPECT_NEAR(result->Width(), result->Bounds().Width(), result->Width() * .1);
}

TEST_F(HarfBuzzShaperTest, ShapeResultCopyRangeBoundsLtr) {
  String string(u". ");
  TextDirection direction = TextDirection::kLtr;
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  // Because a space character does not have ink, the bounds of "." should be
  // the same as the bounds of ". ".
  scoped_refptr<ShapeResult> sub_range = result->SubRange(0, 1);
  EXPECT_EQ(sub_range->Bounds().Width(), result->Bounds().Width());
}

TEST_F(HarfBuzzShaperTest, ShapeResultCopyRangeBoundsRtl) {
  String string(u". ");
  TextDirection direction = TextDirection::kRtl;
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  // Because a space character does not have ink, the bounds of "." should be
  // the same as the bounds of ". ".
  scoped_refptr<ShapeResult> sub_range = result->SubRange(0, 1);
  EXPECT_EQ(sub_range->Bounds().Width(), result->Bounds().Width());
}

TEST_F(HarfBuzzShaperTest, SubRange) {
  String string(u"Hello world");
  TextDirection direction = TextDirection::kRtl;
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  scoped_refptr<ShapeResult> sub_range = result->SubRange(4, 7);
  DCHECK_EQ(4u, sub_range->StartIndexForResult());
  DCHECK_EQ(7u, sub_range->EndIndexForResult());
  DCHECK_EQ(3u, sub_range->NumCharacters());
  DCHECK_EQ(result->Direction(), sub_range->Direction());
}

TEST_F(HarfBuzzShaperTest, SafeToBreakLatinCommonLigatures) {
  FontDescription::VariantLigatures ligatures;
  ligatures.common = FontDescription::kEnabledLigaturesState;

  // MEgalopolis Extra has a lot of ligatures which this test relies on.
  Font testFont = blink::test::CreateTestFont(
      "MEgalopolis",
      blink::test::PlatformTestDataPath(
          "third_party/MEgalopolis/MEgalopolisExtra.woff"),
      16, &ligatures);

  String string = To16Bit("ffi ff", 6);
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&testFont, TextDirection::kLtr);

  EXPECT_EQ(0u, result->NextSafeToBreakOffset(0));  // At start of string.
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(1));  // At end of "ffi" ligature.
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(2));  // At end of "ffi" ligature.
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(3));  // At end of "ffi" ligature.
  EXPECT_EQ(4u, result->NextSafeToBreakOffset(4));  // After space.
  EXPECT_EQ(6u, result->NextSafeToBreakOffset(5));  // At end of "ff" ligature.
  EXPECT_EQ(6u, result->NextSafeToBreakOffset(6));  // At end of "ff" ligature.

  // Verify safe to break information in copied results to ensure that both
  // copying and multi-run break information works.
  scoped_refptr<ShapeResult> copied_result =
      ShapeResult::Create(&testFont, 0, TextDirection::kLtr);
  result->CopyRange(0, 3, copied_result.get());
  result->CopyRange(3, string.length(), copied_result.get());

  EXPECT_EQ(0u, copied_result->NextSafeToBreakOffset(0));
  EXPECT_EQ(3u, copied_result->NextSafeToBreakOffset(1));
  EXPECT_EQ(3u, copied_result->NextSafeToBreakOffset(2));
  EXPECT_EQ(3u, copied_result->NextSafeToBreakOffset(3));
  EXPECT_EQ(4u, copied_result->NextSafeToBreakOffset(4));
  EXPECT_EQ(6u, copied_result->NextSafeToBreakOffset(5));
  EXPECT_EQ(6u, copied_result->NextSafeToBreakOffset(6));
}

TEST_F(HarfBuzzShaperTest, SafeToBreakPreviousLatinCommonLigatures) {
  FontDescription::VariantLigatures ligatures;
  ligatures.common = FontDescription::kEnabledLigaturesState;

  // MEgalopolis Extra has a lot of ligatures which this test relies on.
  Font testFont = blink::test::CreateTestFont(
      "MEgalopolis",
      blink::test::PlatformTestDataPath(
          "third_party/MEgalopolis/MEgalopolisExtra.woff"),
      16, &ligatures);

  String string = To16Bit("ffi ff", 6);
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&testFont, TextDirection::kLtr);

  EXPECT_EQ(6u, result->PreviousSafeToBreakOffset(6));  // At end of "ff" liga.
  EXPECT_EQ(4u, result->PreviousSafeToBreakOffset(5));  // At end of "ff" liga.
  EXPECT_EQ(4u, result->PreviousSafeToBreakOffset(4));  // After space.
  EXPECT_EQ(3u, result->PreviousSafeToBreakOffset(3));  // At end of "ffi" liga.
  EXPECT_EQ(0u, result->PreviousSafeToBreakOffset(2));  // At start of string.
  EXPECT_EQ(0u, result->PreviousSafeToBreakOffset(1));  // At start of string.
  EXPECT_EQ(0u, result->PreviousSafeToBreakOffset(0));  // At start of string.

  // Verify safe to break information in copied results to ensure that both
  // copying and multi-run break information works.
  scoped_refptr<ShapeResult> copied_result =
      ShapeResult::Create(&testFont, 0, TextDirection::kLtr);
  result->CopyRange(0, 3, copied_result.get());
  result->CopyRange(3, string.length(), copied_result.get());

  EXPECT_EQ(6u, copied_result->PreviousSafeToBreakOffset(6));
  EXPECT_EQ(4u, copied_result->PreviousSafeToBreakOffset(5));
  EXPECT_EQ(4u, copied_result->PreviousSafeToBreakOffset(4));
  EXPECT_EQ(3u, copied_result->PreviousSafeToBreakOffset(3));
  EXPECT_EQ(0u, copied_result->PreviousSafeToBreakOffset(2));
  EXPECT_EQ(0u, copied_result->PreviousSafeToBreakOffset(1));
  EXPECT_EQ(0u, copied_result->PreviousSafeToBreakOffset(0));
}

TEST_F(HarfBuzzShaperTest, SafeToBreakLatinDiscretionaryLigatures) {
  FontDescription::VariantLigatures ligatures;
  ligatures.common = FontDescription::kEnabledLigaturesState;
  ligatures.discretionary = FontDescription::kEnabledLigaturesState;

  // MEgalopolis Extra has a lot of ligatures which this test relies on.
  Font testFont = blink::test::CreateTestFont(
      "MEgalopolis",
      blink::test::PlatformTestDataPath(
          "third_party/MEgalopolis/MEgalopolisExtra.woff"),
      16, &ligatures);

  // RA and CA form ligatures, most glyph pairs have kerning.
  String string(u"ABRACADABRA");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&testFont, TextDirection::kLtr);
  EXPECT_EQ(6u, result->NextSafeToBreakOffset(1));    // After CA ligature.
  EXPECT_EQ(6u, result->NextSafeToBreakOffset(6));    // After CA ligature.
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(7));   // At end of string.
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(9));   // At end of string.
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(10));  // At end of string.

  // Add zero-width spaces at the safe to break offsets.
  String refString(u"ABRACA\u200BDAB\u200BRA");
  HarfBuzzShaper refShaper(refString.Characters16(), refString.length());
  scoped_refptr<ShapeResult> referenceResult =
      refShaper.Shape(&testFont, TextDirection::kLtr);

  // Results should be identical if it truly is safe to break at the designated
  // safe-to-break offsets
  EXPECT_EQ(result->SnappedWidth(), referenceResult->SnappedWidth());
  EXPECT_EQ(result->Bounds(), referenceResult->Bounds());
  EXPECT_EQ(result->SnappedStartPositionForOffset(0),
            referenceResult->SnappedStartPositionForOffset(0));
  EXPECT_EQ(result->SnappedStartPositionForOffset(1),
            referenceResult->SnappedStartPositionForOffset(1));
  EXPECT_EQ(result->SnappedStartPositionForOffset(2),
            referenceResult->SnappedStartPositionForOffset(2));
  EXPECT_EQ(result->SnappedStartPositionForOffset(3),
            referenceResult->SnappedStartPositionForOffset(3));
  EXPECT_EQ(result->SnappedStartPositionForOffset(4),
            referenceResult->SnappedStartPositionForOffset(4));
  EXPECT_EQ(result->SnappedStartPositionForOffset(5),
            referenceResult->SnappedStartPositionForOffset(5));

  // First zero-width space is at position 6 so the the matching character in
  // the reference results is 7.
  EXPECT_EQ(result->SnappedStartPositionForOffset(6),
            referenceResult->SnappedStartPositionForOffset(7));
  EXPECT_EQ(result->SnappedStartPositionForOffset(7),
            referenceResult->SnappedStartPositionForOffset(8));
  EXPECT_EQ(result->SnappedStartPositionForOffset(8),
            referenceResult->SnappedStartPositionForOffset(9));

  // Second zero-width space is at position 9 so the the matching character in
  // the reference results is 11.
  EXPECT_EQ(result->SnappedStartPositionForOffset(9),
            referenceResult->SnappedStartPositionForOffset(11));
  EXPECT_EQ(result->SnappedStartPositionForOffset(10),
            referenceResult->SnappedStartPositionForOffset(12));
}

// TODO(layout-dev): This test fails on Mac due to AAT shaping.
TEST_F(HarfBuzzShaperTest, DISABLED_SafeToBreakArabicCommonLigatures) {
  FontDescription::VariantLigatures ligatures;
  ligatures.common = FontDescription::kEnabledLigaturesState;

  // كسر الاختبار
  String string(
      u"\u0643\u0633\u0631\u0020\u0627\u0644\u0627\u062E\u062A\u0628\u0627"
      u"\u0631");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kRtl);

  // Safe to break at 0, 3, 4, 5, 7, and 11.
  EXPECT_EQ(0u, result->NextSafeToBreakOffset(0));
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(1));
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(2));
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(3));
  EXPECT_EQ(4u, result->NextSafeToBreakOffset(4));
  EXPECT_EQ(5u, result->NextSafeToBreakOffset(5));
  EXPECT_EQ(7u, result->NextSafeToBreakOffset(6));
  EXPECT_EQ(7u, result->NextSafeToBreakOffset(7));
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(8));
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(9));
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(10));
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(11));
  EXPECT_EQ(12u, result->NextSafeToBreakOffset(12));
}

// TODO(layout-dev): Expand RTL test coverage and add tests for mixed
// directionality strings.

// Test when some characters are missing in |runs_|.
// RTL on Mac may not have runs for all characters. crbug.com/774034
TEST_P(ShapeParameterTest, SafeToBreakMissingRun) {
  TextDirection direction = GetParam();
  scoped_refptr<ShapeResult> result = ShapeResult::Create(&font, 7, direction);
  result->InsertRunForTesting(2, 1, direction, {0});
  result->InsertRunForTesting(3, 3, direction, {0, 1});
  // The character index 7 is missing.
  result->InsertRunForTesting(8, 2, direction, {0});

  EXPECT_EQ(2u, result->NextSafeToBreakOffset(2));
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(3));
  EXPECT_EQ(4u, result->NextSafeToBreakOffset(4));
  EXPECT_EQ(6u, result->NextSafeToBreakOffset(5));
  EXPECT_EQ(6u, result->NextSafeToBreakOffset(6));
  EXPECT_EQ(8u, result->NextSafeToBreakOffset(7));
  EXPECT_EQ(8u, result->NextSafeToBreakOffset(8));
  EXPECT_EQ(10u, result->NextSafeToBreakOffset(9));

  EXPECT_EQ(2u, result->PreviousSafeToBreakOffset(2));
  EXPECT_EQ(3u, result->PreviousSafeToBreakOffset(3));
  EXPECT_EQ(4u, result->PreviousSafeToBreakOffset(4));
  EXPECT_EQ(4u, result->PreviousSafeToBreakOffset(5));
  EXPECT_EQ(6u, result->PreviousSafeToBreakOffset(6));
  EXPECT_EQ(6u, result->PreviousSafeToBreakOffset(7));
  EXPECT_EQ(8u, result->PreviousSafeToBreakOffset(8));
  EXPECT_EQ(8u, result->PreviousSafeToBreakOffset(9));
}

// Call this to ensure your test string has some kerning going on.
static bool KerningIsHappening(const FontDescription& font_description,
                               TextDirection direction,
                               const String& str) {
  FontDescription no_kern = font_description;
  no_kern.SetKerning(FontDescription::kNoneKerning);

  FontDescription kern = font_description;
  kern.SetKerning(FontDescription::kAutoKerning);

  Font font_no_kern(no_kern);
  font_no_kern.Update(nullptr);

  Font font_kern(kern);
  font_kern.Update(nullptr);

  HarfBuzzShaper shaper(str.Characters16(), str.length());

  scoped_refptr<ShapeResult> result_no_kern =
      shaper.Shape(&font_no_kern, direction);
  scoped_refptr<ShapeResult> result_kern = shaper.Shape(&font_kern, direction);

  for (unsigned i = 0; i < str.length(); i++) {
    if (result_no_kern->PositionForOffset(i) !=
        result_kern->PositionForOffset(i))
      return true;
  }
  return false;
}

TEST_F(HarfBuzzShaperTest, KerningIsHappeningWorks) {
  EXPECT_TRUE(
      KerningIsHappening(font_description, TextDirection::kLtr, u"AVOID"));
  EXPECT_FALSE(
      KerningIsHappening(font_description, TextDirection::kLtr, u"NOID"));

  // We won't kern vertically with the default font.
  font_description.SetOrientation(FontOrientation::kVerticalUpright);

  EXPECT_FALSE(
      KerningIsHappening(font_description, TextDirection::kLtr, u"AVOID"));
  EXPECT_FALSE(
      KerningIsHappening(font_description, TextDirection::kLtr, u"NOID"));
}

TEST_F(HarfBuzzShaperTest,
       ShapeHorizontalWithoutSubpixelPositionWithoutKerningIsRounded) {
  ScopedSubpixelOverride subpixel_override(false);

  String string(u"NOID");
  TextDirection direction = TextDirection::kLtr;
  ASSERT_FALSE(KerningIsHappening(font_description, direction, string));

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  for (unsigned i = 0; i < string.length(); i++) {
    float position = result->PositionForOffset(i);
    EXPECT_EQ(round(position), position)
        << "Position not rounded at offset " << i;
  }
}

TEST_F(HarfBuzzShaperTest,
       ShapeHorizontalWithSubpixelPositionWithoutKerningIsNotRounded) {
  ScopedSubpixelOverride subpixel_override(true);

  String string(u"NOID");
  TextDirection direction = TextDirection::kLtr;
  ASSERT_FALSE(KerningIsHappening(font_description, direction, string));

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  for (unsigned i = 0; i < string.length(); i++) {
    float position = result->PositionForOffset(i);
    if (round(position) != position)
      return;
  }

  EXPECT_TRUE(false) << "No unrounded positions found";
}

TEST_F(HarfBuzzShaperTest,
       ShapeHorizontalWithoutSubpixelPositionWithKerningIsRounded) {
  ScopedSubpixelOverride subpixel_override(false);

  String string(u"AVOID");
  TextDirection direction = TextDirection::kLtr;
  ASSERT_TRUE(KerningIsHappening(font_description, direction, string));

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  for (unsigned i = 0; i < string.length(); i++) {
    float position = result->PositionForOffset(i);
    EXPECT_EQ(round(position), position)
        << "Position not rounded at offset " << i;
  }
}

TEST_F(HarfBuzzShaperTest,
       ShapeHorizontalWithSubpixelPositionWithKerningIsNotRounded) {
  ScopedSubpixelOverride subpixel_override(true);

  String string(u"AVOID");
  TextDirection direction = TextDirection::kLtr;
  ASSERT_TRUE(KerningIsHappening(font_description, direction, string));

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  for (unsigned i = 0; i < string.length(); i++) {
    float position = result->PositionForOffset(i);
    if (round(position) != position)
      return;
  }

  EXPECT_TRUE(false) << "No unrounded positions found";
}

TEST_F(HarfBuzzShaperTest, ShapeVerticalWithoutSubpixelPositionIsRounded) {
  ScopedSubpixelOverride subpixel_override(false);

  font_description.SetOrientation(FontOrientation::kVerticalUpright);
  font = Font(font_description);
  font.Update(nullptr);

  String string(u"\u65E5\u65E5\u65E5");
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  for (unsigned i = 0; i < string.length(); i++) {
    float position = result->PositionForOffset(i);
    EXPECT_EQ(round(position), position)
        << "Position not rounded at offset " << i;
  }
}

TEST_F(HarfBuzzShaperTest, ShapeVerticalWithSubpixelPositionIsRounded) {
  ScopedSubpixelOverride subpixel_override(true);

  font_description.SetOrientation(FontOrientation::kVerticalUpright);
  font = Font(font_description);
  font.Update(nullptr);

  String string(u"\u65E5\u65E5\u65E5");
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  // Vertical text is never subpixel positioned.
  for (unsigned i = 0; i < string.length(); i++) {
    float position = result->PositionForOffset(i);
    EXPECT_EQ(round(position), position)
        << "Position not rounded at offset " << i;
  }
}

}  // namespace blink
