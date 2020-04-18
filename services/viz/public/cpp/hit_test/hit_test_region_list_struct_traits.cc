// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/viz/public/cpp/hit_test/hit_test_region_list_struct_traits.h"

namespace mojo {

// static
bool StructTraits<viz::mojom::HitTestRegionDataView, viz::HitTestRegion>::Read(
    viz::mojom::HitTestRegionDataView data,
    viz::HitTestRegion* out) {
  if (!data.ReadFrameSinkId(&out->frame_sink_id))
    return false;
  if (!data.ReadRect(&out->rect))
    return false;
  if (!data.ReadTransform(&out->transform))
    return false;
  out->flags = data.flags();
  return true;
}

// static
bool StructTraits<
    viz::mojom::HitTestRegionListDataView,
    viz::HitTestRegionList>::Read(viz::mojom::HitTestRegionListDataView data,
                                  viz::HitTestRegionList* out) {
  if (!data.ReadRegions(&out->regions))
    return false;
  if (!data.ReadBounds(&out->bounds))
    return false;
  if (!data.ReadTransform(&out->transform))
    return false;
  out->flags = data.flags();
  return true;
}

}  // namespace mojo
