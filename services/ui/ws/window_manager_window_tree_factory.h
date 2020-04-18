// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_MANAGER_WINDOW_TREE_FACTORY_H_
#define SERVICES_UI_WS_WINDOW_MANAGER_WINDOW_TREE_FACTORY_H_

#include <stdint.h>

#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/window_manager_window_tree_factory.mojom.h"

namespace ui {
namespace ws {

class WindowManagerWindowTreeFactoryObserver;
class WindowServer;
class WindowTree;

// Implementation of mojom::WindowManagerWindowTreeFactory.
class WindowManagerWindowTreeFactory
    : public mojom::WindowManagerWindowTreeFactory {
 public:
  explicit WindowManagerWindowTreeFactory(WindowServer* window_server);
  ~WindowManagerWindowTreeFactory() override;

  WindowTree* window_tree() { return window_tree_; }

  bool is_bound() const { return binding_.is_bound(); }

  void Bind(
      mojo::InterfaceRequest<mojom::WindowManagerWindowTreeFactory> request);

  void AddObserver(WindowManagerWindowTreeFactoryObserver* observer);
  void RemoveObserver(WindowManagerWindowTreeFactoryObserver* observer);

  void OnTreeDestroyed();

  // mojom::WindowManagerWindowTreeFactory:
  void CreateWindowTree(mojom::WindowTreeRequest window_tree_request,
                        mojom::WindowTreeClientPtr window_tree_client,
                        bool window_manager_creates_roots) override;

 private:
  void SetWindowTree(WindowTree* window_tree);

  WindowServer* window_server_;
  mojo::Binding<mojom::WindowManagerWindowTreeFactory> binding_;

  // Owned by WindowServer.
  WindowTree* window_tree_ = nullptr;

  base::ObserverList<WindowManagerWindowTreeFactoryObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(WindowManagerWindowTreeFactory);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_MANAGER_WINDOW_TREE_FACTORY_H_
