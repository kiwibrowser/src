// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_QUIC_CONNECTION_H_
#define OSP_IMPL_QUIC_QUIC_CONNECTION_H_

#include <memory>
#include <vector>

#include "platform/api/udp_socket.h"
#include "platform/base/event_loop.h"

namespace openscreen {

class QuicStream {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual void OnReceived(QuicStream* stream,
                            const char* data,
                            size_t data_size) = 0;
    virtual void OnClose(uint64_t stream_id) = 0;
  };

  QuicStream(Delegate* delegate, uint64_t id) : delegate_(delegate), id_(id) {}
  virtual ~QuicStream() = default;

  uint64_t id() const { return id_; }
  virtual void Write(const uint8_t* data, size_t data_size) = 0;
  virtual void CloseWriteEnd() = 0;

 protected:
  Delegate* const delegate_;
  uint64_t id_;
};

class QuicConnection {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the QUIC handshake has successfully completed.
    virtual void OnCryptoHandshakeComplete(uint64_t connection_id) = 0;

    // Called when a new stream on this connection is initiated by the other
    // endpoint.  |stream| will use a delegate returned by NextStreamDelegate.
    virtual void OnIncomingStream(uint64_t connection_id,
                                  std::unique_ptr<QuicStream> stream) = 0;

    // Called when the QUIC connection was closed.  The QuicConnection should
    // not be destroyed immediately, because the QUIC implementation will still
    // reference it briefly.  Instead, it should be destroyed during the next
    // event loop.
    // TODO(btolsch): Hopefully this can be changed with future QUIC
    // implementations.
    virtual void OnConnectionClosed(uint64_t connection_id) = 0;

    // This is used to get a QuicStream::Delegate for an incoming stream, which
    // will be returned via OnIncomingStream immediately after this call.
    virtual QuicStream::Delegate* NextStreamDelegate(uint64_t connection_id,
                                                     uint64_t stream_id) = 0;
  };

  explicit QuicConnection(Delegate* delegate) : delegate_(delegate) {}
  virtual ~QuicConnection() = default;

  // Passes a received UDP packet to the QUIC implementation.  If this contains
  // any stream data, it will be passed automatically to the relevant
  // QuicStream::Delegate objects.
  virtual void OnDataReceived(const platform::ReceivedData& data) = 0;

  virtual std::unique_ptr<QuicStream> MakeOutgoingStream(
      QuicStream::Delegate* delegate) = 0;
  virtual void Close() = 0;

 protected:
  Delegate* const delegate_;
};

}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_QUIC_CONNECTION_H_
