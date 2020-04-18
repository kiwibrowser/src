// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "net/base/completion_callback.h"
#include "net/base/completion_once_callback.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/socket/server_socket.h"
#include "net/socket/socket_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/mojo_socket_test_util.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/socket_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {

// Message sent over the tcp connection.
const char kMsg[] = "please start tls!";
const size_t kMsgSize = strlen(kMsg);

// Message sent over the tls connection.
const char kSecretMsg[] = "here is secret.";
const size_t kSecretMsgSize = strlen(kSecretMsg);

class TLSClientSocketTestBase {
 public:
  TLSClientSocketTestBase()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        url_request_context_(true) {}
  ~TLSClientSocketTestBase() {}

 protected:
  // Initializes the test fixture. If |use_mock_sockets|, mock client socket
  // factory will be used.
  void Init(bool use_mock_sockets) {
    if (use_mock_sockets) {
      mock_client_socket_factory_.set_enable_read_if_ready(true);
      url_request_context_.set_client_socket_factory(
          &mock_client_socket_factory_);
    }
    url_request_context_.Init();
    factory_ = std::make_unique<SocketFactory>(nullptr /*net_log*/,
                                               &url_request_context_);
  }

  // Reads |num_bytes| from |handle| or reads until an error occurs. Returns the
  // bytes read as a string.
  std::string Read(mojo::ScopedDataPipeConsumerHandle* handle,
                   size_t num_bytes) {
    std::string received_contents;
    while (received_contents.size() < num_bytes) {
      base::RunLoop().RunUntilIdle();
      std::vector<char> buffer(num_bytes);
      uint32_t read_size = static_cast<uint32_t>(num_bytes);
      MojoResult result = handle->get().ReadData(buffer.data(), &read_size,
                                                 MOJO_READ_DATA_FLAG_NONE);
      if (result == MOJO_RESULT_SHOULD_WAIT)
        continue;
      if (result != MOJO_RESULT_OK)
        return received_contents;
      received_contents.append(buffer.data(), read_size);
    }
    return received_contents;
  }

  int CreateTCPConnectedSocketSync(mojom::TCPConnectedSocketRequest request,
                                   const net::IPEndPoint& remote_addr) {
    net::AddressList remote_addr_list(remote_addr);
    base::RunLoop run_loop;
    int net_error = net::ERR_FAILED;
    factory_->CreateTCPConnectedSocket(
        base::nullopt /* local_addr */, remote_addr_list,
        TRAFFIC_ANNOTATION_FOR_TESTS, std::move(request),
        pre_tls_observer()->GetObserverPtr(),
        base::BindLambdaForTesting(
            [&](int result,
                mojo::ScopedDataPipeConsumerHandle receive_pipe_handle,
                mojo::ScopedDataPipeProducerHandle send_pipe_handle) {
              net_error = result;
              pre_tls_recv_handle_ = std::move(receive_pipe_handle);
              pre_tls_send_handle_ = std::move(send_pipe_handle);
              run_loop.Quit();
            }));
    run_loop.Run();
    return net_error;
  }

  void UpgradeToTLS(mojom::TCPConnectedSocket* client_socket,
                    const net::HostPortPair& host_port_pair,
                    mojom::TLSClientSocketRequest request,
                    net::CompletionOnceCallback callback) {
    client_socket->UpgradeToTLS(
        host_port_pair,
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
        std::move(request), post_tls_observer()->GetObserverPtr(),
        base::BindOnce(
            [](net::CompletionOnceCallback cb,
               mojo::ScopedDataPipeConsumerHandle* consumer_handle,
               mojo::ScopedDataPipeProducerHandle* producer_handle, int result,
               mojo::ScopedDataPipeConsumerHandle receive_pipe_handle,
               mojo::ScopedDataPipeProducerHandle send_pipe_handle) {
              *consumer_handle = std::move(receive_pipe_handle);
              *producer_handle = std::move(send_pipe_handle);
              std::move(cb).Run(result);
            },
            std::move(callback), &post_tls_recv_handle_,
            &post_tls_send_handle_));
  }

