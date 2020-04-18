// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/app_list_page.h"

#include "ui/app_list/views/contents_view.h"

namespace app_list {

AppListPage::AppListPage() : contents_view_(nullptr) {}

AppListPage::~AppListPage() {}

void AppListPage::OnShown() {}

void AppListPage::OnWillBeShown() {}

void AppListPage::OnHidden() {}

void AppListPage::OnWillBeHidden() {}

void AppListPage::OnAnimationUpdated(double progress,
                                     ash::AppListState from_state,
                                     ash::AppListState to_state) {}

gfx::Rect AppListPage::GetSearchBoxBounds() const {
  DCHECK(contents_view_);
  return contents_view_->GetDefaultSearchBoxBounds();
}

gfx::Rect AppListPage::GetSearchBoxBoundsForState(
    ash::AppListState state) const {
  return GetSearchBoxBounds();
}

views::View* AppListPage::GetSelectedView() const {
  return nullptr;
}

views::View* AppListPage::GetFirstFocusableView() {
  return nullptr;
}

views::View* AppListPage::GetLastFocusableView() {
  return nullptr;
}

bool AppListPage::ShouldShowSearchBox() const {
  return true;
}

gfx::Rect AppListPage::GetAboveContentsOffscreenBounds(
    const gfx::Size& size) const {
  gfx::Rect rect(size);
  rect.set_y(-rect.height());
  return rect;
}

gfx::Rect AppListPage::GetBelowContentsOffscreenBounds(
    const gfx::Size& size) const {
  DCHECK(contents_view_);
  gfx::Rect rect(size);
  rect.set_y(contents_view_->GetContentsBounds().height());
  return rect;
}

gfx::Rect AppListPage::GetFullContentsBounds() const {
  DCHECK(contents_view_);
  return contents_view_->GetContentsBounds();
}

gfx::Rect AppListPage::GetDefaultContentsBounds() const {
  DCHECK(contents_view_);
  return contents_view_->GetDefaultContentsBounds();
}

}  // namespace app_list
