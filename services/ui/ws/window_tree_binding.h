// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_TREE_BINDING_H_
#define SERVICES_UI_WS_WINDOW_TREE_BINDING_H_

#include <memory>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"

namespace ui {
namespace ws {

class WindowServer;
class WindowTree;

// WindowTreeBinding manages the binding between a WindowTree and its
// WindowTreeClient. WindowTreeBinding exists so that a mock implementation
// of WindowTreeClient can be injected for tests. WindowTree owns its
// associated WindowTreeBinding.
class WindowTreeBinding {
 public:
  explicit WindowTreeBinding(mojom::WindowTreeClient* client);
  virtual ~WindowTreeBinding();

  mojom::WindowTreeClient* client() { return client_; }

  // Obtains a new WindowManager. This should only be called once.
  virtual mojom::WindowManager* GetWindowManager() = 0;

  virtual void SetIncomingMethodCallProcessingPaused(bool paused) = 0;

  // Called when the WindowServer is destroyed. Sets |client_| to
  // CreateClientForShutdown().
  void ResetClientForShutdown();

 protected:
  virtual mojom::WindowTreeClient* CreateClientForShutdown() = 0;

 private:
  mojom::WindowTreeClient* client_;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeBinding);
};

// Bindings implementation of WindowTreeBinding.
class DefaultWindowTreeBinding : public WindowTreeBinding {
 public:
  DefaultWindowTreeBinding(WindowTree* tree,
                           WindowServer* window_server,
                           mojom::WindowTreeRequest service_request,
                           mojom::WindowTreeClientPtr client);
  DefaultWindowTreeBinding(WindowTree* tree,
                           mojom::WindowTreeClientPtr client);
  ~DefaultWindowTreeBinding() override;

  // Use when created with the constructor that does not take a
  // WindowTreeRequest.
  mojom::WindowTreePtr CreateInterfacePtrAndBind();

  // WindowTreeBinding:
  mojom::WindowManager* GetWindowManager() override;
  void SetIncomingMethodCallProcessingPaused(bool paused) override;

 protected:
  // WindowTreeBinding:
  mojom::WindowTreeClient* CreateClientForShutdown() override;

 private:
  mojo::Binding<mojom::WindowTree> binding_;
  mojom::WindowTreeClientPtr client_;
  mojom::WindowManagerAssociatedPtr window_manager_internal_;

  DISALLOW_COPY_AND_ASSIGN(DefaultWindowTreeBinding);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_TREE_BINDING_H_
