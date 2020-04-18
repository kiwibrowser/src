// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/contents_layout_manager.h"

#include "ui/views/view.h"

ContentsLayoutManager::ContentsLayoutManager(
    views::View* devtools_view,
    views::View* contents_view)
    : devtools_view_(devtools_view),
      contents_view_(contents_view),
      host_(nullptr),
      active_top_margin_(0) {
}

ContentsLayoutManager::~ContentsLayoutManager() {
}

void ContentsLayoutManager::SetContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy) {
  if (strategy_.Equals(strategy))
    return;

  strategy_.CopyFrom(strategy);
  if (host_)
    host_->InvalidateLayout();
}

void ContentsLayoutManager::SetActiveTopMargin(int margin) {
  if (active_top_margin_ == margin)
    return;

  active_top_margin_ = margin;
  if (host_)
    host_->InvalidateLayout();
}

void ContentsLayoutManager::Layout(views::View* contents_container) {
  DCHECK(host_ == contents_container);

  int top = active_top_margin_;
  int height = std::max(0, contents_container->height() - top);
  int width = contents_container->width();

  gfx::Size container_size(width, height);
  gfx::Rect new_devtools_bounds;
  gfx::Rect new_contents_bounds;

  ApplyDevToolsContentsResizingStrategy(strategy_, container_size,
      &new_devtools_bounds, &new_contents_bounds);
  new_devtools_bounds.Offset(0, top);
  new_contents_bounds.Offset(0, top);

  // DevTools cares about the specific position, so we have to compensate RTL
  // layout here.
  devtools_view_->SetBoundsRect(host_->GetMirroredRect(new_devtools_bounds));
  contents_view_->SetBoundsRect(host_->GetMirroredRect(new_contents_bounds));
}

gfx::Size ContentsLayoutManager::GetPreferredSize(
    const views::View* host) const {
  return gfx::Size();
}

void ContentsLayoutManager::Installed(views::View* host) {
  DCHECK(!host_);
  host_ = host;
}
