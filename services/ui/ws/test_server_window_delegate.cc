// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/test_server_window_delegate.h"

#include "services/ui/ws/server_window.h"

namespace ui {
namespace ws {

TestServerWindowDelegate::TestServerWindowDelegate(VizHostProxy* viz_host_proxy)
    : viz_host_proxy_(viz_host_proxy) {}

TestServerWindowDelegate::~TestServerWindowDelegate() {}

void TestServerWindowDelegate::AddRootWindow(ServerWindow* window) {
  roots_.insert(window);
}

VizHostProxy* TestServerWindowDelegate::GetVizHostProxy() {
  return viz_host_proxy_;
}

ServerWindow* TestServerWindowDelegate::GetRootWindowForDrawn(
    const ServerWindow* window) {
  for (ServerWindow* root : roots_) {
    if (root->Contains(window))
      return root;
  }
  return root_window_;
}

void TestServerWindowDelegate::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info,
    ServerWindow* window) {}

}  // namespace ws
}  // namespace ui
