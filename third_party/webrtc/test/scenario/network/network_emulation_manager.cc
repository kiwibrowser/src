/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "test/scenario/network/network_emulation_manager.h"

#include <algorithm>
#include <memory>

#include "absl/memory/memory.h"
#include "api/units/time_delta.h"
#include "api/units/timestamp.h"

namespace webrtc {
namespace test {
namespace {

constexpr int64_t kPacketProcessingIntervalMs = 1;

}  // namespace

NetworkEmulationManager::NetworkEmulationManager(webrtc::Clock* clock)
    : clock_(clock),
      next_node_id_(1),
      task_queue_("network_emulation_manager") {}
NetworkEmulationManager::~NetworkEmulationManager() {
  Stop();
}

EmulatedNetworkNode* NetworkEmulationManager::CreateEmulatedNode(
    std::unique_ptr<NetworkBehaviorInterface> network_behavior) {
  auto node =
      absl::make_unique<EmulatedNetworkNode>(std::move(network_behavior));
  EmulatedNetworkNode* out = node.get();

  struct Closure {
    void operator()() { manager->network_nodes_.push_back(std::move(node)); }
    NetworkEmulationManager* manager;
    std::unique_ptr<EmulatedNetworkNode> node;
  };
  task_queue_.PostTask(Closure{this, std::move(node)});
  return out;
}

EndpointNode* NetworkEmulationManager::CreateEndpoint(rtc::IPAddress ip) {
  auto node = absl::make_unique<EndpointNode>(next_node_id_++, ip, clock_);
  EndpointNode* out = node.get();
  endpoints_.push_back(std::move(node));
  return out;
}

void NetworkEmulationManager::CreateRoute(
    EndpointNode* from,
    std::vector<EmulatedNetworkNode*> via_nodes,
    EndpointNode* to) {
  // Because endpoint has no send node by default at least one should be
  // provided here.
  RTC_CHECK(!via_nodes.empty());

  from->SetSendNode(via_nodes[0]);
  EmulatedNetworkNode* cur_node = via_nodes[0];
  for (size_t i = 1; i < via_nodes.size(); ++i) {
    cur_node->SetReceiver(to->GetId(), via_nodes[i]);
    cur_node = via_nodes[i];
  }
  cur_node->SetReceiver(to->GetId(), to);
  from->SetConnectedEndpointId(to->GetId());
}

void NetworkEmulationManager::ClearRoute(
    EndpointNode* from,
    std::vector<EmulatedNetworkNode*> via_nodes,
    EndpointNode* to) {
  // Remove receiver from intermediate nodes.
  for (auto* node : via_nodes) {
    node->RemoveReceiver(to->GetId());
  }
  // Detach endpoint from current send node.
  if (from->GetSendNode()) {
    from->GetSendNode()->RemoveReceiver(to->GetId());
    from->SetSendNode(nullptr);
  }
}

rtc::Thread* NetworkEmulationManager::CreateNetworkThread(
    std::vector<EndpointNode*> endpoints) {
  FakeNetworkSocketServer* socket_server = CreateSocketServer(endpoints);
  std::unique_ptr<rtc::Thread> network_thread =
      absl::make_unique<rtc::Thread>(socket_server);
  network_thread->SetName("network_thread" + std::to_string(threads_.size()),
                          nullptr);
  network_thread->Start();
  rtc::Thread* out = network_thread.get();
  threads_.push_back(std::move(network_thread));
  return out;
}

void NetworkEmulationManager::Start() {
  process_task_handle_ = RepeatingTaskHandle::Start(&task_queue_, [this] {
    ProcessNetworkPackets();
    return TimeDelta::ms(kPacketProcessingIntervalMs);
  });
}

void NetworkEmulationManager::Stop() {
  process_task_handle_.PostStop();
}

FakeNetworkSocketServer* NetworkEmulationManager::CreateSocketServer(
    std::vector<EndpointNode*> endpoints) {
  auto socket_server =
      absl::make_unique<FakeNetworkSocketServer>(clock_, endpoints);
  FakeNetworkSocketServer* out = socket_server.get();
  socket_servers_.push_back(std::move(socket_server));
  return out;
}

void NetworkEmulationManager::ProcessNetworkPackets() {
  Timestamp current_time = Now();
  for (auto& node : network_nodes_) {
    node->Process(current_time);
  }
}

Timestamp NetworkEmulationManager::Now() const {
  return Timestamp::us(clock_->TimeInMicroseconds());
}

}  // namespace test
}  // namespace webrtc
