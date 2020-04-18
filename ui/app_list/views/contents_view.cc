// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/contents_view.h"

#include <algorithm>
#include <vector>

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/cpp/app_list/app_list_features.h"
#include "ash/public/cpp/app_list/app_list_switches.h"
#include "base/logging.h"
#include "ui/app_list/app_list_view_delegate.h"
#include "ui/app_list/views/app_list_folder_view.h"
#include "ui/app_list/views/app_list_main_view.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/app_list/views/apps_container_view.h"
#include "ui/app_list/views/apps_grid_view.h"
#include "ui/app_list/views/horizontal_page_container.h"
#include "ui/app_list/views/search_box_view.h"
#include "ui/app_list/views/search_result_answer_card_view.h"
#include "ui/app_list/views/search_result_list_view.h"
#include "ui/app_list/views/search_result_page_view.h"
#include "ui/app_list/views/search_result_tile_item_list_view.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/views/view_model.h"
#include "ui/views/widget/widget.h"

namespace app_list {

namespace {

void DoAnimation(base::TimeDelta animation_duration,
                 ui::Layer* layer,
                 float target_opacity) {
  ui::ScopedLayerAnimationSettings animation(layer->GetAnimator());
  animation.SetTransitionDuration(animation_duration);
  animation.SetTweenType(gfx::Tween::EASE_OUT);
  animation.SetPreemptionStrategy(
      ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
  layer->SetOpacity(target_opacity);
}

}  // namespace

ContentsView::ContentsView(AppListView* app_list_view)
    : app_list_view_(app_list_view) {
  pagination_model_.SetTransitionDurations(kPageTransitionDurationInMs,
                                           kOverscrollPageTransitionDurationMs);
  pagination_model_.AddObserver(this);
}

ContentsView::~ContentsView() {
  pagination_model_.RemoveObserver(this);
}

void ContentsView::Init(AppListModel* model) {
  DCHECK(model);
  model_ = model;

  AppListViewDelegate* view_delegate = GetAppListMainView()->view_delegate();

  horizontal_page_container_ = new HorizontalPageContainer(this, model);

  // Add |horizontal_page_container_| as STATE_START corresponding page for
  // fullscreen app list.
  AddLauncherPage(horizontal_page_container_, ash::AppListState::kStateStart);

  // Search results UI.
  search_results_page_view_ = new SearchResultPageView();

  // Search result containers.
  SearchModel::SearchResults* results =
      view_delegate->GetSearchModel()->results();

  if (features::IsAnswerCardEnabled()) {
    search_result_answer_card_view_ =
        new SearchResultAnswerCardView(view_delegate);
    search_results_page_view_->AddSearchResultContainerView(
        results, search_result_answer_card_view_);
  }

  search_result_tile_item_list_view_ = new SearchResultTileItemListView(
      search_results_page_view_, GetSearchBoxView()->search_box(),
      view_delegate);
  search_results_page_view_->AddSearchResultContainerView(
      results, search_result_tile_item_list_view_);

  search_result_list_view_ =
      new SearchResultListView(GetAppListMainView(), view_delegate);
  search_results_page_view_->AddSearchResultContainerView(
      results, search_result_list_view_);

  AddLauncherPage(search_results_page_view_,
                  ash::AppListState::kStateSearchResults);

  AddLauncherPage(horizontal_page_container_, ash::AppListState::kStateApps);

  int initial_page_index = GetPageIndexForState(ash::AppListState::kStateStart);
  DCHECK_GE(initial_page_index, 0);

  page_before_search_ = initial_page_index;
  // Must only call SetTotalPages once all the launcher pages have been added
  // (as it will trigger a SelectedPageChanged call).
  pagination_model_.SetTotalPages(app_list_pages_.size());

  // Page 0 is selected by SetTotalPages and needs to be 'hidden' when selecting
  // the initial page.
  app_list_pages_[GetActivePageIndex()]->OnWillBeHidden();

  pagination_model_.SelectPage(initial_page_index, false);

  ActivePageChanged();
}

void ContentsView::CancelDrag() {
  if (GetAppsContainerView()->apps_grid_view()->has_dragged_view())
    GetAppsContainerView()->apps_grid_view()->EndDrag(true);
  if (GetAppsContainerView()
          ->app_list_folder_view()
          ->items_grid_view()
          ->has_dragged_view()) {
    GetAppsContainerView()->app_list_folder_view()->items_grid_view()->EndDrag(
        true);
  }
}

void ContentsView::SetDragAndDropHostOfCurrentAppList(
    ApplicationDragAndDropHost* drag_and_drop_host) {
  GetAppsContainerView()->SetDragAndDropHostOfCurrentAppList(
      drag_and_drop_host);
}

void ContentsView::SetActiveState(ash::AppListState state) {
  SetActiveState(state, true);
}

void ContentsView::SetActiveState(ash::AppListState state, bool animate) {
  if (IsStateActive(state))
    return;

  SetActiveStateInternal(GetPageIndexForState(state), false, animate);
}

int ContentsView::GetActivePageIndex() const {
  // The active page is changed at the beginning of an animation, not the end.
  return pagination_model_.SelectedTargetPage();
}

ash::AppListState ContentsView::GetActiveState() const {
  return GetStateForPageIndex(GetActivePageIndex());
}

bool ContentsView::IsStateActive(ash::AppListState state) const {
  int active_page_index = GetActivePageIndex();
  return active_page_index >= 0 &&
         GetPageIndexForState(state) == active_page_index;
}

int ContentsView::GetPageIndexForState(ash::AppListState state) const {
  // Find the index of the view corresponding to the given state.
  std::map<ash::AppListState, int>::const_iterator it =
      state_to_view_.find(state);
  if (it == state_to_view_.end())
    return -1;

  return it->second;
}

ash::AppListState ContentsView::GetStateForPageIndex(int index) const {
  std::map<int, ash::AppListState>::const_iterator it =
      view_to_state_.find(index);
  if (it == view_to_state_.end())
    return ash::AppListState::kInvalidState;

  return it->second;
}

int ContentsView::NumLauncherPages() const {
  return pagination_model_.total_pages();
}

AppsContainerView* ContentsView::GetAppsContainerView() {
  return horizontal_page_container_->apps_container_view();
}

void ContentsView::SetActiveStateInternal(int page_index,
                                          bool show_search_results,
                                          bool animate) {
  if (!GetPageView(page_index)->visible())
    return;

  if (!show_search_results)
    page_before_search_ = page_index;

  app_list_pages_[GetActivePageIndex()]->OnWillBeHidden();

  // Start animating to the new page.
  pagination_model_.SelectPage(page_index, animate);
  ActivePageChanged();

  if (!animate)
    Layout();
}

void ContentsView::ActivePageChanged() {
  ash::AppListState state = ash::AppListState::kInvalidState;

  std::map<int, ash::AppListState>::const_iterator it =
      view_to_state_.find(GetActivePageIndex());
  if (it != view_to_state_.end())
    state = it->second;

  app_list_pages_[GetActivePageIndex()]->OnWillBeShown();

  GetAppListMainView()->model()->SetState(state);
}

void ContentsView::ShowSearchResults(bool show) {
  int search_page =
      GetPageIndexForState(ash::AppListState::kStateSearchResults);
  DCHECK_GE(search_page, 0);

  SetActiveStateInternal(show ? search_page : page_before_search_, show, true);
}

bool ContentsView::IsShowingSearchResults() const {
  return IsStateActive(ash::AppListState::kStateSearchResults);
}

void ContentsView::UpdatePageBounds() {
  // The bounds calculations will potentially be mid-transition (depending on
  // the state of the PaginationModel).
  int current_page = std::max(0, pagination_model_.selected_page());
  int target_page = current_page;
  double progress = 1;
  if (pagination_model_.has_transition()) {
    const PaginationModel::Transition& transition =
        pagination_model_.transition();
    if (pagination_model_.is_valid_page(transition.target_page)) {
      target_page = transition.target_page;
      progress = transition.progress;
    }
  }

  ash::AppListState current_state = GetStateForPageIndex(current_page);
  ash::AppListState target_state = GetStateForPageIndex(target_page);

  // Update app list pages.
  for (AppListPage* page : app_list_pages_) {
    page->OnAnimationUpdated(progress, current_state, target_state);

    gfx::Rect to_rect = page->GetPageBoundsForState(target_state);
    gfx::Rect from_rect = page->GetPageBoundsForState(current_state);
    if (from_rect == to_rect)
      continue;

    // Animate linearly (the PaginationModel handles easing).
    gfx::Rect bounds(
        gfx::Tween::RectValueBetween(progress, from_rect, to_rect));

    page->SetBoundsRect(bounds);
  }

  // Update the search box.
  UpdateSearchBox(progress, current_state, target_state);
}

void ContentsView::UpdateSearchBox(double progress,
                                   ash::AppListState current_state,
                                   ash::AppListState target_state) {
  AppListPage* from_page = GetPageView(GetPageIndexForState(current_state));
  AppListPage* to_page = GetPageView(GetPageIndexForState(target_state));

  SearchBoxView* search_box = GetSearchBoxView();

  gfx::Rect search_box_from(from_page->GetSearchBoxBounds());
  gfx::Rect search_box_to(to_page->GetSearchBoxBounds());
  gfx::Rect search_box_rect =
      gfx::Tween::RectValueBetween(progress, search_box_from, search_box_to);

  search_box->UpdateLayout(progress, current_state, target_state);
  search_box->UpdateBackground(progress, current_state, target_state);
  search_box->GetWidget()->SetBounds(
      search_box->GetViewBoundsForSearchBoxContentsBounds(
          ConvertRectToWidget(search_box_rect)));
}

PaginationModel* ContentsView::GetAppsPaginationModel() {
  return GetAppsContainerView()->apps_grid_view()->pagination_model();
}

void ContentsView::ShowFolderContent(AppListFolderItem* item) {
  GetAppsContainerView()->ShowActiveFolder(item);
}

AppListPage* ContentsView::GetPageView(int index) const {
  DCHECK_GT(static_cast<int>(app_list_pages_.size()), index);
  return app_list_pages_[index];
}

SearchBoxView* ContentsView::GetSearchBoxView() const {
  return GetAppListMainView()->search_box_view();
}

AppListMainView* ContentsView::GetAppListMainView() const {
  return app_list_view_->app_list_main_view();
}

int ContentsView::AddLauncherPage(AppListPage* view) {
  view->set_contents_view(this);
  AddChildView(view);
  app_list_pages_.push_back(view);
  return app_list_pages_.size() - 1;
}

int ContentsView::AddLauncherPage(AppListPage* view, ash::AppListState state) {
  int page_index = AddLauncherPage(view);
  bool success =
      state_to_view_.insert(std::make_pair(state, page_index)).second;
  success = success &&
            view_to_state_.insert(std::make_pair(page_index, state)).second;

  // There shouldn't be duplicates in either map.
  DCHECK(success);
  return page_index;
}

gfx::Rect ContentsView::GetDefaultSearchBoxBounds() const {
  gfx::Rect search_box_bounds;
  search_box_bounds.set_size(GetSearchBoxView()->GetPreferredSize());
  search_box_bounds.Offset((bounds().width() - search_box_bounds.width()) / 2,
                           0);
  search_box_bounds.set_y(kSearchBoxTopPadding);
  return search_box_bounds;
}

gfx::Rect ContentsView::GetSearchBoxBoundsForState(
    ash::AppListState state) const {
  AppListPage* page = GetPageView(GetPageIndexForState(state));
  return page->GetSearchBoxBoundsForState(state);
}

gfx::Rect ContentsView::GetDefaultContentsBounds() const {
  return GetContentsBounds();
}

gfx::Size ContentsView::GetMaximumContentsSize() const {
  int max_width = 0;
  int max_height = 0;
  for (AppListPage* page : app_list_pages_) {
    const gfx::Size size(page->GetPreferredSize());
    max_width = std::max(size.width(), max_width);
    max_height = std::max(size.height(), max_height);
  }
  return gfx::Size(max_width, max_height);
}

bool ContentsView::Back() {
  ash::AppListState state = view_to_state_[GetActivePageIndex()];
  switch (state) {
    case ash::AppListState::kStateStart:
      // Close the app list when Back() is called from the start page.
      return false;
    case ash::AppListState::kStateApps:
      if (GetAppsContainerView()->IsInFolderView()) {
        GetAppsContainerView()->app_list_folder_view()->CloseFolderPage();
      } else {
        // Close the app list when Back() is called from the apps page.
        return false;
      }
      break;
    case ash::AppListState::kStateSearchResults:
      GetSearchBoxView()->ClearSearch();
      GetSearchBoxView()->SetSearchBoxActive(false);
      ShowSearchResults(false);
      break;
    case ash::AppListState::kStateCustomLauncherPageDeprecated:
    case ash::AppListState::kInvalidState:  // Falls through.
      NOTREACHED();
      break;
  }
  return true;
}

gfx::Size ContentsView::GetDefaultContentsSize() const {
  return horizontal_page_container_->GetPreferredSize();
}

gfx::Size ContentsView::CalculatePreferredSize() const {
  return gfx::Size(GetDisplayWidth(), GetDisplayHeight());
}

void ContentsView::Layout() {
  // Immediately finish all current animations.
  pagination_model_.FinishAnimation();

  if (GetContentsBounds().IsEmpty())
    return;

  for (AppListPage* page : app_list_pages_) {
    // Ensures re-layout happens even when the page bounds does not change. So
    // that |horizontal_page_container_| can layout its children in response to
    // user dragging.
    page->InvalidateLayout();
    page->SetBoundsRect(page->GetPageBoundsForState(GetActiveState()));
  }

  // The search box is contained in a widget so set the bounds of the widget
  // rather than the SearchBoxView.
  views::Widget* search_box_widget = GetSearchBoxView()->GetWidget();
  if (search_box_widget && search_box_widget != GetWidget()) {
    gfx::Rect search_box_bounds = GetSearchBoxBoundsForState(GetActiveState());
    search_box_widget->SetBounds(ConvertRectToWidget(
        GetSearchBoxView()->GetViewBoundsForSearchBoxContentsBounds(
            search_box_bounds)));
  }
}

const char* ContentsView::GetClassName() const {
  return "ContentsView";
}

void ContentsView::TotalPagesChanged() {}

void ContentsView::SelectedPageChanged(int old_selected, int new_selected) {
  if (old_selected >= 0)
    app_list_pages_[old_selected]->OnHidden();

  if (new_selected >= 0)
    app_list_pages_[new_selected]->OnShown();
}

void ContentsView::TransitionStarted() {}

void ContentsView::TransitionChanged() {
  UpdatePageBounds();
}

void ContentsView::TransitionEnded() {}

int ContentsView::GetDisplayHeight() const {
  return display::Screen::GetScreen()
      ->GetDisplayNearestView(GetWidget()->GetNativeView())
      .work_area()
      .size()
      .height();
}

int ContentsView::GetDisplayWidth() const {
  return display::Screen::GetScreen()
      ->GetDisplayNearestView(GetWidget()->GetNativeView())
      .work_area()
      .size()
      .width();
}

void ContentsView::FadeOutOnClose(base::TimeDelta animation_duration) {
  DoAnimation(animation_duration, layer(), 0.0f);
  DoAnimation(animation_duration, GetSearchBoxView()->layer(), 0.0f);
}

void ContentsView::FadeInOnOpen(base::TimeDelta animation_duration) {
  GetSearchBoxView()->layer()->SetOpacity(0.0f);
  layer()->SetOpacity(0.0f);
  DoAnimation(animation_duration, layer(), 1.0f);
  DoAnimation(animation_duration, GetSearchBoxView()->layer(), 1.0f);
}

views::View* ContentsView::GetSelectedView() const {
  return app_list_pages_[GetActivePageIndex()]->GetSelectedView();
}

}  // namespace app_list
