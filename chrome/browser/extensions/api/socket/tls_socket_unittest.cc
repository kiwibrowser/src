// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/callback_helpers.h"
#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "extensions/browser/api/socket/tls_socket.h"
#include "net/base/address_list.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/rand_callback.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/next_proto.h"
#include "net/socket/socket_tag.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/tcp_client_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Gt;
using testing::Return;
using testing::SaveArg;
using testing::WithArgs;
using base::StringPiece;

namespace extensions {

class MockSSLClientSocket : public net::SSLClientSocket {
 public:
  MockSSLClientSocket() {}
  int Read(net::IOBuffer* buffer,
           int bytes,
           net::CompletionOnceCallback callback) override {
    return Read(buffer, bytes,
                base::AdaptCallbackForRepeating(std::move(callback)));
  }

  int Write(net::IOBuffer* buffer,
            int bytes,
            net::CompletionOnceCallback callback,
            const net::NetworkTrafficAnnotationTag& tag) override {
    return Write(buffer, bytes,
                 base::AdaptCallbackForRepeating(std::move(callback)), tag);
  }

  int Connect(net::CompletionOnceCallback callback) override {
    return Connect(base::AdaptCallbackForRepeating(std::move(callback)));
  }

  MOCK_METHOD0(Disconnect, void());
  MOCK_METHOD3(Read,
               int(net::IOBuffer* buf,
                   int buf_len,
                   const net::CompletionCallback& callback));
  MOCK_METHOD4(Write,
               int(net::IOBuffer* buf,
                   int buf_len,
                   const net::CompletionCallback& callback,
                   const net::NetworkTrafficAnnotationTag&));
  MOCK_METHOD1(SetReceiveBufferSize, int(int32_t));
  MOCK_METHOD1(SetSendBufferSize, int(int32_t));
  MOCK_METHOD1(Connect, int(const CompletionCallback&));
  MOCK_CONST_METHOD0(IsConnectedAndIdle, bool());
  MOCK_CONST_METHOD1(GetPeerAddress, int(net::IPEndPoint*));
  MOCK_CONST_METHOD1(GetLocalAddress, int(net::IPEndPoint*));
  MOCK_CONST_METHOD0(NetLog, const net::NetLogWithSource&());
  MOCK_CONST_METHOD0(WasEverUsed, bool());
  MOCK_CONST_METHOD0(UsingTCPFastOpen, bool());
  MOCK_CONST_METHOD0(WasAlpnNegotiated, bool());
  MOCK_CONST_METHOD0(GetNegotiatedProtocol, net::NextProto());
  MOCK_METHOD1(GetSSLInfo, bool(net::SSLInfo*));
  MOCK_CONST_METHOD1(GetConnectionAttempts, void(net::ConnectionAttempts*));
  MOCK_METHOD0(ClearConnectionAttempts, void());
  MOCK_METHOD1(AddConnectionAttempts, void(const net::ConnectionAttempts&));
  MOCK_CONST_METHOD0(GetTotalReceivedBytes, int64_t());
  MOCK_METHOD1(ApplySocketTag, void(const net::SocketTag&));
  MOCK_METHOD5(ExportKeyingMaterial,
               int(const StringPiece&,
                   bool,
                   const StringPiece&,
                   unsigned char*,
                   unsigned int));
  MOCK_CONST_METHOD1(GetSSLCertRequestInfo, void(net::SSLCertRequestInfo*));
  MOCK_CONST_METHOD0(GetUnverifiedServerCertificateChain,
                     scoped_refptr<net::X509Certificate>());
  MOCK_CONST_METHOD0(GetChannelIDService, net::ChannelIDService*());
  MOCK_METHOD3(GetTokenBindingSignature,
               net::Error(crypto::ECPrivateKey*,
                          net::TokenBindingType,
                          std::vector<uint8_t>*));
  MOCK_CONST_METHOD0(GetChannelIDKey, crypto::ECPrivateKey*());
  bool IsConnected() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSSLClientSocket);
};

class MockTCPSocket : public net::TCPClientSocket {
 public:
  explicit MockTCPSocket(const net::AddressList& address_list)
      : net::TCPClientSocket(address_list, NULL, NULL, net::NetLogSource()) {}

  int Read(net::IOBuffer* buffer,
           int bytes,
           net::CompletionOnceCallback callback) override {
    return Read(buffer, bytes,
                base::AdaptCallbackForRepeating(std::move(callback)));
  }

  int Write(net::IOBuffer* buffer,
            int bytes,
            net::CompletionOnceCallback callback,
            const net::NetworkTrafficAnnotationTag& tag) override {
    return Write(buffer, bytes,
                 base::AdaptCallbackForRepeating(std::move(callback)), tag);
  }

  MOCK_METHOD3(Read,
               int(net::IOBuffer* buf,
                   int buf_len,
                   const net::CompletionCallback& callback));
  MOCK_METHOD4(Write,
               int(net::IOBuffer* buf,
                   int buf_len,
                   const net::CompletionCallback& callback,
                   const net::NetworkTrafficAnnotationTag&));
  MOCK_METHOD2(SetKeepAlive, bool(bool enable, int delay));
  MOCK_METHOD1(SetNoDelay, bool(bool no_delay));

