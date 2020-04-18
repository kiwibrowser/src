// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_EMBED_ROOT_DELEGATE_H_
#define UI_AURA_MUS_EMBED_ROOT_DELEGATE_H_

#include "ui/aura/aura_export.h"

namespace base {
class UnguessableToken;
}

namespace aura {

class Window;

// Called from EmbedRoot at key points during the life-time of the embedding.
class AURA_EXPORT EmbedRootDelegate {
 public:
  // Called first in the process of establishing an embedding. |token| is used
  // by a remote client to embed this client in one of the remote client's
  // windows. The delegate will typically pass the supplied token over mojo and
  // the remote client will then call WindowTreeClient::EmbedUsingToken().
  virtual void OnEmbedTokenAvailable(const base::UnguessableToken& token) = 0;

  // Called once the embedding has been established. |window| is the root of
  // the embedding and owned by the remote client.
  virtual void OnEmbed(Window* window) = 0;

  // Called if the remote client embeds another client in the root. The delegate
  // will typically delete the EmbedRoot shortly after receiving this.
  virtual void OnUnembed() = 0;

 protected:
  virtual ~EmbedRootDelegate() {}
};

}  // namespace aura

#endif  // UI_AURA_MUS_EMBED_ROOT_DELEGATE_H_
