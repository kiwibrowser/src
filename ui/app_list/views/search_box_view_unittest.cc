// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/search_box_view.h"

#include <cctype>
#include <map>

#include "ash/public/cpp/app_list/app_list_features.h"
#include "ash/public/cpp/app_list/vector_icons/vector_icons.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/app_list/test/app_list_test_view_delegate.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/chromeos/search_box/search_box_constants.h"
#include "ui/chromeos/search_box/search_box_view_delegate.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/test/widget_test.h"

namespace app_list {
namespace test {

class KeyPressCounterView : public views::View {
 public:
  KeyPressCounterView() : count_(0) {}
  ~KeyPressCounterView() override {}

  int GetCountAndReset() {
    int count = count_;
    count_ = 0;
    return count;
  }

 private:
  // Overridden from views::View:
  bool OnKeyPressed(const ui::KeyEvent& key_event) override {
    if (!::isalnum(static_cast<int>(key_event.key_code()))) {
      ++count_;
      return true;
    }
    return false;
  }
  int count_;

  DISALLOW_COPY_AND_ASSIGN(KeyPressCounterView);
};

class SearchBoxViewTest : public views::test::WidgetTest,
                          public search_box::SearchBoxViewDelegate {
 public:
  SearchBoxViewTest() = default;
  ~SearchBoxViewTest() override = default;

  // Overridden from testing::Test:
  void SetUp() override {
    views::test::WidgetTest::SetUp();

    app_list_view_ = new AppListView(&view_delegate_);
    AppListView::InitParams params;
    params.parent = GetContext();
    app_list_view()->Initialize(params);

    widget_ = CreateTopLevelPlatformWidget();
    view_.reset(new SearchBoxView(this, &view_delegate_, app_list_view()));
    view_->Init();
    widget_->SetBounds(gfx::Rect(0, 0, 300, 200));
    counter_view_ = new KeyPressCounterView();
    widget_->GetContentsView()->AddChildView(view());
    widget_->GetContentsView()->AddChildView(counter_view_);
    view()->set_contents_view(counter_view_);
  }

  void TearDown() override {
    view_.reset();
    app_list_view_->GetWidget()->Close();
    widget_->CloseNow();
    views::test::WidgetTest::TearDown();
  }

 protected:
  views::Widget* widget() { return widget_; }
  SearchBoxView* view() { return view_.get(); }
  AppListView* app_list_view() { return app_list_view_; }

  void SetSearchEngineIsGoogle(bool is_google) {
    view_delegate_.SetSearchEngineIsGoogle(is_google);
  }

  void SetSearchBoxActive(bool active) { view()->SetSearchBoxActive(active); }

  int GetContentsViewKeyPressCountAndReset() {
    return counter_view_->GetCountAndReset();
  }

  void KeyPress(ui::KeyboardCode key_code) {
    ui::KeyEvent event(ui::ET_KEY_PRESSED, key_code, ui::EF_NONE);
    view()->search_box()->OnKeyEvent(&event);
    // Emulates the input method.
    if (::isalnum(static_cast<int>(key_code))) {
      base::char16 character = ::tolower(static_cast<int>(key_code));
      view()->search_box()->InsertText(base::string16(1, character));
    }
  }

  std::string GetLastQueryAndReset() {
    base::string16 query = last_query_;
    last_query_.clear();
    return base::UTF16ToUTF8(query);
  }

  int GetQueryChangedCountAndReset() {
    int result = query_changed_count_;
    query_changed_count_ = 0;
    return result;
  }

 private:
  // Overridden from SearchBoxViewDelegate:
  void QueryChanged(search_box::SearchBoxViewBase* sender) override {
    ++query_changed_count_;
    last_query_ = sender->search_box()->text();
  }

  void BackButtonPressed() override {}
  void ActiveChanged(search_box::SearchBoxViewBase* sender) override {}

  AppListTestViewDelegate view_delegate_;
  views::Widget* widget_;
  AppListView* app_list_view_ = nullptr;
  std::unique_ptr<SearchBoxView> view_;
  KeyPressCounterView* counter_view_;
  base::string16 last_query_;
  int query_changed_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(SearchBoxViewTest);
};

// Tests that the close button is invisible by default.
TEST_F(SearchBoxViewTest, CloseButtonInvisibleByDefault) {
  EXPECT_FALSE(view()->close_button()->visible());
}

// Tests that the close button becomes visible after typing in the search box.
TEST_F(SearchBoxViewTest, CloseButtonVisibleAfterTyping) {
  KeyPress(ui::VKEY_A);
  EXPECT_TRUE(view()->close_button()->visible());
}

// Tests that the close button is still invisible after the search box is
// activated.
TEST_F(SearchBoxViewTest, CloseButtonInvisibleAfterSearchBoxActived) {
  view()->SetSearchBoxActive(true);
  EXPECT_FALSE(view()->close_button()->visible());
}

// Tests that the close button becomes invisible after close button is clicked.
TEST_F(SearchBoxViewTest, CloseButtonInvisibleAfterCloseButtonClicked) {
  KeyPress(ui::VKEY_A);
  view()->ButtonPressed(
      view()->close_button(),
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                     base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
                     ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_FALSE(view()->close_button()->visible());
}

// Tests that the search box becomes empty after close button is clicked.
TEST_F(SearchBoxViewTest, SearchBoxEmptyAfterCloseButtonClicked) {
  KeyPress(ui::VKEY_A);
  view()->ButtonPressed(
      view()->close_button(),
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                     base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
                     ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_TRUE(view()->search_box()->text().empty());
}

// Tests that the search box is still active after close button is clicked.
TEST_F(SearchBoxViewTest, SearchBoxActiveAfterCloseButtonClicked) {
  KeyPress(ui::VKEY_A);
  view()->ButtonPressed(
      view()->close_button(),
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                     base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
                     ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_TRUE(view()->is_search_box_active());
}

// Tests that the search box is inactive by default.
TEST_F(SearchBoxViewTest, SearchBoxInactiveByDefault) {
  ASSERT_FALSE(view()->is_search_box_active());
}

// Tests that the black Google icon is used for an inactive Google search.
TEST_F(SearchBoxViewTest, SearchBoxInactiveSearchBoxGoogle) {
  SetSearchEngineIsGoogle(true);
  SetSearchBoxActive(false);
  const gfx::ImageSkia expected_icon =
      gfx::CreateVectorIcon(kIcGoogleBlackIcon, search_box::kSearchIconSize,
                            search_box::kDefaultSearchboxColor);
  view()->ModelChanged();

  const gfx::ImageSkia actual_icon =
      view()->get_search_icon_for_test()->GetImage();

  EXPECT_TRUE(gfx::test::AreBitmapsEqual(*expected_icon.bitmap(),
                                         *actual_icon.bitmap()));
}

// Tests that the colored Google icon is used for an active Google search.
TEST_F(SearchBoxViewTest, SearchBoxActiveSearchEngineGoogle) {
  SetSearchEngineIsGoogle(true);
  SetSearchBoxActive(true);
  const gfx::ImageSkia expected_icon =
      gfx::CreateVectorIcon(kIcGoogleColorIcon, search_box::kSearchIconSize,
                            search_box::kDefaultSearchboxColor);
  view()->ModelChanged();

  const gfx::ImageSkia actual_icon =
      view()->get_search_icon_for_test()->GetImage();

  EXPECT_TRUE(gfx::test::AreBitmapsEqual(*expected_icon.bitmap(),
                                         *actual_icon.bitmap()));
}

// Tests that the non-Google icon is used for an inactive non-Google search.
TEST_F(SearchBoxViewTest, SearchBoxInactiveSearchEngineNotGoogle) {
  SetSearchEngineIsGoogle(false);
  SetSearchBoxActive(false);
  const gfx::ImageSkia expected_icon = gfx::CreateVectorIcon(
      kIcSearchEngineNotGoogleIcon, search_box::kSearchIconSize,
      search_box::kDefaultSearchboxColor);
  view()->ModelChanged();

  const gfx::ImageSkia actual_icon =
      view()->get_search_icon_for_test()->GetImage();

  EXPECT_TRUE(gfx::test::AreBitmapsEqual(*expected_icon.bitmap(),
                                         *actual_icon.bitmap()));
}

// Tests that the non-Google icon is used for an active non-Google search.
TEST_F(SearchBoxViewTest, SearchBoxActiveSearchEngineNotGoogle) {
  SetSearchEngineIsGoogle(false);
  SetSearchBoxActive(true);
  const gfx::ImageSkia expected_icon = gfx::CreateVectorIcon(
      kIcSearchEngineNotGoogleIcon, search_box::kSearchIconSize,
      search_box::kDefaultSearchboxColor);
  view()->ModelChanged();

  const gfx::ImageSkia actual_icon =
      view()->get_search_icon_for_test()->GetImage();

  EXPECT_TRUE(gfx::test::AreBitmapsEqual(*expected_icon.bitmap(),
                                         *actual_icon.bitmap()));
}

}  // namespace test
}  // namespace app_list
