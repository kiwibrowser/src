// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIZ_PUBLIC_CPP_HIT_TEST_HIT_TEST_REGION_LIST_STRUCT_TRAITS_H_
#define SERVICES_VIZ_PUBLIC_CPP_HIT_TEST_HIT_TEST_REGION_LIST_STRUCT_TRAITS_H_

#include "components/viz/common/hit_test/hit_test_region_list.h"
#include "services/viz/public/cpp/compositing/frame_sink_id_struct_traits.h"
#include "services/viz/public/interfaces/hit_test/hit_test_region_list.mojom-shared.h"
#include "ui/gfx/geometry/mojo/geometry_struct_traits.h"
#include "ui/gfx/mojo/transform_struct_traits.h"

namespace mojo {

template <>
struct StructTraits<viz::mojom::HitTestRegionDataView, viz::HitTestRegion> {
  static const viz::FrameSinkId& frame_sink_id(
      const viz::HitTestRegion& region) {
    return region.frame_sink_id;
  }
  static uint32_t flags(const viz::HitTestRegion& region) {
    return region.flags;
  }
  static const gfx::Rect& rect(const viz::HitTestRegion& region) {
    return region.rect;
  }
  static const gfx::Transform& transform(const viz::HitTestRegion& region) {
    return region.transform;
  }

  static bool Read(viz::mojom::HitTestRegionDataView data,
                   viz::HitTestRegion* out);
};

template <>
struct StructTraits<viz::mojom::HitTestRegionListDataView,
                    viz::HitTestRegionList> {
  static uint32_t flags(const viz::HitTestRegionList& list) {
    return list.flags;
  }
  static const gfx::Rect& bounds(const viz::HitTestRegionList& list) {
    return list.bounds;
  }
  static const gfx::Transform& transform(const viz::HitTestRegionList& list) {
    return list.transform;
  }
  static const std::vector<viz::HitTestRegion>& regions(
      const viz::HitTestRegionList& list) {
    return list.regions;
  }

  static bool Read(viz::mojom::HitTestRegionListDataView data,
                   viz::HitTestRegionList* out);
};

}  // namespace mojo

#endif  // SERVICES_VIZ_PUBLIC_CPP_HIT_TEST_HIT_TEST_REGION_LIST_STRUCT_TRAITS_H_
