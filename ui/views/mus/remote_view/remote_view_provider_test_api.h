// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_REMOTE_VIEW_REMOTE_VIEW_PROVIDER_TEST_API_H_
#define UI_VIEWS_MUS_REMOTE_VIEW_REMOTE_VIEW_PROVIDER_TEST_API_H_

#include "base/macros.h"

namespace aura {
class WindowTreeClient;
}

namespace views {
namespace test {

class RemoteViewProviderTestApi {
 public:
  // Sets an aura::WindowTreeClient to use with RemoteViewProvider.
  static void SetWindowTreeClient(aura::WindowTreeClient* window_tree_client);

 private:
  DISALLOW_COPY_AND_ASSIGN(RemoteViewProviderTestApi);
};

}  // namespace test
}  // namespace views

#endif  // UI_VIEWS_MUS_REMOTE_VIEW_REMOTE_VIEW_PROVIDER_TEST_API_H_
