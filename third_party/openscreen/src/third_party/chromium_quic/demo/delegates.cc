// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/chromium_quic/demo/delegates.h"

#include <stdio.h>

UdpTransport::UdpTransport(int fd, struct sockaddr_in peer_address)
    : fd_(fd), peer_address_(peer_address) {}
UdpTransport::~UdpTransport() = default;

int UdpTransport::Write(const char* buffer,
                        size_t buf_len,
                        const PacketInfo& info) {
  printf("writing...\n");
  return sendto(fd_, buffer, buf_len, 0, (struct sockaddr*)&peer_address_,
                sizeof(peer_address_));
}

StreamDelegate::StreamDelegate() = default;
StreamDelegate::~StreamDelegate() = default;

void StreamDelegate::OnReceived(quic::QuartcStream* stream,
                                const char* data,
                                size_t size) {
  if (!size) {
    printf("  -- (fin)\n");
    if (!stream->fin_sent()) {
      stream->WriteMemSlices(quic::QuicMemSliceSpan(quic::QuicMemSliceSpanImpl(
                                 nullptr, nullptr, 0)),
                             true);
    }
  } else {
    printf("  :: %*s\n", (int)size, data);
  }
}

void StreamDelegate::OnClose(quic::QuartcStream* stream) {
  closed_ = true;
  printf("stream closed\n");
}

void StreamDelegate::OnBufferChanged(quic::QuartcStream* stream) {}

SessionDelegate::SessionDelegate() = default;
SessionDelegate::~SessionDelegate() = default;

void SessionDelegate::OnCryptoHandshakeComplete() {
  printf("crypto handshake complete\n");
}

void SessionDelegate::OnIncomingStream(quic::QuartcStream* stream) {
  if (connection_closed_) {
    return;
  }
  auto next_delegate = std::make_unique<StreamDelegate>();
  stream->SetDelegate(next_delegate.get());
  stream_delegates_.push_back(std::move(next_delegate));
}

void SessionDelegate::OnConnectionClosed(quic::QuicErrorCode error_code,
                                         const quic::QuicString& error_details,
                                         quic::ConnectionCloseSource source) {
  connection_closed_ = true;
  stream_delegates_.clear();
  printf("connection closed: %s\n", error_details.c_str());
}

FakeTaskRunner::FakeTaskRunner(std::vector<FakeTask>* tasks) : tasks_(tasks) {}
FakeTaskRunner::~FakeTaskRunner() = default;

bool FakeTaskRunner::PostDelayedTask(const base::Location& whence,
                                     base::OnceClosure task,
                                     base::TimeDelta delay) {
  tasks_->push_back({whence, std::move(task), delay});
  return true;
}

bool FakeTaskRunner::RunsTasksInCurrentSequence() const {
  return true;
}
