// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/client_change.h"

#include <utility>

#include "services/ui/ws2/client_change_tracker.h"
#include "ui/aura/window.h"

namespace ui {
namespace ws2 {

ClientChange::ClientChange(ClientChangeTracker* tracker,
                           aura::Window* window,
                           ClientChangeType type)
    : tracker_(tracker), type_(type) {
  DCHECK(!tracker_->current_change_);
  tracker_->current_change_ = this;
  window_tracker_.Add(window);
}

ClientChange::~ClientChange() {
  DCHECK_EQ(this, tracker_->current_change_);
  tracker_->current_change_ = nullptr;
}

}  // namespace ws2
}  // namespace ui