  TestSocketObserver* pre_tls_observer() { return &pre_tls_observer_; }
  TestSocketObserver* post_tls_observer() { return &post_tls_observer_; }

  mojo::ScopedDataPipeConsumerHandle* pre_tls_recv_handle() {
    return &pre_tls_recv_handle_;
  }

  mojo::ScopedDataPipeProducerHandle* pre_tls_send_handle() {
    return &pre_tls_send_handle_;
  }

  mojo::ScopedDataPipeConsumerHandle* post_tls_recv_handle() {
    return &post_tls_recv_handle_;
  }

  mojo::ScopedDataPipeProducerHandle* post_tls_send_handle() {
    return &post_tls_send_handle_;
  }

  net::MockClientSocketFactory* mock_client_socket_factory() {
    return &mock_client_socket_factory_;
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Mojo data handles obtained from CreateTCPConnectedSocket.
  mojo::ScopedDataPipeConsumerHandle pre_tls_recv_handle_;
  mojo::ScopedDataPipeProducerHandle pre_tls_send_handle_;

  // Mojo data handles obtained from UpgradeToTLS.
  mojo::ScopedDataPipeConsumerHandle post_tls_recv_handle_;
  mojo::ScopedDataPipeProducerHandle post_tls_send_handle_;

  net::TestURLRequestContext url_request_context_;
  net::MockClientSocketFactory mock_client_socket_factory_;
  std::unique_ptr<SocketFactory> factory_;
  TestSocketObserver pre_tls_observer_;
  TestSocketObserver post_tls_observer_;
  mojo::StrongBindingSet<mojom::TCPServerSocket> tcp_server_socket_bindings_;
  mojo::StrongBindingSet<mojom::TCPConnectedSocket>
      tcp_connected_socket_bindings_;

  DISALLOW_COPY_AND_ASSIGN(TLSClientSocketTestBase);
};

}  // namespace
class TLSClientSocketTest : public TLSClientSocketTestBase,
                            public testing::Test {
 public:
  TLSClientSocketTest() : TLSClientSocketTestBase() {
    Init(true /* use_mock_sockets */);
  }

  ~TLSClientSocketTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TLSClientSocketTest);
};

