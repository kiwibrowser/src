// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mirroring/service/fake_network_service.h"

#include "media/cast/test/utility/net_utility.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/network/test/test_url_loader_factory.h"

namespace mirroring {

MockUdpSocket::MockUdpSocket(network::mojom::UDPSocketRequest request,
                             network::mojom::UDPSocketReceiverPtr receiver)
    : binding_(this, std::move(request)), receiver_(std::move(receiver)) {}

MockUdpSocket::~MockUdpSocket() {}

void MockUdpSocket::Connect(const net::IPEndPoint& remote_addr,
                            network::mojom::UDPSocketOptionsPtr options,
                            ConnectCallback callback) {
  std::move(callback).Run(net::OK, media::cast::test::GetFreeLocalPort());
}

void MockUdpSocket::ReceiveMore(uint32_t num_additional_datagrams) {
  num_ask_for_receive_ += num_additional_datagrams;
}

void MockUdpSocket::Send(
    base::span<const uint8_t> data,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    SendCallback callback) {
  sending_packet_ =
      std::make_unique<media::cast::Packet>(data.begin(), data.end());
  std::move(callback).Run(net::OK);
  OnSend();
}

void MockUdpSocket::OnReceivedPacket(const media::cast::Packet& packet) {
  if (num_ask_for_receive_) {
    receiver_->OnReceived(
        net::OK, base::nullopt,
        base::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(packet.data()), packet.size()));
    ASSERT_LT(0, num_ask_for_receive_);
    --num_ask_for_receive_;
  }
}

void MockUdpSocket::VerifySendingPacket(const media::cast::Packet& packet) {
  EXPECT_TRUE(
      std::equal(packet.begin(), packet.end(), sending_packet_->begin()));
}

MockNetworkContext::MockNetworkContext(
    network::mojom::NetworkContextRequest request)
    : binding_(this, std::move(request)) {}
MockNetworkContext::~MockNetworkContext() {}

void MockNetworkContext::CreateUDPSocket(
    network::mojom::UDPSocketRequest request,
    network::mojom::UDPSocketReceiverPtr receiver) {
  udp_socket_ =
      std::make_unique<MockUdpSocket>(std::move(request), std::move(receiver));
  OnUDPSocketCreated();
}

void MockNetworkContext::CreateURLLoaderFactory(
    network::mojom::URLLoaderFactoryRequest request,
    network::mojom::URLLoaderFactoryParamsPtr params) {
  ASSERT_TRUE(params);
  mojo::MakeStrongBinding(std::make_unique<network::TestURLLoaderFactory>(),
                          std::move(request));
}

}  // namespace mirroring
