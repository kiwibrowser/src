// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/focus_handler.h"

#include "services/ui/ws2/client_change.h"
#include "services/ui/ws2/client_change_tracker.h"
#include "services/ui/ws2/client_window.h"
#include "services/ui/ws2/window_service.h"
#include "services/ui/ws2/window_service_client.h"
#include "ui/aura/client/focus_client.h"

namespace ui {
namespace ws2 {

FocusHandler::FocusHandler(WindowServiceClient* window_service_client)
    : window_service_client_(window_service_client) {
  window_service_client_->window_service_->focus_client()->AddObserver(this);
}

FocusHandler::~FocusHandler() {
  window_service_client_->window_service_->focus_client()->RemoveObserver(this);
}

bool FocusHandler::SetFocus(aura::Window* window) {
  if (window && !IsFocusableWindow(window)) {
    DVLOG(1) << "SetFocus failed (access denied or invalid window)";
    return false;
  }

  aura::client::FocusClient* focus_client =
      window_service_client_->window_service_->focus_client();
  ClientWindow* client_window = ClientWindow::GetMayBeNull(window);
  if (window == focus_client->GetFocusedWindow()) {
    if (!window)
      return true;

    if (client_window->focus_owner() != window_service_client_) {
      // The focused window didn't change, but the client that owns focus did
      // (see |ClientWindow::focus_owner_| for details on this). Notify the
      // current owner that it lost focus.
      if (client_window->focus_owner()) {
        client_window->focus_owner()->window_tree_client_->OnWindowFocused(
            kInvalidTransportId);
      }
      client_window->set_focus_owner(window_service_client_);
    }
    return true;
  }

  ClientChange change(window_service_client_->property_change_tracker_.get(),
                      window, ClientChangeType::kFocus);
  focus_client->FocusWindow(window);
  if (focus_client->GetFocusedWindow() != window)
    return false;

  if (client_window)
    client_window->set_focus_owner(window_service_client_);
  return true;
}

void FocusHandler::SetCanFocus(aura::Window* window, bool can_focus) {
  if (window && (window_service_client_->IsClientCreatedWindow(window) ||
                 window_service_client_->IsClientRootWindow(window))) {
    ClientWindow::GetMayBeNull(window)->set_can_focus(can_focus);
  } else {
    DVLOG(1) << "SetCanFocus failed (invalid or unknown window)";
  }
}

bool FocusHandler::IsFocusableWindow(aura::Window* window) const {
  if (!window)
    return true;  // Used to clear focus.

  if (!window->IsVisible() || !window->GetRootWindow())
    return false;  // The window must be drawn an in attached to a root.

  return (window_service_client_->IsClientCreatedWindow(window) ||
          window_service_client_->IsClientRootWindow(window));
}

bool FocusHandler::IsEmbeddedClient(ClientWindow* client_window) const {
  return client_window->embedded_window_service_client() ==
         window_service_client_;
}

bool FocusHandler::IsOwningClient(ClientWindow* client_window) const {
  return client_window->owning_window_service_client() ==
         window_service_client_;
}

void FocusHandler::OnWindowFocused(aura::Window* gained_focus,
                                   aura::Window* lost_focus) {
  ClientChangeTracker* change_tracker =
      window_service_client_->property_change_tracker_.get();
  if (change_tracker->IsProcessingChangeForWindow(lost_focus,
                                                  ClientChangeType::kFocus) ||
      change_tracker->IsProcessingChangeForWindow(gained_focus,
                                                  ClientChangeType::kFocus)) {
    // The client initiated the change, don't notify the client.
    return;
  }

  // The client did not request the focus change. Update state appropriately.
  // Prefer the embedded client over the owning client.
  bool notified_gained = false;
  if (gained_focus) {
    ClientWindow* client_window = ClientWindow::GetMayBeNull(gained_focus);
    if (client_window && (IsEmbeddedClient(client_window) ||
                          (!client_window->embedded_window_service_client() &&
                           IsOwningClient(client_window)))) {
      client_window->set_focus_owner(window_service_client_);
      window_service_client_->window_tree_client_->OnWindowFocused(
          window_service_client_->TransportIdForWindow(gained_focus));
      notified_gained = true;
    }
  }

  if (lost_focus && !notified_gained) {
    ClientWindow* client_window = ClientWindow::GetMayBeNull(lost_focus);
    if (client_window &&
        client_window->focus_owner() == window_service_client_) {
      client_window->set_focus_owner(nullptr);
      window_service_client_->window_tree_client_->OnWindowFocused(
          kInvalidTransportId);
    }
  }
}

}  // namespace ws2
}  // namespace ui
