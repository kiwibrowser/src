/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_SCENARIO_NETWORK_NETWORK_EMULATION_MANAGER_H_
#define TEST_SCENARIO_NETWORK_NETWORK_EMULATION_MANAGER_H_

#include <memory>
#include <utility>
#include <vector>

#include "api/test/simulated_network.h"
#include "api/units/time_delta.h"
#include "api/units/timestamp.h"
#include "rtc_base/logging.h"
#include "rtc_base/task_queue.h"
#include "rtc_base/task_utils/repeating_task.h"
#include "rtc_base/thread.h"
#include "test/scenario/network/fake_network_socket_server.h"
#include "test/scenario/network/network_emulation.h"

namespace webrtc {
namespace test {

class NetworkEmulationManager {
 public:
  explicit NetworkEmulationManager(Clock* clock);
  ~NetworkEmulationManager();

  EmulatedNetworkNode* CreateEmulatedNode(
      std::unique_ptr<NetworkBehaviorInterface> network_behavior);

  // TODO(titovartem) add method without IP address, where manager
  // will provided some unique generated address.
  EndpointNode* CreateEndpoint(rtc::IPAddress ip);

  void CreateRoute(EndpointNode* from,
                   std::vector<EmulatedNetworkNode*> via_nodes,
                   EndpointNode* to);
  void ClearRoute(EndpointNode* from,
                  std::vector<EmulatedNetworkNode*> via_nodes,
                  EndpointNode* to);

  rtc::Thread* CreateNetworkThread(std::vector<EndpointNode*> endpoints);

  void Start();
  void Stop();

 private:
  FakeNetworkSocketServer* CreateSocketServer(
      std::vector<EndpointNode*> endpoints);
  void ProcessNetworkPackets();
  Timestamp Now() const;

  Clock* const clock_;
  int next_node_id_;

  RepeatingTaskHandle process_task_handle_;

  // All objects can be added to the manager only when it is idle.
  std::vector<std::unique_ptr<EndpointNode>> endpoints_;
  std::vector<std::unique_ptr<EmulatedNetworkNode>> network_nodes_;
  std::vector<std::unique_ptr<FakeNetworkSocketServer>> socket_servers_;
  std::vector<std::unique_ptr<rtc::Thread>> threads_;

  // Must be the last field, so it will be deleted first, because tasks
  // in the TaskQueue can access other fields of the instance of this class.
  rtc::TaskQueue task_queue_;
};

}  // namespace test
}  // namespace webrtc

#endif  // TEST_SCENARIO_NETWORK_NETWORK_EMULATION_MANAGER_H_
