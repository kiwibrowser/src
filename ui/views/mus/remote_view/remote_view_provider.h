// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_REMOTE_VIEW_REMOTE_VIEW_PROVIDER_H_
#define UI_VIEWS_MUS_REMOTE_VIEW_REMOTE_VIEW_PROVIDER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/unguessable_token.h"
#include "ui/aura/mus/embed_root_delegate.h"

namespace aura {
class EmbedRoot;
class Window;
class WindowTreeClient;
}  // namespace aura

namespace gfx {
class Size;
}

namespace views {

namespace test {
class RemoteViewProviderTestApi;
}

// Creates an EmbedRoot for an embedded aura::Window on the client side and
// updates the embedded window size when the embedder changes size. For example,
// allows app list in ash to embed web-based "answer cards" that are rendered
// by the browser. Note this works only with mus.
class RemoteViewProvider : public aura::EmbedRootDelegate {
 public:
  explicit RemoteViewProvider(aura::Window* embedded);
  ~RemoteViewProvider() override;

  static void SetWindowTreeClientForTest(aura::WindowTreeClient* tree_client);

  // Gets the embed token. An embed token is obtained from one of WindowTree's
  // schedule embed calls (ScheduleEmbed or ScheduleEmbedForExistingClient). An
  // embedder calls EmbedUsingToken using the token to embed desired contents.
  // The embed token here is from an aura::EmbedRoot that uses
  // ScheduleEmbedForExistingClient for embedding part of an existing
  // WindowTreeClient. |callback| is invoked when the token is available.
  using GetEmbedTokenCallback =
      base::OnceCallback<void(const base::UnguessableToken& token)>;
  void GetEmbedToken(GetEmbedTokenCallback callback);

  // Optional callbacks to be invoked when embed or unembed happens.
  using OnEmbedCallback = base::RepeatingCallback<void(aura::Window* embedder)>;
  using OnUnembedCallback = base::RepeatingClosure;
  void SetCallbacks(const OnEmbedCallback& on_embed,
                    const OnUnembedCallback& on_unembed);

 private:
  friend class test::RemoteViewProviderTestApi;

  class EmbeddedWindowObserver;
  class EmbeddingWindowObserver;

  // Invoked when |embedded_| is destroyed.
  void OnEmbeddedWindowDestroyed();

  // Invoked when embedder changes size.
  void OnEmbeddingWindowResized(const gfx::Size& size);

  // aura::EmbedRootDelegate:
  void OnEmbedTokenAvailable(const base::UnguessableToken& token) override;
  void OnEmbed(aura::Window* window) override;
  void OnUnembed() override;

  // An aura::Window to be embedded. Not owned.
  aura::Window* embedded_;

  std::unique_ptr<aura::EmbedRoot> embed_root_;
  GetEmbedTokenCallback get_embed_token_callback_;
  OnEmbedCallback on_embed_callback_;
  OnUnembedCallback on_unembed_callback_;

  // An aura::WindowTreeClient for test. Use RemoteViewProviderTestApi to set
  // it.
  static aura::WindowTreeClient* window_tree_client_for_test;

  // Observes the |embedded_| and clears it when it is gone.
  std::unique_ptr<EmbeddedWindowObserver> embedded_window_observer_;

  // Observes the embeddding window provided by embedder.
  std::unique_ptr<EmbeddingWindowObserver> embedding_window_observer_;

  DISALLOW_COPY_AND_ASSIGN(RemoteViewProvider);
};

}  // namespace views

#endif  // UI_VIEWS_MUS_REMOTE_VIEW_REMOTE_VIEW_PROVIDER_H_
