// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_TESTING_FAKE_QUIC_CONNECTION_H_
#define OSP_IMPL_QUIC_TESTING_FAKE_QUIC_CONNECTION_H_

#include <map>
#include <vector>

#include "osp/impl/quic/quic_connection.h"

namespace openscreen {

class FakeQuicConnectionFactoryBridge;

class FakeQuicStream final : public QuicStream {
 public:
  FakeQuicStream(Delegate* delegate, uint64_t id);
  ~FakeQuicStream() override;

  void ReceiveData(const uint8_t* data, size_t size);
  void CloseReadEnd();

  std::vector<uint8_t> TakeReceivedData();
  std::vector<uint8_t> TakeWrittenData();

  bool both_ends_closed() const {
    return write_end_closed_ && read_end_closed_;
  }
  bool write_end_closed() const { return write_end_closed_; }
  bool read_end_closed() const { return read_end_closed_; }

  Delegate* delegate() { return delegate_; }

  void Write(const uint8_t* data, size_t size) override;
  void CloseWriteEnd() override;

 private:
  bool write_end_closed_ = false;
  bool read_end_closed_ = false;
  std::vector<uint8_t> write_buffer_;
  std::vector<uint8_t> read_buffer_;
};

class FakeQuicConnection final : public QuicConnection {
 public:
  FakeQuicConnection(FakeQuicConnectionFactoryBridge* parent_factory,
                     uint64_t connection_id,
                     Delegate* delegate);
  ~FakeQuicConnection() override;

  Delegate* delegate() { return delegate_; }
  uint64_t id() const { return connection_id_; }
  std::map<uint64_t, FakeQuicStream*>& streams() { return streams_; }

  std::unique_ptr<FakeQuicStream> MakeIncomingStream();

  // QuicConnection overrides.
  void OnDataReceived(const platform::ReceivedData& data) override;
  std::unique_ptr<QuicStream> MakeOutgoingStream(
      QuicStream::Delegate* delegate) override;
  void Close() override;

 private:
  FakeQuicConnectionFactoryBridge* const parent_factory_;
  const uint64_t connection_id_;
  uint64_t next_stream_id_ = 1;
  std::map<uint64_t, FakeQuicStream*> streams_;
};

}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_TESTING_FAKE_QUIC_CONNECTION_H_
