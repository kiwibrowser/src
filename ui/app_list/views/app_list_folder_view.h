// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_APP_LIST_FOLDER_VIEW_H_
#define UI_APP_LIST_VIEWS_APP_LIST_FOLDER_VIEW_H_

#include <string>

#include "ash/app_list/model/app_list_item_list_observer.h"
#include "base/macros.h"
#include "ui/app_list/views/apps_grid_view.h"
#include "ui/app_list/views/apps_grid_view_folder_delegate.h"
#include "ui/app_list/views/folder_header_view.h"
#include "ui/app_list/views/folder_header_view_delegate.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"
#include "ui/views/view_model.h"

namespace gfx {
class SlideAnimation;
}  // namespace gfx

namespace app_list {

class AppsContainerView;
class AppsGridView;
class AppListFolderItem;
class AppListItemView;
class AppListModel;
class FolderHeaderView;
class PageSwitcher;

class APP_LIST_EXPORT AppListFolderView : public views::View,
                                          public FolderHeaderViewDelegate,
                                          public AppListModelObserver,
                                          public AppsGridViewFolderDelegate {
 public:
  AppListFolderView(AppsContainerView* container_view,
                    AppListModel* model,
                    ContentsView* contents_view);
  ~AppListFolderView() override;

  // An interface for the folder opening and closing animations.
  class Animation {
   public:
    virtual ~Animation() {}
    virtual void ScheduleAnimation() = 0;
    virtual bool IsAnimationRunning() = 0;
  };

  void SetAppListFolderItem(AppListFolderItem* folder);

  // Schedules an animation to show or hide the view.
  // If |show| is false, the view should be set to invisible after the
  // animation is done unless |hide_for_reparent| is true.
  void ScheduleShowHideAnimation(bool show, bool hide_for_reparent);

  // Hides the view immediately without animation.
  void HideViewImmediately();

  // Closes the folder page and goes back the top level page.
  void CloseFolderPage();

  // views::View
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;

  // AppListModelObserver
  void OnAppListItemWillBeDeleted(AppListItem* item) override;

  // Updates preferred bounds of this view based on the activated folder item
  // icon's bounds.
  void UpdatePreferredBounds();

  // Returns true if this view's child views are in animation for opening or
  // closing the folder.
  bool IsAnimationRunning() const;

  AppsGridView* items_grid_view() { return items_grid_view_; }

  FolderHeaderView* folder_header_view() { return folder_header_view_; }

  views::View* background_view() { return background_view_; }

  views::View* contents_container() { return contents_container_; }

  const AppListFolderItem* folder_item() const { return folder_item_; }

  const gfx::Rect& folder_item_icon_bounds() const {
    return folder_item_icon_bounds_;
  }

  const gfx::Rect& preferred_bounds() const { return preferred_bounds_; }

  AppListItemView* GetActivatedFolderItemView();

  // Records the smoothness of folder show/hide animations mixed with the
  // BackgroundAnimation, FolderItemTitleAnimation, TopIconAnimation, and
  // ContentsContainerAnimation.
  void RecordAnimationSmoothness();

 private:
  void CalculateIdealBounds();

  // Starts setting up drag in root level apps grid view for re-parenting a
  // folder item.
  // |drag_point_in_root_grid| is in the cooridnates of root level AppsGridView.
  void StartSetupDragInRootLevelAppsGridView(
      AppListItemView* original_drag_view,
      const gfx::Point& drag_point_in_root_grid,
      bool has_native_drag);

  // Overridden from views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // Overridden from FolderHeaderViewDelegate:
  void NavigateBack(AppListFolderItem* item,
                    const ui::Event& event_flags) override;
  void GiveBackFocusToSearchBox() override;
  void SetItemName(AppListFolderItem* item, const std::string& name) override;

  // Overridden from AppsGridViewFolderDelegate:
  void ReparentItem(AppListItemView* original_drag_view,
                    const gfx::Point& drag_point_in_folder_grid,
                    bool has_native_drag) override;
  void DispatchDragEventForReparent(
      AppsGridView::Pointer pointer,
      const gfx::Point& drag_point_in_folder_grid) override;
  void DispatchEndDragEventForReparent(bool events_forwarded_to_drag_drop_host,
                                       bool cancel_drag) override;
  bool IsPointOutsideOfFolderBoundary(const gfx::Point& point) override;
  bool IsOEMFolder() const override;
  void SetRootLevelDragViewVisible(bool visible) override;

  // Returns the compositor associated to the widget containing this view.
  // Returns nullptr if there isn't one associated with this widget.
  ui::Compositor* GetCompositor();

  // Views below are not owned by views hierarchy.
  AppsContainerView* container_view_;
  ContentsView* contents_view_;

  // The view is used to draw a background with corner radius.
  views::View* background_view_;  // Owned by views hierarchy.

  // The view is used as a container for all following views.
  views::View* contents_container_;  // Owned by views hierarchy.

  FolderHeaderView* folder_header_view_;  // Owned by views hierarchy.
  AppsGridView* items_grid_view_;         // Owned by views hierarchy.
  PageSwitcher* page_switcher_;           // Owned by views hierarchy.

  std::unique_ptr<views::ViewModel> view_model_;

  AppListModel* model_;             // Not owned.
  AppListFolderItem* folder_item_;  // Not owned.

  // The bounds of the activated folder item icon relative to this view.
  gfx::Rect folder_item_icon_bounds_;

  // The preferred bounds of this view relative to AppsContainerView.
  gfx::Rect preferred_bounds_;

  bool hide_for_reparent_;

  base::string16 accessible_name_;

  std::unique_ptr<gfx::SlideAnimation> background_animation_;
  std::unique_ptr<gfx::SlideAnimation> folder_item_title_animation_;
  std::unique_ptr<Animation> top_icon_animation_;
  std::unique_ptr<Animation> contents_container_animation_;

  // The compositor frame number when animation starts.
  int animation_start_frame_number_;

  DISALLOW_COPY_AND_ASSIGN(AppListFolderView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_APP_LIST_FOLDER_VIEW_H_
