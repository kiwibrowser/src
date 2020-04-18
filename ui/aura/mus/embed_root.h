// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_EMBED_ROOT_H_
#define UI_AURA_MUS_EMBED_ROOT_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "services/ui/common/types.h"
#include "ui/aura/aura_export.h"

namespace aura {

class EmbedRootDelegate;
class Window;
class WindowTreeClient;
class WindowTreeHost;

namespace client {
class FocusClient;
}

// EmbedRoot represents a secondary embedding from the perspective of the
// embedded client. More specifically an EmbedRoot allows a remote client to
// embed this client in one of the remote client's Windows.
//
// See EmbedRootDelegate for details on how to use.
//
// EmbedRoot is created by way of WindowTreeClient::CreateEmbedRoot().
class AURA_EXPORT EmbedRoot {
 public:
  ~EmbedRoot();

  // Token for the embedding. Empty until OnEmbedTokenAvailable() is called on
  // the delegate.
  const base::UnguessableToken& token() const { return token_; }

  // Window for the embedding, null until the OnEmbed() is called on the
  // delegate.
  aura::Window* window();

 private:
  friend class WindowTreeClient;
  friend class WindowTreeClientPrivate;

  EmbedRoot(WindowTreeClient* window_tree_client,
            EmbedRootDelegate* delegate,
            ui::ClientSpecificId window_id);

  // Callback from WindowTreeClient once the token has been determined.
  void OnScheduledEmbedForExistingClient(const base::UnguessableToken& token);

  // Called from WindowTreeClient when the embedding is established.
  void OnEmbed(std::unique_ptr<WindowTreeHost> window_tree_host);

  // Called from WindowTreeClient when unembedded from the Window.
  void OnUnembed();

  WindowTreeClient* window_tree_client_;

  EmbedRootDelegate* delegate_;

  base::UnguessableToken token_;

  std::unique_ptr<client::FocusClient> focus_client_;

  std::unique_ptr<WindowTreeHost> window_tree_host_;

  base::WeakPtrFactory<EmbedRoot> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(EmbedRoot);
};

}  // namespace aura

#endif  // UI_AURA_MUS_EMBED_ROOT_H_
