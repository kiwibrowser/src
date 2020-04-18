// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/window_test_api.h"

#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"

namespace aura {
namespace test {

WindowTestApi::WindowTestApi(Window* window) : window_(window) {
}

bool WindowTestApi::OwnsLayer() const {
  return window_->OwnsLayer();
}

bool WindowTestApi::ContainsMouse() const {
  if (!window_->IsVisible())
    return false;
  WindowTreeHost* host = window_->GetHost();
  return host &&
         window_->ContainsPointInRoot(
             host->dispatcher()->GetLastMouseLocationInRoot());
}

void WindowTestApi::DisableFrameSinkRegistration() {
  window_->disable_frame_sink_id_registration_ = true;
}

}  // namespace test
}  // namespace aura
