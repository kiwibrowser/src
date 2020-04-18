// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/omnibox_popup_model.h"

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_popup_view.h"
#include "components/omnibox/browser/test_omnibox_client.h"
#include "components/omnibox/browser/test_omnibox_edit_controller.h"
#include "components/omnibox/browser/test_omnibox_view.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"

namespace {

class TestOmniboxPopupView : public OmniboxPopupView {
 public:
  ~TestOmniboxPopupView() override {}
  bool IsOpen() const override { return false; }
  void InvalidateLine(size_t line) override {}
  void OnLineSelected(size_t line) override {}
  void UpdatePopupAppearance() override {}
  void OnMatchIconUpdated(size_t match_index) override {}
  void PaintUpdatesNow() override {}
  void OnDragCanceled() override {}
};

}  // namespace

class OmniboxPopupModelTest : public ::testing::Test {
 public:
  OmniboxPopupModelTest()
      : view_(&controller_),
        model_(&view_, &controller_, std::make_unique<TestOmniboxClient>()),
        popup_model_(&popup_view_, &model_) {}

  OmniboxEditModel* model() { return &model_; }
  OmniboxPopupModel* popup_model() { return &popup_model_; }

 private:
  base::test::ScopedTaskEnvironment task_environment_;
  TestOmniboxEditController controller_;
  TestOmniboxView view_;
  OmniboxEditModel model_;
  TestOmniboxPopupView popup_view_;
  OmniboxPopupModel popup_model_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxPopupModelTest);
};

// This verifies that the new treatment of the user's selected match in
// |SetSelectedLine()| with removed |AutocompleteResult::Selection::empty()|
// is correct in the face of various replacement versions of |empty()|.
TEST_F(OmniboxPopupModelTest, SetSelectedLine) {
  ACMatches matches;
  for (size_t i = 0; i < 2; ++i) {
    AutocompleteMatch match(nullptr, 1000, false,
                            AutocompleteMatchType::URL_WHAT_YOU_TYPED);
    match.keyword = base::ASCIIToUTF16("match");
    match.allowed_to_be_default_match = true;
    matches.push_back(match);
  }
  auto* result = &model()->autocomplete_controller()->result_;
  AutocompleteInput input(base::UTF8ToUTF16("match"),
                          metrics::OmniboxEventProto::NTP,
                          TestSchemeClassifier());
  result->AppendMatches(input, matches);
  result->SortAndCull(input, nullptr);
  popup_model()->OnResultChanged();
  EXPECT_FALSE(popup_model()->has_selected_match());
  popup_model()->SetSelectedLine(0, true, false);
  EXPECT_FALSE(popup_model()->has_selected_match());
  popup_model()->SetSelectedLine(0, false, false);
  EXPECT_TRUE(popup_model()->has_selected_match());
}

TEST_F(OmniboxPopupModelTest, ComputeMatchMaxWidths) {
  int contents_max_width, description_max_width;
  const int separator_width = 10;
  const int kMinimumContentsWidth = 300;
  int contents_width, description_width, available_width;

  // Both contents and description fit fine.
  contents_width = 100;
  description_width = 50;
  available_width = 200;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width,
      false, true, &contents_max_width, &description_max_width);
  EXPECT_EQ(contents_width, contents_max_width);
  EXPECT_EQ(description_width, description_max_width);

  // Contents should be given as much space as it wants up to 300 pixels.
  contents_width = 100;
  description_width = 50;
  available_width = 100;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width,
      false, true, &contents_max_width, &description_max_width);
  EXPECT_EQ(contents_width, contents_max_width);
  EXPECT_EQ(0, description_max_width);

  // Description should be hidden if it's at least 75 pixels wide but doesn't
  // get 75 pixels of space.
  contents_width = 300;
  description_width = 100;
  available_width = 384;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width,
      false, true, &contents_max_width, &description_max_width);
  EXPECT_EQ(contents_width, contents_max_width);
  EXPECT_EQ(0, description_max_width);

  // If contents and description are on separate lines, each can take the full
  // available width.
  contents_width = 300;
  description_width = 100;
  available_width = 384;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width, true,
      true, &contents_max_width, &description_max_width);
  EXPECT_EQ(contents_width, contents_max_width);
  EXPECT_EQ(description_width, description_max_width);

  // Both contents and description will be limited.
  contents_width = 310;
  description_width = 150;
  available_width = 400;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width,
      false, true, &contents_max_width, &description_max_width);
  EXPECT_EQ(kMinimumContentsWidth, contents_max_width);
  EXPECT_EQ(available_width - kMinimumContentsWidth - separator_width,
            description_max_width);

  // Contents takes all available space.
  contents_width = 400;
  description_width = 0;
  available_width = 200;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width,
      false, true, &contents_max_width, &description_max_width);
  EXPECT_EQ(available_width, contents_max_width);
  EXPECT_EQ(0, description_max_width);

  // Large contents will be truncated but small description won't if two line
  // suggestion.
  contents_width = 400;
  description_width = 100;
  available_width = 200;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width, true,
      true, &contents_max_width, &description_max_width);
  EXPECT_EQ(available_width, contents_max_width);
  EXPECT_EQ(description_width, description_max_width);

  // Large description will be truncated but small contents won't if two line
  // suggestion.
  contents_width = 100;
  description_width = 400;
  available_width = 200;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width, true,
      true, &contents_max_width, &description_max_width);
  EXPECT_EQ(contents_width, contents_max_width);
  EXPECT_EQ(available_width, description_max_width);

  // Half and half.
  contents_width = 395;
  description_width = 395;
  available_width = 700;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width,
      false, true, &contents_max_width, &description_max_width);
  EXPECT_EQ(345, contents_max_width);
  EXPECT_EQ(345, description_max_width);

  // When we disallow shrinking the contents, it should get as much space as
  // it wants.
  contents_width = 395;
  description_width = 395;
  available_width = 700;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width,
      false, false, &contents_max_width, &description_max_width);
  EXPECT_EQ(contents_width, contents_max_width);
  EXPECT_EQ((available_width - contents_width - separator_width),
            description_max_width);

  // (available_width - separator_width) is odd, so contents gets the extra
  // pixel.
  contents_width = 395;
  description_width = 395;
  available_width = 699;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width,
      false, true, &contents_max_width, &description_max_width);
  EXPECT_EQ(345, contents_max_width);
  EXPECT_EQ(344, description_max_width);

  // Not enough space to draw anything.
  contents_width = 1;
  description_width = 1;
  available_width = 0;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      contents_width, separator_width, description_width, available_width,
      false, true, &contents_max_width, &description_max_width);
  EXPECT_EQ(0, contents_max_width);
  EXPECT_EQ(0, description_max_width);
}
