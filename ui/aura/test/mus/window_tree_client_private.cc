// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/mus/window_tree_client_private.h"

#include "base/unguessable_token.h"
#include "ui/aura/mus/embed_root.h"
#include "ui/aura/mus/in_flight_change.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/mus/window_tree_host_mus_init_params.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"

namespace aura {
namespace {

constexpr int64_t kDisplayId = 1;

}  // namespace

WindowTreeClientPrivate::WindowTreeClientPrivate(
    WindowTreeClient* tree_client_impl)
    : tree_client_impl_(tree_client_impl) {}

WindowTreeClientPrivate::WindowTreeClientPrivate(Window* window)
    : WindowTreeClientPrivate(WindowPortMus::Get(window)->window_tree_client_) {
}

WindowTreeClientPrivate::~WindowTreeClientPrivate() {}

// static
std::unique_ptr<WindowTreeClient>
WindowTreeClientPrivate::CreateWindowTreeClient(
    WindowTreeClientDelegate* window_tree_delegate,
    WindowManagerDelegate* window_manager_delegate,
    WindowTreeClient::Config config) {
  std::unique_ptr<WindowTreeClient> wtc(new WindowTreeClient(
      nullptr, window_tree_delegate, window_manager_delegate, nullptr, nullptr,
      false, config));
  return wtc;
}

void WindowTreeClientPrivate::OnEmbed(ui::mojom::WindowTree* window_tree) {
  const ui::Id focused_window_id = 0;
  tree_client_impl_->OnEmbedImpl(window_tree, CreateWindowDataForEmbed(),
                                 kDisplayId, focused_window_id, true,
                                 base::nullopt);
}

WindowTreeHostMus* WindowTreeClientPrivate::CallWmNewDisplayAdded(
    const display::Display& display) {
  ui::mojom::WindowDataPtr root_data(ui::mojom::WindowData::New());
  root_data->parent_id = 0;
  // Windows representing displays are owned by mus, which is identified by
  // non-zero high word.
  root_data->window_id = next_window_id_++ | 0x00010000;
  root_data->visible = true;
  root_data->bounds = gfx::Rect(display.bounds().size());
  const bool parent_drawn = true;
  return CallWmNewDisplayAdded(display, std::move(root_data), parent_drawn);
}

WindowTreeHostMus* WindowTreeClientPrivate::CallWmNewDisplayAdded(
    const display::Display& display,
    ui::mojom::WindowDataPtr root_data,
    bool parent_drawn) {
  return tree_client_impl_->WmNewDisplayAddedImpl(display, std::move(root_data),
                                                  parent_drawn, base::nullopt);
}

void WindowTreeClientPrivate::CallOnPointerEventObserved(
    Window* window,
    std::unique_ptr<ui::Event> event) {
  const int64_t display_id = 0;
  const uint32_t window_id =
      window ? WindowPortMus::Get(window)->server_id() : 0u;
  tree_client_impl_->OnPointerEventObserved(std::move(event), window_id,
                                            display_id);
}

void WindowTreeClientPrivate::CallOnCaptureChanged(Window* new_capture,
                                                   Window* old_capture) {
  tree_client_impl_->OnCaptureChanged(
      new_capture ? WindowPortMus::Get(new_capture)->server_id() : 0,
      old_capture ? WindowPortMus::Get(old_capture)->server_id() : 0);
}

void WindowTreeClientPrivate::CallOnConnect() {
  tree_client_impl_->OnConnect();
}

void WindowTreeClientPrivate::CallOnEmbedFromToken(EmbedRoot* embed_root) {
  embed_root->OnScheduledEmbedForExistingClient(
      base::UnguessableToken::Create());
  tree_client_impl_->OnEmbedFromToken(embed_root->token(),
                                      CreateWindowDataForEmbed(), kDisplayId,
                                      base::Optional<viz::LocalSurfaceId>());
}

WindowTreeHostMusInitParams
WindowTreeClientPrivate::CallCreateInitParamsForNewDisplay() {
  return tree_client_impl_->CreateInitParamsForNewDisplay();
}

void WindowTreeClientPrivate::SetTree(ui::mojom::WindowTree* window_tree) {
  tree_client_impl_->WindowTreeConnectionEstablished(window_tree);
}

void WindowTreeClientPrivate::SetWindowManagerClient(
    ui::mojom::WindowManagerClient* client) {
  tree_client_impl_->window_manager_client_ = client;
  // Mirrors what CreateForWindowManager() does.
  tree_client_impl_->CreatePlatformEventSourceIfNecessary();
}

bool WindowTreeClientPrivate::HasPointerWatcher() {
  return tree_client_impl_->has_pointer_watcher_;
}

Window* WindowTreeClientPrivate::GetWindowByServerId(ui::Id id) {
  WindowMus* window = tree_client_impl_->GetWindowByServerId(id);
  return window ? window->GetWindow() : nullptr;
}

WindowMus* WindowTreeClientPrivate::NewWindowFromWindowData(
    WindowMus* parent,
    const ui::mojom::WindowData& window_data) {
  return tree_client_impl_->NewWindowFromWindowData(parent, window_data);
}

bool WindowTreeClientPrivate::HasInFlightChanges() {
  return !tree_client_impl_->in_flight_map_.empty();
}

bool WindowTreeClientPrivate::HasChangeInFlightOfType(ChangeType type) {
  for (auto& pair : tree_client_impl_->in_flight_map_) {
    if (pair.second->change_type() == type)
      return true;
  }
  return false;
}

void WindowTreeClientPrivate::WaitForInitialDisplays() {
  tree_client_impl_->WaitForInitialDisplays();
}

ui::mojom::WindowDataPtr WindowTreeClientPrivate::CreateWindowDataForEmbed() {
  ui::mojom::WindowDataPtr root_data(ui::mojom::WindowData::New());
  root_data->parent_id = 0;
  root_data->window_id = next_window_id_++;
  root_data->visible = true;
  return root_data;
}

}  // namespace aura
