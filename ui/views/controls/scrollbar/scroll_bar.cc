// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/scrollbar/scroll_bar.h"

#include "ui/accessibility/ax_node_data.h"

namespace views {

ScrollBar::~ScrollBar() {
}

void ScrollBar::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kScrollBar;
}

bool ScrollBar::IsHorizontal() const {
  return is_horiz_;
}

void ScrollBar::Update(int viewport_size, int content_size, int current_pos) {
  max_pos_ = std::max(0, content_size - viewport_size);
}

int ScrollBar::GetMaxPosition() const {
  return max_pos_;
}

int ScrollBar::GetMinPosition() const {
  return 0;
}

void ScrollBar::ObserveScrollEvent(const ui::ScrollEvent& event) {}

ScrollBar::ScrollBar(bool is_horiz)
    : is_horiz_(is_horiz),
      controller_(NULL),
      max_pos_(0) {
}

}  // namespace views
