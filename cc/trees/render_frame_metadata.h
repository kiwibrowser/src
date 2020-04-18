// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_RENDER_FRAME_METADATA_H_
#define CC_TREES_RENDER_FRAME_METADATA_H_

#include "base/optional.h"
#include "cc/cc_export.h"
#include "components/viz/common/quads/selection.h"
#include "components/viz/common/surfaces/local_surface_id.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/vector2d_f.h"
#include "ui/gfx/selection_bound.h"

namespace cc {

class CC_EXPORT RenderFrameMetadata {
 public:
  RenderFrameMetadata();
  RenderFrameMetadata(const RenderFrameMetadata& other);
  RenderFrameMetadata(RenderFrameMetadata&& other);
  ~RenderFrameMetadata();

  // Certain fields should always have their changes reported. This will return
  // true when there is a difference between |rfm1| and |rfm2| for those fields.
  // These fields have a low frequency rate of change.
  static bool HasAlwaysUpdateMetadataChanged(const RenderFrameMetadata& rfm1,
                                             const RenderFrameMetadata& rfm2);

  RenderFrameMetadata& operator=(const RenderFrameMetadata&);
  RenderFrameMetadata& operator=(RenderFrameMetadata&& other);
  bool operator==(const RenderFrameMetadata& other) const;
  bool operator!=(const RenderFrameMetadata& other) const;

  // Indicates whether the scroll offset of the root layer is at top, i.e.,
  // whether scroll_offset.y() == 0.
  bool is_scroll_offset_at_top = true;

  // The background color of a CompositorFrame. It can be used for filling the
  // content area if the primary surface is unavailable and fallback is not
  // specified.
  SkColor root_background_color = SK_ColorWHITE;

  // Scroll offset of the root layer. This optional parameter is only valid
  // during tests.
  base::Optional<gfx::Vector2dF> root_scroll_offset;

  // Selection region relative to the current viewport. If the selection is
  // empty or otherwise unused, the bound types will indicate such.
  viz::Selection<gfx::SelectionBound> selection;

  // Determines whether the page is mobile optimized or not, which means at
  // least one of the following has to be true:
  // - page has a width=device-width or narrower viewport.
  // - page prevents zooming in or out (i.e. min and max page scale factors
  // are the same).
  bool is_mobile_optimized = false;

  // The device scale factor used to generate a CompositorFrame.
  float device_scale_factor = 1.f;

  // The size of the viewport used to generate a CompositorFrame.
  gfx::Size viewport_size_in_pixels;

  // The last viz::LocalSurfaceId used to submit a CompositorFrame.
  base::Optional<viz::LocalSurfaceId> local_surface_id;

  // Used to position the Android location top bar and page content, whose
  // precise position is computed by the renderer compositor.
  float top_controls_height = 0.f;
  float top_controls_shown_ratio = 0.f;

  // Used to position Android bottom bar, whose position is computed by the
  // renderer compositor.
  float bottom_controls_height = 0.f;
  float bottom_controls_shown_ratio = 0.f;
};

}  // namespace cc

#endif  // CC_TREES_RENDER_FRAME_METADATA_H_
