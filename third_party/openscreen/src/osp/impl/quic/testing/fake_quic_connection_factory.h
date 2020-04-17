// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_TESTING_FAKE_QUIC_CONNECTION_FACTORY_H_
#define OSP_IMPL_QUIC_TESTING_FAKE_QUIC_CONNECTION_FACTORY_H_

#include <vector>

#include "osp/impl/quic/quic_connection_factory.h"
#include "osp/impl/quic/testing/fake_quic_connection.h"
#include "osp/public/message_demuxer.h"

namespace openscreen {

class FakeQuicConnectionFactoryBridge {
 public:
  explicit FakeQuicConnectionFactoryBridge(
      const IPEndpoint& controller_endpoint);

  bool idle() const { return idle_; }

  void OnConnectionClosed(QuicConnection* connection);
  void OnOutgoingStream(QuicConnection* connection, QuicStream* stream);

  void SetServerDelegate(QuicConnectionFactory::ServerDelegate* delegate,
                         const IPEndpoint& endpoint);
  void RunTasks();
  std::unique_ptr<QuicConnection> Connect(
      const IPEndpoint& endpoint,
      QuicConnection::Delegate* connection_delegate);

 private:
  struct ConnectionPair {
    FakeQuicConnection* controller;
    FakeQuicConnection* receiver;
  };

  const IPEndpoint controller_endpoint_;
  IPEndpoint receiver_endpoint_;
  bool idle_ = true;
  uint64_t next_connection_id_ = 0;
  bool connections_pending_ = true;
  ConnectionPair connections_ = {};
  QuicConnectionFactory::ServerDelegate* delegate_ = nullptr;
};

class FakeClientQuicConnectionFactory final : public QuicConnectionFactory {
 public:
  explicit FakeClientQuicConnectionFactory(
      FakeQuicConnectionFactoryBridge* bridge);
  ~FakeClientQuicConnectionFactory() override;

  // QuicConnectionFactory overrides.
  void SetServerDelegate(ServerDelegate* delegate,
                         const std::vector<IPEndpoint>& endpoints) override;
  void RunTasks() override;
  std::unique_ptr<QuicConnection> Connect(
      const IPEndpoint& endpoint,
      QuicConnection::Delegate* connection_delegate) override;

 private:
  FakeQuicConnectionFactoryBridge* bridge_;
};

class FakeServerQuicConnectionFactory final : public QuicConnectionFactory {
 public:
  explicit FakeServerQuicConnectionFactory(
      FakeQuicConnectionFactoryBridge* bridge);
  ~FakeServerQuicConnectionFactory() override;

  // QuicConnectionFactory overrides.
  void SetServerDelegate(ServerDelegate* delegate,
                         const std::vector<IPEndpoint>& endpoints) override;
  void RunTasks() override;
  std::unique_ptr<QuicConnection> Connect(
      const IPEndpoint& endpoint,
      QuicConnection::Delegate* connection_delegate) override;

 private:
  FakeQuicConnectionFactoryBridge* bridge_;
};

}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_TESTING_FAKE_QUIC_CONNECTION_FACTORY_H_
