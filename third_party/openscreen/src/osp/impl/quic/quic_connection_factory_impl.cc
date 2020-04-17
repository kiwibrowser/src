// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/quic/quic_connection_factory_impl.h"

#include <algorithm>
#include <memory>

#include "osp/impl/quic/quic_connection_impl.h"
#include "osp_base/error.h"
#include "platform/api/logging.h"
#include "platform/base/event_loop.h"
#include "third_party/chromium_quic/src/base/location.h"
#include "third_party/chromium_quic/src/base/task_runner.h"
#include "third_party/chromium_quic/src/net/third_party/quic/core/quic_constants.h"
#include "third_party/chromium_quic/src/net/third_party/quic/platform/impl/quic_chromium_clock.h"

namespace openscreen {

struct Task {
  ::base::Location whence;
  ::base::OnceClosure task;
  ::base::TimeDelta delay;
};

class QuicTaskRunner final : public ::base::TaskRunner {
 public:
  QuicTaskRunner();
  ~QuicTaskRunner() override;

  void RunTasks();

  // base::TaskRunner overrides.
  bool PostDelayedTask(const ::base::Location& whence,
                       ::base::OnceClosure task,
                       ::base::TimeDelta delay) override;

  bool RunsTasksInCurrentSequence() const override;

 private:
  uint64_t last_run_unix_;
  std::list<Task> tasks_;
};

QuicTaskRunner::QuicTaskRunner() = default;
QuicTaskRunner::~QuicTaskRunner() = default;

void QuicTaskRunner::RunTasks() {
  auto* clock = ::quic::QuicChromiumClock::GetInstance();
  ::quic::QuicWallTime now = clock->WallNow();
  uint64_t now_unix = now.ToUNIXMicroseconds();
  for (auto it = tasks_.begin(); it != tasks_.end();) {
    Task& next_task = *it;
    next_task.delay -=
        ::base::TimeDelta::FromMicroseconds(now_unix - last_run_unix_);
    if (next_task.delay.InMicroseconds() < 0) {
      std::move(next_task.task).Run();
      it = tasks_.erase(it);
    } else {
      ++it;
    }
  }
  last_run_unix_ = now_unix;
}

bool QuicTaskRunner::PostDelayedTask(const ::base::Location& whence,
                                     ::base::OnceClosure task,
                                     ::base::TimeDelta delay) {
  tasks_.push_back({whence, std::move(task), delay});
  return true;
}

bool QuicTaskRunner::RunsTasksInCurrentSequence() const {
  return true;
}

QuicConnectionFactoryImpl::QuicConnectionFactoryImpl() {
  task_runner_ = ::base::MakeRefCounted<QuicTaskRunner>();
  alarm_factory_ = std::make_unique<::net::QuicChromiumAlarmFactory>(
      task_runner_.get(), ::quic::QuicChromiumClock::GetInstance());
  ::quic::QuartcFactoryConfig factory_config;
  factory_config.alarm_factory = alarm_factory_.get();
  factory_config.clock = ::quic::QuicChromiumClock::GetInstance();
  quartc_factory_ = std::make_unique<::quic::QuartcFactory>(factory_config);
  waiter_ = platform::CreateEventWaiter();
}

QuicConnectionFactoryImpl::~QuicConnectionFactoryImpl() {
  OSP_DCHECK(connections_.empty());
  platform::DestroyEventWaiter(waiter_);
}

void QuicConnectionFactoryImpl::SetServerDelegate(
    ServerDelegate* delegate,
    const std::vector<IPEndpoint>& endpoints) {
  OSP_DCHECK(!delegate != !server_delegate_);

  server_delegate_ = delegate;
  sockets_.reserve(sockets_.size() + endpoints.size());

  for (const auto& endpoint : endpoints) {
    // TODO(mfoltz): Need to notify the caller and/or ServerDelegate if socket
    // create/bind errors occur. Maybe return an Error immediately, and undo
    // partial progress (i.e. "unwatch" all the sockets and call
    // sockets_.clear() to close the sockets)?
    auto create_result =
        platform::UdpSocket::Create(endpoint.address.version());
    if (!create_result) {
      OSP_LOG_ERROR << "failed to create socket (for " << endpoint
                    << "): " << create_result.error().message();
      continue;
    }
    platform::UdpSocketUniquePtr server_socket = create_result.MoveValue();
    Error bind_result = server_socket->Bind(endpoint);
    if (!bind_result.ok()) {
      OSP_LOG_ERROR << "failed to bind socket (for " << endpoint
                    << "): " << bind_result.message();
      continue;
    }
    platform::WatchUdpSocketReadable(waiter_, server_socket.get());
    sockets_.emplace_back(std::move(server_socket));
  }
}

void QuicConnectionFactoryImpl::RunTasks() {
  for (const auto& packet : platform::OnePlatformLoopIteration(waiter_)) {
    // Ensure that |packet.socket| is one of the instances owned by
    // QuicConnectionFactoryImpl.
    OSP_DCHECK(std::find_if(sockets_.begin(), sockets_.end(),
                            [&packet](const platform::UdpSocketUniquePtr& s) {
                              return s.get() == packet.socket;
                            }) != sockets_.end());

    // TODO(btolsch): We will need to rethink this both for ICE and connection
    // migration support.
    auto conn_it = connections_.find(packet.source);
    if (conn_it == connections_.end()) {
      if (server_delegate_) {
        OSP_VLOG << __func__ << ": spawning connection from " << packet.source;
        auto transport =
            std::make_unique<UdpTransport>(packet.socket, packet.source);
        ::quic::QuartcSessionConfig session_config;
        session_config.perspective = ::quic::Perspective::IS_SERVER;
        session_config.packet_transport = transport.get();

        auto result = std::make_unique<QuicConnectionImpl>(
            this, server_delegate_->NextConnectionDelegate(packet.source),
            std::move(transport),
            quartc_factory_->CreateQuartcSession(session_config));
        auto* result_ptr = result.get();
        connections_.emplace(packet.source,
                             OpenConnection{result_ptr, packet.socket});
        server_delegate_->OnIncomingConnection(std::move(result));
        result_ptr->OnDataReceived(packet);
      }
    } else {
      OSP_VLOG << __func__ << ": data for existing connection from "
               << packet.source;
      conn_it->second.connection->OnDataReceived(packet);
    }
  }
}

std::unique_ptr<QuicConnection> QuicConnectionFactoryImpl::Connect(
    const IPEndpoint& endpoint,
    QuicConnection::Delegate* connection_delegate) {
  auto create_result = platform::UdpSocket::Create(endpoint.address.version());
  if (!create_result) {
    OSP_LOG_ERROR << "failed to create socket: "
                  << create_result.error().message();
    // TODO(mfoltz): This method should return ErrorOr<uni_ptr<QuicConnection>>.
    return nullptr;
  }
  platform::UdpSocketUniquePtr socket = create_result.MoveValue();
  auto transport = std::make_unique<UdpTransport>(socket.get(), endpoint);

  ::quic::QuartcSessionConfig session_config;
  session_config.perspective = ::quic::Perspective::IS_CLIENT;
  // TODO(btolsch): Proper server id.  Does this go in the QUIC server name
  // parameter?
  session_config.unique_remote_server_id = "turtle";
  session_config.packet_transport = transport.get();

  auto result = std::make_unique<QuicConnectionImpl>(
      this, connection_delegate, std::move(transport),
      quartc_factory_->CreateQuartcSession(session_config));

  platform::WatchUdpSocketReadable(waiter_, socket.get());

  // TODO(btolsch): This presents a problem for multihomed receivers, which may
  // register as a different endpoint in their response.  I think QUIC is
  // already tolerant of this via connection IDs but this hasn't been tested
  // (and even so, those aren't necessarily stable either).
  connections_.emplace(endpoint, OpenConnection{result.get(), socket.get()});
  sockets_.emplace_back(std::move(socket));

  return result;
}

void QuicConnectionFactoryImpl::OnConnectionClosed(QuicConnection* connection) {
  auto entry = std::find_if(
      connections_.begin(), connections_.end(),
      [connection](const decltype(connections_)::value_type& entry) {
        return entry.second.connection == connection;
      });
  OSP_DCHECK(entry != connections_.end());
  platform::UdpSocket* const socket = entry->second.socket;
  connections_.erase(entry);

  // If none of the remaining |connections_| reference the socket, close/destroy
  // it.
  if (std::find_if(connections_.begin(), connections_.end(),
                   [socket](const decltype(connections_)::value_type& entry) {
                     return entry.second.socket == socket;
                   }) == connections_.end()) {
    platform::StopWatchingUdpSocketReadable(waiter_, socket);
    auto socket_it =
        std::find_if(sockets_.begin(), sockets_.end(),
                     [socket](const platform::UdpSocketUniquePtr& s) {
                       return s.get() == socket;
                     });
    OSP_DCHECK(socket_it != sockets_.end());
    sockets_.erase(socket_it);
  }
}

}  // namespace openscreen
