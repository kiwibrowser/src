// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_SERVER_WINDOW_DELEGATE_H_
#define SERVICES_UI_WS_SERVER_WINDOW_DELEGATE_H_

#include <memory>

#include "services/ui/public/interfaces/mus_constants.mojom.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_manager.mojom.h"

namespace viz {
class HitTestQuery;
class HostFrameSinkClient;
}

namespace ui {

namespace ws {

class ServerWindow;

// A proxy to communicate to the viz host (usually viz::HostFrameSinkManager).
// If mus is hosting viz, then it forwards all the calls to the appropriate viz
// host. If mus is not hosting viz, then it drops all calls. It is always safe
// to call any method on this proxy. See documentation in
// viz::HostFrameSinkManager for the documentation for the methods.
class VizHostProxy {
 public:
  virtual ~VizHostProxy() {}

  virtual void RegisterFrameSinkId(const viz::FrameSinkId& frame_sink_id,
                                   viz::HostFrameSinkClient* client) = 0;

  virtual void SetFrameSinkDebugLabel(const viz::FrameSinkId& frame_sink_id,
                                      const std::string& name) = 0;

  virtual void InvalidateFrameSinkId(const viz::FrameSinkId& frame_sink_id) = 0;

  virtual void RegisterFrameSinkHierarchy(const viz::FrameSinkId& new_parent,
                                          const viz::FrameSinkId& child) = 0;
  virtual void UnregisterFrameSinkHierarchy(const viz::FrameSinkId& old_parent,
                                            const viz::FrameSinkId& child) = 0;

  virtual void CreateRootCompositorFrameSink(
      viz::mojom::RootCompositorFrameSinkParamsPtr params) = 0;

  virtual void CreateCompositorFrameSink(
      const viz::FrameSinkId& frame_sink_id,
      viz::mojom::CompositorFrameSinkRequest request,
      viz::mojom::CompositorFrameSinkClientPtr client) = 0;

  virtual viz::HitTestQuery* GetHitTestQuery(
      const viz::FrameSinkId& frame_sink_id) = 0;
};

class ServerWindowDelegate {
 public:
  // Returns a proxy to communicate to the viz host. This must always return
  // non-null.
  virtual VizHostProxy* GetVizHostProxy() = 0;

  // Returns the root of the window tree to which this |window| is attached.
  // Returns null if this window is not attached up through to a root window.
  // The returned root is used for drawn checks and may differ from that used
  // for event dispatch purposes.
  virtual ServerWindow* GetRootWindowForDrawn(const ServerWindow* window) = 0;

  // Called when a CompositorFrame with a new SurfaceId activates for the first
  // time for |window|.
  virtual void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info,
                                        ServerWindow* window) = 0;

 protected:
  virtual ~ServerWindowDelegate() {}
};

}  // namespace ws

}  // namespace ui

#endif  // SERVICES_UI_WS_SERVER_WINDOW_DELEGATE_H_
