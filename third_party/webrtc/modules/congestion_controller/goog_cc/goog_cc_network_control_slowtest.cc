/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <queue>

#include "api/transport/goog_cc_factory.h"
#include "test/field_trial.h"
#include "test/gtest.h"
#include "test/scenario/scenario.h"

namespace webrtc {
namespace test {
namespace {
// Count dips from a constant high bandwidth level within a short window.
int CountBandwidthDips(std::queue<DataRate> bandwidth_history,
                       DataRate threshold) {
  if (bandwidth_history.empty())
    return true;
  DataRate first = bandwidth_history.front();
  bandwidth_history.pop();

  int dips = 0;
  bool state_high = true;
  while (!bandwidth_history.empty()) {
    if (bandwidth_history.front() + threshold < first && state_high) {
      ++dips;
      state_high = false;
    } else if (bandwidth_history.front() == first) {
      state_high = true;
    } else if (bandwidth_history.front() > first) {
      // If this is toggling we will catch it later when front becomes first.
      state_high = false;
    }
    bandwidth_history.pop();
  }
  return dips;
}
}  // namespace

TEST(GoogCcNetworkControllerTest, MaintainsLowRateInSafeResetTrial) {
  const DataRate kLinkCapacity = DataRate::kbps(200);
  const DataRate kStartRate = DataRate::kbps(300);

  ScopedFieldTrials trial("WebRTC-Bwe-SafeResetOnRouteChange/Enabled/");
  Scenario s("googcc_unit/safe_reset_low", true);
  auto* send_net = s.CreateSimulationNode([&](NetworkNodeConfig* c) {
    c->simulation.bandwidth = kLinkCapacity;
    c->simulation.delay = TimeDelta::ms(10);
  });
  // TODO(srte): replace with SimulatedTimeClient when it supports probing.
  auto* client = s.CreateClient("send", [&](CallClientConfig* c) {
    c->transport.cc = TransportControllerConfig::CongestionController::kGoogCc;
    c->transport.rates.start_rate = kStartRate;
  });
  auto* route = s.CreateRoutes(client, {send_net},
                               s.CreateClient("return", CallClientConfig()),
                               {s.CreateSimulationNode(NetworkNodeConfig())});
  s.CreateVideoStream(route->forward(), VideoStreamConfig());
  // Allow the controller to stabilize.
  s.RunFor(TimeDelta::ms(500));
  EXPECT_NEAR(client->send_bandwidth().kbps(), kLinkCapacity.kbps(), 50);
  s.ChangeRoute(route->forward(), {send_net});
  // Allow new settings to propagate.
  s.RunFor(TimeDelta::ms(100));
  // Under the trial, the target should be unchanged for low rates.
  EXPECT_NEAR(client->send_bandwidth().kbps(), kLinkCapacity.kbps(), 50);
}

TEST(GoogCcNetworkControllerTest, CutsHighRateInSafeResetTrial) {
  const DataRate kLinkCapacity = DataRate::kbps(1000);
  const DataRate kStartRate = DataRate::kbps(300);

  ScopedFieldTrials trial("WebRTC-Bwe-SafeResetOnRouteChange/Enabled/");
  Scenario s("googcc_unit/safe_reset_high_cut", true);
  auto send_net = s.CreateSimulationNode([&](NetworkNodeConfig* c) {
    c->simulation.bandwidth = kLinkCapacity;
    c->simulation.delay = TimeDelta::ms(50);
  });
  // TODO(srte): replace with SimulatedTimeClient when it supports probing.
  auto* client = s.CreateClient("send", [&](CallClientConfig* c) {
    c->transport.cc = TransportControllerConfig::CongestionController::kGoogCc;
    c->transport.rates.start_rate = kStartRate;
  });
  auto* route = s.CreateRoutes(client, {send_net},
                               s.CreateClient("return", CallClientConfig()),
                               {s.CreateSimulationNode(NetworkNodeConfig())});
  s.CreateVideoStream(route->forward(), VideoStreamConfig());
  // Allow the controller to stabilize.
  s.RunFor(TimeDelta::ms(500));
  EXPECT_NEAR(client->send_bandwidth().kbps(), kLinkCapacity.kbps(), 300);
  s.ChangeRoute(route->forward(), {send_net});
  // Allow new settings to propagate.
  s.RunFor(TimeDelta::ms(50));
  // Under the trial, the target should be reset from high values.
  EXPECT_NEAR(client->send_bandwidth().kbps(), kStartRate.kbps(), 30);
}

TEST(GoogCcNetworkControllerTest, DetectsHighRateInSafeResetTrial) {
  ScopedFieldTrials trial(
      "WebRTC-Bwe-SafeResetOnRouteChange/Enabled,ack/"
      "WebRTC-Bwe-ProbeRateFallback/Enabled/");
  const DataRate kInitialLinkCapacity = DataRate::kbps(200);
  const DataRate kNewLinkCapacity = DataRate::kbps(800);
  const DataRate kStartRate = DataRate::kbps(300);

  Scenario s("googcc_unit/safe_reset_high_detect", true);
  auto* initial_net = s.CreateSimulationNode([&](NetworkNodeConfig* c) {
    c->simulation.bandwidth = kInitialLinkCapacity;
    c->simulation.delay = TimeDelta::ms(50);
  });
  auto* new_net = s.CreateSimulationNode([&](NetworkNodeConfig* c) {
    c->simulation.bandwidth = kNewLinkCapacity;
    c->simulation.delay = TimeDelta::ms(50);
  });
  // TODO(srte): replace with SimulatedTimeClient when it supports probing.
  auto* client = s.CreateClient("send", [&](CallClientConfig* c) {
    c->transport.cc = TransportControllerConfig::CongestionController::kGoogCc;
    c->transport.rates.start_rate = kStartRate;
  });
  auto* route = s.CreateRoutes(client, {initial_net},
                               s.CreateClient("return", CallClientConfig()),
                               {s.CreateSimulationNode(NetworkNodeConfig())});
  s.CreateVideoStream(route->forward(), VideoStreamConfig());
  // Allow the controller to stabilize.
  s.RunFor(TimeDelta::ms(1000));
  EXPECT_NEAR(client->send_bandwidth().kbps(), kInitialLinkCapacity.kbps(), 50);
  s.ChangeRoute(route->forward(), {new_net});
  // Allow new settings to propagate, but not probes to be received.
  s.RunFor(TimeDelta::ms(50));
  // Under the field trial, the target rate should be unchanged since it's lower
  // than the starting rate.
  EXPECT_NEAR(client->send_bandwidth().kbps(), kInitialLinkCapacity.kbps(), 50);
  // However, probing should have made us detect the higher rate.
  s.RunFor(TimeDelta::ms(2000));
  EXPECT_GT(client->send_bandwidth().kbps(), kNewLinkCapacity.kbps() - 300);
}

TEST(GoogCcNetworkControllerTest,
     TargetRateReducedOnPacingBufferBuildupInTrial) {
  // Configure strict pacing to ensure build-up.
  ScopedFieldTrials trial(
      "WebRTC-CongestionWindowPushback/Enabled/WebRTC-CwndExperiment/"
      "Enabled-100/WebRTC-Video-Pacing/factor:1.0/"
      "WebRTC-AddPacingToCongestionWindowPushback/Enabled/");

  const DataRate kLinkCapacity = DataRate::kbps(1000);
  const DataRate kStartRate = DataRate::kbps(1000);

  Scenario s("googcc_unit/pacing_buffer_buildup", true);
  auto* net = s.CreateSimulationNode([&](NetworkNodeConfig* c) {
    c->simulation.bandwidth = kLinkCapacity;
    c->simulation.delay = TimeDelta::ms(50);
  });
  // TODO(srte): replace with SimulatedTimeClient when it supports pacing.
  auto* client = s.CreateClient("send", [&](CallClientConfig* c) {
    c->transport.cc = TransportControllerConfig::CongestionController::kGoogCc;
    c->transport.rates.start_rate = kStartRate;
  });
  auto* route = s.CreateRoutes(client, {net},
                               s.CreateClient("return", CallClientConfig()),
                               {s.CreateSimulationNode(NetworkNodeConfig())});
  s.CreateVideoStream(route->forward(), VideoStreamConfig());
  // Allow some time for the buffer to build up.
  s.RunFor(TimeDelta::seconds(5));

  // Without trial, pacer delay reaches ~250 ms.
  EXPECT_LT(client->GetStats().pacer_delay_ms, 150);
}

TEST(GoogCcNetworkControllerTest, NoBandwidthTogglingInLossControlTrial) {
  ScopedFieldTrials trial("WebRTC-Bwe-LossBasedControl/Enabled/");
  Scenario s("googcc_unit/no_toggling", true);
  auto* send_net = s.CreateSimulationNode([&](NetworkNodeConfig* c) {
    c->simulation.bandwidth = DataRate::kbps(2000);
    c->simulation.loss_rate = 0.2;
    c->simulation.delay = TimeDelta::ms(10);
  });

  // TODO(srte): replace with SimulatedTimeClient when it supports probing.
  auto* client = s.CreateClient("send", [&](CallClientConfig* c) {
    c->transport.cc = TransportControllerConfig::CongestionController::kGoogCc;
    c->transport.rates.start_rate = DataRate::kbps(300);
  });
  auto* route = s.CreateRoutes(client, {send_net},
                               s.CreateClient("return", CallClientConfig()),
                               {s.CreateSimulationNode(NetworkNodeConfig())});
  s.CreateVideoStream(route->forward(), VideoStreamConfig());
  // Allow the controller to initialize.
  s.RunFor(TimeDelta::ms(250));

  std::queue<DataRate> bandwidth_history;
  const TimeDelta step = TimeDelta::ms(50);
  for (TimeDelta time = TimeDelta::Zero(); time < TimeDelta::ms(2000);
       time += step) {
    s.RunFor(step);
    const TimeDelta window = TimeDelta::ms(500);
    if (bandwidth_history.size() >= window / step)
      bandwidth_history.pop();
    bandwidth_history.push(client->send_bandwidth());
    EXPECT_LT(CountBandwidthDips(bandwidth_history, DataRate::kbps(100)), 2);
  }
}

}  // namespace test
}  // namespace webrtc
