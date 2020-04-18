// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_TREE_FACTORY_H_
#define SERVICES_UI_WS2_WINDOW_TREE_FACTORY_H_

#include <memory>
#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"

namespace ui {
namespace ws2 {

class WindowService;
class WindowServiceClientBinding;

// Implementation of mojom::WindowTreeFactory. This creates a
// WindowServiceClientBinding for each request for a WindowTree. Any
// WindowServiceClientBindings created by WindowTreeFactory are owned by the
// WindowTreeFactory.
class COMPONENT_EXPORT(WINDOW_SERVICE) WindowTreeFactory
    : public mojom::WindowTreeFactory {
 public:
  explicit WindowTreeFactory(WindowService* window_service);
  ~WindowTreeFactory() override;

  void AddBinding(mojom::WindowTreeFactoryRequest request);

  // mojom::WindowTreeFactory:
  void CreateWindowTree(mojom::WindowTreeRequest tree_request,
                        mojom::WindowTreeClientPtr client) override;

 private:
  void OnLostConnectionToClient(WindowServiceClientBinding* binding);

  using WindowServiceClientBindings =
      std::vector<std::unique_ptr<WindowServiceClientBinding>>;

  WindowService* window_service_;
  mojo::BindingSet<mojom::WindowTreeFactory> bindings_;
  WindowServiceClientBindings window_service_client_bindings_;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeFactory);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_TREE_FACTORY_H_
