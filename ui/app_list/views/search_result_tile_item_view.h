// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_SEARCH_RESULT_TILE_ITEM_VIEW_H_
#define UI_APP_LIST_VIEWS_SEARCH_RESULT_TILE_ITEM_VIEW_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "ui/app_list/app_list_export.h"
#include "ui/app_list/views/app_list_view_context_menu.h"
#include "ui/app_list/views/search_result_base_view.h"
#include "ui/views/context_menu_controller.h"

namespace views {
class ImageView;
class Label;
}  // namespace views

namespace app_list {

class AppListViewDelegate;
class SearchResult;
class SearchResultContainerView;
class PaginationModel;

// A tile view that displays a search result. It hosts view for search result
// that has SearchResult::DisplayType DISPLAY_TILE or DISPLAY_RECOMMENDATION.
class APP_LIST_EXPORT SearchResultTileItemView
    : public SearchResultBaseView,
      public views::ContextMenuController,
      public AppListViewContextMenu::Delegate {
 public:
  SearchResultTileItemView(SearchResultContainerView* result_container,
                           AppListViewDelegate* view_delegate,
                           PaginationModel* pagination_model);
  ~SearchResultTileItemView() override;

  SearchResult* result() { return item_; }
  void SetSearchResult(SearchResult* item);

  // Informs the SearchResultTileItemView of its parent's background color. The
  // controls within the SearchResultTileItemView will adapt to suit the given
  // color.
  void SetParentBackgroundColor(SkColor color);

  // Records the context menu user journey time.
  void OnContextMenuClosed(const base::TimeTicks& open_time);

  // Overridden from views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // Overridden from views::Button:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;
  void StateChanged(ButtonState old_state) override;
  void PaintButtonContents(gfx::Canvas* canvas) override;

  // Overridden from SearchResultObserver:
  void OnMetadataChanged() override;
  void OnResultDestroying() override;

  // views::ContextMenuController overrides:
  void ShowContextMenuForView(views::View* source,
                              const gfx::Point& point,
                              ui::MenuSourceType source_type) override;

  // AppListViewContextMenu::Delegate overrides:
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  // Bound by ShowContextMenuForView().
  void OnGetContextMenuModel(views::View* source,
                             const gfx::Point& point,
                             ui::MenuSourceType source_type,
                             std::vector<ash::mojom::MenuItemPtr> menu);

  void SetIcon(const gfx::ImageSkia& icon);
  void SetBadgeIcon(const gfx::ImageSkia& badge_icon);
  void SetTitle(const base::string16& title);
  void SetRating(float rating);
  void SetPrice(const base::string16& price);

  // Whether the tile view is a suggested app.
  bool IsSuggestedAppTile() const;

  // Records an app being launched.
  void LogAppLaunch() const;

  void UpdateBackgroundColor();

  // Overridden from views::View:
  void Layout() override;
  const char* GetClassName() const override;
  gfx::Size CalculatePreferredSize() const override;
  bool GetTooltipText(const gfx::Point& p,
                      base::string16* tooltip) const override;

  SearchResultContainerView* const result_container_;  // Parent view
  AppListViewDelegate* const view_delegate_;           // Owned by AppListView.
  PaginationModel* const pagination_model_;            // Owned by AppsGridView.

  // Owned by the model provided by the AppListViewDelegate.
  SearchResult* item_ = nullptr;

  views::ImageView* icon_ = nullptr;         // Owned by views hierarchy.
  views::ImageView* badge_ = nullptr;        // Owned by views hierarchy.
  views::Label* title_ = nullptr;            // Owned by views hierarchy.
  views::Label* rating_ = nullptr;           // Owned by views hierarchy.
  views::Label* price_ = nullptr;            // Owned by views hierarchy.
  views::ImageView* rating_star_ = nullptr;  // Owned by views hierarchy.

  SkColor parent_background_color_ = SK_ColorTRANSPARENT;

  const bool is_play_store_app_search_enabled_;

  AppListViewContextMenu context_menu_;

  base::WeakPtrFactory<SearchResultTileItemView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SearchResultTileItemView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_SEARCH_RESULT_TILE_ITEM_VIEW_H_
