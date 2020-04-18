// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_SEARCH_RESULT_LIST_VIEW_H_
#define UI_APP_LIST_VIEWS_SEARCH_RESULT_LIST_VIEW_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/app_list/views/search_result_container_view.h"
#include "ui/views/view.h"

namespace app_list {
namespace test {
class SearchResultListViewTest;
}

class AppListMainView;
class AppListViewDelegate;
class SearchResultView;

// SearchResultListView displays SearchResultList with a list of
// SearchResultView.
class APP_LIST_EXPORT SearchResultListView : public SearchResultContainerView {
 public:
  SearchResultListView(AppListMainView* main_view,
                       AppListViewDelegate* view_delegate);
  ~SearchResultListView() override;

  bool IsResultViewSelected(const SearchResultView* result_view) const;

  // Helper function to get SearchResultView at given |index|.
  SearchResultView* GetResultViewAt(size_t index);

  void SearchResultActivated(SearchResultView* view, int event_flags);

  void SearchResultActionActivated(SearchResultView* view,
                                   size_t action_index,
                                   int event_flags);

  void OnSearchResultInstalled(SearchResultView* view);

  // Overridden from views::View:
  gfx::Size CalculatePreferredSize() const override;
  const char* GetClassName() const override;

  // Overridden from ui::ListModelObserver:
  void ListItemsRemoved(size_t start, size_t count) override;

  // Overridden from SearchResultContainerView:
  void OnContainerSelected(bool from_bottom,
                           bool directional_movement) override;
  void NotifyFirstResultYIndex(int y_index) override;
  int GetYSize() override;
  views::View* GetSelectedView() override;
  SearchResultBaseView* GetFirstResultView() override;

 private:
  friend class test::SearchResultListViewTest;

  // Overridden from SearchResultContainerView:
  int DoUpdate() override;
  void UpdateSelectedIndex(int old_selected, int new_selected) override;

  // Overridden from views::View:
  void Layout() override;
  int GetHeightForWidth(int w) const override;

  AppListMainView* main_view_;          // Owned by views hierarchy.
  AppListViewDelegate* view_delegate_;  // Not owned.

  views::View* results_container_;

  std::vector<SearchResultView*> search_result_views_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(SearchResultListView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_SEARCH_RESULT_LIST_VIEW_H_
