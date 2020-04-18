// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MIRRORING_SERVICE_FAKE_NETWORK_SERVICE_H_
#define COMPONENTS_MIRRORING_SERVICE_FAKE_NETWORK_SERVICE_H_

#include "base/callback.h"
#include "media/cast/net/cast_transport_config.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/network/public/mojom/udp_socket.mojom.h"
#include "services/network/test/test_network_context.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mirroring {

class MockUdpSocket final : public network::mojom::UDPSocket {
 public:
  MockUdpSocket(network::mojom::UDPSocketRequest request,
                network::mojom::UDPSocketReceiverPtr receiver);
  ~MockUdpSocket() override;

  MOCK_METHOD0(OnSend, void());

  // network::mojom::UDPSocket implementation.
  void Connect(const net::IPEndPoint& remote_addr,
               network::mojom::UDPSocketOptionsPtr options,
               ConnectCallback callback) override;
  void Bind(const net::IPEndPoint& local_addr,
            network::mojom::UDPSocketOptionsPtr options,
            BindCallback callback) override {}
  void SetBroadcast(bool broadcast, SetBroadcastCallback callback) override {}
  void JoinGroup(const net::IPAddress& group_address,
                 JoinGroupCallback callback) override {}
  void LeaveGroup(const net::IPAddress& group_address,
                  LeaveGroupCallback callback) override {}
  void ReceiveMore(uint32_t num_additional_datagrams) override;
  void ReceiveMoreWithBufferSize(uint32_t num_additional_datagrams,
                                 uint32_t buffer_size) override {}
  void SendTo(const net::IPEndPoint& dest_addr,
              base::span<const uint8_t> data,
              const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
              SendToCallback callback) override {}
  void Send(base::span<const uint8_t> data,
            const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
            SendCallback callback) override;
  void Close() override {}

  // Simulate receiving a packet from the network.
  void OnReceivedPacket(const media::cast::Packet& packet);

  void VerifySendingPacket(const media::cast::Packet& packet);

 private:
  mojo::Binding<network::mojom::UDPSocket> binding_;
  network::mojom::UDPSocketReceiverPtr receiver_;
  std::unique_ptr<media::cast::Packet> sending_packet_;
  int num_ask_for_receive_ = 0;

  DISALLOW_COPY_AND_ASSIGN(MockUdpSocket);
};

class MockNetworkContext final : public network::TestNetworkContext {
 public:
  explicit MockNetworkContext(network::mojom::NetworkContextRequest request);
  ~MockNetworkContext() override;

  MOCK_METHOD0(OnUDPSocketCreated, void());

  // network::mojom::NetworkContext implementation:
  void CreateUDPSocket(network::mojom::UDPSocketRequest request,
                       network::mojom::UDPSocketReceiverPtr receiver) override;
  void CreateURLLoaderFactory(
      network::mojom::URLLoaderFactoryRequest request,
      network::mojom::URLLoaderFactoryParamsPtr params) override;

  MockUdpSocket* udp_socket() const { return udp_socket_.get(); }

 private:
  mojo::Binding<network::mojom::NetworkContext> binding_;
  std::unique_ptr<MockUdpSocket> udp_socket_;
  DISALLOW_COPY_AND_ASSIGN(MockNetworkContext);
};

}  // namespace mirroring

#endif  // COMPONENTS_MIRRORING_SERVICE_FAKE_NETWORK_SERVICE_H_
