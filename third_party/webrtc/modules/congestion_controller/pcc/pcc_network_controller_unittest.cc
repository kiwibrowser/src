/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>

#include "modules/congestion_controller/pcc/pcc_factory.h"
#include "modules/congestion_controller/pcc/pcc_network_controller.h"
#include "test/scenario/scenario.h"

#include "test/gmock.h"
#include "test/gtest.h"

using testing::Field;
using testing::Matcher;
using testing::AllOf;
using testing::Ge;
using testing::Le;
using testing::Property;

namespace webrtc {
namespace test {
namespace {

const DataRate kInitialBitrate = DataRate::kbps(60);
const Timestamp kDefaultStartTime = Timestamp::ms(10000000);

constexpr double kDataRateMargin = 0.20;
constexpr double kMinDataRateFactor = 1 - kDataRateMargin;
constexpr double kMaxDataRateFactor = 1 + kDataRateMargin;
inline Matcher<TargetTransferRate> TargetRateCloseTo(DataRate rate) {
  DataRate min_data_rate = rate * kMinDataRateFactor;
  DataRate max_data_rate = rate * kMaxDataRateFactor;
  return Field(&TargetTransferRate::target_rate,
               AllOf(Ge(min_data_rate), Le(max_data_rate)));
}

NetworkControllerConfig InitialConfig(
    int starting_bandwidth_kbps = kInitialBitrate.kbps(),
    int min_data_rate_kbps = 0,
    int max_data_rate_kbps = 5 * kInitialBitrate.kbps()) {
  NetworkControllerConfig config;
  config.constraints.at_time = kDefaultStartTime;
  config.constraints.min_data_rate = DataRate::kbps(min_data_rate_kbps);
  config.constraints.max_data_rate = DataRate::kbps(max_data_rate_kbps);
  config.constraints.starting_rate = DataRate::kbps(starting_bandwidth_kbps);
  return config;
}

ProcessInterval InitialProcessInterval() {
  ProcessInterval process_interval;
  process_interval.at_time = kDefaultStartTime;
  return process_interval;
}

}  // namespace

TEST(PccNetworkControllerTest, SendsConfigurationOnFirstProcess) {
  std::unique_ptr<NetworkControllerInterface> controller_;
  controller_.reset(new pcc::PccNetworkController(InitialConfig()));

  NetworkControlUpdate update =
      controller_->OnProcessInterval(InitialProcessInterval());
  EXPECT_THAT(*update.target_rate, TargetRateCloseTo(kInitialBitrate));
  EXPECT_THAT(*update.pacer_config,
              Property(&PacerConfig::data_rate, Ge(kInitialBitrate)));
}

TEST(PccNetworkControllerTest, UpdatesTargetSendRate) {
  PccNetworkControllerFactory factory;
  Scenario s("pcc_unit/updates_rate", false);
  SimulatedTimeClientConfig config;
  config.transport.cc =
      TransportControllerConfig::CongestionController::kInjected;
  config.transport.cc_factory = &factory;
  config.transport.rates.min_rate = DataRate::kbps(10);
  config.transport.rates.max_rate = DataRate::kbps(1500);
  config.transport.rates.start_rate = DataRate::kbps(300);
  NetworkNodeConfig net_conf;
  auto send_net = s.CreateSimulationNode([](NetworkNodeConfig* c) {
    c->simulation.bandwidth = DataRate::kbps(500);
    c->simulation.delay = TimeDelta::ms(100);
    c->simulation.loss_rate = 0.0;
    c->update_frequency = TimeDelta::ms(5);
  });
  auto ret_net = s.CreateSimulationNode([](NetworkNodeConfig* c) {
    c->simulation.delay = TimeDelta::ms(100);
    c->update_frequency = TimeDelta::ms(5);
  });
  SimulatedTimeClient* client = s.CreateSimulatedTimeClient(
      "send", config, {PacketStreamConfig()}, {send_net}, {ret_net});

  s.RunFor(TimeDelta::seconds(25));
  EXPECT_NEAR(client->target_rate_kbps(), 450, 100);

  send_net->UpdateConfig([](NetworkNodeConfig* c) {
    c->simulation.bandwidth = DataRate::kbps(800);
    c->simulation.delay = TimeDelta::ms(100);
  });

  s.RunFor(TimeDelta::seconds(20));
  EXPECT_NEAR(client->target_rate_kbps(), 750, 150);

  send_net->UpdateConfig([](NetworkNodeConfig* c) {
    c->simulation.bandwidth = DataRate::kbps(200);
    c->simulation.delay = TimeDelta::ms(200);
  });
  ret_net->UpdateConfig(
      [](NetworkNodeConfig* c) { c->simulation.delay = TimeDelta::ms(200); });

  s.RunFor(TimeDelta::seconds(20));
  EXPECT_NEAR(client->target_rate_kbps(), 200, 40);
}

}  // namespace test
}  // namespace webrtc
