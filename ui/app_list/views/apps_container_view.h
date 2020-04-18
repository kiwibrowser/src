// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_APPS_CONTAINER_VIEW_H_
#define UI_APP_LIST_VIEWS_APPS_CONTAINER_VIEW_H_

#include <stddef.h>

#include <vector>

#include "ash/app_list/model/app_list_folder_item.h"
#include "base/macros.h"
#include "ui/app_list/views/horizontal_page.h"

namespace app_list {

class AppsGridView;
class ApplicationDragAndDropHost;
class AppListFolderItem;
class AppListFolderView;
class AppListModel;
class ContentsView;
class FolderBackgroundView;
class PageSwitcher;

// AppsContainerView contains a root level AppsGridView to render the root level
// app items, and a AppListFolderView to render the app items inside the
// active folder. Only one if them is visible to user at any time.
class APP_LIST_EXPORT AppsContainerView : public HorizontalPage {
 public:
  AppsContainerView(ContentsView* contents_view, AppListModel* model);
  ~AppsContainerView() override;

  // Shows the active folder content specified by |folder_item|.
  void ShowActiveFolder(AppListFolderItem* folder_item);

  // Shows the root level apps list. This is called when UI navigate back from
  // a folder view with |folder_item|. If |folder_item| is NULL skips animation.
  void ShowApps(AppListFolderItem* folder_item);

  // Resets the app list to a state where it shows the main grid view. This is
  // called when the user opens the launcher for the first time or when the user
  // hides and then shows it. This is necessary because we only hide and show
  // the launcher on Windows and Linux so we need to reset to a fresh state.
  void ResetForShowApps();

  // Sets |drag_and_drop_host_| for the current app list in both
  // app_list_folder_view_ and root level apps_grid_view_.
  void SetDragAndDropHostOfCurrentAppList(
      ApplicationDragAndDropHost* drag_and_drop_host);

  // Transits the UI from folder view to root lelve apps grid view when
  // re-parenting a child item of |folder_item|.
  void ReparentFolderItemTransit(AppListFolderItem* folder_item);

  // Returns true if it is currently showing an active folder page.
  bool IsInFolderView() const;

  // Called to notify the AppsContainerView that a reparent drag has completed.
  void ReparentDragEnded();

  // Updates the visibility of the items in this view according to
  // |app_list_state| and |is_in_drag|.
  void UpdateControlVisibility(AppListViewState app_list_state,
                               bool is_in_drag);

  // Updates the opacity of the items in this view during dragging.
  void UpdateOpacity();

  // views::View overrides:
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  const char* GetClassName() const override;

  // HorizontalPage overrides:
  void OnWillBeHidden() override;
  views::View* GetFirstFocusableView() override;
  gfx::Rect GetPageBoundsForState(ash::AppListState state) const override;

  // Returns the expected search box bounds in the screen when the given state
  // is active.
  gfx::Rect GetSearchBoxBoundsForState(ash::AppListState state) const;

  AppsGridView* apps_grid_view() { return apps_grid_view_; }
  FolderBackgroundView* folder_background_view() {
    return folder_background_view_;
  }
  AppListFolderView* app_list_folder_view() { return app_list_folder_view_; }

 private:
  enum ShowState {
    SHOW_NONE,  // initial state
    SHOW_APPS,
    SHOW_ACTIVE_FOLDER,
    SHOW_ITEM_REPARENT,
  };

  void SetShowState(ShowState show_state, bool show_apps_with_animation);

  // Gets the final top padding of search box.
  int GetSearchBoxFinalTopPadding() const;

  // Gets the top padding of search box during dragging.
  int GetSearchBoxTopPaddingDuringDragging() const;

  // Returns the bounds of the page in the parent view during dragging.
  gfx::Rect GetPageBoundsDuringDragging(ash::AppListState state) const;

  ContentsView* contents_view_;  // Not owned.

  // The views below are owned by views hierarchy.
  AppsGridView* apps_grid_view_ = nullptr;
  AppListFolderView* app_list_folder_view_ = nullptr;
  PageSwitcher* page_switcher_ = nullptr;
  FolderBackgroundView* folder_background_view_ = nullptr;

  ShowState show_state_ = SHOW_NONE;

  DISALLOW_COPY_AND_ASSIGN(AppsContainerView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_APPS_CONTAINER_VIEW_H_
