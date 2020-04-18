// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SURFACE_TREE_HOST_H_
#define COMPONENTS_EXO_SURFACE_TREE_HOST_H_

#include <memory>

#include "base/macros.h"
#include "components/exo/layer_tree_frame_sink_holder.h"
#include "components/exo/surface.h"
#include "components/exo/surface_delegate.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "ui/gfx/geometry/rect.h"

namespace aura {
class Window;
}  // namespace aura

namespace gfx {
class Path;
}  // namespace gfx

namespace viz {
class BeginFrameSource;
}  // namespace viz

namespace exo {
class LayerTreeFrameSinkHolder;

// This class provides functionality for hosting a surface tree. The surface
// tree is hosted in the |host_window_|.
class SurfaceTreeHost : public SurfaceDelegate,
                        public viz::BeginFrameObserverBase,
                        public ui::ContextFactoryObserver {
 public:
  explicit SurfaceTreeHost(const std::string& window_name);
  ~SurfaceTreeHost() override;

  // Sets a root surface of a surface tree. This surface tree will be hosted in
  // the |host_window_|.
  void SetRootSurface(Surface* root_surface);

  // Returns false if the hit test region is empty.
  bool HasHitTestRegion() const;

  // Sets |mask| to the path that delineates the hit test region of the hosted
  // surface tree.
  void GetHitTestMask(gfx::Path* mask) const;

  // Call this to indicate that the previous CompositorFrame is processed and
  // the surface is being scheduled for a draw.
  void DidReceiveCompositorFrameAck();

  // Call this to indicate that the CompositorFrame with given
  // |presentation_token| has been first time presented to user.
  void DidPresentCompositorFrame(uint32_t presentation_token,
                                 base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags);

  // Call this to indicate that the CompositorFrame with given
  // |presentation_token| has been discard. It has not been and will not be
  // presented to user.
  void DidDiscardCompositorFrame(uint32_t presentation_token);

  // Called when the begin frame source has changed.
  void SetBeginFrameSource(viz::BeginFrameSource* begin_frame_source);

  // Adds/Removes begin frame observer based on state.
  void UpdateNeedsBeginFrame();

  aura::Window* host_window() { return host_window_.get(); }
  const aura::Window* host_window() const { return host_window_.get(); }

  Surface* root_surface() { return root_surface_; }
  const Surface* root_surface() const { return root_surface_; }

  const gfx::Point& root_surface_origin() const { return root_surface_origin_; }

  LayerTreeFrameSinkHolder* layer_tree_frame_sink_holder() {
    return layer_tree_frame_sink_holder_.get();
  }

  // Overridden from SurfaceDelegate:
  void OnSurfaceCommit() override;
  bool IsSurfaceSynchronized() const override;
  bool IsInputEnabled(Surface* surface) const override;
  void OnSetFrame(SurfaceFrameType type) override {}
  void OnSetFrameColors(SkColor active_color, SkColor inactive_color) override {
  }
  void OnSetParent(Surface* parent, const gfx::Point& position) override {}
  void OnSetStartupId(const char* startup_id) override {}
  void OnSetApplicationId(const char* application_id) override {}

  // Overridden from cc::BeginFrameObserverBase:
  bool OnBeginFrameDerivedImpl(const viz::BeginFrameArgs& args) override;
  void OnBeginFrameSourcePausedChanged(bool paused) override {}

  // Overridden from ui::ContextFactoryObserver:
  void OnLostResources() override;

 protected:
  // Call this to submit a compositor frame.
  void SubmitCompositorFrame();

 private:
  void UpdateHostWindowBounds();

  Surface* root_surface_ = nullptr;

  // Position of root surface relative to topmost, leftmost sub-surface. The
  // host window should be translated by the negation of this vector.
  gfx::Point root_surface_origin_;

  std::unique_ptr<aura::Window> host_window_;
  std::unique_ptr<LayerTreeFrameSinkHolder> layer_tree_frame_sink_holder_;

  // The begin frame source being observed.
  viz::BeginFrameSource* begin_frame_source_ = nullptr;
  bool needs_begin_frame_ = false;
  viz::BeginFrameAck current_begin_frame_ack_;

  // These lists contain the callbacks to notify the client when it is a good
  // time to start producing a new frame. These callbacks move to
  // |frame_callbacks_| when Commit() is called. Later they are moved to
  // |active_frame_callbacks_| when the effect of the Commit() is scheduled to
  // be drawn. They fire at the first begin frame notification after this.
  std::list<Surface::FrameCallback> frame_callbacks_;
  std::list<Surface::FrameCallback> active_frame_callbacks_;

  // These lists contains the callbacks to notify the client when surface
  // contents have been presented.
  using PresentationCallbacks = std::list<Surface::PresentationCallback>;
  PresentationCallbacks presentation_callbacks_;
  base::flat_map<uint32_t, PresentationCallbacks>
      active_presentation_callbacks_;

  uint32_t presentation_token_ = 0;

  DISALLOW_COPY_AND_ASSIGN(SurfaceTreeHost);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SURFACE_TREE_HOST_H_
