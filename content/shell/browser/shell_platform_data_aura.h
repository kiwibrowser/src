// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_SHELL_PLATFORM_DATA_AURA_H_
#define CONTENT_SHELL_BROWSER_SHELL_PLATFORM_DATA_AURA_H_

#include <memory>

#include "base/macros.h"
#include "ui/aura/window_tree_host.h"

namespace aura {
namespace client {
class DefaultCaptureClient;
class FocusClient;
class WindowParentingClient;
}
}

namespace gfx {
class Size;
}

namespace content {

class ShellPlatformDataAura {
 public:
  explicit ShellPlatformDataAura(const gfx::Size& initial_size);
  ~ShellPlatformDataAura();

  void ShowWindow();
  void ResizeWindow(const gfx::Size& size);

  aura::WindowTreeHost* host() { return host_.get(); }

 private:
  std::unique_ptr<aura::WindowTreeHost> host_;
  std::unique_ptr<aura::client::FocusClient> focus_client_;
  std::unique_ptr<aura::client::DefaultCaptureClient> capture_client_;
  std::unique_ptr<aura::client::WindowParentingClient> window_parenting_client_;

  DISALLOW_COPY_AND_ASSIGN(ShellPlatformDataAura);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_SHELL_PLATFORM_DATA_AURA_H_
