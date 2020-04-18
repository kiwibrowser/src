/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef TEST_SCENARIO_NETWORK_NODE_H_
#define TEST_SCENARIO_NETWORK_NODE_H_

#include <deque>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "api/call/transport.h"
#include "api/units/timestamp.h"
#include "call/call.h"
#include "call/simulated_network.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/copy_on_write_buffer.h"
#include "test/scenario/column_printer.h"
#include "test/scenario/network/network_emulation.h"
#include "test/scenario/scenario_config.h"

namespace webrtc {
namespace test {

class NullReceiver : public EmulatedNetworkReceiverInterface {
 public:
  void OnPacketReceived(EmulatedIpPacket packet) override;
};
class ActionReceiver : public EmulatedNetworkReceiverInterface {
 public:
  explicit ActionReceiver(std::function<void()> action);
  virtual ~ActionReceiver() = default;

  void OnPacketReceived(EmulatedIpPacket packet) override;

 private:
  std::function<void()> action_;
};

// SimulationNode is a EmulatedNetworkNode that expose an interface for changing
// run time behavior of the underlying simulation.
class SimulationNode : public EmulatedNetworkNode {
 public:
  void UpdateConfig(std::function<void(NetworkNodeConfig*)> modifier);
  void PauseTransmissionUntil(Timestamp until);
  ColumnPrinter ConfigPrinter() const;

 private:
  friend class Scenario;

  SimulationNode(NetworkNodeConfig config,
                 std::unique_ptr<NetworkBehaviorInterface> behavior,
                 SimulatedNetwork* simulation);
  static std::unique_ptr<SimulationNode> Create(NetworkNodeConfig config);

  SimulatedNetwork* const simulated_network_;
  NetworkNodeConfig config_;
};

class NetworkNodeTransport : public Transport {
 public:
  NetworkNodeTransport(Clock* sender_clock, Call* sender_call);
  ~NetworkNodeTransport() override;

  bool SendRtp(const uint8_t* packet,
               size_t length,
               const PacketOptions& options) override;
  bool SendRtcp(const uint8_t* packet, size_t length) override;

  void Connect(EmulatedNetworkNode* send_node,
               uint64_t receiver_id,
               DataSize packet_overhead);

  DataSize packet_overhead() {
    rtc::CritScope crit(&crit_sect_);
    return packet_overhead_;
  }

 private:
  rtc::CriticalSection crit_sect_;
  Clock* const sender_clock_;
  Call* const sender_call_;
  EmulatedNetworkNode* send_net_ RTC_GUARDED_BY(crit_sect_) = nullptr;
  uint64_t receiver_id_ RTC_GUARDED_BY(crit_sect_) = 0;
  DataSize packet_overhead_ RTC_GUARDED_BY(crit_sect_) = DataSize::Zero();
};

// CrossTrafficSource is created by a Scenario and generates cross traffic. It
// provides methods to access and print internal state.
class CrossTrafficSource {
 public:
  DataRate TrafficRate() const;
  ColumnPrinter StatsPrinter();
  ~CrossTrafficSource();

 private:
  friend class Scenario;
  CrossTrafficSource(EmulatedNetworkReceiverInterface* target,
                     uint64_t receiver_id,
                     CrossTrafficConfig config);
  void Process(Timestamp at_time, TimeDelta delta);

  EmulatedNetworkReceiverInterface* const target_;
  const uint64_t receiver_id_;
  CrossTrafficConfig config_;
  webrtc::Random random_;

  TimeDelta time_since_update_ = TimeDelta::Zero();
  double intensity_ = 0;
  DataSize pending_size_ = DataSize::Zero();
};
}  // namespace test
}  // namespace webrtc
#endif  // TEST_SCENARIO_NETWORK_NODE_H_