// Basic test to call UpgradeToTLS, and then read/write after UpgradeToTLS is
// successful.
TEST_F(TLSClientSocketTest, UpgradeToTLS) {
  const net::MockRead kReads[] = {net::MockRead(net::ASYNC, kMsg, kMsgSize, 1),
                                  net::MockRead(net::SYNCHRONOUS, net::OK, 2)};
  const net::MockWrite kWrites[] = {
      net::MockWrite(net::SYNCHRONOUS, kMsg, kMsgSize, 0)};
  net::SequencedSocketData data_provider(kReads, kWrites);
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);
  net::SSLSocketDataProvider ssl_socket(net::ASYNC, net::OK);
  mock_client_socket_factory()->AddSSLSocketDataProvider(&ssl_socket);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);
  pre_tls_recv_handle()->reset();
  pre_tls_send_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());
  client_socket.reset();

  uint32_t num_bytes = strlen(kMsg);
  EXPECT_EQ(MOJO_RESULT_OK, post_tls_send_handle()->get().WriteData(
                                &kMsg, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kMsg, Read(post_tls_recv_handle(), kMsgSize));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(ssl_socket.ConnectDataConsumed());
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

// Same as the UpgradeToTLS test above, except this test calls
// base::RunLoop().RunUntilIdle() after destroying the pre-tls data pipes.
TEST_F(TLSClientSocketTest, ClosePipesRunUntilIdleAndUpgradeToTLS) {
  const net::MockRead kReads[] = {net::MockRead(net::ASYNC, kMsg, kMsgSize, 1),
                                  net::MockRead(net::SYNCHRONOUS, net::OK, 2)};
  const net::MockWrite kWrites[] = {
      net::MockWrite(net::SYNCHRONOUS, kMsg, kMsgSize, 0)};
  net::SequencedSocketData data_provider(kReads, kWrites);
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);
  net::SSLSocketDataProvider ssl_socket(net::ASYNC, net::OK);
  mock_client_socket_factory()->AddSSLSocketDataProvider(&ssl_socket);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);

  // Call RunUntilIdle() to test the case that pipes are closed before
  // UpgradeToTLS.
  pre_tls_recv_handle()->reset();
  pre_tls_send_handle()->reset();
  base::RunLoop().RunUntilIdle();

  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());
  client_socket.reset();

  uint32_t num_bytes = strlen(kMsg);
  EXPECT_EQ(MOJO_RESULT_OK, post_tls_send_handle()->get().WriteData(
                                &kMsg, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kMsg, Read(post_tls_recv_handle(), kMsgSize));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(ssl_socket.ConnectDataConsumed());
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

// Calling UpgradeToTLS on the same TCPConnectedSocketPtr is illegal and should
// receive an error.
TEST_F(TLSClientSocketTest, UpgradeToTLSTwice) {
  const net::MockRead kReads[] = {net::MockRead(net::ASYNC, net::OK, 0)};
  net::SequencedSocketData data_provider(kReads, base::span<net::MockWrite>());
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);
  net::SSLSocketDataProvider ssl_socket(net::ASYNC, net::OK);
  mock_client_socket_factory()->AddSSLSocketDataProvider(&ssl_socket);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);
  pre_tls_recv_handle()->reset();
  pre_tls_send_handle()->reset();

  // First UpgradeToTLS should complete successfully.
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());

  // Second time UpgradeToTLS is called, it should fail.
  mojom::TLSClientSocketPtr tls_socket2;
  base::RunLoop run_loop;
  int net_error = net::ERR_FAILED;
  client_socket->UpgradeToTLS(
      host_port_pair,
      net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
      mojo::MakeRequest(&tls_socket2), nullptr /*observer */,
      base::BindLambdaForTesting(
          [&](int result,
              mojo::ScopedDataPipeConsumerHandle receive_pipe_handle,
              mojo::ScopedDataPipeProducerHandle send_pipe_handle) {
            net_error = result;
            run_loop.Quit();
          }));
  run_loop.Run();
  ASSERT_EQ(net::ERR_SOCKET_NOT_CONNECTED, net_error);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(ssl_socket.ConnectDataConsumed());
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

