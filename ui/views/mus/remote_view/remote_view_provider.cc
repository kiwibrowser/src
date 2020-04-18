// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/remote_view/remote_view_provider.h"

#include <utility>

#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/scoped_observer.h"
#include "ui/aura/mus/embed_root.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/mus/mus_client.h"

namespace views {

// static
aura::WindowTreeClient* RemoteViewProvider::window_tree_client_for_test =
    nullptr;

// Observes a window and invokes a callback when the window is destroyed.
class RemoteViewProvider::EmbeddedWindowObserver : public aura::WindowObserver {
 public:
  EmbeddedWindowObserver(aura::Window* window, base::OnceClosure on_destroyed)
      : window_observer_(this), on_destroyed_(std::move(on_destroyed)) {
    window_observer_.Add(window);
  }
  ~EmbeddedWindowObserver() override = default;

  // aura::WindowObserver:
  void OnWindowDestroyed(aura::Window* window) override {
    DCHECK(!on_destroyed_.is_null());

    window_observer_.RemoveAll();
    base::ResetAndReturn(&on_destroyed_).Run();
  }

 private:
  ScopedObserver<aura::Window, EmbeddedWindowObserver> window_observer_;
  base::OnceClosure on_destroyed_;

  DISALLOW_COPY_AND_ASSIGN(EmbeddedWindowObserver);
};

// Observes a window and invokes a callback when the window size changes.
class RemoteViewProvider::EmbeddingWindowObserver
    : public aura::WindowObserver {
 public:
  using SizeChangedCallback = base::RepeatingCallback<void(const gfx::Size&)>;
  EmbeddingWindowObserver(aura::Window* window,
                          const SizeChangedCallback& callback)
      : window_observer_(this), on_size_changed_(callback) {
    window_observer_.Add(window);
  }
  ~EmbeddingWindowObserver() override = default;

  // aura::WindowObserver:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override {
    on_size_changed_.Run(new_bounds.size());
  }
  void OnWindowDestroyed(aura::Window* window) override {
    window_observer_.RemoveAll();
  }

 private:
  ScopedObserver<aura::Window, EmbeddingWindowObserver> window_observer_;
  SizeChangedCallback on_size_changed_;

  DISALLOW_COPY_AND_ASSIGN(EmbeddingWindowObserver);
};

RemoteViewProvider::RemoteViewProvider(aura::Window* embedded)
    : embedded_(embedded) {
  DCHECK(embedded_);
  embedded_window_observer_ = std::make_unique<EmbeddedWindowObserver>(
      embedded_, base::BindOnce(&RemoteViewProvider::OnEmbeddedWindowDestroyed,
                                base::Unretained(this)));
}

RemoteViewProvider::~RemoteViewProvider() = default;

void RemoteViewProvider::GetEmbedToken(GetEmbedTokenCallback callback) {
  DCHECK(embedded_);

  if (embed_root_) {
    std::move(callback).Run(embed_root_->token());
    return;
  }

  DCHECK(get_embed_token_callback_.is_null());
  get_embed_token_callback_ = std::move(callback);

  aura::WindowTreeClient* window_tree_client = window_tree_client_for_test;
  if (!window_tree_client) {
    DCHECK(views::MusClient::Exists());
    window_tree_client = views::MusClient::Get()->window_tree_client();
  }

  embed_root_ = window_tree_client->CreateEmbedRoot(this);
}

void RemoteViewProvider::SetCallbacks(const OnEmbedCallback& on_embed,
                                      const OnUnembedCallback& on_unembed) {
  on_embed_callback_ = on_embed;
  on_unembed_callback_ = on_unembed;
}

void RemoteViewProvider::OnEmbeddedWindowDestroyed() {
  embedded_ = nullptr;
}

void RemoteViewProvider::OnEmbeddingWindowResized(const gfx::Size& size) {
  // |embedded_| is owned by external code. Bail if it is destroyed while being
  // embedded.
  if (!embedded_)
    return;
  embedded_->SetBounds(gfx::Rect(size));
}

void RemoteViewProvider::OnEmbedTokenAvailable(
    const base::UnguessableToken& token) {
  if (get_embed_token_callback_)
    std::move(get_embed_token_callback_).Run(token);
}

void RemoteViewProvider::OnEmbed(aura::Window* window) {
  DCHECK(embedded_);

  embedding_window_observer_ = std::make_unique<EmbeddingWindowObserver>(
      window, base::BindRepeating(&RemoteViewProvider::OnEmbeddingWindowResized,
                                  base::Unretained(this)));
  OnEmbeddingWindowResized(window->bounds().size());
  window->AddChild(embedded_);

  if (on_embed_callback_)
    on_embed_callback_.Run(window);
}

void RemoteViewProvider::OnUnembed() {
  embedding_window_observer_.reset();
  embed_root_.reset();

  if (on_unembed_callback_)
    on_unembed_callback_.Run();
}

}  // namespace views
