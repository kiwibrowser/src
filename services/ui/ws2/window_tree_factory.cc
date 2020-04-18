// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_tree_factory.h"

#include <stddef.h>

#include "base/bind.h"
#include "services/ui/ws2/window_service.h"
#include "services/ui/ws2/window_service_client_binding.h"

namespace ui {
namespace ws2 {

WindowTreeFactory::WindowTreeFactory(WindowService* window_service)
    : window_service_(window_service) {}

WindowTreeFactory::~WindowTreeFactory() = default;

void WindowTreeFactory::AddBinding(mojom::WindowTreeFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void WindowTreeFactory::CreateWindowTree(mojom::WindowTreeRequest tree_request,
                                         mojom::WindowTreeClientPtr client) {
  std::unique_ptr<WindowServiceClientBinding> binding =
      std::make_unique<WindowServiceClientBinding>();
  const bool intercepts_events = false;
  binding->InitFromFactory(
      window_service_, std::move(tree_request), std::move(client),
      intercepts_events,
      base::BindOnce(&WindowTreeFactory::OnLostConnectionToClient,
                     base::Unretained(this), binding.get()));
  window_service_client_bindings_.push_back(std::move(binding));
}

void WindowTreeFactory::OnLostConnectionToClient(
    WindowServiceClientBinding* binding) {
  for (auto iter = window_service_client_bindings_.begin();
       iter != window_service_client_bindings_.end(); ++iter) {
    if (iter->get() == binding) {
      window_service_client_bindings_.erase(iter);
      return;
    }
  }
  NOTREACHED();
}

}  // namespace ws2
}  // namespace ui
