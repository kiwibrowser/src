// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_SEARCH_RESULT_TILE_ITEM_LIST_VIEW_H_
#define UI_APP_LIST_VIEWS_SEARCH_RESULT_TILE_ITEM_LIST_VIEW_H_

#include <vector>

#include "base/macros.h"
#include "ui/app_list/views/search_result_container_view.h"

namespace views {
class Textfield;
class Separator;
}

namespace app_list {

class AppListViewDelegate;
class SearchResultPageView;
class SearchResultTileItemView;

// Displays a list of SearchResultTileItemView.
class APP_LIST_EXPORT SearchResultTileItemListView
    : public SearchResultContainerView {
 public:
  SearchResultTileItemListView(SearchResultPageView* search_result_page_view,
                               views::Textfield* search_box,
                               AppListViewDelegate* view_delegate);
  ~SearchResultTileItemListView() override;

  // Overridden from SearchResultContainerView:
  void OnContainerSelected(bool from_bottom,
                           bool directional_movement) override;
  void NotifyFirstResultYIndex(int y_index) override;
  int GetYSize() override;
  views::View* GetSelectedView() override;
  SearchResultBaseView* GetFirstResultView() override;

  // Overridden from views::View:
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  const char* GetClassName() const override;

  const std::vector<SearchResultTileItemView*>& tile_views_for_test() const {
    return tile_views_;
  }

 private:
  // Overridden from SearchResultContainerView:
  int DoUpdate() override;
  void UpdateSelectedIndex(int old_selected, int new_selected) override;

  std::vector<SearchResultTileItemView*> tile_views_;

  std::vector<views::Separator*> separator_views_;

  // Owned by the views hierarchy.
  SearchResultPageView* const search_result_page_view_;
  views::Textfield* search_box_;

  const bool is_play_store_app_search_enabled_;

  DISALLOW_COPY_AND_ASSIGN(SearchResultTileItemListView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_SEARCH_RESULT_TILE_ITEM_LIST_VIEW_H_
