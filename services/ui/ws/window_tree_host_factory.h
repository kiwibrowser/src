// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_TREE_HOST_FACTORY_H_
#define SERVICES_UI_WS_WINDOW_TREE_HOST_FACTORY_H_

#include <stdint.h>

#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/ui/public/interfaces/window_tree_host_factory.mojom.h"

namespace ui {
namespace ws {

class WindowServer;

class WindowTreeHostFactory : public mojom::WindowTreeHostFactory {
 public:
  explicit WindowTreeHostFactory(WindowServer* window_server);
  ~WindowTreeHostFactory() override;

  void AddBinding(mojom::WindowTreeHostFactoryRequest request);

 private:
  // mojom::WindowTreeHostFactory implementation.
  void CreateWindowTreeHost(mojom::WindowTreeHostRequest host,
                            mojom::WindowTreeClientPtr tree_client) override;

  WindowServer* window_server_;
  mojo::BindingSet<mojom::WindowTreeHostFactory> bindings_;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeHostFactory);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_TREE_HOST_FACTORY_H_
