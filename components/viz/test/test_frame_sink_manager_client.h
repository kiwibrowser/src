// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_TEST_FRAME_SINK_MANAGER_CLIENT_H_
#define COMPONENTS_VIZ_TEST_TEST_FRAME_SINK_MANAGER_CLIENT_H_

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_manager.mojom.h"

namespace viz {

// A test mojom::FrameSinkManagerClient that handles assigning temporary
// references. When a new surface is created this will check the FrameSinkId
// hierarchy and assign an owner for the temporary reference, or drop the
// temporary reference if there is no owner.
class TestFrameSinkManagerClient : public mojom::FrameSinkManagerClient {
 public:
  explicit TestFrameSinkManagerClient(mojom::FrameSinkManager* manager);
  ~TestFrameSinkManagerClient() override;

  // Sets frame sink hierarchy. When a new surface from |child_frame_sink_id| is
  // created it will be assigned |parent_frame_sink_id| as the owner.
  void SetFrameSinkHierarchy(const FrameSinkId& parent_frame_sink_id,
                             const FrameSinkId& child_frame_sink_id);

  // Stops assigning or dropping temporary references. This allows tests to
  // manually assign or drop temporary references.
  void DisableAssignTemporaryReferences();

  // Resets to default constructed state.
  void Reset();

  // mojom::FrameSinkManagerClient:
  void OnSurfaceCreated(const SurfaceId& surface_id) override;
  void OnFirstSurfaceActivation(const SurfaceInfo& surface_info) override {}
  void OnAggregatedHitTestRegionListUpdated(
      const FrameSinkId& frame_sink_id,
      const std::vector<AggregatedHitTestRegion>& hit_test_data) override {}
  void OnFrameTokenChanged(const FrameSinkId& frame_sink_id,
                           uint32_t frame_token) override {}

 private:
  mojom::FrameSinkManager* const manager_;
  bool disabled_ = false;
  base::flat_map<FrameSinkId, FrameSinkId> frame_sink_hierarchy_;

  DISALLOW_COPY_AND_ASSIGN(TestFrameSinkManagerClient);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_TEST_FRAME_SINK_MANAGER_CLIENT_H_
