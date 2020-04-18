// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/embedding.h"

#include <utility>

#include "base/bind.h"
#include "services/ui/ws2/window_service.h"
#include "services/ui/ws2/window_service_client.h"
#include "ui/aura/window.h"

namespace ui {
namespace ws2 {

Embedding::Embedding(WindowServiceClient* embedding_client,
                     aura::Window* window)
    : embedding_client_(embedding_client), window_(window) {
  DCHECK(embedding_client_);
  DCHECK(window_);
}

Embedding::~Embedding() = default;

void Embedding::Init(WindowService* window_service,
                     mojom::WindowTreeClientPtr window_tree_client_ptr,
                     mojom::WindowTreeClient* window_tree_client,
                     bool intercepts_events,
                     base::OnceClosure connection_lost_callback) {
  binding_.InitForEmbed(window_service, std::move(window_tree_client_ptr),
                        window_tree_client, intercepts_events, window_,
                        std::move(connection_lost_callback));
}

}  // namespace ws2
}  // namespace ui
