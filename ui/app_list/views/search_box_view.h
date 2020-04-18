// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_SEARCH_BOX_VIEW_H_
#define UI_APP_LIST_VIEWS_SEARCH_BOX_VIEW_H_

#include <vector>

#include "ash/app_list/model/search/search_box_model_observer.h"
#include "ash/public/cpp/app_list/app_list_types.h"
#include "ui/app_list/app_list_export.h"
#include "ui/app_list/app_list_view_delegate.h"
#include "ui/chromeos/search_box/search_box_view_base.h"

namespace views {
class Textfield;
class View;
}  // namespace views

namespace app_list {

class AppListView;
class AppListViewDelegate;
class SearchModel;

// Subclass of search_box::SearchBoxViewBase. SearchBoxModel is its data model
// that controls what icon to display, what placeholder text to use for
// Textfield. The text and selection model part could be set to change the
// contents and selection model of the Textfield.
class APP_LIST_EXPORT SearchBoxView : public search_box::SearchBoxViewBase,
                                      public SearchBoxModelObserver {
 public:
  SearchBoxView(search_box::SearchBoxViewDelegate* delegate,
                AppListViewDelegate* view_delegate,
                AppListView* app_list_view = nullptr);
  ~SearchBoxView() override;

  // Overridden from search_box::SearchBoxViewBase:
  void ClearSearch() override;
  views::View* GetSelectedViewInContentsView() override;
  void HandleSearchBoxEvent(ui::LocatedEvent* located_event) override;
  void ModelChanged() override;
  void UpdateKeyboardVisibility() override;
  void UpdateModel(bool initiated_by_user) override;
  void UpdateSearchIcon() override;
  void UpdateSearchBoxBorder() override;
  void SetupCloseButton() override;
  void SetupBackButton() override;

  // Overridden from views::View:
  void OnKeyEvent(ui::KeyEvent* evetn) override;

  // Updates the search box's background corner radius and color based on the
  // state of AppListModel.
  void UpdateBackground(double progress,
                        ash::AppListState current_state,
                        ash::AppListState target_state);

  // Updates the search box's layout based on the state of AppListModel.
  void UpdateLayout(double progress,
                    ash::AppListState current_state,
                    ash::AppListState target_state);

  // Returns background border corner radius in the given state.
  int GetSearchBoxBorderCornerRadiusForState(ash::AppListState state) const;

  // Returns background color for the given state.
  SkColor GetBackgroundColorForState(ash::AppListState state) const;

  // Updates the opacity of the searchbox.
  void UpdateOpacity();

  // Called when the wallpaper colors change.
  void OnWallpaperColorsChanged();

 private:
  // Gets the wallpaper prominent colors.
  void GetWallpaperProminentColors(
      AppListViewDelegate::GetWallpaperProminentColorsCallback callback);

  // Callback invoked when the wallpaper prominent colors are returned after
  // calling |AppListViewDelegate::GetWallpaperProminentColors|.
  void OnWallpaperProminentColorsReceived(
      const std::vector<SkColor>& prominent_colors);

  // Overridden from views::TextfieldController:
  void ContentsChanged(views::Textfield* sender,
                       const base::string16& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;
  bool HandleMouseEvent(views::Textfield* sender,
                        const ui::MouseEvent& mouse_event) override;

  // Overridden from SearchBoxModelObserver:
  void HintTextChanged() override;
  void SelectionModelChanged() override;
  void Update() override;
  void SearchEngineChanged() override;

  AppListViewDelegate* view_delegate_;  // Not owned.
  SearchModel* search_model_ = nullptr;  // Owned by the profile-keyed service.

  // Owned by views hierarchy.
  app_list::AppListView* app_list_view_;

  base::WeakPtrFactory<SearchBoxView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SearchBoxView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_SEARCH_BOX_VIEW_H_
