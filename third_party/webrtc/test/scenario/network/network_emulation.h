/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_SCENARIO_NETWORK_NETWORK_EMULATION_H_
#define TEST_SCENARIO_NETWORK_NETWORK_EMULATION_H_

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "api/test/simulated_network.h"
#include "api/units/timestamp.h"
#include "rtc_base/async_socket.h"
#include "rtc_base/copy_on_write_buffer.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/socket_address.h"
#include "rtc_base/thread.h"
#include "system_wrappers/include/clock.h"

namespace webrtc {
namespace test {

struct EmulatedIpPacket {
 public:
  EmulatedIpPacket(const rtc::SocketAddress& from,
                   const rtc::SocketAddress& to,
                   uint64_t dest_endpoint_id,
                   rtc::CopyOnWriteBuffer data,
                   Timestamp arrival_time);
  ~EmulatedIpPacket();
  // This object is not copyable or assignable.
  EmulatedIpPacket(const EmulatedIpPacket&) = delete;
  EmulatedIpPacket& operator=(const EmulatedIpPacket&) = delete;
  // This object is only moveable.
  EmulatedIpPacket(EmulatedIpPacket&&);
  EmulatedIpPacket& operator=(EmulatedIpPacket&&);

  size_t size() const { return data.size(); }
  const uint8_t* cdata() const { return data.cdata(); }

  rtc::SocketAddress from;
  rtc::SocketAddress to;
  uint64_t dest_endpoint_id;
  rtc::CopyOnWriteBuffer data;
  Timestamp arrival_time;
};

class EmulatedNetworkReceiverInterface {
 public:
  virtual ~EmulatedNetworkReceiverInterface() = default;

  virtual void OnPacketReceived(EmulatedIpPacket packet) = 0;
};

// Represents node in the emulated network. Nodes can be connected with each
// other to form different networks with different behavior. The behavior of
// the node itself is determined by a concrete implementation of
// NetworkBehaviorInterface that is provided on construction.
class EmulatedNetworkNode : public EmulatedNetworkReceiverInterface {
 public:
  // Creates node based on |network_behavior|. The specified |packet_overhead|
  // is added to the size of each packet in the information provided to
  // |network_behavior|.
  explicit EmulatedNetworkNode(
      std::unique_ptr<NetworkBehaviorInterface> network_behavior);
  ~EmulatedNetworkNode() override;
  RTC_DISALLOW_COPY_AND_ASSIGN(EmulatedNetworkNode);

  void OnPacketReceived(EmulatedIpPacket packet) override;
  void Process(Timestamp at_time);
  void SetReceiver(uint64_t dest_endpoint_id,
                   EmulatedNetworkReceiverInterface* receiver);
  void RemoveReceiver(uint64_t dest_endpoint_id);

  // Creates a route for the given receiver_id over all the given nodes to the
  // given receiver.
  static void CreateRoute(uint64_t receiver_id,
                          std::vector<EmulatedNetworkNode*> nodes,
                          EmulatedNetworkReceiverInterface* receiver);
  static void ClearRoute(uint64_t receiver_id,
                         std::vector<EmulatedNetworkNode*> nodes);

 private:
  struct StoredPacket {
    uint64_t id;
    EmulatedIpPacket packet;
    bool removed;
  };

  rtc::CriticalSection lock_;
  std::map<uint64_t, EmulatedNetworkReceiverInterface*> routing_
      RTC_GUARDED_BY(lock_);
  const std::unique_ptr<NetworkBehaviorInterface> network_behavior_
      RTC_GUARDED_BY(lock_);
  std::deque<StoredPacket> packets_ RTC_GUARDED_BY(lock_);

  uint64_t next_packet_id_ RTC_GUARDED_BY(lock_) = 1;
};

// Represents single network interface on the device.
// It will be used as sender from socket side to send data to the network and
// will act as packet receiver from emulated network side to receive packets
// from other EmulatedNetworkNodes.
class EndpointNode : public EmulatedNetworkReceiverInterface {
 public:
  EndpointNode(uint64_t id, rtc::IPAddress, Clock* clock);
  ~EndpointNode() override;

  uint64_t GetId() const;

  // Set network node, that will be used to send packets to the network.
  void SetSendNode(EmulatedNetworkNode* send_node);
  // Send packet into network.
  // |from| will be used to set source address for the packet in destination
  // socket.
  // |to| will be used for routing verification and picking right socket by port
  // on destination endpoint.
  void SendPacket(const rtc::SocketAddress& from,
                  const rtc::SocketAddress& to,
                  rtc::CopyOnWriteBuffer packet);

  // Binds receiver to this endpoint to send and receive data.
  // |desired_port| is a port that should be used. If it is equal to 0,
  // endpoint will pick the first available port starting from
  // |kFirstEphemeralPort|.
  //
  // Returns the port, that should be used (it will be equals to desired, if
  // |desired_port| != 0 and is free or will be the one, selected by endpoint)
  // or absl::nullopt if desired_port in used. Also fails if there are no more
  // free ports to bind to.
  absl::optional<uint16_t> BindReceiver(
      uint16_t desired_port,
      EmulatedNetworkReceiverInterface* receiver);
  void UnbindReceiver(uint16_t port);

  rtc::IPAddress GetPeerLocalAddress() const;

  // Will be called to deliver packet into endpoint from network node.
  void OnPacketReceived(EmulatedIpPacket packet) override;

 protected:
  friend class NetworkEmulationManager;

  EmulatedNetworkNode* GetSendNode() const;
  void SetConnectedEndpointId(uint64_t endpoint_id);

 private:
  static constexpr uint16_t kFirstEphemeralPort = 49152;
  uint16_t NextPort() RTC_EXCLUSIVE_LOCKS_REQUIRED(receiver_lock_);

  rtc::CriticalSection receiver_lock_;

  uint64_t id_;
  // Peer's local IP address for this endpoint network interface.
  const rtc::IPAddress peer_local_addr_;
  EmulatedNetworkNode* send_node_;
  Clock* const clock_;

  uint16_t next_port_ RTC_GUARDED_BY(receiver_lock_);
  std::map<uint16_t, EmulatedNetworkReceiverInterface*> port_to_receiver_
      RTC_GUARDED_BY(receiver_lock_);

  absl::optional<uint64_t> connected_endpoint_id_;
};

}  // namespace test
}  // namespace webrtc

#endif  // TEST_SCENARIO_NETWORK_NETWORK_EMULATION_H_
