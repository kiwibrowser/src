// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_EMBEDDING_H_
#define SERVICES_UI_WS2_EMBEDDING_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/component_export.h"
#include "base/macros.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws2/window_service_client_binding.h"

namespace aura {
class Window;
}

namespace ui {
namespace ws2 {

class WindowService;
class WindowServiceClient;

// Embedding is created any time a client calls Embed() (Embedding is not
// created for top-levels). Embedding owns the embedded WindowServiceClient
// (by way of owning WindowServiceClientBinding). Embedding is owned by the
// Window associated with the embedding.
class COMPONENT_EXPORT(WINDOW_SERVICE) Embedding {
 public:
  Embedding(WindowServiceClient* embedding_client, aura::Window* window);
  ~Embedding();

  void Init(WindowService* window_service,
            mojom::WindowTreeClientPtr window_tree_client_ptr,
            mojom::WindowTreeClient* window_tree_client,
            bool intercepts_events,
            base::OnceClosure connection_lost_callback);

  WindowServiceClient* embedding_client() { return embedding_client_; }

  WindowServiceClient* embedded_client() {
    return binding_.window_service_client();
  }

  aura::Window* window() { return window_; }

 private:
  // The client that initiated the embedding.
  WindowServiceClient* embedding_client_;

  // The window the embedding is in.
  aura::Window* window_;

  WindowServiceClientBinding binding_;

  DISALLOW_COPY_AND_ASSIGN(Embedding);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_EMBEDDING_H_
