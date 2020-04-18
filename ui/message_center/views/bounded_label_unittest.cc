// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/bounded_label.h"

#include <limits>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/views_test_base.h"

namespace message_center {

namespace test {

/* Test fixture ***************************************************************/

class BoundedLabelTest : public views::ViewsTestBase {
 public:
  BoundedLabelTest() {
    digit_pixels_ = gfx::GetStringWidthF(base::UTF8ToUTF16("0"), font_list_);
    space_pixels_ = gfx::GetStringWidthF(base::UTF8ToUTF16(" "), font_list_);
    ellipsis_pixels_ =
        gfx::GetStringWidthF(base::UTF8ToUTF16("\xE2\x80\xA6"), font_list_);
  }

  ~BoundedLabelTest() override {}

  // Replaces all occurences of three periods ("...") in the specified string
  // with an ellipses character (UTF8 "\xE2\x80\xA6") and returns a string16
  // with the results. This allows test strings to be specified as ASCII const
  // char* strings, making tests more readable and easier to write.
  base::string16 ToString(const char* string) {
    const char kPeriods[] = "...";
    const char kEllipses[] = "\xE2\x80\xA6";
    std::string result = string;
    base::ReplaceSubstringsAfterOffset(&result, 0, kPeriods, kEllipses);
    return base::UTF8ToUTF16(result);
  }

  // Converts the specified elision width to pixels. To make tests somewhat
  // independent of the fonts of the platform on which they're run, the elision
  // widths are specified as XYZ integers, with the corresponding width in
  // pixels being X times the width of digit characters plus Y times the width
  // of spaces plus Z times the width of ellipses in the default font of the
  // test plaform. It is assumed that all digits have the same width in that
  // font, that this width is greater than the width of spaces, and that the
  // width of 3 digits is greater than the width of ellipses.
  int ToPixels(int width) {
    return std::ceil(digit_pixels_ * (width / 100) +
                     space_pixels_ * ((width % 100) / 10) +
                     ellipsis_pixels_ * (width % 10));
  }

  // Exercise BounderLabel::GetWrappedText() using the fixture's test label.
  base::string16 GetWrappedText(int width) {
    return label_->GetWrappedTextForTest(width, lines_);
  }

  // Exercise BounderLabel::GetLinesForWidthAndLimit() using the test label.
  int GetLinesForWidth(int width) {
    label_->SetBounds(0, 0, width, font_list_.GetHeight() * lines_);
    return label_->GetLinesForWidthAndLimit(width, lines_);
  }

 protected:
  // Creates a label to test with. Returns this fixture, which can be used to
  // test the newly created label using the exercise methods above.
  BoundedLabelTest& Label(base::string16 text, int lines) {
    lines_ = lines;
    label_.reset(new BoundedLabel(text, font_list_));
    label_->SetLineLimit(lines_);
    return *this;
  }

