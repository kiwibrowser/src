// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_DISPLAY_BINDING_H_
#define SERVICES_UI_WS_DISPLAY_BINDING_H_

#include <memory>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/public/interfaces/window_tree_host.mojom.h"

namespace ui {
namespace ws {

class Display;
class ServerWindow;
class WindowServer;
class WindowTree;

// DisplayBinding manages the binding between a Display and it's mojo clients.
// DisplayBinding is used when a Display is created via a
// WindowTreeHostFactory.
//
// DisplayBinding is owned by Display.
class DisplayBinding {
 public:
  virtual ~DisplayBinding() {}

  virtual WindowTree* CreateWindowTree(ServerWindow* root) = 0;
};

// Live implementation of DisplayBinding.
class DisplayBindingImpl : public DisplayBinding {
 public:
  DisplayBindingImpl(mojom::WindowTreeHostRequest request,
                     Display* display,
                     mojom::WindowTreeClientPtr client,
                     WindowServer* window_server);
  ~DisplayBindingImpl() override;

 private:
  // DisplayBinding:
  WindowTree* CreateWindowTree(ServerWindow* root) override;

  WindowServer* window_server_;
  mojo::Binding<mojom::WindowTreeHost> binding_;
  mojom::WindowTreeClientPtr client_;

  DISALLOW_COPY_AND_ASSIGN(DisplayBindingImpl);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_DISPLAY_BINDING_H_
