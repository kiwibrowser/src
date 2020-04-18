// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/remote_view/remote_view_provider_test_api.h"

#include "ui/views/mus/remote_view/remote_view_provider.h"

namespace views {
namespace test {

void RemoteViewProviderTestApi::SetWindowTreeClient(
    aura::WindowTreeClient* window_tree_client) {
  RemoteViewProvider::window_tree_client_for_test = window_tree_client;
}

}  // namespace test
}  // namespace views
