// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_server_delegate.h"

#include "services/ui/ws/window_tree_binding.h"

namespace ui {
namespace ws {

void WindowServerDelegate::OnFirstDisplayReady() {}

std::unique_ptr<WindowTreeBinding>
WindowServerDelegate::CreateWindowTreeBinding(
    BindingType type,
    ws::WindowServer* window_server,
    ws::WindowTree* tree,
    mojom::WindowTreeRequest* tree_request,
    mojom::WindowTreeClientPtr* client) {
  return nullptr;
}

}  // namespace ws
}  // namespace ui
