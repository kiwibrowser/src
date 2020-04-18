// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/glue/fake_ssl_client_socket.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_address.h"
#include "net/base/test_completion_callback.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/socket_tag.h"
#include "net/socket/socket_test_util.h"
#include "net/socket/stream_socket.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace jingle_glue {

namespace {

using ::testing::Return;
using ::testing::ReturnRef;

// Used by RunUnsuccessfulHandshakeTestHelper.  Represents where in
// the handshake step an error should be inserted.
enum HandshakeErrorLocation {
  CONNECT_ERROR,
  SEND_CLIENT_HELLO_ERROR,
  VERIFY_SERVER_HELLO_ERROR,
};

// Private error codes appended to the net::Error set.
enum {
  // An error representing a server hello that has been corrupted in
  // transit.
  ERR_MALFORMED_SERVER_HELLO = -15000,
};

// Used by PassThroughMethods test.
class MockClientSocket : public net::StreamSocket {
 public:
  ~MockClientSocket() override {}

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

  MOCK_METHOD3(Read, int(net::IOBuffer*, int,
                         const net::CompletionCallback&));
  MOCK_METHOD4(Write,
               int(net::IOBuffer*,
                   int,
                   const net::CompletionCallback&,
                   const net::NetworkTrafficAnnotationTag&));
  MOCK_METHOD1(SetReceiveBufferSize, int(int32_t));
  MOCK_METHOD1(SetSendBufferSize, int(int32_t));
  MOCK_METHOD1(Connect, int(const net::CompletionCallback&));
  MOCK_METHOD0(Disconnect, void());
  MOCK_CONST_METHOD0(IsConnected, bool());
  MOCK_CONST_METHOD0(IsConnectedAndIdle, bool());
  MOCK_CONST_METHOD1(GetPeerAddress, int(net::IPEndPoint*));
  MOCK_CONST_METHOD1(GetLocalAddress, int(net::IPEndPoint*));
  MOCK_CONST_METHOD0(NetLog, const net::NetLogWithSource&());
  MOCK_CONST_METHOD0(WasEverUsed, bool());
  MOCK_CONST_METHOD0(UsingTCPFastOpen, bool());
  MOCK_CONST_METHOD0(NumBytesRead, int64_t());
  MOCK_CONST_METHOD0(GetConnectTimeMicros, base::TimeDelta());
  MOCK_CONST_METHOD0(WasAlpnNegotiated, bool());
  MOCK_CONST_METHOD0(GetNegotiatedProtocol, net::NextProto());
  MOCK_METHOD1(GetSSLInfo, bool(net::SSLInfo*));
  MOCK_CONST_METHOD1(GetConnectionAttempts, void(net::ConnectionAttempts*));
  MOCK_METHOD0(ClearConnectionAttempts, void());
  MOCK_METHOD1(AddConnectionAttempts, void(const net::ConnectionAttempts&));
  MOCK_CONST_METHOD0(GetTotalReceivedBytes, int64_t());
  MOCK_METHOD1(ApplySocketTag, void(const net::SocketTag&));
};

// Break up |data| into a bunch of chunked MockReads/Writes and push
// them onto |ops|.
template <net::MockReadWriteType type>
void AddChunkedOps(base::StringPiece data, size_t chunk_size, net::IoMode mode,
                   std::vector<net::MockReadWrite<type> >* ops) {
  DCHECK_GT(chunk_size, 0U);
  size_t offset = 0;
  while (offset < data.size()) {
    size_t bounded_chunk_size = std::min(data.size() - offset, chunk_size);
    ops->push_back(net::MockReadWrite<type>(mode, data.data() + offset,
                                            bounded_chunk_size));
    offset += bounded_chunk_size;
  }
}

class FakeSSLClientSocketTest : public testing::Test {
 protected:
  FakeSSLClientSocketTest() {}

  ~FakeSSLClientSocketTest() override {}

  std::unique_ptr<net::StreamSocket> MakeClientSocket() {
    return mock_client_socket_factory_.CreateTransportClientSocket(
        net::AddressList(), NULL, NULL, net::NetLogSource());
  }

  void SetData(const net::MockConnect& mock_connect,
               std::vector<net::MockRead>* reads,
               std::vector<net::MockWrite>* writes) {
    static_socket_data_provider_.reset(
        new net::StaticSocketDataProvider(*reads, *writes));
    static_socket_data_provider_->set_connect_data(mock_connect);
    mock_client_socket_factory_.AddSocketDataProvider(
        static_socket_data_provider_.get());
  }

  void ExpectStatus(
      net::IoMode mode, int expected_status, int immediate_status,
      net::TestCompletionCallback* test_completion_callback) {
    if (mode == net::ASYNC) {
      EXPECT_EQ(net::ERR_IO_PENDING, immediate_status);
      int status = test_completion_callback->WaitForResult();
      EXPECT_EQ(expected_status, status);
    } else {
      EXPECT_EQ(expected_status, immediate_status);
    }
  }

