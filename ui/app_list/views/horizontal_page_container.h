// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_HORIZONTAL_PAGE_CONTAINER_H_
#define UI_APP_LIST_VIEWS_HORIZONTAL_PAGE_CONTAINER_H_

#include "base/macros.h"
#include "ui/app_list/app_list_export.h"
#include "ui/app_list/pagination_model.h"
#include "ui/app_list/pagination_model_observer.h"
#include "ui/app_list/views/app_list_page.h"

namespace app_list {

class AppsContainerView;
class AssistantContainerView;
class PaginationController;
class HorizontalPage;

// HorizontalPageContainer contains a list of HorizontalPage that are
// horizontally laid out. These pages can be switched with gesture scrolling.
class APP_LIST_EXPORT HorizontalPageContainer : public AppListPage,
                                                public PaginationModelObserver {
 public:
  HorizontalPageContainer(ContentsView* contents_view, AppListModel* model);
  ~HorizontalPageContainer() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  // AppListPage overrides:
  void OnWillBeHidden() override;
  void OnAnimationUpdated(double progress,
                          ash::AppListState from_state,
                          ash::AppListState to_state) override;
  gfx::Rect GetSearchBoxBounds() const override;
  gfx::Rect GetSearchBoxBoundsForState(ash::AppListState state) const override;
  gfx::Rect GetPageBoundsForState(ash::AppListState state) const override;
  views::View* GetFirstFocusableView() override;
  views::View* GetLastFocusableView() override;
  bool ShouldShowSearchBox() const override;

  AppsContainerView* apps_container_view() { return apps_container_view_; }

 private:
  // PaginationModelObserver:
  void TotalPagesChanged() override;
  void SelectedPageChanged(int old_selected, int new_selected) override;
  void TransitionStarted() override;
  void TransitionChanged() override;
  void TransitionEnded() override;

  // Adds a horizontal page to this view.
  int AddHorizontalPage(HorizontalPage* view);

  // Gets the index of a horizontal page in |horizontal_pages_|. Returns -1 if
  // there is no such view.
  int GetIndexForPage(HorizontalPage* view) const;

  // Gets the currently selected horizontal page.
  HorizontalPage* GetSelectedPage();
  const HorizontalPage* GetSelectedPage() const;

  // Gets the offset for the horizontal page with specified index.
  gfx::Vector2d GetOffsetForPageIndex(int index) const;

  // Manages the pagination for the horizontal pages.
  PaginationModel pagination_model_;

  // Must appear after |pagination_model_|.
  std::unique_ptr<PaginationController> pagination_controller_;

  ContentsView* contents_view_;  // Not owned

  // Owned by view hierarchy:
  AppsContainerView* apps_container_view_ = nullptr;

  // Owned by view hierarchy:
  AssistantContainerView* assistant_container_view_ = nullptr;

  // The child page views. Owned by the views hierarchy.
  std::vector<HorizontalPage*> horizontal_pages_;

  DISALLOW_COPY_AND_ASSIGN(HorizontalPageContainer);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_HORIZONTAL_PAGE_CONTAINER_H_
