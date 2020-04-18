// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/search_result_list_view.h"

#include <stddef.h>

#include <map>
#include <memory>
#include <utility>

#include "ash/app_list/model/search/search_model.h"
#include "ash/public/cpp/app_list/app_list_features.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/app_list/test/app_list_test_view_delegate.h"
#include "ui/app_list/test/test_search_result.h"
#include "ui/app_list/views/search_result_view.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/test/views_test_base.h"

namespace app_list {
namespace test {

namespace {
int kDefaultSearchItems = 5;
}  // namespace

class SearchResultListViewTest : public views::ViewsTestBase {
 public:
  SearchResultListViewTest() = default;
  ~SearchResultListViewTest() override = default;

  // Overridden from testing::Test:
  void SetUp() override {
    views::ViewsTestBase::SetUp();
    view_.reset(new SearchResultListView(nullptr, &view_delegate_));
    view_->SetResults(view_delegate_.GetSearchModel()->results());
  }

 protected:
  SearchResultListView* view() const { return view_.get(); }

  SearchResultView* GetResultViewAt(int index) const {
    return view_->GetResultViewAt(index);
  }

  SearchModel::SearchResults* GetResults() {
    return view_delegate_.GetSearchModel()->results();
  }

  void SetUpSearchResults() {
    SearchModel::SearchResults* results = GetResults();
    for (int i = 0; i < kDefaultSearchItems; ++i) {
      std::unique_ptr<TestSearchResult> result =
          std::make_unique<TestSearchResult>();
      result->set_display_type(ash::SearchResultDisplayType::kList);
      result->set_title(base::UTF8ToUTF16(base::StringPrintf("Result %d", i)));
      if (i < 2)
        result->set_details(base::ASCIIToUTF16("Detail"));
      results->Add(std::move(result));
    }

    // Adding results will schedule Update().
    RunPendingMessages();
    view_->OnContainerSelected(false, false);
  }

  int GetOpenResultCountAndReset(int ranking) {
    EXPECT_GT(view_delegate_.open_search_result_counts().count(ranking), 0u);
    int result = view_delegate_.open_search_result_counts()[ranking];
    view_delegate_.open_search_result_counts().clear();
    return result;
  }

  int GetResultCount() const { return view_->num_results(); }

  int GetSelectedIndex() const { return view_->selected_index(); }

  void ResetSelectedIndex() { view_->SetSelectedIndex(0); }

  void AddTestResultAtIndex(int index) {
    GetResults()->Add(std::make_unique<TestSearchResult>());
  }

  void DeleteResultAt(int index) { GetResults()->DeleteAt(index); }

  bool KeyPress(ui::KeyboardCode key_code) {
    ui::KeyEvent event(ui::ET_KEY_PRESSED, key_code, ui::EF_NONE);
    return view_->OnKeyPressed(event);
  }

  void ExpectConsistent() {
    // Adding results will schedule Update().
    RunPendingMessages();

    SearchModel::SearchResults* results = GetResults();
    for (size_t i = 0; i < results->item_count(); ++i) {
      EXPECT_EQ(results->GetItemAt(i), GetResultViewAt(i)->result());
    }
  }

  views::ProgressBar* GetProgressBarAt(size_t index) const {
    return GetResultViewAt(index)->progress_bar_;
  }

 private:
  AppListTestViewDelegate view_delegate_;
  std::unique_ptr<SearchResultListView> view_;

  DISALLOW_COPY_AND_ASSIGN(SearchResultListViewTest);
};

TEST_F(SearchResultListViewTest, SpokenFeedback) {
  SetUpSearchResults();

  // Result 0 has a detail text. Expect that the detail is appended to the
  // accessibility name.
  EXPECT_EQ(base::ASCIIToUTF16("Result 0, Detail"),
            GetResultViewAt(0)->ComputeAccessibleName());

  // Result 2 has no detail text.
  EXPECT_EQ(base::ASCIIToUTF16("Result 2"),
            GetResultViewAt(2)->ComputeAccessibleName());
}

TEST_F(SearchResultListViewTest, ModelObservers) {
  SetUpSearchResults();
  ExpectConsistent();

  // Remove from end.
  DeleteResultAt(kDefaultSearchItems - 1);
  ExpectConsistent();

  // Insert at start.
  AddTestResultAtIndex(0);
  ExpectConsistent();

  // Remove from end.
  DeleteResultAt(kDefaultSearchItems - 1);
  ExpectConsistent();

  // Insert at end.
  AddTestResultAtIndex(kDefaultSearchItems);
  ExpectConsistent();

  // Delete from start.
  DeleteResultAt(0);
  ExpectConsistent();
}

// Regression test for http://crbug.com/402859 to ensure ProgressBar is
// initialized properly in SearchResultListView::SetResult().
TEST_F(SearchResultListViewTest, ProgressBar) {
  SetUpSearchResults();

  GetResults()->GetItemAt(0)->SetIsInstalling(true);
  EXPECT_EQ(0.0f, GetProgressBarAt(0)->current_value());
  GetResults()->GetItemAt(0)->SetPercentDownloaded(10);

  DeleteResultAt(0);
  RunPendingMessages();
  EXPECT_EQ(0.0f, GetProgressBarAt(0)->current_value());
}

}  // namespace test
}  // namespace app_list