  // Sets up the mock socket to generate a successful handshake
  // (sliced up according to the parameters) and makes sure the
  // FakeSSLClientSocket behaves as expected.
  void RunSuccessfulHandshakeTest(
      net::IoMode mode, size_t read_chunk_size, size_t write_chunk_size,
      int num_resets) {
    base::StringPiece ssl_client_hello =
        FakeSSLClientSocket::GetSslClientHello();
    base::StringPiece ssl_server_hello =
        FakeSSLClientSocket::GetSslServerHello();

    net::MockConnect mock_connect(mode, net::OK);
    std::vector<net::MockRead> reads;
    std::vector<net::MockWrite> writes;
    static const char kReadTestData[] = "read test data";
    static const char kWriteTestData[] = "write test data";
    for (int i = 0; i < num_resets + 1; ++i) {
      SCOPED_TRACE(i);
      AddChunkedOps(ssl_server_hello, read_chunk_size, mode, &reads);
      AddChunkedOps(ssl_client_hello, write_chunk_size, mode, &writes);
      reads.push_back(
          net::MockRead(mode, kReadTestData, arraysize(kReadTestData)));
      writes.push_back(
          net::MockWrite(mode, kWriteTestData, arraysize(kWriteTestData)));
    }
    SetData(mock_connect, &reads, &writes);

    FakeSSLClientSocket fake_ssl_client_socket(MakeClientSocket());

    for (int i = 0; i < num_resets + 1; ++i) {
      SCOPED_TRACE(i);
      net::TestCompletionCallback test_completion_callback;
      int status = fake_ssl_client_socket.Connect(
          test_completion_callback.callback());
      if (mode == net::ASYNC) {
        EXPECT_FALSE(fake_ssl_client_socket.IsConnected());
      }
      ExpectStatus(mode, net::OK, status, &test_completion_callback);
      if (fake_ssl_client_socket.IsConnected()) {
        int read_len = arraysize(kReadTestData);
        int read_buf_len = 2 * read_len;
        scoped_refptr<net::IOBuffer> read_buf(
            new net::IOBuffer(read_buf_len));
        int read_status = fake_ssl_client_socket.Read(
            read_buf.get(), read_buf_len, test_completion_callback.callback());
        ExpectStatus(mode, read_len, read_status, &test_completion_callback);

        scoped_refptr<net::IOBuffer> write_buf(
            new net::StringIOBuffer(kWriteTestData));
        int write_status = fake_ssl_client_socket.Write(
            write_buf.get(), arraysize(kWriteTestData),
            test_completion_callback.callback(), TRAFFIC_ANNOTATION_FOR_TESTS);
        ExpectStatus(mode, arraysize(kWriteTestData), write_status,
                     &test_completion_callback);
      } else {
        ADD_FAILURE();
      }
      fake_ssl_client_socket.Disconnect();
      EXPECT_FALSE(fake_ssl_client_socket.IsConnected());
    }
  }

  // Sets up the mock socket to generate an unsuccessful handshake
  // FakeSSLClientSocket fails as expected.
  void RunUnsuccessfulHandshakeTestHelper(
      net::IoMode mode, int error, HandshakeErrorLocation location) {
    DCHECK_NE(error, net::OK);
    base::StringPiece ssl_client_hello =
        FakeSSLClientSocket::GetSslClientHello();
    base::StringPiece ssl_server_hello =
        FakeSSLClientSocket::GetSslServerHello();

    net::MockConnect mock_connect(mode, net::OK);
    std::vector<net::MockRead> reads;
    std::vector<net::MockWrite> writes;
    const size_t kChunkSize = 1;
    AddChunkedOps(ssl_server_hello, kChunkSize, mode, &reads);
    AddChunkedOps(ssl_client_hello, kChunkSize, mode, &writes);
    switch (location) {
      case CONNECT_ERROR:
        mock_connect.result = error;
        writes.clear();
        reads.clear();
        break;
      case SEND_CLIENT_HELLO_ERROR: {
        // Use a fixed index for repeatability.
        size_t index = 100 % writes.size();
        writes[index].result = error;
        writes[index].data = NULL;
        writes[index].data_len = 0;
        writes.resize(index + 1);
        reads.clear();
        break;
      }
      case VERIFY_SERVER_HELLO_ERROR: {
        // Use a fixed index for repeatability.
        size_t index = 50 % reads.size();
        if (error == ERR_MALFORMED_SERVER_HELLO) {
          static const char kBadData[] = "BAD_DATA";
          reads[index].data = kBadData;
          reads[index].data_len = arraysize(kBadData);
        } else {
          reads[index].result = error;
          reads[index].data = NULL;
          reads[index].data_len = 0;
        }
        reads.resize(index + 1);
        if (error ==
            net::ERR_TEST_PEER_CLOSE_AFTER_NEXT_MOCK_READ) {
          static const char kDummyData[] = "DUMMY";
          reads.push_back(net::MockRead(mode, kDummyData));
        }
        break;
      }
    }
    SetData(mock_connect, &reads, &writes);

    FakeSSLClientSocket fake_ssl_client_socket(MakeClientSocket());

    // The two errors below are interpreted by FakeSSLClientSocket as
    // an unexpected event.
    int expected_status =
        ((error == net::ERR_TEST_PEER_CLOSE_AFTER_NEXT_MOCK_READ) ||
         (error == ERR_MALFORMED_SERVER_HELLO)) ?
        net::ERR_UNEXPECTED : error;

    net::TestCompletionCallback test_completion_callback;
    int status = fake_ssl_client_socket.Connect(
        test_completion_callback.callback());
    EXPECT_FALSE(fake_ssl_client_socket.IsConnected());
    ExpectStatus(mode, expected_status, status, &test_completion_callback);
    EXPECT_FALSE(fake_ssl_client_socket.IsConnected());
  }

