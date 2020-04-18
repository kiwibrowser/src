// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_TEST_SERVER_WINDOW_DELEGATE_H_
#define SERVICES_UI_WS_TEST_SERVER_WINDOW_DELEGATE_H_

#include <set>

#include "base/macros.h"
#include "services/ui/ws/server_window_delegate.h"

namespace ui {
namespace ws {

class TestServerWindowDelegate : public ServerWindowDelegate {
 public:
  explicit TestServerWindowDelegate(VizHostProxy* viz_host_proxy);
  ~TestServerWindowDelegate() override;

  // GetRootWindowForDrawn() returns the first ServerWindow added by way of
  // AddRootWindow() that contains the supplied window. If none of the
  // ServerWindows added by way of AddRootWindow() contain the supplied window,
  // then the value passed to set_root_window() is returned.
  void set_root_window(ServerWindow* window) { root_window_ = window; }
  void AddRootWindow(ServerWindow* window);

 private:
  // ServerWindowDelegate:
  VizHostProxy* GetVizHostProxy() override;
  ServerWindow* GetRootWindowForDrawn(const ServerWindow* window) override;
  void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info,
                                ServerWindow* window) override;

  ServerWindow* root_window_ = nullptr;
  VizHostProxy* viz_host_proxy_ = nullptr;
  std::set<ServerWindow*> roots_;

  DISALLOW_COPY_AND_ASSIGN(TestServerWindowDelegate);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_TEST_SERVER_WINDOW_DELEGATE_H_
