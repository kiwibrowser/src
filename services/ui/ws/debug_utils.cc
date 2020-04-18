// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/debug_utils.h"

#include "services/ui/ws/server_window.h"

namespace ui {
namespace ws {

std::string DebugWindowId(const ServerWindow* window) {
  return window ? window->frame_sink_id().ToString() : "null";
}

}  // namespace ws
}  // namespace ui