// Same as the UpgradeToTLS test, except this also reads and writes to the tcp
// connection before UpgradeToTLS is called.
TEST_F(TLSClientSocketTest, ReadWriteBeforeUpgradeToTLS) {
  const net::MockRead kReads[] = {
      net::MockRead(net::SYNCHRONOUS, kMsg, kMsgSize, 0),
      net::MockRead(net::ASYNC, kSecretMsg, kSecretMsgSize, 3),
      net::MockRead(net::SYNCHRONOUS, net::OK, 4)};
  const net::MockWrite kWrites[] = {
      net::MockWrite(net::SYNCHRONOUS, kMsg, kMsgSize, 1),
      net::MockWrite(net::SYNCHRONOUS, kSecretMsg, kSecretMsgSize, 2),
  };
  net::SequencedSocketData data_provider(kReads, kWrites);
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);
  net::SSLSocketDataProvider ssl_socket(net::ASYNC, net::OK);
  mock_client_socket_factory()->AddSSLSocketDataProvider(&ssl_socket);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  EXPECT_EQ(kMsg, Read(pre_tls_recv_handle(), kMsgSize));

  uint32_t num_bytes = kMsgSize;
  EXPECT_EQ(MOJO_RESULT_OK, pre_tls_send_handle()->get().WriteData(
                                &kMsg, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));

  net::HostPortPair host_port_pair("example.org", 443);
  pre_tls_recv_handle()->reset();
  pre_tls_send_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());
  client_socket.reset();

  num_bytes = strlen(kSecretMsg);
  EXPECT_EQ(MOJO_RESULT_OK,
            post_tls_send_handle()->get().WriteData(&kSecretMsg, &num_bytes,
                                                    MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kSecretMsg, Read(post_tls_recv_handle(), kSecretMsgSize));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(ssl_socket.ConnectDataConsumed());
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

// Tests that a read error is encountered after UpgradeToTLS completes
// successfully.
TEST_F(TLSClientSocketTest, ReadErrorAfterUpgradeToTLS) {
  const net::MockRead kReads[] = {
      net::MockRead(net::ASYNC, kSecretMsg, kSecretMsgSize, 1),
      net::MockRead(net::SYNCHRONOUS, net::ERR_CONNECTION_CLOSED, 2)};
  const net::MockWrite kWrites[] = {
      net::MockWrite(net::SYNCHRONOUS, kSecretMsg, kSecretMsgSize, 0)};
  net::SequencedSocketData data_provider(kReads, kWrites);
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);
  net::SSLSocketDataProvider ssl_socket(net::ASYNC, net::OK);
  mock_client_socket_factory()->AddSSLSocketDataProvider(&ssl_socket);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);
  pre_tls_recv_handle()->reset();
  pre_tls_send_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());
  client_socket.reset();

  uint32_t num_bytes = strlen(kSecretMsg);
  EXPECT_EQ(MOJO_RESULT_OK,
            post_tls_send_handle()->get().WriteData(&kSecretMsg, &num_bytes,
                                                    MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kSecretMsg, Read(post_tls_recv_handle(), kSecretMsgSize));
  EXPECT_EQ(net::ERR_CONNECTION_CLOSED,
            post_tls_observer()->WaitForReadError());

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(ssl_socket.ConnectDataConsumed());
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

// Tests that a read error is encountered after UpgradeToTLS completes
// successfully.
TEST_F(TLSClientSocketTest, WriteErrorAfterUpgradeToTLS) {
  const net::MockRead kReads[] = {net::MockRead(net::ASYNC, net::OK, 0)};
  const net::MockWrite kWrites[] = {
      net::MockWrite(net::SYNCHRONOUS, net::ERR_CONNECTION_CLOSED, 1)};
  net::SequencedSocketData data_provider(kReads, kWrites);
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);
  net::SSLSocketDataProvider ssl_socket(net::ASYNC, net::OK);
  mock_client_socket_factory()->AddSSLSocketDataProvider(&ssl_socket);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);
  pre_tls_recv_handle()->reset();
  pre_tls_send_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());
  client_socket.reset();

  uint32_t num_bytes = strlen(kSecretMsg);
  EXPECT_EQ(MOJO_RESULT_OK,
            post_tls_send_handle()->get().WriteData(&kSecretMsg, &num_bytes,
                                                    MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(net::ERR_CONNECTION_CLOSED,
            post_tls_observer()->WaitForWriteError());

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(ssl_socket.ConnectDataConsumed());
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

// Tests that reading from the pre-tls data pipe is okay even after UpgradeToTLS
// is called.
TEST_F(TLSClientSocketTest, ReadFromPreTlsDataPipeAfterUpgradeToTLS) {
  const net::MockRead kReads[] = {
      net::MockRead(net::ASYNC, kMsg, kMsgSize, 0),
      net::MockRead(net::ASYNC, kSecretMsg, kSecretMsgSize, 2),
      net::MockRead(net::SYNCHRONOUS, net::OK, 3)};
  const net::MockWrite kWrites[] = {
      net::MockWrite(net::SYNCHRONOUS, kSecretMsg, kSecretMsgSize, 1)};
  net::SequencedSocketData data_provider(kReads, kWrites);
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);
  net::SSLSocketDataProvider ssl_socket(net::ASYNC, net::OK);
  mock_client_socket_factory()->AddSSLSocketDataProvider(&ssl_socket);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);
  pre_tls_send_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(kMsg, Read(pre_tls_recv_handle(), kMsgSize));

  // Reset pre-tls receive pipe now and UpgradeToTLS should complete.
  pre_tls_recv_handle()->reset();
  ASSERT_EQ(net::OK, callback.WaitForResult());
  client_socket.reset();

  uint32_t num_bytes = strlen(kSecretMsg);
  EXPECT_EQ(MOJO_RESULT_OK,
            post_tls_send_handle()->get().WriteData(&kSecretMsg, &num_bytes,
                                                    MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kSecretMsg, Read(post_tls_recv_handle(), kSecretMsgSize));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(ssl_socket.ConnectDataConsumed());
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

// Tests that writing to the pre-tls data pipe is okay even after UpgradeToTLS
// is called.
TEST_F(TLSClientSocketTest, WriteToPreTlsDataPipeAfterUpgradeToTLS) {
  const net::MockRead kReads[] = {
      net::MockRead(net::ASYNC, kSecretMsg, kSecretMsgSize, 2),
      net::MockRead(net::SYNCHRONOUS, net::OK, 3)};
  const net::MockWrite kWrites[] = {
      net::MockWrite(net::SYNCHRONOUS, kMsg, kMsgSize, 0),
      net::MockWrite(net::SYNCHRONOUS, kSecretMsg, kSecretMsgSize, 1)};
  net::SequencedSocketData data_provider(kReads, kWrites);
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);
  net::SSLSocketDataProvider ssl_socket(net::ASYNC, net::OK);
  mock_client_socket_factory()->AddSSLSocketDataProvider(&ssl_socket);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);
  pre_tls_recv_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  base::RunLoop().RunUntilIdle();

  uint32_t num_bytes = strlen(kMsg);
  EXPECT_EQ(MOJO_RESULT_OK, pre_tls_send_handle()->get().WriteData(
                                &kMsg, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));

  // Reset pre-tls send pipe now and UpgradeToTLS should complete.
  pre_tls_send_handle()->reset();
  ASSERT_EQ(net::OK, callback.WaitForResult());
  client_socket.reset();

  num_bytes = strlen(kSecretMsg);
  EXPECT_EQ(MOJO_RESULT_OK,
            post_tls_send_handle()->get().WriteData(&kSecretMsg, &num_bytes,
                                                    MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kSecretMsg, Read(post_tls_recv_handle(), kSecretMsgSize));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(ssl_socket.ConnectDataConsumed());
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

// Tests that reading from and writing to pre-tls data pipe is okay even after
// UpgradeToTLS is called.
TEST_F(TLSClientSocketTest, ReadAndWritePreTlsDataPipeAfterUpgradeToTLS) {
  const net::MockRead kReads[] = {
      net::MockRead(net::ASYNC, kMsg, kMsgSize, 0),
      net::MockRead(net::ASYNC, kSecretMsg, kSecretMsgSize, 3),
      net::MockRead(net::SYNCHRONOUS, net::OK, 4)};
  const net::MockWrite kWrites[] = {
      net::MockWrite(net::SYNCHRONOUS, kMsg, kMsgSize, 1),
      net::MockWrite(net::SYNCHRONOUS, kSecretMsg, kSecretMsgSize, 2)};
  net::SequencedSocketData data_provider(kReads, kWrites);
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);
  net::SSLSocketDataProvider ssl_socket(net::ASYNC, net::OK);
  mock_client_socket_factory()->AddSSLSocketDataProvider(&ssl_socket);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);
  base::RunLoop run_loop;
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  EXPECT_EQ(kMsg, Read(pre_tls_recv_handle(), kMsgSize));
  uint32_t num_bytes = strlen(kMsg);
  EXPECT_EQ(MOJO_RESULT_OK, pre_tls_send_handle()->get().WriteData(
                                &kMsg, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));

  // Reset pre-tls pipes now and UpgradeToTLS should complete.
  pre_tls_recv_handle()->reset();
  pre_tls_send_handle()->reset();
  ASSERT_EQ(net::OK, callback.WaitForResult());
  client_socket.reset();

  num_bytes = strlen(kSecretMsg);
  EXPECT_EQ(MOJO_RESULT_OK,
            post_tls_send_handle()->get().WriteData(&kSecretMsg, &num_bytes,
                                                    MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kSecretMsg, Read(post_tls_recv_handle(), kSecretMsgSize));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(ssl_socket.ConnectDataConsumed());
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

// Tests that a read error is encountered before UpgradeToTLS completes.
TEST_F(TLSClientSocketTest, ReadErrorBeforeUpgradeToTLS) {
  const net::MockRead kReads[] = {
      net::MockRead(net::ASYNC, kMsg, kMsgSize, 0),
      net::MockRead(net::SYNCHRONOUS, net::ERR_CONNECTION_CLOSED, 1)};
  net::SequencedSocketData data_provider(kReads, base::span<net::MockWrite>());
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);
  pre_tls_send_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());

  EXPECT_EQ(kMsg, Read(pre_tls_recv_handle(), kMsgSize));
  EXPECT_EQ(net::ERR_CONNECTION_CLOSED, pre_tls_observer()->WaitForReadError());

  // Reset pre-tls receive pipe now and UpgradeToTLS should complete.
  pre_tls_recv_handle()->reset();
  ASSERT_EQ(net::ERR_SOCKET_NOT_CONNECTED, callback.WaitForResult());
  client_socket.reset();

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

// Tests that a write error is encountered before UpgradeToTLS completes.
TEST_F(TLSClientSocketTest, WriteErrorBeforeUpgradeToTLS) {
  const net::MockRead kReads[] = {net::MockRead(net::ASYNC, net::OK, 1)};
  const net::MockWrite kWrites[] = {
      net::MockWrite(net::SYNCHRONOUS, net::ERR_CONNECTION_CLOSED, 0)};
  net::SequencedSocketData data_provider(kReads, kWrites);
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);
  pre_tls_recv_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  uint32_t num_bytes = strlen(kMsg);
  EXPECT_EQ(MOJO_RESULT_OK, pre_tls_send_handle()->get().WriteData(
                                &kMsg, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));

  EXPECT_EQ(net::ERR_CONNECTION_CLOSED,
            pre_tls_observer()->WaitForWriteError());
  // Reset pre-tls send pipe now and UpgradeToTLS should complete.
  pre_tls_send_handle()->reset();
  ASSERT_EQ(net::ERR_SOCKET_NOT_CONNECTED, callback.WaitForResult());
  client_socket.reset();

  base::RunLoop().RunUntilIdle();
  // Write failed before the mock read can be consumed.
  EXPECT_FALSE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

class TLSClientSocketParameterizedTest
    : public TLSClientSocketTestBase,
      public testing::TestWithParam<net::IoMode> {
 public:
  TLSClientSocketParameterizedTest() : TLSClientSocketTestBase() {
    Init(true /* use_mock_sockets*/);
  }

  ~TLSClientSocketParameterizedTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TLSClientSocketParameterizedTest);
};