  void RunUnsuccessfulHandshakeTest(
      int error, HandshakeErrorLocation location) {
    RunUnsuccessfulHandshakeTestHelper(net::SYNCHRONOUS, error, location);
    RunUnsuccessfulHandshakeTestHelper(net::ASYNC, error, location);
  }

  // MockTCPClientSocket needs a message loop.
  base::MessageLoop message_loop_;

  net::MockClientSocketFactory mock_client_socket_factory_;
  std::unique_ptr<net::StaticSocketDataProvider> static_socket_data_provider_;
};

TEST_F(FakeSSLClientSocketTest, PassThroughMethods) {
  std::unique_ptr<MockClientSocket> mock_client_socket(new MockClientSocket());
  const int kReceiveBufferSize = 10;
  const int kSendBufferSize = 20;
  net::IPEndPoint ip_endpoint(net::IPAddress::IPv4AllZeros(), 80);
  const int kPeerAddress = 30;
  net::NetLogWithSource net_log;
  EXPECT_CALL(*mock_client_socket, SetReceiveBufferSize(kReceiveBufferSize));
  EXPECT_CALL(*mock_client_socket, SetSendBufferSize(kSendBufferSize));
  EXPECT_CALL(*mock_client_socket, GetPeerAddress(&ip_endpoint)).
      WillOnce(Return(kPeerAddress));
  EXPECT_CALL(*mock_client_socket, NetLog()).WillOnce(ReturnRef(net_log));

  // Takes ownership of |mock_client_socket|.
  FakeSSLClientSocket fake_ssl_client_socket(std::move(mock_client_socket));
  fake_ssl_client_socket.SetReceiveBufferSize(kReceiveBufferSize);
  fake_ssl_client_socket.SetSendBufferSize(kSendBufferSize);
  EXPECT_EQ(kPeerAddress,
            fake_ssl_client_socket.GetPeerAddress(&ip_endpoint));
  EXPECT_EQ(&net_log, &fake_ssl_client_socket.NetLog());
}

TEST_F(FakeSSLClientSocketTest, SuccessfulHandshakeSync) {
  for (size_t i = 1; i < 100; i += 3) {
    SCOPED_TRACE(i);
    for (size_t j = 1; j < 100; j += 5) {
      SCOPED_TRACE(j);
      RunSuccessfulHandshakeTest(net::SYNCHRONOUS, i, j, 0);
    }
  }
}

TEST_F(FakeSSLClientSocketTest, SuccessfulHandshakeAsync) {
  for (size_t i = 1; i < 100; i += 7) {
    SCOPED_TRACE(i);
    for (size_t j = 1; j < 100; j += 9) {
      SCOPED_TRACE(j);
      RunSuccessfulHandshakeTest(net::ASYNC, i, j, 0);
    }
  }
}

TEST_F(FakeSSLClientSocketTest, ResetSocket) {
  RunSuccessfulHandshakeTest(net::ASYNC, 1, 2, 3);
}

TEST_F(FakeSSLClientSocketTest, UnsuccessfulHandshakeConnectError) {
  RunUnsuccessfulHandshakeTest(net::ERR_ACCESS_DENIED, CONNECT_ERROR);
}

TEST_F(FakeSSLClientSocketTest, UnsuccessfulHandshakeWriteError) {
  RunUnsuccessfulHandshakeTest(net::ERR_OUT_OF_MEMORY,
                               SEND_CLIENT_HELLO_ERROR);
}

TEST_F(FakeSSLClientSocketTest, UnsuccessfulHandshakeReadError) {
  RunUnsuccessfulHandshakeTest(net::ERR_CONNECTION_CLOSED,
                               VERIFY_SERVER_HELLO_ERROR);
}

TEST_F(FakeSSLClientSocketTest, PeerClosedDuringHandshake) {
  RunUnsuccessfulHandshakeTest(
      net::ERR_TEST_PEER_CLOSE_AFTER_NEXT_MOCK_READ,
      VERIFY_SERVER_HELLO_ERROR);
}

TEST_F(FakeSSLClientSocketTest, MalformedServerHello) {
  RunUnsuccessfulHandshakeTest(ERR_MALFORMED_SERVER_HELLO,
                               VERIFY_SERVER_HELLO_ERROR);
}

}  // namespace

}  // namespace jingle_glue
