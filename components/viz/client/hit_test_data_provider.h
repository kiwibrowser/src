// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_H_
#define COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "components/viz/client/viz_client_export.h"
#include "components/viz/common/quads/compositor_frame.h"

namespace viz {
struct HitTestRegionList;

class VIZ_CLIENT_EXPORT HitTestDataProvider {
 public:
  HitTestDataProvider() = default;
  virtual ~HitTestDataProvider() = default;

  // Returns an array of hit-test regions. May return nullptr to disable
  // hit-testing.
  virtual base::Optional<HitTestRegionList> GetHitTestData(
      const CompositorFrame& compositor_frame) const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(HitTestDataProvider);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_H_