INSTANTIATE_TEST_CASE_P(/* no prefix */,
                        TLSClientSocketParameterizedTest,
                        testing::Values(net::SYNCHRONOUS, net::ASYNC));

TEST_P(TLSClientSocketParameterizedTest, MultipleWriteToTLSSocket) {
  const int kNumIterations = 3;
  std::vector<net::MockRead> reads;
  std::vector<net::MockWrite> writes;
  int sequence_number = 0;
  net::IoMode mode = GetParam();
  for (int j = 0; j < kNumIterations; ++j) {
    for (size_t i = 0; i < kSecretMsgSize; ++i) {
      writes.push_back(
          net::MockWrite(mode, &kSecretMsg[i], 1, sequence_number++));
    }
    for (size_t i = 0; i < kSecretMsgSize; ++i) {
      reads.push_back(
          net::MockRead(net::ASYNC, &kSecretMsg[i], 1, sequence_number++));
    }
    if (j == kNumIterations - 1) {
      reads.push_back(net::MockRead(mode, net::OK, sequence_number++));
    }
  }
  net::SequencedSocketData data_provider(reads, writes);
  data_provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
  mock_client_socket_factory()->AddSocketDataProvider(&data_provider);
  net::SSLSocketDataProvider ssl_socket(net::ASYNC, net::OK);
  mock_client_socket_factory()->AddSSLSocketDataProvider(&ssl_socket);

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  net::HostPortPair host_port_pair("example.org", 443);
  pre_tls_recv_handle()->reset();
  pre_tls_send_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), host_port_pair,
               mojo::MakeRequest(&tls_socket), callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());
  client_socket.reset();

  // Loop kNumIterations times to test that writes can follow reads, and reads
  // can follow writes.
  for (int j = 0; j < kNumIterations; ++j) {
    // Write multiple times.
    for (size_t i = 0; i < kSecretMsgSize; ++i) {
      uint32_t num_bytes = 1;
      EXPECT_EQ(MOJO_RESULT_OK,
                post_tls_send_handle()->get().WriteData(
                    &kSecretMsg[i], &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));
      // Flush the 1 byte write.
      base::RunLoop().RunUntilIdle();
    }
    // Reading kSecretMsgSize should coalesce the 1-byte mock reads.
    EXPECT_EQ(kSecretMsg, Read(post_tls_recv_handle(), kSecretMsgSize));
  }
  EXPECT_TRUE(ssl_socket.ConnectDataConsumed());
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