  bool IsConnected() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTCPSocket);
};

class CompleteHandler {
 public:
  CompleteHandler() {}
  MOCK_METHOD1(OnComplete, void(int result_code));
  MOCK_METHOD3(OnReadComplete,
               void(int result_code,
                    scoped_refptr<net::IOBuffer> io_buffer,
                    bool socket_destroying));
  MOCK_METHOD2(OnAccept, void(int, net::TCPClientSocket*));

 private:
  DISALLOW_COPY_AND_ASSIGN(CompleteHandler);
};

class TLSSocketTest : public ::testing::Test {
 public:
  TLSSocketTest() {}

  void SetUp() override {
    net::AddressList address_list;
    // |ssl_socket_| is owned by |socket_|. TLSSocketTest keeps a pointer to
    // it to expect invocations from TLSSocket to |ssl_socket_|.
    std::unique_ptr<MockSSLClientSocket> ssl_sock(new MockSSLClientSocket);
    ssl_socket_ = ssl_sock.get();
    socket_.reset(new TLSSocket(std::move(ssl_sock), "test_extension_id"));
    EXPECT_CALL(*ssl_socket_, Disconnect()).Times(1);
  };

  void TearDown() override {
    ssl_socket_ = NULL;
    socket_.reset();
  };

 protected:
  MockSSLClientSocket* ssl_socket_;
  std::unique_ptr<TLSSocket> socket_;
};

// Verify that a Read() on TLSSocket will pass through into a Read() on
// |ssl_socket_| and invoke its completion callback.
TEST_F(TLSSocketTest, TestTLSSocketRead) {
  CompleteHandler handler;

  EXPECT_CALL(*ssl_socket_, Read(_, _, _)).Times(1);
  EXPECT_CALL(handler, OnReadComplete(_, _, _)).Times(1);

  const int count = 512;
  socket_->Read(
      count,
      base::Bind(&CompleteHandler::OnReadComplete, base::Unretained(&handler)));
}

// Verify that a Write() on a TLSSocket will pass through to Write()
// invocations on |ssl_socket_|, handling partial writes correctly, and calls
// the completion callback correctly.
TEST_F(TLSSocketTest, TestTLSSocketWrite) {
  CompleteHandler handler;
  net::CompletionCallback callback;

  EXPECT_CALL(*ssl_socket_, Write(_, _, _, _))
      .Times(2)
      .WillRepeatedly(DoAll(SaveArg<2>(&callback), Return(128)));
  EXPECT_CALL(handler, OnComplete(_)).Times(1);

  scoped_refptr<net::IOBufferWithSize> io_buffer(
      new net::IOBufferWithSize(256));
  socket_->Write(
      io_buffer.get(),
      io_buffer->size(),
      base::Bind(&CompleteHandler::OnComplete, base::Unretained(&handler)));
}

// Simulate a blocked Write, and verify that, when simulating the Write going
// through, the callback gets invoked.
TEST_F(TLSSocketTest, TestTLSSocketBlockedWrite) {
  CompleteHandler handler;
  net::CompletionCallback callback;

  // Return ERR_IO_PENDING to say the Write()'s blocked. Save the |callback|
  // Write()'s passed.
  EXPECT_CALL(*ssl_socket_, Write(_, _, _, _))
      .Times(2)
      .WillRepeatedly(
          DoAll(SaveArg<2>(&callback), Return(net::ERR_IO_PENDING)));

  scoped_refptr<net::IOBufferWithSize> io_buffer(new net::IOBufferWithSize(42));
  socket_->Write(
      io_buffer.get(),
      io_buffer->size(),
      base::Bind(&CompleteHandler::OnComplete, base::Unretained(&handler)));

  // After the simulated asynchronous writes come back (via calls to
  // callback.Run()), hander's OnComplete() should get invoked with the total
  // amount written.
  EXPECT_CALL(handler, OnComplete(42)).Times(1);
  callback.Run(40);
  callback.Run(2);
}

// Simulate multiple blocked Write()s.
TEST_F(TLSSocketTest, TestTLSSocketBlockedWriteReentry) {
  const int kNumIOs = 5;
  CompleteHandler handlers[kNumIOs];
  net::CompletionCallback callback;
  scoped_refptr<net::IOBufferWithSize> io_buffers[kNumIOs];

  // The implementation of TLSSocket::Write() is inherited from
  // Socket::Write(), which implements an internal write queue that wraps
  // TLSSocket::WriteImpl(). Each call from TLSSocket::WriteImpl() will invoke
  // |ssl_socket_|'s Write() (mocked here). Save the |callback| (assume they
  // will all be equivalent), and return ERR_IO_PENDING, to indicate a blocked
  // request. The mocked SSLClientSocket::Write() will get one request per
  // TLSSocket::Write() request invoked on |socket_| below.
  EXPECT_CALL(*ssl_socket_, Write(_, _, _, _))
      .Times(kNumIOs)
      .WillRepeatedly(
          DoAll(SaveArg<2>(&callback), Return(net::ERR_IO_PENDING)));

  // Send out |kNuMIOs| requests, each with a different size.
  for (int i = 0; i < kNumIOs; i++) {
    io_buffers[i] = new net::IOBufferWithSize(128 + i * 50);
    socket_->Write(io_buffers[i].get(),
                   io_buffers[i]->size(),
                   base::Bind(&CompleteHandler::OnComplete,
                              base::Unretained(&handlers[i])));

    // Set up expectations on all |kNumIOs| handlers.
    EXPECT_CALL(handlers[i], OnComplete(io_buffers[i]->size())).Times(1);
  }

  // Finish each pending I/O. This should satisfy the expectations on the
  // handlers.
  for (int i = 0; i < kNumIOs; i++) {
    callback.Run(128 + i * 50);
  }
}

