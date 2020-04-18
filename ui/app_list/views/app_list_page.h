// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_APP_LIST_PAGE_H_
#define UI_APP_LIST_VIEWS_APP_LIST_PAGE_H_

#include "ash/app_list/model/app_list_model.h"
#include "base/macros.h"
#include "ui/app_list/app_list_export.h"
#include "ui/views/view.h"

namespace app_list {

class ContentsView;

class APP_LIST_EXPORT AppListPage : public views::View {
 public:
  AppListPage();
  ~AppListPage() override;

  // Triggered when the page is about to be shown.
  virtual void OnWillBeShown();

  // Triggered after the page has been shown.
  virtual void OnShown();

  // Triggered when the page is about to be hidden.
  virtual void OnWillBeHidden();

  // Triggered after the page has been hidden.
  virtual void OnHidden();

  // Triggered after the animation has updated.
  virtual void OnAnimationUpdated(double progress,
                                  ash::AppListState from_state,
                                  ash::AppListState to_state);

  // Returns where the search box should be when this page is shown. Is at the
  // top of the app list by default, in the contents view's coordinate space.
  virtual gfx::Rect GetSearchBoxBounds() const;

  // Returns the bounds of the search box according to |state|.
  virtual gfx::Rect GetSearchBoxBoundsForState(ash::AppListState state) const;

  // Returns where this page should move to when the given state is active.
  virtual gfx::Rect GetPageBoundsForState(ash::AppListState state) const = 0;

  const ContentsView* contents_view() const { return contents_view_; }
  void set_contents_view(ContentsView* contents_view) {
    contents_view_ = contents_view;
  }

  // Returns selected view in this page.
  virtual views::View* GetSelectedView() const;

  // Returns the first focusable view in this page.
  virtual views::View* GetFirstFocusableView();

  // Returns the last focusable view in this page.
  virtual views::View* GetLastFocusableView();

  // Returns true if the search box should be shown in this page.
  virtual bool ShouldShowSearchBox() const;

  // Returns the area above the contents view, given the desired size of this
  // page, in the contents view's coordinate space.
  gfx::Rect GetAboveContentsOffscreenBounds(const gfx::Size& size) const;

  // Returns the area below the contents view, given the desired size of this
  // page, in the contents view's coordinate space.
  gfx::Rect GetBelowContentsOffscreenBounds(const gfx::Size& size) const;

  // Returns the entire bounds of the contents view, in the contents view's
  // coordinate space.
  gfx::Rect GetFullContentsBounds() const;

  // Returns the default bounds of pages inside the contents view, in the
  // contents view's coordinate space. This is the area of the contents view
  // below the search box.
  gfx::Rect GetDefaultContentsBounds() const;

 private:
  ContentsView* contents_view_;

  DISALLOW_COPY_AND_ASSIGN(AppListPage);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_APP_LIST_PAGE_H_
