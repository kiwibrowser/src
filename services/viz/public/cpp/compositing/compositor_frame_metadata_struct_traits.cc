// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/viz/public/cpp/compositing/compositor_frame_metadata_struct_traits.h"

#include "services/viz/public/cpp/compositing/begin_frame_args_struct_traits.h"
#include "services/viz/public/cpp/compositing/selection_struct_traits.h"
#include "services/viz/public/cpp/compositing/surface_id_struct_traits.h"
#include "ui/gfx/mojo/selection_bound_struct_traits.h"
#include "ui/latency/mojo/latency_info_struct_traits.h"

namespace mojo {

// static
bool StructTraits<viz::mojom::CompositorFrameMetadataDataView,
                  viz::CompositorFrameMetadata>::
    Read(viz::mojom::CompositorFrameMetadataDataView data,
         viz::CompositorFrameMetadata* out) {
  out->device_scale_factor = data.device_scale_factor();
  if (!data.ReadRootScrollOffset(&out->root_scroll_offset))
    return false;

  out->page_scale_factor = data.page_scale_factor();
  if (!data.ReadScrollableViewportSize(&out->scrollable_viewport_size) ||
      !data.ReadRootLayerSize(&out->root_layer_size)) {
    return false;
  }

  out->min_page_scale_factor = data.min_page_scale_factor();
  out->max_page_scale_factor = data.max_page_scale_factor();
  out->root_overflow_y_hidden = data.root_overflow_y_hidden();
  out->may_contain_video = data.may_contain_video();
  out->is_resourceless_software_draw_with_scroll_or_animation =
      data.is_resourceless_software_draw_with_scroll_or_animation();
  out->top_controls_height = data.top_controls_height();
  out->top_controls_shown_ratio = data.top_controls_shown_ratio();
  out->bottom_controls_height = data.bottom_controls_height();
  out->bottom_controls_shown_ratio = data.bottom_controls_shown_ratio();
  out->content_source_id = data.content_source_id();
  out->frame_token = data.frame_token();
  out->presentation_token = data.presentation_token();
  out->root_background_color = data.root_background_color();
  return data.ReadSelection(&out->selection) &&
         data.ReadLatencyInfo(&out->latency_info) &&
         data.ReadReferencedSurfaces(&out->referenced_surfaces) &&
         data.ReadDeadline(&out->deadline) &&
         data.ReadActivationDependencies(&out->activation_dependencies) &&
         data.ReadBeginFrameAck(&out->begin_frame_ack);
}

}  // namespace mojo
