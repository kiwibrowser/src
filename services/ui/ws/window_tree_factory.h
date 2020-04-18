// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_TREE_FACTORY_H_
#define SERVICES_UI_WS_WINDOW_TREE_FACTORY_H_

#include "base/macros.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"

namespace ui {
namespace ws {

class WindowServer;

class WindowTreeFactory : public ui::mojom::WindowTreeFactory {
 public:
  WindowTreeFactory(WindowServer* window_server,
                    const std::string& client_name);
  ~WindowTreeFactory() override;

 private:
  // ui::mojom::WindowTreeFactory:
  void CreateWindowTree(mojo::InterfaceRequest<mojom::WindowTree> tree_request,
                        mojom::WindowTreeClientPtr client) override;

  WindowServer* window_server_;
  const std::string client_name_;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeFactory);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_TREE_FACTORY_H_
