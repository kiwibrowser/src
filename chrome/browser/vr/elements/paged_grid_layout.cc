// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/paged_grid_layout.h"

namespace vr {

PagedGridLayout::PagedGridLayout(size_t rows,
                                 size_t columns,
                                 const gfx::SizeF& tile_size)
    : rows_(rows), columns_(columns), tile_size_(tile_size) {
  DCHECK_NE(0lu, rows);
  DCHECK_NE(0lu, columns);
}

PagedGridLayout::~PagedGridLayout() {}

void PagedGridLayout::LayOutNonContributingChildren() {
  current_page_ = NumPages() > 0 ? std::min(NumPages() - 1, current_page_) : 0;

  gfx::SizeF page_size(columns_ * tile_size_.width() + (columns_ - 1) * margin_,
                       rows_ * tile_size_.height() + (rows_ - 1) * margin_);
  gfx::Vector2dF initial_offset(
      -0.5 * (page_size.width() * NumPages() + margin_ * (NumPages() - 1) -
              tile_size_.width()),
      -0.5 * (page_size.height() - tile_size_.height()));

  for (size_t i = 0; i < children().size(); i++) {
    if (!children()[i]->IsVisible()) {
      continue;
    }

    size_t child_page = i / (rows_ * columns_);
    gfx::Vector2dF page_offset(
        child_page * columns_ * (tile_size_.width() + margin_), 0.0f);

    size_t in_page_index = i % (rows_ * columns_);
    gfx::Vector2dF in_page_offset(
        (in_page_index % columns_) * (tile_size_.width() + margin_),
        (in_page_index / columns_) * (tile_size_.height() + margin_));

    gfx::Vector2dF child_offset = initial_offset + page_offset + in_page_offset;
    children()[i]->SetLayoutOffset(child_offset.x(), -child_offset.y());
  }

  SetSize(page_size.width() * NumPages() + margin_ * (NumPages() - 1),
          page_size.height());
}

void PagedGridLayout::SetCurrentPage(size_t current_page) {
  DCHECK(current_page == 0 || (NumPages() > 0 && current_page < NumPages()));
  current_page_ = current_page;
}

size_t PagedGridLayout::NumPages() const {
  size_t page_size = rows_ * columns_;
  return (children().size() + (page_size - 1)) / page_size;
}

PagedGridLayout::PageState PagedGridLayout::GetPageState(UiElement* child) {
  auto child_iter =
      std::find_if(children().begin(), children().end(),
                   [child](const std::unique_ptr<UiElement>& current_child) {
                     return current_child.get() == child;
                   });
  if (child_iter == children().end()) {
    return kNone;
  }
  size_t child_index = std::distance(children().begin(), child_iter);
  size_t child_page = child_index / (rows_ * columns_);
  if (child_page == current_page()) {
    return kActive;
  }
  if (child_page + 1 == current_page() || child_page == current_page() + 1) {
    return kInactive;
  }
  return kHidden;
}

}  // namespace vr
