// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/drag_window_resizer_mash.h"

namespace ash {

DragWindowResizerMash::DragWindowResizerMash(
    std::unique_ptr<WindowResizer> next_window_resizer,
    wm::WindowState* window_state)
    : WindowResizer(window_state),
      next_window_resizer_(std::move(next_window_resizer)) {}

DragWindowResizerMash::~DragWindowResizerMash() {
  if (window_state_)
    window_state_->DeleteDragDetails();
}

void DragWindowResizerMash::Drag(const gfx::Point& location, int event_flags) {
  next_window_resizer_->Drag(location, event_flags);
  // http://crbug.com/613199.
  NOTIMPLEMENTED_LOG_ONCE();
}

void DragWindowResizerMash::CompleteDrag() {
  next_window_resizer_->CompleteDrag();
  // http://crbug.com/613199.
  NOTIMPLEMENTED_LOG_ONCE();
}

void DragWindowResizerMash::RevertDrag() {
  next_window_resizer_->RevertDrag();
  // http://crbug.com/613199.
  NOTIMPLEMENTED_LOG_ONCE();
}

}  // namespace ash
