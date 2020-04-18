// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MUS_MUS_EMBEDDED_FRAME_H_
#define CONTENT_RENDERER_MUS_MUS_EMBEDDED_FRAME_H_

#include <memory>

#include "base/macros.h"
#include "base/unguessable_token.h"
#include "components/viz/common/surfaces/local_surface_id.h"
#include "services/ui/common/types.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {
namespace mojom {
class WindowTree;
}
}  // namespace ui

namespace content {

class MusEmbeddedFrameDelegate;
class RendererWindowTreeClient;

// MusEmbeddedFrame represents an embedding of an OOPIF frame. Internally this
// creates a window in the window server and calls Embed().
class MusEmbeddedFrame {
 public:
  ~MusEmbeddedFrame();

  // Sets the bounds (in pixels) of the embedded frame.
  void SetWindowBounds(const viz::LocalSurfaceId& local_surface_id,
                       const gfx::Rect& bounds);

 private:
  friend class RendererWindowTreeClient;

  // Stores state that needs to pushed to the server once the connection has
  // been established (OnEmbed() is called).
  struct PendingState {
    PendingState();
    ~PendingState();

    base::UnguessableToken token;
    viz::LocalSurfaceId local_surface_id;
    gfx::Rect bounds;
    // True if SetWindowBounds() was called.
    bool was_set_window_bounds_called = false;
  };

  MusEmbeddedFrame(RendererWindowTreeClient* renderer_window_tree_client,
                   MusEmbeddedFrameDelegate* delegate,
                   ui::ClientSpecificId window_id,
                   const base::UnguessableToken& token);

  // Called once the WindowTree has been obtained. This is only called if
  // the MusEmbeddedFrame is created before the WindowTree has been obtained.
  void OnTreeAvailable();

  // Called when the WindowTree held by |renderer_window_tree_client_| is about
  // to change. This means the renderer was reembeded.
  void OnTreeWillChange();

  // Does the actual embedding.
  void CreateChildWindowAndEmbed(const base::UnguessableToken& token);

  uint32_t GetAndAdvanceNextChangeId();

  ui::mojom::WindowTree* window_tree();

  RendererWindowTreeClient* renderer_window_tree_client_;
  MusEmbeddedFrameDelegate* delegate_;
  const ui::ClientSpecificId window_id_;

  std::unique_ptr<PendingState> pending_state_;

  // If set, the WindowTree that state was created on has changed.
  bool tree_changed_ = false;

  DISALLOW_COPY_AND_ASSIGN(MusEmbeddedFrame);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MUS_MUS_EMBEDDED_FRAME_H_
