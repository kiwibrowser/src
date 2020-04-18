// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/latency/mojo/latency_info_struct_traits.h"
#include "ui/latency/mojo/traits_test_service.mojom.h"

namespace ui {

namespace {

class StructTraitsTest : public testing::Test, public mojom::TraitsTestService {
 public:
  StructTraitsTest() {}

 protected:
  mojom::TraitsTestServicePtr GetTraitsTestProxy() {
    mojom::TraitsTestServicePtr proxy;
    traits_test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // TraitsTestService:
  void EchoLatencyComponent(const LatencyInfo::LatencyComponent& l,
                            EchoLatencyComponentCallback callback) override {
    std::move(callback).Run(l);
  }

  void EchoLatencyComponentId(
      const std::pair<LatencyComponentType, int64_t>& id,
      EchoLatencyComponentIdCallback callback) override {
    std::move(callback).Run(id);
  }

  void EchoLatencyInfo(const LatencyInfo& info,
                       EchoLatencyInfoCallback callback) override {
    std::move(callback).Run(info);
  }

  base::MessageLoop loop_;
  mojo::BindingSet<TraitsTestService> traits_test_bindings_;
  DISALLOW_COPY_AND_ASSIGN(StructTraitsTest);
};

}  // namespace

TEST_F(StructTraitsTest, LatencyComponent) {
  const base::TimeTicks event_time = base::TimeTicks::Now();
  const uint32_t event_count = 1234;
  LatencyInfo::LatencyComponent input;
  input.event_time = event_time;
  input.event_count = event_count;
  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  LatencyInfo::LatencyComponent output;
  proxy->EchoLatencyComponent(input, &output);
  EXPECT_EQ(event_time, output.event_time);
  EXPECT_EQ(event_count, output.event_count);
}

TEST_F(StructTraitsTest, LatencyComponentId) {
  const LatencyComponentType type =
      INPUT_EVENT_LATENCY_SCROLL_UPDATE_ORIGINAL_COMPONENT;
  const int64_t id = 1337;
  std::pair<LatencyComponentType, int64_t> input(type, id);
  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  std::pair<LatencyComponentType, int64_t> output;
  proxy->EchoLatencyComponentId(input, &output);
  EXPECT_EQ(type, output.first);
  EXPECT_EQ(id, output.second);
}

TEST_F(StructTraitsTest, LatencyInfo) {
  LatencyInfo latency;
  latency.set_trace_id(5);
  latency.set_ukm_source_id(10);
  ASSERT_FALSE(latency.terminated());
  latency.AddLatencyNumber(INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT, 1234);
  latency.AddLatencyNumber(INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT, 1234);
  latency.AddLatencyNumber(INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT,
                           1234);

  EXPECT_EQ(5, latency.trace_id());
  EXPECT_EQ(10, latency.ukm_source_id());
  EXPECT_TRUE(latency.terminated());

  latency.set_source_event_type(ui::TOUCH);

  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  LatencyInfo output;
  proxy->EchoLatencyInfo(latency, &output);

  EXPECT_EQ(latency.trace_id(), output.trace_id());
  EXPECT_EQ(latency.ukm_source_id(), output.ukm_source_id());
  EXPECT_EQ(latency.terminated(), output.terminated());
  EXPECT_EQ(latency.source_event_type(), output.source_event_type());

  EXPECT_TRUE(output.FindLatency(INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT, 1234,
                                 nullptr));

  LatencyInfo::LatencyComponent rwh_comp;
  EXPECT_TRUE(output.FindLatency(INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT, 1234,
                                 &rwh_comp));
  EXPECT_EQ(1u, rwh_comp.event_count);

  EXPECT_TRUE(output.FindLatency(
      INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT, 1234, nullptr));
}

}  // namespace ui
