// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_SURFACES_FRAME_SINK_ID_ALLOCATOR_H_
#define COMPONENTS_VIZ_COMMON_SURFACES_FRAME_SINK_ID_ALLOCATOR_H_

#include "components/viz/common/surfaces/frame_sink_id.h"

namespace viz {

// This class generates FrameSinkId with a fixed client_id and an
// incrementally-increasing sink_id.
class FrameSinkIdAllocator {
 public:
  constexpr explicit FrameSinkIdAllocator(uint32_t client_id)
      : client_id_(client_id), next_sink_id_(1u) {}

  FrameSinkId NextFrameSinkId() {
    return FrameSinkId(client_id_, next_sink_id_++);
  }

 private:
  const uint32_t client_id_;
  uint32_t next_sink_id_;

  DISALLOW_COPY_AND_ASSIGN(FrameSinkIdAllocator);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_SURFACES_FRAME_SINK_ID_ALLOCATOR_H_
