// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_COMPOSITOR_FRAME_STRUCT_TRAITS_H_
#define SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_COMPOSITOR_FRAME_STRUCT_TRAITS_H_

#include <vector>

#include "components/viz/common/quads/compositor_frame.h"
#include "services/viz/public/cpp/compositing/transferable_resource_struct_traits.h"
#include "services/viz/public/interfaces/compositing/compositor_frame.mojom-shared.h"

namespace mojo {

template <>
struct StructTraits<viz::mojom::CompositorFrameDataView, viz::CompositorFrame> {
  static const viz::CompositorFrameMetadata& metadata(
      const viz::CompositorFrame& input) {
    return input.metadata;
  }

  static const std::vector<viz::TransferableResource>& resources(
      const viz::CompositorFrame& input) {
    return input.resource_list;
  }

  static const viz::RenderPassList& passes(const viz::CompositorFrame& input) {
    return input.render_pass_list;
  }

  static bool Read(viz::mojom::CompositorFrameDataView data,
                   viz::CompositorFrame* out);
};

}  // namespace mojo

#endif  // SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_COMPOSITOR_FRAME_STRUCT_TRAITS_H_
