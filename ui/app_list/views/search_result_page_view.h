// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_SEARCH_RESULT_PAGE_VIEW_H_
#define UI_APP_LIST_VIEWS_SEARCH_RESULT_PAGE_VIEW_H_

#include <vector>

#include "ash/app_list/model/app_list_model.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/app_list/app_list_export.h"
#include "ui/app_list/views/app_list_page.h"
#include "ui/app_list/views/search_result_container_view.h"

namespace app_list {

class SearchResultBaseView;

// The search results page for the app list.
class APP_LIST_EXPORT SearchResultPageView
    : public AppListPage,
      public SearchResultContainerView::Delegate {
 public:
  SearchResultPageView();
  ~SearchResultPageView() override;

  void AddSearchResultContainerView(
      SearchModel::SearchResults* result_model,
      SearchResultContainerView* result_container);

  const std::vector<SearchResultContainerView*>& result_container_views() {
    return result_container_views_;
  }

  // Overridden from views::View:
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  const char* GetClassName() const override;
  gfx::Size CalculatePreferredSize() const override;

  // AppListPage overrides:
  gfx::Rect GetPageBoundsForState(ash::AppListState state) const override;
  void OnAnimationUpdated(double progress,
                          ash::AppListState from_state,
                          ash::AppListState to_state) override;
  gfx::Rect GetSearchBoxBounds() const override;
  views::View* GetFirstFocusableView() override;
  views::View* GetLastFocusableView() override;

  // Overridden from SearchResultContainerView::Delegate :
  void OnSearchResultContainerResultsChanged() override;

  views::View* contents_view() { return contents_view_; }

  SearchResultBaseView* first_result_view() const { return first_result_view_; }

 private:
  // Separator between SearchResultContainerView.
  class HorizontalSeparator;

  // Sort the result container views.
  void ReorderSearchResultContainers();

  // The SearchResultContainerViews that compose the search page. All owned by
  // the views hierarchy.
  std::vector<SearchResultContainerView*> result_container_views_;

  std::vector<HorizontalSeparator*> separators_;

  // View containing SearchCardView instances. Owned by view hierarchy.
  views::View* const contents_view_;

  // The first search result's view or nullptr if there's no search result.
  SearchResultBaseView* first_result_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SearchResultPageView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_SEARCH_RESULT_PAGE_VIEW_H_
