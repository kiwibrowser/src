// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_server_test_impl.h"

#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/window_server.h"
#include "services/ui/ws/window_tree.h"

namespace ui {
namespace ws {

WindowServerTestImpl::WindowServerTestImpl(WindowServer* window_server)
    : window_server_(window_server) {}

WindowServerTestImpl::~WindowServerTestImpl() {}

void WindowServerTestImpl::OnSurfaceActivated(
    const std::string& name,
    EnsureClientHasDrawnWindowCallback cb,
    ServerWindow* window) {
  // This api is used to detect when a client has painted once, which is
  // dictated by whether there is a CompositorFrameSink.
  WindowTree* tree = window_server_->GetTreeWithClientName(name);
  if (tree && tree->HasRoot(window) &&
      window->has_created_compositor_frame_sink()) {
    std::move(cb).Run(true);
  } else {
    // No tree with the given name, or it hasn't painted yet. Install a callback
    // for the next time a client creates a CompositorFramesink.
    InstallCallback(name, std::move(cb));
  }
}

void WindowServerTestImpl::InstallCallback(
    const std::string& client_name,
    EnsureClientHasDrawnWindowCallback cb) {
  window_server_->SetSurfaceActivationCallback(
      base::BindOnce(&WindowServerTestImpl::OnSurfaceActivated,
                     base::Unretained(this), client_name, std::move(cb)));
}

void WindowServerTestImpl::EnsureClientHasDrawnWindow(
    const std::string& client_name,
    EnsureClientHasDrawnWindowCallback callback) {
  WindowTree* tree = window_server_->GetTreeWithClientName(client_name);
  if (tree) {
    for (const ServerWindow* window : tree->roots()) {
      if (window->has_created_compositor_frame_sink()) {
        std::move(callback).Run(true);
        return;
      }
    }
  }
  InstallCallback(client_name, std::move(callback));
}

}  // namespace ws
}  // namespace ui
