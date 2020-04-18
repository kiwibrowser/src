// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_QUADS_COMPOSITOR_FRAME_METADATA_H_
#define COMPONENTS_VIZ_COMMON_QUADS_COMPOSITOR_FRAME_METADATA_H_

#include <stdint.h>

#include <vector>

#include "base/optional.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/quads/frame_deadline.h"
#include "components/viz/common/quads/selection.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/viz_common_export.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/geometry/vector2d_f.h"
#include "ui/gfx/selection_bound.h"
#include "ui/latency/latency_info.h"

namespace viz {

class VIZ_COMMON_EXPORT CompositorFrameMetadata {
 public:
  CompositorFrameMetadata();
  CompositorFrameMetadata(CompositorFrameMetadata&& other);
  ~CompositorFrameMetadata();

  CompositorFrameMetadata& operator=(CompositorFrameMetadata&& other);

  CompositorFrameMetadata Clone() const;

  // The device scale factor used to generate this compositor frame.
  float device_scale_factor = 0.f;

  // Scroll offset and scale of the root layer. This can be used for tasks
  // like positioning windowed plugins.
  gfx::Vector2dF root_scroll_offset;
  float page_scale_factor = 0.f;

  // These limits can be used together with the scroll/scale fields above to
  // determine if scrolling/scaling in a particular direction is possible.
  gfx::SizeF scrollable_viewport_size;
  gfx::SizeF root_layer_size;
  float min_page_scale_factor = 0.f;
  float max_page_scale_factor = 0.f;
  bool root_overflow_y_hidden = false;
  bool may_contain_video = false;

  // WebView makes quality decisions for rastering resourceless software frames
  // based on information that a scroll or animation is active.
  // TODO(aelias): Remove this and always enable filtering if there aren't apps
  // depending on this anymore.
  bool is_resourceless_software_draw_with_scroll_or_animation = false;

  // Used to position the Android location top bar and page content, whose
  // precise position is computed by the renderer compositor.
  float top_controls_height = 0.f;
  float top_controls_shown_ratio = 0.f;

  // Used to position Android bottom bar, whose position is computed by the
  // renderer compositor.
  float bottom_controls_height = 0.f;
  float bottom_controls_shown_ratio = 0.f;

  // This color is usually obtained from the background color of the <body>
  // element. It can be used for filling in gutter areas around the frame when
  // it's too small to fill the box the parent reserved for it.
  SkColor root_background_color = SK_ColorWHITE;

  // Provides selection region updates relative to the current viewport. If the
  // selection is empty or otherwise unused, the bound types will indicate such.
  Selection<gfx::SelectionBound> selection;

  std::vector<ui::LatencyInfo> latency_info;

  // This is the set of Surfaces that are referenced by this frame.
  // Note: this includes occluded and clipped surfaces and surfaces that may
  // be accessed by this CompositorFrame in the future.
  // TODO(fsamuel): In the future, a generalized frame eviction system will
  // determine which surfaces to retain and which to evict. It will likely
  // be unnecessary for the embedder to explicitly specify which surfaces to
  // retain. Thus, this field will likely go away.
  std::vector<SurfaceId> referenced_surfaces;

  // This is the set of dependent SurfaceIds that should be active in the
  // display compositor before this CompositorFrame can be activated. Note
  // that if |can_activate_before_dependencies| then the display compositor
  // can choose to activate a CompositorFrame before all dependencies are
  // available.
  // Note: |activation_dependencies| and |referenced_surfaces| are disjoint
  //       sets of surface IDs. If a surface ID is known to exist and can be
  //       used without additional synchronization, then it is placed in
  //       |referenced_surfaces|. |activation_dependencies| is the set of
  //       surface IDs that this frame would like to block on until they
  //       become available or a deadline hits.
  std::vector<SurfaceId> activation_dependencies;

  // This specifies a deadline for this CompositorFrame to synchronize with its
  // activation dependencies. Once this deadline passes, this CompositorFrame
  // should be forcibly activated. This deadline may be lower-bounded by the
  // default synchronization deadline specified by the system.
  FrameDeadline deadline;

  // This is a value that allows the browser to associate compositor frames
  // with the content that they represent -- typically top-level page loads.
  // TODO(kenrb, fsamuel): This should eventually by SurfaceID, when they
  // become available in all renderer processes. See https://crbug.com/695579.
  uint32_t content_source_id = 0;

  // BeginFrameAck for the BeginFrame that this CompositorFrame answers.
  BeginFrameAck begin_frame_ack;

  // Once the display compositor processes a frame containing a non-zero frame
  // token, the token is sent to embedder of the frame. This is helpful when
  // the embedder wants to do something after a particular frame is processed.
  uint32_t frame_token = 0;

  // Once the display compositor presents a frame containing a non-zero
  // presentation token, a presentation feedback will be provided to
  // CompositorFrameSinkClient.
  uint32_t presentation_token = 0;

 private:
  CompositorFrameMetadata(const CompositorFrameMetadata& other);
  CompositorFrameMetadata operator=(const CompositorFrameMetadata&) = delete;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_QUADS_COMPOSITOR_FRAME_METADATA_H_
