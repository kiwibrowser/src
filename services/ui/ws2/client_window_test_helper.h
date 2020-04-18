// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_CLIENT_WINDOW_TEST_HELPER_H_
#define SERVICES_UI_WS2_CLIENT_WINDOW_TEST_HELPER_H_

#include "base/macros.h"

namespace ui {
namespace ws2 {

class ClientWindow;

// Used for accessing private members of ClientWindow in tests.
class ClientWindowTestHelper {
 public:
  explicit ClientWindowTestHelper(ClientWindow* client_window);
  ~ClientWindowTestHelper();

  bool IsInPointerPressed();

 private:
  ClientWindow* client_window_;

  DISALLOW_COPY_AND_ASSIGN(ClientWindowTestHelper);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_CLIENT_WINDOW_TEST_HELPER_H_
