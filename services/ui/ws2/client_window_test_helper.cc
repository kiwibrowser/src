// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/client_window_test_helper.h"

#include "services/ui/ws2/client_window.h"

namespace ui {
namespace ws2 {

ClientWindowTestHelper::ClientWindowTestHelper(ClientWindow* client_window)
    : client_window_(client_window) {}

ClientWindowTestHelper::~ClientWindowTestHelper() = default;

bool ClientWindowTestHelper::IsInPointerPressed() {
  return client_window_->IsInPointerPressedForTesting();
}

}  // namespace ws2
}  // namespace ui
