// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_CLIENT_ROOT_H_
#define SERVICES_UI_WS2_CLIENT_ROOT_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "components/viz/common/surfaces/local_surface_id.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "ui/aura/window_observer.h"
#include "ui/gfx/geometry/size.h"

namespace aura {
class ClientSurfaceEmbedder;
class Window;
}  // namespace aura

namespace ui {
namespace ws2 {

class WindowHostFrameSinkClient;
class WindowServiceClient;

// WindowServiceClient creates a ClientRoot for each window the client is
// embedded in. A ClientRoot is created as the result of another client
// using Embed(), or this client requesting a top-level window. ClientRoot is
// responsible for maintaining state associated with the root, as well as
// notifying the client of any changes to windows parented to the root.
class COMPONENT_EXPORT(WINDOW_SERVICE) ClientRoot
    : public aura::WindowObserver {
 public:
  ClientRoot(WindowServiceClient* window_service_client,
             aura::Window* window,
             bool is_top_level);
  ~ClientRoot() override;

  // Registers the necessary state needed for embedding in viz.
  void RegisterVizEmbeddingSupport();

  aura::Window* window() { return window_; }

  const viz::LocalSurfaceId& GetLocalSurfaceId();

  bool is_top_level() const { return is_top_level_; }

 private:
  void UpdatePrimarySurfaceId();

  // aura::WindowObserver:
  void OnWindowPropertyChanged(aura::Window* window,
                               const void* key,
                               intptr_t old) override;
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;

 private:
  WindowServiceClient* window_service_client_;
  aura::Window* window_;
  const bool is_top_level_;
  gfx::Size last_surface_size_in_pixels_;
  viz::LocalSurfaceId local_surface_id_;
  viz::ParentLocalSurfaceIdAllocator parent_local_surface_id_allocator_;
  std::unique_ptr<aura::ClientSurfaceEmbedder> client_surface_embedder_;
  // viz::HostFrameSinkClient registered with the HostFrameSinkManager for the
  // window.
  std::unique_ptr<WindowHostFrameSinkClient> window_host_frame_sink_client_;

  DISALLOW_COPY_AND_ASSIGN(ClientRoot);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_CLIENT_ROOT_H_
