// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/embed_root.h"

#include "base/auto_reset.h"
#include "base/bind.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/mus/embed_root_delegate.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/aura/window_tracker.h"
#include "ui/aura/window_tree_host.h"

namespace aura {
namespace {

// FocusClient implementation used for embedded windows. This has minimal
// checks as to what can get focus.
class EmbeddedFocusClient : public client::FocusClient, public WindowObserver {
 public:
  explicit EmbeddedFocusClient(Window* root) : root_(root) {
    client::SetFocusClient(root, this);
  }

  ~EmbeddedFocusClient() override {
    client::SetFocusClient(root_, nullptr);
    if (focused_window_)
      focused_window_->RemoveObserver(this);
  }

  // client::FocusClient:
  void AddObserver(client::FocusChangeObserver* observer) override {
    observers_.AddObserver(observer);
  }
  void RemoveObserver(client::FocusChangeObserver* observer) override {
    observers_.RemoveObserver(observer);
  }
  void FocusWindow(Window* window) override {
    if (IsValidWindowForFocus(window) && window != GetFocusedWindow())
      FocusWindowImpl(window);
  }
  void ResetFocusWithinActiveWindow(Window* window) override {
    // This is never called in the embedding case.
    NOTREACHED();
  }
  Window* GetFocusedWindow() override { return focused_window_; }

 private:
  bool IsValidWindowForFocus(Window* window) const {
    return !window || (root_->Contains(window) && window->CanFocus());
  }

  void FocusWindowImpl(Window* window) {
    Window* previously_focused_window = focused_window_;

    if (previously_focused_window)
      previously_focused_window->RemoveObserver(this);
    focused_window_ = window;
    if (focused_window_)
      focused_window_->AddObserver(this);

    WindowTracker window_tracker;
    if (previously_focused_window)
      window_tracker.Add(previously_focused_window);
    for (auto& observer : observers_) {
      observer.OnWindowFocused(
          focused_window_, window_tracker.Contains(previously_focused_window)
                               ? previously_focused_window
                               : nullptr);
    }
  }

  // WindowObserver:
  void OnWindowDestroying(Window* window) override {
    DCHECK_EQ(window, focused_window_);
  }

  // Root of the hierarchy this is the FocusClient for.
  Window* const root_;

  Window* focused_window_ = nullptr;

  base::ObserverList<client::FocusChangeObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(EmbeddedFocusClient);
};

}  // namespace

EmbedRoot::~EmbedRoot() {
  window_tree_client_->OnEmbedRootDestroyed(this);
  // Makes use of window_tree_host_->window(), so needs to be destroyed before
  // |window_tree_host_|.
  focus_client_.reset();
}

Window* EmbedRoot::window() {
  return window_tree_host_ ? window_tree_host_->window() : nullptr;
}

EmbedRoot::EmbedRoot(WindowTreeClient* window_tree_client,
                     EmbedRootDelegate* delegate,
                     ui::ClientSpecificId window_id)
    : window_tree_client_(window_tree_client),
      delegate_(delegate),
      weak_factory_(this) {
  window_tree_client_->tree_->ScheduleEmbedForExistingClient(
      window_id, base::BindOnce(&EmbedRoot::OnScheduledEmbedForExistingClient,
                                weak_factory_.GetWeakPtr()));
}

void EmbedRoot::OnScheduledEmbedForExistingClient(
    const base::UnguessableToken& token) {
  token_ = token;
  delegate_->OnEmbedTokenAvailable(token);
}

void EmbedRoot::OnEmbed(std::unique_ptr<WindowTreeHost> window_tree_host) {
  focus_client_ =
      std::make_unique<EmbeddedFocusClient>(window_tree_host->window());
  window_tree_host_ = std::move(window_tree_host);
  delegate_->OnEmbed(window());
}

void EmbedRoot::OnUnembed() {
  delegate_->OnUnembed();
}

}  // namespace aura
