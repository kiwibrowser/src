// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_service_client_test_helper.h"

#include "services/ui/ws2/client_window.h"
#include "services/ui/ws2/window_service_client.h"
#include "services/ui/ws2/window_service_client_binding.h"

namespace ui {
namespace ws2 {

WindowServiceClientTestHelper::WindowServiceClientTestHelper(
    WindowServiceClient* window_service_client)
    : window_service_client_(window_service_client) {}

WindowServiceClientTestHelper::~WindowServiceClientTestHelper() = default;

mojom::WindowTree* WindowServiceClientTestHelper::window_tree() {
  return static_cast<mojom::WindowTree*>(window_service_client_);
}

mojom::WindowDataPtr WindowServiceClientTestHelper::WindowToWindowData(
    aura::Window* window) {
  return window_service_client_->WindowToWindowData(window);
}

aura::Window* WindowServiceClientTestHelper::NewWindow(
    Id transport_window_id,
    base::flat_map<std::string, std::vector<uint8_t>> properties) {
  const uint32_t change_id = 1u;
  window_service_client_->NewWindow(change_id, transport_window_id, properties);
  return window_service_client_->GetWindowByClientId(
      window_service_client_->MakeClientWindowId(transport_window_id));
}

void WindowServiceClientTestHelper::DeleteWindow(aura::Window* window) {
  const int change_id = 1u;
  window_service_client_->DeleteWindow(change_id, TransportIdForWindow(window));
}

aura::Window* WindowServiceClientTestHelper::NewTopLevelWindow(
    Id transport_window_id,
    base::flat_map<std::string, std::vector<uint8_t>> properties) {
  const uint32_t change_id = 1u;
  window_service_client_->NewTopLevelWindow(change_id, transport_window_id,
                                            properties);
  return window_service_client_->GetWindowByClientId(
      window_service_client_->MakeClientWindowId(transport_window_id));
}

bool WindowServiceClientTestHelper::SetCapture(aura::Window* window) {
  return window_service_client_->SetCaptureImpl(
      ClientWindowIdForWindow(window));
}

bool WindowServiceClientTestHelper::ReleaseCapture(aura::Window* window) {
  return window_service_client_->ReleaseCaptureImpl(
      ClientWindowIdForWindow(window));
}

bool WindowServiceClientTestHelper::SetWindowBounds(aura::Window* window,
                                                    const gfx::Rect& bounds) {
  base::Optional<viz::LocalSurfaceId> local_surface_id;
  return window_service_client_->SetWindowBoundsImpl(
      ClientWindowIdForWindow(window), bounds, local_surface_id);
}

void WindowServiceClientTestHelper::SetWindowBoundsWithAck(
    aura::Window* window,
    const gfx::Rect& bounds,
    uint32_t change_id) {
  base::Optional<viz::LocalSurfaceId> local_surface_id;
  window_service_client_->SetWindowBounds(
      change_id, TransportIdForWindow(window), bounds, local_surface_id);
}

void WindowServiceClientTestHelper::SetClientArea(
    aura::Window* window,
    const gfx::Insets& insets,
    base::Optional<std::vector<gfx::Rect>> additional_client_areas) {
  window_service_client_->SetClientArea(TransportIdForWindow(window), insets,
                                        additional_client_areas);
}

void WindowServiceClientTestHelper::SetWindowProperty(
    aura::Window* window,
    const std::string& name,
    const std::vector<uint8_t>& value,
    uint32_t change_id) {
  window_service_client_->SetWindowProperty(
      change_id, TransportIdForWindow(window), name, value);
}

Embedding* WindowServiceClientTestHelper::Embed(
    aura::Window* window,
    mojom::WindowTreeClientPtr client_ptr,
    mojom::WindowTreeClient* client,
    uint32_t embed_flags) {
  if (!window_service_client_->EmbedImpl(
          window_service_client_->MakeClientWindowId(
              TransportIdForWindow(window)),
          std::move(client_ptr), client, embed_flags)) {
    return nullptr;
  }
  return ClientWindow::GetMayBeNull(window)->embedding();
}

void WindowServiceClientTestHelper::SetEventTargetingPolicy(
    aura::Window* window,
    mojom::EventTargetingPolicy policy) {
  window_service_client_->SetEventTargetingPolicy(TransportIdForWindow(window),
                                                  policy);
}

Id WindowServiceClientTestHelper::TransportIdForWindow(aura::Window* window) {
  return window_service_client_->TransportIdForWindow(window);
}

bool WindowServiceClientTestHelper::SetFocus(aura::Window* window) {
  return window_service_client_->SetFocusImpl(
      window_service_client_->MakeClientWindowId(
          window_service_client_->TransportIdForWindow(window)));
}

void WindowServiceClientTestHelper::SetCanFocus(aura::Window* window,
                                                bool can_focus) {
  window_service_client_->SetCanFocus(
      window_service_client_->TransportIdForWindow(window), can_focus);
}

void WindowServiceClientTestHelper::DestroyEmbedding(Embedding* embedding) {
  // Triggers WindowServiceClient deleting the Embedding.
  window_service_client_->OnEmbeddedClientConnectionLost(embedding);
}

ClientWindowId WindowServiceClientTestHelper::ClientWindowIdForWindow(
    aura::Window* window) {
  return window_service_client_->MakeClientWindowId(
      TransportIdForWindow(window));
}

}  // namespace ws2
}  // namespace ui