typedef std::pair<net::CompletionCallback, int> PendingCallback;

class CallbackList : public base::circular_deque<PendingCallback> {
 public:
  void append(const net::CompletionCallback& cb, int arg) {
    push_back(std::make_pair(cb, arg));
  }
};

// Simulate Write()s above and below a SSLClientSocket size limit.
TEST_F(TLSSocketTest, TestTLSSocketLargeWrites) {
  const int kSizeIncrement = 4096;
  const int kNumIncrements = 10;
  const int kFragmentIncrement = 4;
  const int kSizeLimit = kSizeIncrement * kFragmentIncrement;
  net::CompletionCallback callback;
  CompleteHandler handler;
  scoped_refptr<net::IOBufferWithSize> io_buffers[kNumIncrements];
  CallbackList pending_callbacks;
  size_t total_bytes_requested = 0;
  size_t total_bytes_written = 0;

  // Some implementations of SSLClientSocket may have write-size limits (e.g,
  // max 1 TLS record, which is 16k). This test mocks a size limit at
  // |kSizeIncrement| and calls Write() above and below that limit. It
  // simulates SSLClientSocket::Write() behavior in only writing up to the size
  // limit, requiring additional calls for the remaining data to be sent.
  // Socket::Write() (and supporting methods) execute the additional calls as
  // needed. This test verifies that this inherited implementation does
  // properly issue additional calls, and that the total amount returned from
  // all mocked SSLClientSocket::Write() calls is the same as originally
  // requested.

  // |ssl_socket_|'s Write() will write at most |kSizeLimit| bytes. The
  // inherited Socket::Write() will repeatedly call |ssl_socket_|'s Write()
  // until the entire original request is sent. Socket::Write() will queue any
  // additional write requests until the current request is complete. A
  // request is complete when the callback passed to Socket::WriteImpl() is
  // invoked with an argument equal to the original number of bytes requested
  // from Socket::Write(). If the callback is invoked with a smaller number,
  // Socket::WriteImpl() will get repeatedly invoked until the sum of the
  // callbacks' arguments is equal to the original requested amount.
  EXPECT_CALL(*ssl_socket_, Write(_, _, _, _))
      .WillRepeatedly(DoAll(
          WithArgs<2, 1>(Invoke(&pending_callbacks, &CallbackList::append)),
          Return(net::ERR_IO_PENDING)));

  // Observe what comes back from Socket::Write() here.
  EXPECT_CALL(handler, OnComplete(Gt(0))).Times(kNumIncrements);

  // Send out |kNumIncrements| requests, each with a different size. The
  // last request is the same size as the first, and the ones in the middle
  // are monotonically increasing from the first.
  for (int i = 0; i < kNumIncrements; i++) {
    const bool last = i == (kNumIncrements - 1);
    io_buffers[i] = new net::IOBufferWithSize(last ? kSizeIncrement
                                                   : kSizeIncrement * (i + 1));
    total_bytes_requested += io_buffers[i]->size();

    // Invoke Socket::Write(). This will invoke |ssl_socket_|'s Write(), which
    // this test mocks out. That mocked Write() is in an asynchronous waiting
    // state until the passed callback (saved in the EXPECT_CALL for
    // |ssl_socket_|'s Write()) is invoked.
    socket_->Write(
        io_buffers[i].get(),
        io_buffers[i]->size(),
        base::Bind(&CompleteHandler::OnComplete, base::Unretained(&handler)));
  }

  // Invoke callbacks for pending I/Os. These can synchronously invoke more of
  // |ssl_socket_|'s Write() as needed. The callback checks how much is left
  // in the request, and then starts issuing any queued Socket::Write()
  // invocations.
  while (!pending_callbacks.empty()) {
    PendingCallback cb = pending_callbacks.front();
    pending_callbacks.pop_front();

    int amount_written_invocation = std::min(kSizeLimit, cb.second);
    total_bytes_written += amount_written_invocation;
    cb.first.Run(amount_written_invocation);
  }

  ASSERT_EQ(total_bytes_requested, total_bytes_written)
      << "There should be exactly as many bytes written as originally "
      << "requested to Write().";
}

}  // namespace extensions
