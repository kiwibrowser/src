// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_TESTING_QUIC_TEST_SUPPORT_H_
#define OSP_IMPL_QUIC_TESTING_QUIC_TEST_SUPPORT_H_

#include <memory>

#include "osp/impl/quic/quic_client.h"
#include "osp/impl/quic/quic_server.h"
#include "osp/impl/quic/testing/fake_quic_connection_factory.h"
#include "platform/test/fake_clock.h"
#include "osp/public/message_demuxer.h"
#include "osp/public/network_metrics.h"
#include "osp/public/protocol_connection_client.h"
#include "osp/public/protocol_connection_server.h"
#include "osp_base/ip_address.h"
#include "platform/api/time.h"
#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"

namespace openscreen {

class MockServiceObserver final : public ProtocolConnectionServiceObserver {
 public:
  ~MockServiceObserver() override = default;

  MOCK_METHOD0(OnRunning, void());
  MOCK_METHOD0(OnStopped, void());
  MOCK_METHOD1(OnMetrics, void(const NetworkMetrics& metrics));
  MOCK_METHOD1(OnError, void(const Error& error));
};

class MockServerObserver final : public ProtocolConnectionServer::Observer {
 public:
  ~MockServerObserver() override = default;

  MOCK_METHOD0(OnRunning, void());
  MOCK_METHOD0(OnStopped, void());
  MOCK_METHOD1(OnMetrics, void(const NetworkMetrics& metrics));
  MOCK_METHOD1(OnError, void(const Error& error));

  MOCK_METHOD0(OnSuspended, void());

  void OnIncomingConnection(
      std::unique_ptr<ProtocolConnection> connection) override {
    OnIncomingConnectionMock(connection);
  }
  MOCK_METHOD1(OnIncomingConnectionMock,
               void(std::unique_ptr<ProtocolConnection>& connection));
};

class FakeQuicBridge {
 public:
  explicit FakeQuicBridge(platform::ClockNowFunctionPtr now_function);
  ~FakeQuicBridge();

  void RunTasksUntilIdle();

  const IPEndpoint kControllerEndpoint{{192, 168, 1, 3}, 4321};
  const IPEndpoint kReceiverEndpoint{{192, 168, 1, 17}, 1234};

  std::unique_ptr<MessageDemuxer> controller_demuxer;
  std::unique_ptr<MessageDemuxer> receiver_demuxer;
  std::unique_ptr<QuicClient> quic_client;
  std::unique_ptr<QuicServer> quic_server;
  std::unique_ptr<FakeQuicConnectionFactoryBridge> fake_bridge;
  MockServiceObserver mock_client_observer;
  MockServerObserver mock_server_observer;
};

}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_TESTING_QUIC_TEST_SUPPORT_H_