class TLSClientSocketTestWithEmbeddedTestServer
    : public TLSClientSocketTestBase,
      public testing::Test {
 public:
  TLSClientSocketTestWithEmbeddedTestServer() : TLSClientSocketTestBase() {
    Init(false /* use_mock_sockets */);
  }

  ~TLSClientSocketTestWithEmbeddedTestServer() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TLSClientSocketTestWithEmbeddedTestServer);
};

TEST_F(TLSClientSocketTestWithEmbeddedTestServer, Basic) {
  net::EmbeddedTestServer server(net::EmbeddedTestServer::TYPE_HTTPS);
  server.RegisterRequestHandler(
      base::BindRepeating([](const net::test_server::HttpRequest& request) {
        if (base::StartsWith(request.relative_url, "/secret",
                             base::CompareCase::INSENSITIVE_ASCII)) {
          return std::unique_ptr<net::test_server::HttpResponse>(
              new net::test_server::RawHttpResponse("HTTP/1.1 200 OK",
                                                    "Hello There!"));
        }
        return std::unique_ptr<net::test_server::HttpResponse>();
      }));
  server.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
  ASSERT_TRUE(server.Start());

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), server.port());
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  pre_tls_recv_handle()->reset();
  pre_tls_send_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), server.host_port_pair(),
               mojo::MakeRequest(&tls_socket), callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());
  client_socket.reset();

  const char kTestMsg[] = "GET /secret HTTP/1.1\r\n\r\n";
  uint32_t num_bytes = strlen(kTestMsg);
  const char kResponse[] = "HTTP/1.1 200 OK\n\n";
  EXPECT_EQ(MOJO_RESULT_OK,
            post_tls_send_handle()->get().WriteData(&kTestMsg, &num_bytes,
                                                    MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kResponse, Read(post_tls_recv_handle(), strlen(kResponse)));
}

TEST_F(TLSClientSocketTestWithEmbeddedTestServer, ServerCertError) {
  net::EmbeddedTestServer server(net::EmbeddedTestServer::TYPE_HTTPS);
  server.SetSSLConfig(net::EmbeddedTestServer::CERT_MISMATCHED_NAME);
  ASSERT_TRUE(server.Start());

  mojom::TCPConnectedSocketPtr client_socket;
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), server.port());
  EXPECT_EQ(net::OK, CreateTCPConnectedSocketSync(
                         mojo::MakeRequest(&client_socket), server_addr));

  pre_tls_recv_handle()->reset();
  pre_tls_send_handle()->reset();
  net::TestCompletionCallback callback;
  mojom::TLSClientSocketPtr tls_socket;
  UpgradeToTLS(client_socket.get(), server.host_port_pair(),
               mojo::MakeRequest(&tls_socket), callback.callback());
  ASSERT_EQ(net::ERR_CERT_COMMON_NAME_INVALID, callback.WaitForResult());
}

}  // namespace network
