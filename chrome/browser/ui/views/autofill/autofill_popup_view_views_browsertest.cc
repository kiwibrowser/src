// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/autofill_popup_view_views.h"

#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "chrome/browser/ui/autofill/autofill_popup_layout_model.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/autofill/core/browser/suggestion.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/widget/widget.h"

using ::testing::NiceMock;
using ::testing::Return;

namespace autofill {
namespace {

constexpr int kNumInitialSuggestions = 3;

class MockAutofillPopupController : public AutofillPopupController {
 public:
  MockAutofillPopupController() {
    gfx::FontList::SetDefaultFontDescription("Arial, Times New Roman, 15px");
    layout_model_.reset(
        new AutofillPopupLayoutModel(this, false /* is_credit_card_field */));
  }

  // AutofillPopupViewDelegate
  MOCK_METHOD0(Hide, void());
  MOCK_METHOD0(ViewDestroyed, void());
  MOCK_METHOD1(SetSelectionAtPoint, void(const gfx::Point& point));
  MOCK_METHOD0(AcceptSelectedLine, bool());
  MOCK_METHOD0(SelectionCleared, void());
  MOCK_CONST_METHOD0(HasSelection, bool());
  MOCK_CONST_METHOD0(popup_bounds, gfx::Rect());
  MOCK_METHOD0(container_view, gfx::NativeView());
  MOCK_CONST_METHOD0(element_bounds, const gfx::RectF&());
  MOCK_CONST_METHOD0(IsRTL, bool());
  const std::vector<autofill::Suggestion> GetSuggestions() override {
    std::vector<Suggestion> suggestions(GetLineCount(),
                                        Suggestion("", "", "", 0));
    return suggestions;
  }
#if !defined(OS_ANDROID)
  MOCK_METHOD1(SetTypesetter, void(gfx::Typesetter typesetter));
  MOCK_METHOD1(GetElidedValueWidthForRow, int(int row));
  MOCK_METHOD1(GetElidedLabelWidthForRow, int(int row));
#endif

  // AutofillPopupController
  MOCK_METHOD0(OnSuggestionsChanged, void());
  MOCK_METHOD1(AcceptSuggestion, void(int index));
  MOCK_CONST_METHOD0(GetLineCount, int());
  const autofill::Suggestion& GetSuggestionAt(int row) const override {
    return suggestion_;
  }
  MOCK_CONST_METHOD1(GetElidedValueAt, const base::string16&(int row));
  MOCK_CONST_METHOD1(GetElidedLabelAt, const base::string16&(int row));
  MOCK_METHOD3(GetRemovalConfirmationText,
               bool(int index, base::string16* title, base::string16* body));
  MOCK_METHOD1(RemoveSuggestion, bool(int index));
  MOCK_CONST_METHOD1(GetBackgroundColorIDForRow,
                     ui::NativeTheme::ColorId(int index));
  MOCK_METHOD1(SetSelectedLine, void(base::Optional<int> selected_line));
  MOCK_CONST_METHOD0(selected_line, base::Optional<int>());
  const AutofillPopupLayoutModel& layout_model() const override {
    return *layout_model_;
  }

 private:
  std::unique_ptr<AutofillPopupLayoutModel> layout_model_;
  autofill::Suggestion suggestion_;
};

class TestAutofillPopupViewViews : public AutofillPopupViewViews {
 public:
  TestAutofillPopupViewViews(AutofillPopupController* controller,
                             views::Widget* parent_widget)
      : AutofillPopupViewViews(controller, parent_widget) {}
  ~TestAutofillPopupViewViews() override {}

  void DoUpdateBoundsAndRedrawPopup() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestAutofillPopupViewViews);
};

}  // namespace

class AutofillPopupViewViewsTest : public InProcessBrowserTest {
 public:
  AutofillPopupViewViewsTest() {}
  ~AutofillPopupViewViewsTest() override {}

  void SetUpOnMainThread() override {
    gfx::NativeView native_view =
        browser()->tab_strip_model()->GetActiveWebContents()->GetNativeView();
    EXPECT_CALL(autofill_popup_controller_, container_view())
        .WillRepeatedly(Return(native_view));
    EXPECT_CALL(autofill_popup_controller_, GetLineCount())
        .WillRepeatedly(Return(kNumInitialSuggestions));
    autofill_popup_view_views_ = new TestAutofillPopupViewViews(
        &autofill_popup_controller_,
        views::Widget::GetWidgetForNativeWindow(
            browser()->window()->GetNativeWindow()));
  }

 protected:
  NiceMock<MockAutofillPopupController> autofill_popup_controller_;
  // We intentionally do not destroy this view in the test because of
  // difficulty in mocking out 'RemoveObserver'.
  TestAutofillPopupViewViews* autofill_popup_view_views_;
};

IN_PROC_BROWSER_TEST_F(AutofillPopupViewViewsTest, OnSelectedRowChanged) {
  // No previous selection -> Selected 1st row.
  autofill_popup_view_views_->OnSelectedRowChanged(base::nullopt, 0);

  // Selected 1st row -> Selected 2nd row.
  autofill_popup_view_views_->OnSelectedRowChanged(0, 1);

  // Increase number of suggestions.
  EXPECT_CALL(autofill_popup_controller_, GetLineCount())
      .WillRepeatedly(Return(kNumInitialSuggestions + 1));

  autofill_popup_view_views_->OnSuggestionsChanged();

  // Selected 2nd row -> Selected last row.
  autofill_popup_view_views_->OnSelectedRowChanged(1, kNumInitialSuggestions);

  // Decrease number of suggestions.
  EXPECT_CALL(autofill_popup_controller_, GetLineCount())
      .WillRepeatedly(Return(kNumInitialSuggestions - 1));

  autofill_popup_view_views_->OnSuggestionsChanged();

  // No previous selection (because previously selected row is out of bounds
  // now)
  // -> Selected 1st row.
  autofill_popup_view_views_->OnSelectedRowChanged(base::nullopt, 0);

  // Selected 1st row -> No selection.
  autofill_popup_view_views_->OnSelectedRowChanged(0, base::nullopt);
}

}  // namespace autofill
