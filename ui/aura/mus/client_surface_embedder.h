// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_CLIENT_SURFACE_EMBEDDER_H_
#define UI_AURA_MUS_CLIENT_SURFACE_EMBEDDER_H_

#include <memory>

#include "base/macros.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "ui/aura/aura_export.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/insets.h"

namespace gfx {
class Insets;
}

namespace aura {

class Window;

// Used by WindowPortMus when it is embedding a client. Responsible for setting
// up layers containing content from the client, parenting them to the window's
// layer, and updating them when the client submits new surfaces.
class AURA_EXPORT ClientSurfaceEmbedder {
 public:
  // TODO(fsamuel): Insets might differ when the window is maximized. We should
  // deal with that case as well.
  ClientSurfaceEmbedder(Window* window,
                        bool inject_gutter,
                        const gfx::Insets& client_area_insets);
  ~ClientSurfaceEmbedder();

  // Updates the clip layer and primary SurfaceId of the surface layer based
  // on the provided |surface_id|.
  void SetPrimarySurfaceId(const viz::SurfaceId& surface_id);

  // Sets the fallback SurfaceInfo of the surface layer. The clip layer is not
  // updated.
  void SetFallbackSurfaceInfo(const viz::SurfaceInfo& surface_info);

  // Update the surface layer size and the right and bottom gutter layers for
  // the current window size.
  void UpdateSizeAndGutters();

  ui::Layer* RightGutterForTesting() { return right_gutter_.get(); }

  ui::Layer* BottomGutterForTesting() { return bottom_gutter_.get(); }

  const viz::SurfaceId& GetPrimarySurfaceIdForTesting() const;

 private:
  // The window which embeds the client.
  Window* window_;

  // Contains the client's content.
  std::unique_ptr<ui::Layer> surface_layer_;

  // Information describing the currently set fallback surface.
  viz::SurfaceInfo fallback_surface_info_;

  // Used for showing a gutter when the content is not available.
  std::unique_ptr<ui::Layer> right_gutter_;
  std::unique_ptr<ui::Layer> bottom_gutter_;

  bool inject_gutter_;
  gfx::Insets client_area_insets_;

  DISALLOW_COPY_AND_ASSIGN(ClientSurfaceEmbedder);
};

}  // namespace aura

#endif  // UI_AURA_MUS_CLIENT_SURFACE_EMBEDDER_H_
