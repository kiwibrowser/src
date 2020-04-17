// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_CHROMIUM_QUIC_DEMO_DELEGATES_H_
#define THIRD_PARTY_CHROMIUM_QUIC_DEMO_DELEGATES_H_

#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <memory>
#include <vector>

#include "base/task/post_task.h"
#include "net/third_party/quic/quartc/quartc_packet_writer.h"
#include "net/third_party/quic/quartc/quartc_session.h"
#include "net/third_party/quic/quartc/quartc_stream.h"

class UdpTransport final : public quic::QuartcPacketTransport {
 public:
  UdpTransport(int fd, struct sockaddr_in peer_address);
  ~UdpTransport() override;

  int Write(const char* buffer,
            size_t buf_len,
            const PacketInfo& info) override;

 private:
  int fd_;
  struct sockaddr_in peer_address_;
};

class StreamDelegate final : public quic::QuartcStream::Delegate {
 public:
  StreamDelegate();
  ~StreamDelegate() override;

  void OnReceived(quic::QuartcStream* stream,
                  const char* data,
                  size_t size) override;
  void OnClose(quic::QuartcStream* stream) override;
  void OnBufferChanged(quic::QuartcStream* stream) override;

  bool closed() const { return closed_; }

 private:
  bool closed_ = false;
};

class SessionDelegate final : public quic::QuartcSession::Delegate {
 public:
  SessionDelegate();
  ~SessionDelegate() override;

  void OnCryptoHandshakeComplete() override;
  void OnIncomingStream(quic::QuartcStream* stream) override;
  void OnConnectionClosed(quic::QuicErrorCode error_code,
                          const quic::QuicString& error_details,
                          quic::ConnectionCloseSource source) override;

  bool last_stream_closed() const {
    return stream_delegates_.empty() ? false
                                     : stream_delegates_.back()->closed();
  }
  bool connection_closed() const { return connection_closed_; }

 private:
  std::vector<std::unique_ptr<StreamDelegate>> stream_delegates_;
  bool connection_closed_ = false;
};

struct FakeTask {
  base::Location whence;
  base::OnceClosure task;
  base::TimeDelta delay;
};

class FakeTaskRunner : public base::TaskRunner {
 public:
  explicit FakeTaskRunner(std::vector<FakeTask>* tasks);

  bool PostDelayedTask(const base::Location& whence,
                       base::OnceClosure task,
                       base::TimeDelta delay) override;

  bool RunsTasksInCurrentSequence() const override;

 private:
  ~FakeTaskRunner() override;

  std::vector<FakeTask>* tasks_;
};

#endif  // THIRD_PARTY_CHROMIUM_QUIC_DEMO_DELEGATES_H_
