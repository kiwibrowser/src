// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/public/scoped_drag_drop_disabler.h"

#include "ui/aura/client/drag_drop_client.h"
#include "ui/aura/window.h"

namespace wm {

class NopDragDropClient : public aura::client::DragDropClient {
 public:
  ~NopDragDropClient() override {}
  int StartDragAndDrop(const ui::OSExchangeData& data,
                       aura::Window* root_window,
                       aura::Window* source_window,
                       const gfx::Point& screen_location,
                       int operation,
                       ui::DragDropTypes::DragEventSource source) override {
    return 0;
  }
  void DragCancel() override {}
  bool IsDragDropInProgress() override {
    return false;
  }
  void AddObserver(aura::client::DragDropClientObserver* observer) override {}
  void RemoveObserver(aura::client::DragDropClientObserver* observer) override {
  }
};

ScopedDragDropDisabler::ScopedDragDropDisabler(aura::Window* window)
    : window_(window),
      old_client_(aura::client::GetDragDropClient(window)),
      new_client_(new NopDragDropClient()) {
  SetDragDropClient(window_, new_client_.get());
  window_->AddObserver(this);
}

ScopedDragDropDisabler::~ScopedDragDropDisabler() {
  if (window_) {
    window_->RemoveObserver(this);
    SetDragDropClient(window_, old_client_);
  }
}

void ScopedDragDropDisabler::OnWindowDestroyed(aura::Window* window) {
  CHECK_EQ(window_, window);
  window_ = NULL;
  new_client_.reset();
}

}  // namespace wm
