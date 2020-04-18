// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/apps_container_view.h"

#include <algorithm>
#include <vector>

#include "ash/public/cpp/app_list/app_list_features.h"
#include "ash/public/cpp/app_list/app_list_switches.h"
#include "base/command_line.h"
#include "ui/app_list/views/app_list_folder_view.h"
#include "ui/app_list/views/app_list_item_view.h"
#include "ui/app_list/views/app_list_main_view.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/app_list/views/apps_grid_view.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/app_list/views/folder_background_view.h"
#include "ui/app_list/views/horizontal_page_container.h"
#include "ui/app_list/views/page_switcher.h"
#include "ui/app_list/views/search_box_view.h"
#include "ui/app_list/views/suggestions_container_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/chromeos/search_box/search_box_constants.h"
#include "ui/events/event.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/controls/textfield/textfield.h"

namespace app_list {

// Initial search box top padding in shelf mode.
constexpr int kSearchBoxInitalTopPadding = 12;

// Top padding of search box in peeking state.
constexpr int kSearchBoxPeekingTopPadding = 24;

// Minimum top padding of search box in fullscreen state.
constexpr int kSearchBoxMinimumTopPadding = 24;

AppsContainerView::AppsContainerView(ContentsView* contents_view,
                                     AppListModel* model)
    : contents_view_(contents_view) {
  apps_grid_view_ = new AppsGridView(contents_view_, nullptr);
  apps_grid_view_->SetLayout(kPreferredCols, kPreferredRows);
  AddChildView(apps_grid_view_);

  // Page switcher should be initialized after AppsGridView.
  page_switcher_ = new PageSwitcher(apps_grid_view_->pagination_model(),
                                    true /* vertical */);
  AddChildView(page_switcher_);

  app_list_folder_view_ = new AppListFolderView(this, model, contents_view_);
  // The folder view is initially hidden.
  app_list_folder_view_->SetVisible(false);
  folder_background_view_ = new FolderBackgroundView(app_list_folder_view_);
  AddChildView(folder_background_view_);
  AddChildView(app_list_folder_view_);

  apps_grid_view_->SetModel(model);
  apps_grid_view_->SetItemList(model->top_level_item_list());
  SetShowState(SHOW_APPS, false);
}

AppsContainerView::~AppsContainerView() {
  // Make sure |page_switcher_| is deleted before |apps_grid_view_| because
  // |page_switcher_| uses the PaginationModel owned by |apps_grid_view_|.
  delete page_switcher_;
}

void AppsContainerView::ShowActiveFolder(AppListFolderItem* folder_item) {
  // Prevent new animations from starting if there are currently animations
  // pending. This fixes crbug.com/357099.
  if (app_list_folder_view_->IsAnimationRunning())
    return;

  app_list_folder_view_->SetAppListFolderItem(folder_item);

  SetShowState(SHOW_ACTIVE_FOLDER, false);

  // Disable all the items behind the folder so that they will not be reached
  // during focus traversal.
  contents_view_->GetSearchBoxView()->search_box()->RequestFocus();
  apps_grid_view_->DisableFocusForShowingActiveFolder(true);
}

void AppsContainerView::ShowApps(AppListFolderItem* folder_item) {
  if (app_list_folder_view_->IsAnimationRunning())
    return;

  SetShowState(SHOW_APPS, folder_item ? true : false);
  apps_grid_view_->DisableFocusForShowingActiveFolder(false);
}

void AppsContainerView::ResetForShowApps() {
  SetShowState(SHOW_APPS, false);
  apps_grid_view_->DisableFocusForShowingActiveFolder(false);
}

void AppsContainerView::SetDragAndDropHostOfCurrentAppList(
    ApplicationDragAndDropHost* drag_and_drop_host) {
  apps_grid_view()->SetDragAndDropHostOfCurrentAppList(drag_and_drop_host);
  app_list_folder_view()->items_grid_view()->SetDragAndDropHostOfCurrentAppList(
      drag_and_drop_host);
}

void AppsContainerView::ReparentFolderItemTransit(
    AppListFolderItem* folder_item) {
  if (app_list_folder_view_->IsAnimationRunning())
    return;
  SetShowState(SHOW_ITEM_REPARENT, false);
  apps_grid_view_->DisableFocusForShowingActiveFolder(false);
}

bool AppsContainerView::IsInFolderView() const {
  return show_state_ == SHOW_ACTIVE_FOLDER;
}

void AppsContainerView::ReparentDragEnded() {
  DCHECK_EQ(SHOW_ITEM_REPARENT, show_state_);
  show_state_ = AppsContainerView::SHOW_APPS;
}

void AppsContainerView::UpdateControlVisibility(AppListViewState app_list_state,
                                                bool is_in_drag) {
  apps_grid_view_->UpdateControlVisibility(app_list_state, is_in_drag);
  page_switcher_->SetVisible(
      app_list_state == AppListViewState::FULLSCREEN_ALL_APPS || is_in_drag);
}

void AppsContainerView::UpdateOpacity() {
  apps_grid_view_->UpdateOpacity();

  // Updates the opacity of page switcher buttons. The same rule as all apps in
  // AppsGridView.
  AppListView* app_list_view = contents_view_->app_list_view();
  bool should_restore_opacity =
      !app_list_view->is_in_drag() &&
      (app_list_view->app_list_state() != AppListViewState::CLOSED);
  int screen_bottom = app_list_view->GetScreenBottom();
  gfx::Rect switcher_bounds = page_switcher_->GetBoundsInScreen();
  float centerline_above_work_area =
      std::max<float>(screen_bottom - switcher_bounds.CenterPoint().y(), 0.f);
  float opacity =
      std::min(std::max((centerline_above_work_area - kAllAppsOpacityStartPx) /
                            (kAllAppsOpacityEndPx - kAllAppsOpacityStartPx),
                        0.f),
               1.0f);
  page_switcher_->layer()->SetOpacity(should_restore_opacity ? 1.0f : opacity);
}

gfx::Size AppsContainerView::CalculatePreferredSize() const {
  gfx::Size size = apps_grid_view_->GetPreferredSize();
  // Add padding to both side of the apps grid to keep it horizontally
  // centered since we place page switcher on the right side.
  size.Enlarge(kAppsGridLeftRightPadding * 2, 0);
  return size;
}

void AppsContainerView::Layout() {
  gfx::Rect rect(GetContentsBounds());
  if (rect.IsEmpty())
    return;

  switch (show_state_) {
    case SHOW_APPS: {
      gfx::Rect grid_rect = rect;
      grid_rect.Inset(kAppsGridLeftRightPadding, 0);
      apps_grid_view_->SetBoundsRect(grid_rect);

      gfx::Rect page_switcher_rect = rect;
      const int page_switcher_width =
          page_switcher_->GetPreferredSize().width();
      page_switcher_rect.set_x(page_switcher_rect.right() -
                               page_switcher_width);
      page_switcher_rect.set_width(page_switcher_width);
      page_switcher_->SetBoundsRect(page_switcher_rect);
      break;
    }
    case SHOW_ACTIVE_FOLDER: {
      folder_background_view_->SetBoundsRect(rect);
      app_list_folder_view_->SetBoundsRect(
          app_list_folder_view_->preferred_bounds());
      break;
    }
    case SHOW_ITEM_REPARENT:
      break;
    default:
      NOTREACHED();
  }
}

bool AppsContainerView::OnKeyPressed(const ui::KeyEvent& event) {
  if (show_state_ == SHOW_APPS)
    return apps_grid_view_->OnKeyPressed(event);
  else
    return app_list_folder_view_->OnKeyPressed(event);
}

const char* AppsContainerView::GetClassName() const {
  return "AppsContainerView";
}

void AppsContainerView::OnWillBeHidden() {
  if (show_state_ == SHOW_APPS || show_state_ == SHOW_ITEM_REPARENT)
    apps_grid_view_->EndDrag(true);
  else if (show_state_ == SHOW_ACTIVE_FOLDER)
    app_list_folder_view_->CloseFolderPage();
}

views::View* AppsContainerView::GetFirstFocusableView() {
  if (IsInFolderView()) {
    // The pagination inside a folder is set horizontally, so focus should be
    // set on the first item view in the selected page when it is moved down
    // from the search box.
    return app_list_folder_view_->items_grid_view()
        ->GetCurrentPageFirstItemViewInFolder();
  }
  return GetFocusManager()->GetNextFocusableView(
      this, GetWidget(), false /* reverse */, false /* dont_loop */);
}

gfx::Rect AppsContainerView::GetPageBoundsForState(
    ash::AppListState state) const {
  if (contents_view_->app_list_view()->is_in_drag())
    return GetPageBoundsDuringDragging(state);

  gfx::Rect bounds = parent()->GetContentsBounds();
  bounds.ClampToCenteredSize(GetPreferredSize());

  // AppsContainerView page is shown in both STATE_START and STATE_APPS.
  if (state == ash::AppListState::kStateApps ||
      state == ash::AppListState::kStateStart) {
    // The bottom left point relative to |contents_view_|.
    gfx::Point bottom_left = GetSearchBoxBoundsForState(state).bottom_left();
    if (state == ash::AppListState::kStateStart) {
      bottom_left.Offset(
          0, kSearchBoxPeekingBottomPadding - kSearchBoxBottomPadding);
    }
    ConvertPointToTarget(contents_view_, parent(), &bottom_left);
    bounds.set_y(bottom_left.y());
  }

  return bounds;
}

gfx::Rect AppsContainerView::GetSearchBoxBoundsForState(
    ash::AppListState state) const {
  gfx::Rect search_box_bounds(contents_view_->GetDefaultSearchBoxBounds());
  bool is_in_drag = contents_view_->app_list_view()->is_in_drag();
  if (is_in_drag) {
    search_box_bounds.set_y(GetSearchBoxTopPaddingDuringDragging());
  } else {
    if (state == ash::AppListState::kStateStart)
      search_box_bounds.set_y(kSearchBoxPeekingTopPadding);
    else
      search_box_bounds.set_y(GetSearchBoxFinalTopPadding());
  }

  return search_box_bounds;
}

int AppsContainerView::GetSearchBoxFinalTopPadding() const {
  gfx::Rect search_box_bounds(contents_view_->GetDefaultSearchBoxBounds());
  const int total_height =
      GetPreferredSize().height() + search_box_bounds.height();

  // Makes search box and content vertically centered in contents_view.
  int y = std::max(search_box_bounds.y(),
                   (contents_view_->GetDisplayHeight() - total_height) / 2);

  // Top padding of the searchbox should not be smaller than
  // |kSearchBoxMinimumTopPadding|
  return std::max(y, kSearchBoxMinimumTopPadding);
}

int AppsContainerView::GetSearchBoxTopPaddingDuringDragging() const {
  float searchbox_final_y = GetSearchBoxFinalTopPadding();
  float peeking_to_fullscreen_height =
      contents_view_->GetDisplayHeight() - kPeekingAppListHeight;
  float drag_amount = std::max(
      0, contents_view_->app_list_view()->GetScreenBottom() - kShelfSize -
             contents_view_->app_list_view()->app_list_y_position_in_screen());

  if (drag_amount <= (kPeekingAppListHeight - kShelfSize)) {
    // App list is dragged from collapsed to peeking, which moved up at most
    // |kPeekingAppListHeight - kShelfSize| (272px). The top padding of search
    // box changes from |kSearchBoxInitalTopPadding| to
    // |kSearchBoxPeekingTopPadding|,
    return std::ceil(
        (kSearchBoxPeekingTopPadding - kSearchBoxInitalTopPadding) +
        ((kSearchBoxPeekingTopPadding - kSearchBoxInitalTopPadding) *
         drag_amount) /
            (kPeekingAppListHeight - kShelfSize));
  } else {
    // App list is dragged from peeking to fullscreen, which moved up at most
    // |peeking_to_fullscreen_height|. The top padding of search box changes
    // from |kSearchBoxPeekingTopPadding| to |searchbox_final_y|.
    int y = (kSearchBoxPeekingTopPadding +
             std::ceil((searchbox_final_y - kSearchBoxPeekingTopPadding) *
                       (drag_amount - (kPeekingAppListHeight - kShelfSize)) /
                       peeking_to_fullscreen_height));
    y = std::max(kSearchBoxPeekingTopPadding,
                 std::min<int>(searchbox_final_y, y));
    return y;
  }
}

gfx::Rect AppsContainerView::GetPageBoundsDuringDragging(
    ash::AppListState state) const {
  float app_list_y_position_in_screen =
      contents_view_->app_list_view()->app_list_y_position_in_screen();
  float drag_amount =
      std::max(0.f, contents_view_->app_list_view()->GetScreenBottom() -
                        kShelfSize - app_list_y_position_in_screen);

  float y = 0;
  float peeking_final_y =
      kSearchBoxPeekingTopPadding + search_box::kSearchBoxPreferredHeight +
      kSearchBoxPeekingBottomPadding - kSearchBoxBottomPadding;
  if (drag_amount <= (kPeekingAppListHeight - kShelfSize)) {
    // App list is dragged from collapsed to peeking, which moved up at most
    // |kPeekingAppListHeight - kShelfSize| (272px). The top padding of apps
    // container view changes from |-kSearchBoxFullscreenBottomPadding| to
    // |kSearchBoxPeekingTopPadding + kSearchBoxPreferredHeight +
    // kSearchBoxPeekingBottomPadding - kSearchBoxFullscreenBottomPadding|.
    y = std::ceil(((peeking_final_y + kSearchBoxBottomPadding) * drag_amount) /
                      (kPeekingAppListHeight - kShelfSize) -
                  kSearchBoxBottomPadding);
  } else {
    // App list is dragged from peeking to fullscreen, which moved up at most
    // |peeking_to_fullscreen_height|. The top padding of apps container view
    // changes from |peeking_final_y| to |final_y|.
    float final_y =
        GetSearchBoxFinalTopPadding() + search_box::kSearchBoxPreferredHeight;
    float peeking_to_fullscreen_height =
        contents_view_->GetDisplayHeight() - kPeekingAppListHeight;
    y = std::ceil((final_y - peeking_final_y) *
                      (drag_amount - (kPeekingAppListHeight - kShelfSize)) /
                      peeking_to_fullscreen_height +
                  peeking_final_y);
    y = std::max(std::min(final_y, y), peeking_final_y);
  }

  gfx::Rect bounds = parent()->GetContentsBounds();
  bounds.ClampToCenteredSize(GetPreferredSize());

  // AppsContainerView page is shown in both STATE_START and STATE_APPS.
  if (state == ash::AppListState::kStateApps ||
      state == ash::AppListState::kStateStart) {
    gfx::Point point(0, y);
    ConvertPointToTarget(contents_view_, parent(), &point);
    bounds.set_y(point.y());
  }

  return bounds;
}

void AppsContainerView::SetShowState(ShowState show_state,
                                     bool show_apps_with_animation) {
  if (show_state_ == show_state)
    return;

  show_state_ = show_state;

  // Layout before showing animation because the animation's target bounds are
  // calculated based on the layout.
  Layout();

  switch (show_state_) {
    case SHOW_APPS:
      folder_background_view_->SetVisible(false);
      apps_grid_view_->ResetForShowApps();
      if (show_apps_with_animation)
        app_list_folder_view_->ScheduleShowHideAnimation(false, false);
      else
        app_list_folder_view_->HideViewImmediately();
      break;
    case SHOW_ACTIVE_FOLDER:
      folder_background_view_->SetVisible(true);
      app_list_folder_view_->ScheduleShowHideAnimation(true, false);
      break;
    case SHOW_ITEM_REPARENT:
      folder_background_view_->SetVisible(false);
      app_list_folder_view_->ScheduleShowHideAnimation(false, true);
      break;
    default:
      NOTREACHED();
  }
}

}  // namespace app_list
