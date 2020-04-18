// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "base/command_line.h"
#include "base/test/fuzzed_data_provider.h"
#include "components/viz/host/hit_test/hit_test_query.h"

namespace {

uint32_t GetNextUInt32(base::FuzzedDataProvider* fuzz) {
  return fuzz->ConsumeUint32InRange(std::numeric_limits<uint32_t>::min(),
                                    std::numeric_limits<uint32_t>::max());
}

void AddHitTestRegion(base::FuzzedDataProvider* fuzz,
                      std::vector<viz::AggregatedHitTestRegion>* regions,
                      std::vector<viz::FrameSinkId>* frame_sink_ids,
                      const uint32_t depth = 0) {
  constexpr uint32_t kMaxDepthAllowed = 25;
  if (fuzz->remaining_bytes() < sizeof(viz::AggregatedHitTestRegion))
    return;
  viz::FrameSinkId frame_sink_id(GetNextUInt32(fuzz), GetNextUInt32(fuzz));
  uint32_t flags = GetNextUInt32(fuzz);
  gfx::Rect rect(fuzz->ConsumeUint8(), fuzz->ConsumeUint8(),
                 fuzz->ConsumeUint16(), fuzz->ConsumeUint16());
  int32_t child_count =
      depth < kMaxDepthAllowed ? fuzz->ConsumeUint32InRange(0, 10) : 0;
  gfx::Transform transform;
  if (fuzz->ConsumeBool() && fuzz->remaining_bytes() >= sizeof(transform)) {
    std::string matrix_bytes = fuzz->ConsumeBytes(sizeof(gfx::Transform));
    memcpy(&transform, matrix_bytes.data(), sizeof(gfx::Transform));
  }
  regions->emplace_back(frame_sink_id, flags, rect, transform, child_count);
  // Always add the first frame sink id, because the root needs to be in the
  // list of FrameSinkId.
  if (regions->size() == 1 || fuzz->ConsumeBool())
    frame_sink_ids->push_back(frame_sink_id);
  while (child_count-- > 0)
    AddHitTestRegion(fuzz, regions, frame_sink_ids, depth + 1);
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t num_bytes) {
  // If there isn't enough memory to have a single AggregatedHitTestRegion, then
  // skip.
  if (num_bytes < sizeof(viz::AggregatedHitTestRegion))
    return 0;

  // Create the list of AggregatedHitTestRegion objects.
  std::vector<viz::AggregatedHitTestRegion> regions;
  std::vector<viz::FrameSinkId> frame_sink_ids;
  base::FuzzedDataProvider fuzz(data, num_bytes);
  AddHitTestRegion(&fuzz, &regions, &frame_sink_ids);

  // Create the HitTestQuery and send hit-test data.
  viz::HitTestQuery query;
  query.OnAggregatedHitTestRegionListUpdated(regions);

  for (float x = 0; x < 1000.; x += 10) {
    for (float y = 0; y < 1000.; y += 10) {
      gfx::PointF location(x, y);
      query.FindTargetForLocation(viz::EventSource::MOUSE, location);
      query.TransformLocationForTarget(viz::EventSource::MOUSE, frame_sink_ids,
                                       location, &location);
    }
  }

  return 0;
}
