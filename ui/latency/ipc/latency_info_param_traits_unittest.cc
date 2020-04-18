// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "ipc/ipc_message_macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/ipc/geometry/gfx_param_traits.h"
#include "ui/latency/ipc/latency_info_param_traits.h"
#include "ui/latency/ipc/latency_info_param_traits_macros.h"

namespace ui {

TEST(LatencyInfoParamTraitsTest, Basic) {
  LatencyInfo latency;
  latency.set_trace_id(5);
  latency.AddSnapshot(7, 15);
  latency.set_ukm_source_id(10);
  ASSERT_FALSE(latency.terminated());
  latency.AddLatencyNumber(INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT, 1234);
  latency.AddLatencyNumber(INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT, 1234);
  latency.AddLatencyNumber(INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT,
                           1234);

  EXPECT_EQ(5, latency.trace_id());
  EXPECT_EQ(1u, latency.Snapshots().size());
  EXPECT_EQ(15, latency.Snapshots().at(7));
  EXPECT_EQ(10, latency.ukm_source_id());
  EXPECT_TRUE(latency.terminated());

  IPC::Message msg(1, 2, IPC::Message::PRIORITY_NORMAL);
  IPC::WriteParam(&msg, latency);
  base::PickleIterator iter(msg);
  LatencyInfo output;
  EXPECT_TRUE(IPC::ReadParam(&msg, &iter, &output));

  EXPECT_EQ(latency.trace_id(), output.trace_id());
  EXPECT_EQ(latency.Snapshots(), output.Snapshots());
  EXPECT_EQ(latency.ukm_source_id(), output.ukm_source_id());
  EXPECT_EQ(latency.terminated(), output.terminated());

  EXPECT_TRUE(output.FindLatency(INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT,
                                 1234,
                                 nullptr));

  LatencyInfo::LatencyComponent rwh_comp;
  EXPECT_TRUE(output.FindLatency(INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT,
                                 1234,
                                 &rwh_comp));
  EXPECT_EQ(1u, rwh_comp.event_count);

  EXPECT_TRUE(output.FindLatency(
      INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT, 1234, nullptr));
}

TEST(LatencyInfoParamTraitsTest, InvalidData) {
  IPC::Message msg(1, 2, IPC::Message::PRIORITY_NORMAL);
  IPC::WriteParam(&msg, std::string());
  ui::LatencyInfo::LatencyMap components;
  IPC::WriteParam(&msg, components);
  IPC::WriteParam(&msg, static_cast<int64_t>(1234));
  IPC::WriteParam(&msg, true);

  base::PickleIterator iter(msg);
  LatencyInfo output;
  EXPECT_FALSE(IPC::ReadParam(&msg, &iter, &output));
}

}  // namespace ui