 private:
  // The default font list, which will be used for tests.
  gfx::FontList font_list_;
  float digit_pixels_;
  float space_pixels_;
  float ellipsis_pixels_;
  std::unique_ptr<BoundedLabel> label_;
  int lines_;
};

/* Test macro *****************************************************************/

#define TEST_WRAP(expected, text, width, lines) \
  EXPECT_EQ(ToString(expected), \
            Label(ToString(text), lines).GetWrappedText(ToPixels(width)))

#define TEST_LINES(expected, text, width, lines) \
  EXPECT_EQ(expected, \
            Label(ToString(text), lines).GetLinesForWidth(ToPixels(width)))

/* Elision tests **************************************************************/

TEST_F(BoundedLabelTest, GetWrappedTextTest) {
  // One word per line: No ellision should be made when not necessary.
  TEST_WRAP("123", "123", 301, 1);
  TEST_WRAP("123", "123", 301, 2);
  TEST_WRAP("123", "123", 301, 3);
  TEST_WRAP("123\n456", "123 456", 301, 2);
  TEST_WRAP("123\n456", "123 456", 301, 3);
  TEST_WRAP("123\n456\n789", "123 456 789", 301, 3);

  // One word per line: Ellisions should be made when necessary.
  TEST_WRAP("123...", "123 456", 301, 1);
  TEST_WRAP("123...", "123 456 789", 301, 1);
  TEST_WRAP("123\n456...", "123 456 789", 301, 2);

  // Two words per line: No ellision should be made when not necessary.
  TEST_WRAP("123 456", "123 456", 621, 1);
  TEST_WRAP("123 456", "123 456", 621, 2);
  TEST_WRAP("123 456", "123 456", 621, 3);
  TEST_WRAP("123 456\n789 012", "123 456 789 012", 621, 2);
  TEST_WRAP("123 456\n789 012", "123 456 789 012", 621, 3);
  TEST_WRAP("123 456\n789 012\n345 678", "123 456 789 012 345 678", 621, 3);

  // Two words per line: Ellisions should be made when necessary.
  TEST_WRAP("123 456...", "123 456 789 012", 621, 1);
  TEST_WRAP("123 456...", "123 456 789 012 345 678", 621, 1);
  TEST_WRAP("123 456\n789 012...", "123 456 789 012 345 678", 621, 2);

  // Single trailing spaces: No ellipses should be added.
  TEST_WRAP("123", "123 ", 301, 1);
  TEST_WRAP("123\n456", "123 456 ", 301, 2);
  TEST_WRAP("123\n456\n789", "123 456 789 ", 301, 3);
  TEST_WRAP("123 456", "123 456 ", 611, 1);
  TEST_WRAP("123 456\n789 012", "123 456 789 012 ", 611, 2);
  TEST_WRAP("123 456\n789 012\n345 678", "123 456 789 012 345 678 ", 611, 3);

  // Multiple trailing spaces: No ellipses should be added.
  TEST_WRAP("123", "123         ", 301, 1);
  TEST_WRAP("123\n456", "123 456         ", 301, 2);
  TEST_WRAP("123\n456\n789", "123 456 789         ", 301, 3);
  TEST_WRAP("123 456", "123 456         ", 611, 1);
  TEST_WRAP("123 456\n789 012", "123 456 789 012         ", 611, 2);
  TEST_WRAP("123 456\n789 012\n345 678",
            "123 456 789 012 345 678         ", 611, 3);

  // Multiple spaces between words on the same line: Spaces should be preserved.
  // Test cases for single spaces between such words are included in the "Two
  // words per line" sections above.
  TEST_WRAP("123  456", "123  456", 621, 1);
  TEST_WRAP("123  456...", "123  456 789   012", 631, 1);
  TEST_WRAP("123  456\n789   012", "123  456 789   012", 631, 2);
  TEST_WRAP("123  456...", "123  456 789   012  345    678", 621, 1);
  TEST_WRAP("123  456\n789   012...", "123  456 789   012 345    678", 631, 2);
  TEST_WRAP("123  456\n789   012\n345    678",
            "123  456 789   012 345    678", 641, 3);

  // Multiple spaces between words split across lines: Spaces should be removed
  // even if lines are wide enough to include those spaces. Test cases for
  // single spaces between such words are included in the "Two words  per line"
  // sections above.
  TEST_WRAP("123\n456", "123  456", 321, 2);
  TEST_WRAP("123\n456", "123         456", 391, 2);
  TEST_WRAP("123\n456...", "123  456  789", 321, 2);
  TEST_WRAP("123\n456...", "123         456         789", 391, 2);
  TEST_WRAP("123  456\n789  012", "123  456  789  012", 641, 2);
  TEST_WRAP("123  456\n789  012...", "123  456  789  012  345  678", 641, 2);

  // Long words without spaces should be wrapped when necessary.
  TEST_WRAP("123\n456", "123456", 300, 9);

  // TODO(dharcourt): Add test cases to verify that:
  // - Spaces before elisions are removed
  // - Leading spaces are preserved
  // - Words are split when they are longer than lines
  // - Words are clipped when they are longer than the last line
  // - No blank line are created before or after clipped word
  // - Spaces at the end of the text are removed

  // TODO(dharcourt): Add test cases for:
  // - Empty and very large strings
  // - Zero, very large, and negative line limit values
  // - Other input boundary conditions
  // TODO(dharcourt): Add some randomly generated fuzz test cases.
}

/* GetLinesTest ***************************************************************/

TEST_F(BoundedLabelTest, GetLinesTest) {
  // Zero and negative width values should yield zero lines.
  TEST_LINES(0, "123 456", 0, 1);
  TEST_LINES(0, "123 456", -1, 1);
  TEST_LINES(0, "123 456", -2, 1);
  TEST_LINES(0, "123 456", std::numeric_limits<int>::min(), 1);

  // Small width values should yield one word per line.
  TEST_LINES(1, "123 456", 1, 1);
  TEST_LINES(2, "123 456", 1, 2);
  TEST_LINES(1, "123 456", 2, 1);
  TEST_LINES(2, "123 456", 2, 2);
  TEST_LINES(1, "123 456", 3, 1);
  TEST_LINES(2, "123 456", 3, 2);

  // Large width values should yield all words on one line.
  TEST_LINES(1, "123 456", 610, 1);
  TEST_LINES(1, "123 456", std::numeric_limits<int>::max(), 1);
}

/* Other tests ****************************************************************/

// TODO(dharcourt): Add test cases to verify that:
// - SetMaxLines() affects the return values of some methods but not others.
// - Bound changes affects GetPreferredLines(), GetTextSize(), and
//   GetWrappedText() return values.
// - GetTextFlags are as expected.

}  // namespace test

}  // namespace message_center
