// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_MUS_TYPES_H_
#define UI_AURA_MUS_MUS_TYPES_H_

#include <stdint.h>

#include "services/ui/common/types.h"

// Typedefs for the transport types. These typedefs match that of the mojom
// file, see it for specifics.

namespace aura {

constexpr ui::Id kInvalidServerId = 0;

enum class WindowMusType {
  // The window is an embed root in the embedded client. That is, the client
  // received this window by way of another client calling Embed(). In other
  // words, this is the embedded side of an embedding.
  // NOTE: in the client that called Embed() the window type is LOCAL (or
  // EMBED_IN_OWNER).
  // TODO(sky): ensure when Embed() is called type is always set to
  // EMBED_IN_OWNER, and if the embedding is removed it goes back to LOCAL.
  // https://crbug.com/834487
  EMBED,

  // Embed() was called on the window by the local client. In other words, this
  // is the embedder side of an embedding.
  EMBED_IN_OWNER,

  // The window was created by requesting a top level
  // (WindowTree::NewTopLevel()).
  // NOTE: in the window manager (the one responsible for actually creating the
  // real window) the window is of type TOP_LEVEL_IN_WM.
  TOP_LEVEL,

  // The window is a top level window in the window manager.
  // TODO(sky): this should be removed when --mash goes away.
  // https://crbug.com/842365.
  TOP_LEVEL_IN_WM,

  // The window is a display root for the window manager and was automatically
  // created by mus.
  // TODO(sky): this should be removed when --mash goes away.
  // https://crbug.com/842365.
  DISPLAY_AUTOMATICALLY_CREATED,

  // The window is a display root for the window manager and was manually
  // created.
  // TODO(sky): this should be removed when --mash goes away.
  // https://crbug.com/842365.
  DISPLAY_MANUALLY_CREATED,

  // The window was created locally.
  LOCAL,

  // Not one of the above. This means the window is visible to the client and
  // not one of the above values. Practically this means this client is the
  // window manager and the window was created by another client.
  OTHER,
};

}  // namespace ui

#endif  // UI_AURA_MUS_MUS_TYPES_H_
