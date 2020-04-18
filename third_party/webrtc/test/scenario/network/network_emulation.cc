/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "test/scenario/network/network_emulation.h"

#include <limits>
#include <memory>

#include "absl/memory/memory.h"
#include "rtc_base/bind.h"
#include "rtc_base/logging.h"

namespace webrtc {
namespace test {

EmulatedIpPacket::EmulatedIpPacket(const rtc::SocketAddress& from,
                                   const rtc::SocketAddress& to,
                                   uint64_t dest_endpoint_id,
                                   rtc::CopyOnWriteBuffer data,
                                   Timestamp arrival_time)
    : from(from),
      to(to),
      dest_endpoint_id(dest_endpoint_id),
      data(data),
      arrival_time(arrival_time) {}
EmulatedIpPacket::~EmulatedIpPacket() = default;
EmulatedIpPacket::EmulatedIpPacket(EmulatedIpPacket&&) = default;
EmulatedIpPacket& EmulatedIpPacket::operator=(EmulatedIpPacket&&) = default;

void EmulatedNetworkNode::CreateRoute(
    uint64_t receiver_id,
    std::vector<EmulatedNetworkNode*> nodes,
    EmulatedNetworkReceiverInterface* receiver) {
  RTC_CHECK(!nodes.empty());
  for (size_t i = 0; i + 1 < nodes.size(); ++i)
    nodes[i]->SetReceiver(receiver_id, nodes[i + 1]);
  nodes.back()->SetReceiver(receiver_id, receiver);
}

void EmulatedNetworkNode::ClearRoute(uint64_t receiver_id,
                                     std::vector<EmulatedNetworkNode*> nodes) {
  for (EmulatedNetworkNode* node : nodes)
    node->RemoveReceiver(receiver_id);
}

EmulatedNetworkNode::EmulatedNetworkNode(
    std::unique_ptr<NetworkBehaviorInterface> network_behavior)
    : network_behavior_(std::move(network_behavior)) {}

EmulatedNetworkNode::~EmulatedNetworkNode() = default;

void EmulatedNetworkNode::OnPacketReceived(EmulatedIpPacket packet) {
  rtc::CritScope crit(&lock_);
  if (routing_.find(packet.dest_endpoint_id) == routing_.end()) {
    return;
  }
  uint64_t packet_id = next_packet_id_++;
  bool sent = network_behavior_->EnqueuePacket(
      PacketInFlightInfo(packet.size(), packet.arrival_time.us(), packet_id));
  if (sent) {
    packets_.emplace_back(StoredPacket{packet_id, std::move(packet), false});
  }
}

void EmulatedNetworkNode::Process(Timestamp at_time) {
  std::vector<PacketDeliveryInfo> delivery_infos;
  {
    rtc::CritScope crit(&lock_);
    absl::optional<int64_t> delivery_us =
        network_behavior_->NextDeliveryTimeUs();
    if (delivery_us && *delivery_us > at_time.us())
      return;

    delivery_infos = network_behavior_->DequeueDeliverablePackets(at_time.us());
  }
  for (PacketDeliveryInfo& delivery_info : delivery_infos) {
    StoredPacket* packet = nullptr;
    EmulatedNetworkReceiverInterface* receiver = nullptr;
    {
      rtc::CritScope crit(&lock_);
      for (auto& stored_packet : packets_) {
        if (stored_packet.id == delivery_info.packet_id) {
          packet = &stored_packet;
          break;
        }
      }
      RTC_CHECK(packet);
      RTC_DCHECK(!packet->removed);
      receiver = routing_[packet->packet.dest_endpoint_id];
      packet->removed = true;
    }
    RTC_CHECK(receiver);
    // We don't want to keep the lock here. Otherwise we would get a deadlock if
    // the receiver tries to push a new packet.
    if (delivery_info.receive_time_us != PacketDeliveryInfo::kNotReceived) {
      packet->packet.arrival_time =
          Timestamp::us(delivery_info.receive_time_us);
      receiver->OnPacketReceived(std::move(packet->packet));
    }
    {
      rtc::CritScope crit(&lock_);
      while (!packets_.empty() && packets_.front().removed) {
        packets_.pop_front();
      }
    }
  }
}

void EmulatedNetworkNode::SetReceiver(
    uint64_t dest_endpoint_id,
    EmulatedNetworkReceiverInterface* receiver) {
  rtc::CritScope crit(&lock_);
  RTC_CHECK(routing_
                .insert(std::pair<uint64_t, EmulatedNetworkReceiverInterface*>(
                    dest_endpoint_id, receiver))
                .second)
      << "Routing for endpoint " << dest_endpoint_id << " already exists";
}

void EmulatedNetworkNode::RemoveReceiver(uint64_t dest_endpoint_id) {
  rtc::CritScope crit(&lock_);
  routing_.erase(dest_endpoint_id);
}

EndpointNode::EndpointNode(uint64_t id, rtc::IPAddress ip, Clock* clock)
    : id_(id),
      peer_local_addr_(ip),
      send_node_(nullptr),
      clock_(clock),
      next_port_(kFirstEphemeralPort),
      connected_endpoint_id_(absl::nullopt) {}
EndpointNode::~EndpointNode() = default;

uint64_t EndpointNode::GetId() const {
  return id_;
}

void EndpointNode::SetSendNode(EmulatedNetworkNode* send_node) {
  send_node_ = send_node;
}

void EndpointNode::SendPacket(const rtc::SocketAddress& from,
                              const rtc::SocketAddress& to,
                              rtc::CopyOnWriteBuffer packet) {
  RTC_CHECK(from.ipaddr() == peer_local_addr_);
  RTC_CHECK(connected_endpoint_id_);
  RTC_CHECK(send_node_);
  send_node_->OnPacketReceived(EmulatedIpPacket(
      from, to, connected_endpoint_id_.value(), std::move(packet),
      Timestamp::us(clock_->TimeInMicroseconds())));
}

absl::optional<uint16_t> EndpointNode::BindReceiver(
    uint16_t desired_port,
    EmulatedNetworkReceiverInterface* receiver) {
  rtc::CritScope crit(&receiver_lock_);
  uint16_t port = desired_port;
  if (port == 0) {
    // Because client can specify its own port, next_port_ can be already in
    // use, so we need to find next available port.
    int ports_pool_size =
        std::numeric_limits<uint16_t>::max() - kFirstEphemeralPort + 1;
    for (int i = 0; i < ports_pool_size; ++i) {
      uint16_t next_port = NextPort();
      if (port_to_receiver_.find(next_port) == port_to_receiver_.end()) {
        port = next_port;
        break;
      }
    }
  }
  RTC_CHECK(port != 0) << "Can't find free port for receiver in endpoint "
                       << id_;
  bool result = port_to_receiver_.insert({port, receiver}).second;
  if (!result) {
    RTC_LOG(INFO) << "Can't bind receiver to used port " << desired_port
                  << " in endpoint " << id_;
    return absl::nullopt;
  }
  RTC_LOG(INFO) << "New receiver is binded to endpoint " << id_ << " on port "
                << port;
  return port;
}

uint16_t EndpointNode::NextPort() {
  uint16_t out = next_port_;
  if (next_port_ == std::numeric_limits<uint16_t>::max()) {
    next_port_ = kFirstEphemeralPort;
  } else {
    next_port_++;
  }
  return out;
}

void EndpointNode::UnbindReceiver(uint16_t port) {
  rtc::CritScope crit(&receiver_lock_);
  port_to_receiver_.erase(port);
}

rtc::IPAddress EndpointNode::GetPeerLocalAddress() const {
  return peer_local_addr_;
}

void EndpointNode::OnPacketReceived(EmulatedIpPacket packet) {
  RTC_CHECK(packet.dest_endpoint_id == id_)
      << "Routing error: wrong destination endpoint. Destination id: "
      << packet.dest_endpoint_id << "; Receiver id: " << id_;
  rtc::CritScope crit(&receiver_lock_);
  auto it = port_to_receiver_.find(packet.to.port());
  if (it == port_to_receiver_.end()) {
    // It can happen, that remote peer closed connection, but there still some
    // packets, that are going to it. It can happen during peer connection close
    // process: one peer closed connection, second still sending data.
    RTC_LOG(INFO) << "No receiver registered in " << id_ << " on port "
                  << packet.to.port();
    return;
  }
  // Endpoint assumes frequent calls to bind and unbind methods, so it holds
  // lock during packet processing to ensure that receiver won't be deleted
  // before call to OnPacketReceived.
  it->second->OnPacketReceived(std::move(packet));
}

EmulatedNetworkNode* EndpointNode::GetSendNode() const {
  return send_node_;
}

void EndpointNode::SetConnectedEndpointId(uint64_t endpoint_id) {
  connected_endpoint_id_ = endpoint_id;
}

}  // namespace test
}  // namespace webrtc
