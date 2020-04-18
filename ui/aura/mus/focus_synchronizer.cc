// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/focus_synchronizer.h"

#include "base/auto_reset.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/mus/focus_synchronizer_delegate.h"
#include "ui/aura/mus/window_mus.h"
#include "ui/aura/window.h"

namespace aura {

FocusSynchronizer::FocusSynchronizer(FocusSynchronizerDelegate* delegate,
                                     ui::mojom::WindowTree* window_tree)
    : delegate_(delegate), window_tree_(window_tree) {}

FocusSynchronizer::~FocusSynchronizer() {
  SetActiveFocusClientInternal(nullptr);
  if (active_focus_client_root_)
    active_focus_client_root_->RemoveObserver(this);
}

void FocusSynchronizer::AddObserver(FocusSynchronizerObserver* observer) {
  observers_.AddObserver(observer);
}

void FocusSynchronizer::RemoveObserver(FocusSynchronizerObserver* observer) {
  observers_.RemoveObserver(observer);
}

void FocusSynchronizer::SetFocusFromServer(WindowMus* window) {
  if (focused_window_ == window)
    return;

  DCHECK(!setting_focus_);
  base::AutoReset<bool> focus_reset(&setting_focus_, true);
  base::AutoReset<WindowMus*> window_setting_focus_to_reset(
      &window_setting_focus_to_, window);
  if (window) {
    Window* root = window->GetWindow()->GetRootWindow();
    // The client should provide a focus client for all roots.
    DCHECK(client::GetFocusClient(root));
    if (is_singleton_focus_client_)
      DCHECK_EQ(active_focus_client_, client::GetFocusClient(root));
    else if (active_focus_client_root_ != root)
      SetActiveFocusClient(client::GetFocusClient(root), root);
    window->GetWindow()->Focus();
  } else if (active_focus_client_) {
    if (is_singleton_focus_client_)
      active_focus_client_->FocusWindow(nullptr);
    else
      SetActiveFocusClient(nullptr, nullptr);
  }
}

void FocusSynchronizer::OnFocusedWindowDestroyed() {
  focused_window_ = nullptr;
}

void FocusSynchronizer::SetSingletonFocusClient(
    client::FocusClient* focus_client) {
  SetActiveFocusClient(focus_client, nullptr);
  if (focus_client)
    is_singleton_focus_client_ = true;
}

void FocusSynchronizer::SetActiveFocusClient(client::FocusClient* focus_client,
                                             Window* focus_client_root) {
  if (focus_client == active_focus_client_ &&
      focus_client_root == active_focus_client_root_) {
    return;
  }

  is_singleton_focus_client_ = false;

  if (active_focus_client_root_)
    active_focus_client_root_->RemoveObserver(this);
  active_focus_client_root_ = focus_client_root;
  if (active_focus_client_root_)
    active_focus_client_root_->AddObserver(this);

  if (focus_client == active_focus_client_)
    return;

  OnActiveFocusClientChanged(focus_client, focus_client_root);
  for (FocusSynchronizerObserver& observer : observers_)
    observer.OnActiveFocusClientChanged(focus_client, focus_client_root);
}

void FocusSynchronizer::SetActiveFocusClientInternal(
    client::FocusClient* focus_client) {
  if (focus_client == active_focus_client_)
    return;

  if (active_focus_client_)
    active_focus_client_->RemoveObserver(this);
  active_focus_client_ = focus_client;
  if (active_focus_client_)
    active_focus_client_->AddObserver(this);
}

void FocusSynchronizer::SetFocusedWindow(WindowMus* window) {
  const uint32_t change_id = delegate_->CreateChangeIdForFocus(focused_window_);
  focused_window_ = window;
  window_tree_->SetFocus(change_id,
                         window ? window->server_id() : kInvalidServerId);
}

void FocusSynchronizer::OnActiveFocusClientChanged(
    client::FocusClient* focus_client,
    Window* focus_client_root) {
  SetActiveFocusClientInternal(focus_client);
  if (setting_focus_)
    return;

  if (focus_client) {
    Window* focused_window = focus_client->GetFocusedWindow();
    SetFocusedWindow(focused_window ? WindowMus::Get(focused_window)
                                    : WindowMus::Get(focus_client_root));
  } else {
    SetFocusedWindow(nullptr);
  }
}

void FocusSynchronizer::OnWindowFocused(Window* gained_focus,
                                        Window* lost_focus) {
  WindowMus* gained_focus_mus = WindowMus::Get(gained_focus);
  if (setting_focus_ && gained_focus_mus == window_setting_focus_to_) {
    focused_window_ = gained_focus_mus;
    return;
  }
  SetFocusedWindow(gained_focus_mus);
}

void FocusSynchronizer::OnWindowDestroying(Window* window) {
  DCHECK(!is_singleton_focus_client_);
  SetActiveFocusClient(nullptr, nullptr);
}

void FocusSynchronizer::OnWindowPropertyChanged(Window* window,
                                                const void* key,
                                                intptr_t old) {
  DCHECK(!is_singleton_focus_client_);
  if (key != client::kFocusClientKey)
    return;

  // Assume if the focus client changes the window is being destroyed.
  SetActiveFocusClient(nullptr, nullptr);
}

}  // namespace aura
