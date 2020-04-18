// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_channel/cast_socket.h"

#include <stdint.h>

#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/sys_byteorder.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/timer/mock_timer.h"
#include "components/cast_channel/cast_auth_util.h"
#include "components/cast_channel/cast_framer.h"
#include "components/cast_channel/cast_message_util.h"
#include "components/cast_channel/cast_test_util.h"
#include "components/cast_channel/cast_transport.h"
#include "components/cast_channel/logger.h"
#include "components/cast_channel/proto/cast_channel.pb.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "crypto/rsa_private_key.h"
#include "net/base/address_list.h"
#include "net/base/net_errors.h"
#include "net/cert/pem_tokenizer.h"
#include "net/log/net_log_source.h"
#include "net/log/test_net_log.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/socket_test_util.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/ssl_server_socket.h"
#include "net/socket/tcp_client_socket.h"
#include "net/socket/tcp_server_socket.h"
#include "net/ssl/ssl_info.h"
#include "net/ssl/ssl_server_config.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

const int64_t kDistantTimeoutMillis = 100000;  // 100 seconds (never hit).

using ::testing::A;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::InvokeArgument;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

namespace cast_channel {
namespace {
const char kAuthNamespace[] = "urn:x-cast:com.google.cast.tp.deviceauth";

// Returns an auth challenge message inline.
CastMessage CreateAuthChallenge() {
  CastMessage output;
  CreateAuthChallengeMessage(&output, AuthContext::Create());
  return output;
}

// Returns an auth challenge response message inline.
CastMessage CreateAuthReply() {
  CastMessage output;
  output.set_protocol_version(CastMessage::CASTV2_1_0);
  output.set_source_id("sender-0");
  output.set_destination_id("receiver-0");
  output.set_payload_type(CastMessage::BINARY);
  output.set_payload_binary("abcd");
  output.set_namespace_(kAuthNamespace);
  return output;
}

CastMessage CreateTestMessage() {
  CastMessage test_message;
  test_message.set_protocol_version(CastMessage::CASTV2_1_0);
  test_message.set_namespace_("ns");
  test_message.set_source_id("source");
  test_message.set_destination_id("dest");
  test_message.set_payload_type(CastMessage::STRING);
  test_message.set_payload_utf8("payload");
  return test_message;
}

base::FilePath GetTestCertsDirectory() {
  base::FilePath path;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.Append(FILE_PATH_LITERAL("components"));
  path = path.Append(FILE_PATH_LITERAL("test"));
  path = path.Append(FILE_PATH_LITERAL("data"));
  path = path.Append(FILE_PATH_LITERAL("cast_channel"));
  return path;
}

class MockTCPSocket : public net::MockTCPClientSocket {
 public:
  MockTCPSocket(bool do_nothing, net::SocketDataProvider* socket_provider)
      : net::MockTCPClientSocket(net::AddressList(), nullptr, socket_provider) {
    do_nothing_ = do_nothing;
  }

  int Connect(net::CompletionOnceCallback callback) override {
    if (do_nothing_) {
      // Stall the I/O event loop.
      return net::ERR_IO_PENDING;
    }
    return net::MockTCPClientSocket::Connect(std::move(callback));
  }

 private:
  bool do_nothing_;

  DISALLOW_COPY_AND_ASSIGN(MockTCPSocket);
};

class CompleteHandler {
 public:
  CompleteHandler() {}
  MOCK_METHOD1(OnCloseComplete, void(int result));
  MOCK_METHOD1(OnConnectComplete, void(CastSocket* socket));
  MOCK_METHOD1(OnWriteComplete, void(int result));
  MOCK_METHOD1(OnReadComplete, void(int result));

 private:
  DISALLOW_COPY_AND_ASSIGN(CompleteHandler);
};

class TestCastSocketBase : public CastSocketImpl {
 public:
  TestCastSocketBase(const CastSocketOpenParams& open_params, Logger* logger)
      : CastSocketImpl(open_params, logger, AuthContext::Create()),
        ip_(open_params.ip_endpoint),
        extract_cert_result_(true),
        verify_challenge_result_(true),
        verify_challenge_disallow_(false),
        mock_timer_(new base::MockTimer(false, false)) {}

  void SetExtractCertResult(bool value) { extract_cert_result_ = value; }

  void SetVerifyChallengeResult(bool value) {
    verify_challenge_result_ = value;
  }

  void TriggerTimeout() { mock_timer_->Fire(); }

  bool TestVerifyChannelPolicyNone() {
    AuthResult authResult;
    return VerifyChannelPolicy(authResult);
  }

  void DisallowVerifyChallengeResult() { verify_challenge_disallow_ = true; }

 protected:
  ~TestCastSocketBase() override {}

  scoped_refptr<net::X509Certificate> ExtractPeerCert() override {
    return extract_cert_result_
               ? net::ImportCertFromFile(GetTestCertsDirectory(),
                                         "self_signed.pem")
               : nullptr;
  }

  bool VerifyChallengeReply() override {
    EXPECT_FALSE(verify_challenge_disallow_);
    return verify_challenge_result_;
  }

  base::Timer* GetTimer() override { return mock_timer_.get(); }

  net::IPEndPoint ip_;
  // Simulated result of peer cert extraction.
  bool extract_cert_result_;
  // Simulated result of verifying challenge reply.
  bool verify_challenge_result_;
  bool verify_challenge_disallow_;
  std::unique_ptr<base::MockTimer> mock_timer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestCastSocketBase);
};

class MockTestCastSocket : public TestCastSocketBase {
 public:
  static std::unique_ptr<MockTestCastSocket> CreateSecure(
      const CastSocketOpenParams& open_params,
      Logger* logger) {
    return std::unique_ptr<MockTestCastSocket>(
        new MockTestCastSocket(open_params, logger));
  }

  using TestCastSocketBase::TestCastSocketBase;

  MockTestCastSocket(const CastSocketOpenParams& open_params, Logger* logger)
      : TestCastSocketBase(open_params, logger) {}

  ~MockTestCastSocket() override {}

  void SetupMockTransport() {
    mock_transport_ = new MockCastTransport;
    SetTransportForTesting(base::WrapUnique(mock_transport_));
  }

  // Socket connection helpers.
  void SetupTcpConnect(net::IoMode mode, int result) {
    tcp_connect_data_.reset(new net::MockConnect(mode, result, ip_));
  }
  void SetupSslConnect(net::IoMode mode, int result) {
    ssl_connect_data_.reset(new net::MockConnect(mode, result, ip_));
  }

  // Socket I/O helpers.
  void AddWriteResult(const net::MockWrite& write) { writes_.push_back(write); }
  void AddWriteResult(net::IoMode mode, int result) {
    AddWriteResult(net::MockWrite(mode, result));
  }
  void AddWriteResultForData(net::IoMode mode, const std::string& msg) {
    AddWriteResult(mode, msg.size());
  }
  void AddReadResult(const net::MockRead& read) { reads_.push_back(read); }
  void AddReadResult(net::IoMode mode, int result) {
    AddReadResult(net::MockRead(mode, result));
  }
  void AddReadResultForData(net::IoMode mode, const std::string& data) {
    AddReadResult(net::MockRead(mode, data.c_str(), data.size()));
  }

  // Helpers for modifying other connection-related behaviors.
  void SetupTcpConnectUnresponsive() { tcp_unresponsive_ = true; }

  bool TestVerifyChannelPolicyAudioOnly() {
    AuthResult authResult;
    authResult.channel_policies |= AuthResult::POLICY_AUDIO_ONLY;
    return VerifyChannelPolicy(authResult);
  }

  MockCastTransport* GetMockTransport() {
    CHECK(mock_transport_);
    return mock_transport_;
  }

 private:
  // Creates a TCP socket. Note that at most one socket created with this method
  // may be live at a time.
  std::unique_ptr<net::TransportClientSocket> CreateTcpSocket() override {
    if (tcp_unresponsive_) {
      socket_data_provider_ = std::make_unique<net::StaticSocketDataProvider>();
      return std::unique_ptr<net::TransportClientSocket>(
          new MockTCPSocket(true, socket_data_provider_.get()));
    } else {
      socket_data_provider_ =
          std::make_unique<net::StaticSocketDataProvider>(reads_, writes_);
      socket_data_provider_->set_connect_data(*tcp_connect_data_);
      return std::unique_ptr<net::TransportClientSocket>(
          new MockTCPSocket(false, socket_data_provider_.get()));
    }
  }

  // Creates an SSL socket. Note that at most one socket created with this
  // method may be live at a time.
  std::unique_ptr<net::SSLClientSocket> CreateSslSocket(
      std::unique_ptr<net::StreamSocket> tcp_socket) override {
    ssl_socket_data_provider_ = std::make_unique<net::SSLSocketDataProvider>(
        ssl_connect_data_->mode, ssl_connect_data_->result);
    auto client_handle = std::make_unique<net::ClientSocketHandle>();
    client_handle->SetSocket(std::move(tcp_socket));
    return std::make_unique<net::MockSSLClientSocket>(
        std::move(client_handle), net::HostPortPair(), net::SSLConfig(),
        ssl_socket_data_provider_.get());
  }

  // Simulated connect data
  std::unique_ptr<net::MockConnect> tcp_connect_data_;
  std::unique_ptr<net::MockConnect> ssl_connect_data_;
  // Simulated read / write data
  std::vector<net::MockWrite> writes_;
  std::vector<net::MockRead> reads_;
  std::unique_ptr<net::SocketDataProvider> socket_data_provider_;
  std::unique_ptr<net::SSLSocketDataProvider> ssl_socket_data_provider_;
  // If true, makes TCP connection process stall. For timeout testing.
  bool tcp_unresponsive_ = false;
  MockCastTransport* mock_transport_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(MockTestCastSocket);
};

class SslTestCastSocket : public TestCastSocketBase {
 public:
  static std::unique_ptr<SslTestCastSocket> CreateSecure(
      const CastSocketOpenParams& open_params,
      Logger* logger) {
    return std::unique_ptr<SslTestCastSocket>(
        new SslTestCastSocket(open_params, logger));
  }

  using TestCastSocketBase::TestCastSocketBase;

  void SetTcpSocket(
      std::unique_ptr<net::TransportClientSocket> tcp_client_socket) {
    tcp_client_socket_ = std::move(tcp_client_socket);
  }

 private:
  std::unique_ptr<net::TransportClientSocket> CreateTcpSocket() override {
    return std::move(tcp_client_socket_);
  }

  std::unique_ptr<net::TransportClientSocket> tcp_client_socket_;
};

class CastSocketTestBase : public testing::Test {
 protected:
  CastSocketTestBase()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        logger_(new Logger()),
        observer_(new MockCastSocketObserver()),
        capturing_net_log_(new net::TestNetLog()),
        socket_open_params_(
            CreateIPEndPointForTest(),
            capturing_net_log_.get(),
            base::TimeDelta::FromMilliseconds(kDistantTimeoutMillis)) {}
  ~CastSocketTestBase() override {}

  void SetUp() override { EXPECT_CALL(*observer_, OnMessage(_, _)).Times(0); }

  // Runs all pending tasks in the message loop.
  void RunPendingTasks() {
    base::RunLoop run_loop;
    run_loop.RunUntilIdle();
  }

  content::TestBrowserThreadBundle thread_bundle_;
  Logger* logger_;
  CompleteHandler handler_;
  std::unique_ptr<MockCastSocketObserver> observer_;
  std::unique_ptr<net::TestNetLog> capturing_net_log_;
  CastSocketOpenParams socket_open_params_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastSocketTestBase);
};

class MockCastSocketTest : public CastSocketTestBase {
 protected:
  MockCastSocketTest() {}

  void TearDown() override {
    if (socket_) {
      EXPECT_CALL(handler_, OnCloseComplete(net::OK));
      socket_->Close(base::Bind(&CompleteHandler::OnCloseComplete,
                                base::Unretained(&handler_)));
    }
  }

  void CreateCastSocketSecure() {
    socket_ = MockTestCastSocket::CreateSecure(socket_open_params_, logger_);
  }

  void HandleAuthHandshake() {
    socket_->SetupMockTransport();
    CastMessage challenge_proto = CreateAuthChallenge();
    EXPECT_CALL(*socket_->GetMockTransport(),
                SendMessage(EqualsProto(challenge_proto), _, _))
        .WillOnce(PostCompletionCallbackTask<1>(net::OK));
    EXPECT_CALL(*socket_->GetMockTransport(), Start());
    EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
    socket_->AddObserver(observer_.get());
    socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                                base::Unretained(&handler_)));
    RunPendingTasks();
    socket_->GetMockTransport()->current_delegate()->OnMessage(
        CreateAuthReply());
    RunPendingTasks();
  }

  std::unique_ptr<MockTestCastSocket> socket_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockCastSocketTest);
};

class SslCastSocketTest : public CastSocketTestBase {
 protected:
  SslCastSocketTest() {}

  void TearDown() override {
    if (socket_) {
      EXPECT_CALL(handler_, OnCloseComplete(net::OK));
      socket_->Close(base::Bind(&CompleteHandler::OnCloseComplete,
                                base::Unretained(&handler_)));
    }
  }

  void CreateSockets() {
    socket_ = SslTestCastSocket::CreateSecure(socket_open_params_, logger_);

    server_cert_ =
        net::ImportCertFromFile(GetTestCertsDirectory(), "self_signed.pem");
    ASSERT_TRUE(server_cert_);
    server_private_key_ = ReadTestKeyFromPEM("self_signed.pem");
    ASSERT_TRUE(server_private_key_);
    server_context_ = CreateSSLServerContext(
        server_cert_.get(), *server_private_key_, server_ssl_config_);

    tcp_server_socket_.reset(
        new net::TCPServerSocket(nullptr, net::NetLogSource()));
    ASSERT_EQ(net::OK,
              tcp_server_socket_->ListenWithAddressAndPort("127.0.0.1", 0, 1));
    net::IPEndPoint server_address;
    ASSERT_EQ(net::OK, tcp_server_socket_->GetLocalAddress(&server_address));
    tcp_client_socket_.reset(
        new net::TCPClientSocket(net::AddressList(server_address), nullptr,
                                 nullptr, net::NetLogSource()));

    std::unique_ptr<net::StreamSocket> accepted_socket;
    accept_result_ = tcp_server_socket_->Accept(
        &accepted_socket, base::Bind(&SslCastSocketTest::TcpAcceptCallback,
                                     base::Unretained(this)));
    connect_result_ = tcp_client_socket_->Connect(base::Bind(
        &SslCastSocketTest::TcpConnectCallback, base::Unretained(this)));
    while (accept_result_ == net::ERR_IO_PENDING ||
           connect_result_ == net::ERR_IO_PENDING) {
      RunPendingTasks();
    }
    ASSERT_EQ(net::OK, accept_result_);
    ASSERT_EQ(net::OK, connect_result_);
    ASSERT_TRUE(accepted_socket);
    ASSERT_TRUE(tcp_client_socket_->IsConnected());

    server_socket_ =
        server_context_->CreateSSLServerSocket(std::move(accepted_socket));
    ASSERT_TRUE(server_socket_);

    socket_->SetTcpSocket(std::move(tcp_client_socket_));
  }

  void ConnectSockets() {
    socket_->AddObserver(observer_.get());
    socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                                base::Unretained(&handler_)));

    net::TestCompletionCallback handshake_callback;
    int server_ret = handshake_callback.GetResult(
        server_socket_->Handshake(handshake_callback.callback()));

    ASSERT_EQ(net::OK, server_ret);
  }

  void TcpAcceptCallback(int result) { accept_result_ = result; }

  void TcpConnectCallback(int result) { connect_result_ = result; }

  std::unique_ptr<crypto::RSAPrivateKey> ReadTestKeyFromPEM(
      const base::StringPiece& name) {
    base::FilePath key_path = GetTestCertsDirectory().AppendASCII(name);
    std::vector<std::string> headers({"PRIVATE KEY"});
    std::string pem_data;
    if (!base::ReadFileToString(key_path, &pem_data)) {
      return nullptr;
    }
    net::PEMTokenizer pem_tokenizer(pem_data, headers);
    if (!pem_tokenizer.GetNext()) {
      return nullptr;
    }
    std::vector<uint8_t> key_vector(pem_tokenizer.data().begin(),
                                    pem_tokenizer.data().end());
    std::unique_ptr<crypto::RSAPrivateKey> key(
        crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(key_vector));
    return key;
  }

  int ReadExactLength(net::IOBuffer* buffer,
                      int buffer_length,
                      net::Socket* socket) {
    scoped_refptr<net::DrainableIOBuffer> draining_buffer(
        new net::DrainableIOBuffer(buffer, buffer_length));
    while (draining_buffer->BytesRemaining() > 0) {
      net::TestCompletionCallback read_callback;
      int read_result = read_callback.GetResult(server_socket_->Read(
          draining_buffer.get(), draining_buffer->BytesRemaining(),
          read_callback.callback()));
      EXPECT_GT(read_result, 0);
      draining_buffer->DidConsume(read_result);
    }
    return buffer_length;
  }

  int WriteExactLength(net::IOBuffer* buffer,
                       int buffer_length,
                       net::Socket* socket) {
    scoped_refptr<net::DrainableIOBuffer> draining_buffer(
        new net::DrainableIOBuffer(buffer, buffer_length));
    while (draining_buffer->BytesRemaining() > 0) {
      net::TestCompletionCallback write_callback;
      int write_result = write_callback.GetResult(server_socket_->Write(
          draining_buffer.get(), draining_buffer->BytesRemaining(),
          write_callback.callback(), TRAFFIC_ANNOTATION_FOR_TESTS));
      EXPECT_GT(write_result, 0);
      draining_buffer->DidConsume(write_result);
    }
    return buffer_length;
  }

  // Result values used for TCP socket setup.  These should contain values from
  // net::Error.
  int accept_result_;
  int connect_result_;

  // Underlying TCP sockets for |socket_| to communicate with |server_socket_|
  // when testing with the real SSL implementation.
  std::unique_ptr<net::TransportClientSocket> tcp_client_socket_;
  std::unique_ptr<net::TCPServerSocket> tcp_server_socket_;

  std::unique_ptr<SslTestCastSocket> socket_;

  // |server_socket_| is used for the *RealSSL tests in order to test the
  // CastSocket over a real SSL socket.  The other members below are used to
  // initialize |server_socket_|.
  std::unique_ptr<net::SSLServerSocket> server_socket_;
  std::unique_ptr<net::SSLServerContext> server_context_;
  std::unique_ptr<crypto::RSAPrivateKey> server_private_key_;
  scoped_refptr<net::X509Certificate> server_cert_;
  net::SSLServerConfig server_ssl_config_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SslCastSocketTest);
};

}  // namespace

// Tests that the following connection flow works:
// - TCP connection succeeds (async)
// - SSL connection succeeds (async)
// - Cert is extracted successfully
// - Challenge request is sent (async)
// - Challenge response is received (async)
// - Credentials are verified successfuly
TEST_F(MockCastSocketTest, TestConnectFullSecureFlowAsync) {
  CreateCastSocketSecure();
  socket_->SetupTcpConnect(net::ASYNC, net::OK);
  socket_->SetupSslConnect(net::ASYNC, net::OK);

  HandleAuthHandshake();

  EXPECT_EQ(ReadyState::OPEN, socket_->ready_state());
  EXPECT_EQ(ChannelError::NONE, socket_->error_state());
}

// Tests that the following connection flow works:
// - TCP connection succeeds (sync)
// - SSL connection succeeds (sync)
// - Cert is extracted successfully
// - Challenge request is sent (sync)
// - Challenge response is received (sync)
// - Credentials are verified successfuly
TEST_F(MockCastSocketTest, TestConnectFullSecureFlowSync) {
  CreateCastSocketSecure();
  socket_->SetupTcpConnect(net::SYNCHRONOUS, net::OK);
  socket_->SetupSslConnect(net::SYNCHRONOUS, net::OK);

  HandleAuthHandshake();

  EXPECT_EQ(ReadyState::OPEN, socket_->ready_state());
  EXPECT_EQ(ChannelError::NONE, socket_->error_state());
}

// Test that an AuthMessage with a mangled namespace triggers cancelation
// of the connection event loop.
TEST_F(MockCastSocketTest, TestConnectAuthMessageCorrupted) {
  CreateCastSocketSecure();
  socket_->SetupMockTransport();

  socket_->SetupTcpConnect(net::ASYNC, net::OK);
  socket_->SetupSslConnect(net::ASYNC, net::OK);

  CastMessage challenge_proto = CreateAuthChallenge();
  EXPECT_CALL(*socket_->GetMockTransport(),
              SendMessage(EqualsProto(challenge_proto), _, _))
      .WillOnce(PostCompletionCallbackTask<1>(net::OK));
  EXPECT_CALL(*socket_->GetMockTransport(), Start());
  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();
  CastMessage mangled_auth_reply = CreateAuthReply();
  mangled_auth_reply.set_namespace_("BOGUS_NAMESPACE");

  socket_->GetMockTransport()->current_delegate()->OnMessage(
      mangled_auth_reply);
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::TRANSPORT_ERROR, socket_->error_state());

  // Verifies that the CastSocket's resources were torn down during channel
  // close. (see http://crbug.com/504078)
  EXPECT_EQ(nullptr, socket_->transport());
}

// Test connection error - TCP connect fails (async)
TEST_F(MockCastSocketTest, TestConnectTcpConnectErrorAsync) {
  CreateCastSocketSecure();

  socket_->SetupTcpConnect(net::ASYNC, net::ERR_FAILED);

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::CONNECT_ERROR, socket_->error_state());
}

// Test connection error - TCP connect fails (sync)
TEST_F(MockCastSocketTest, TestConnectTcpConnectErrorSync) {
  CreateCastSocketSecure();

  socket_->SetupTcpConnect(net::SYNCHRONOUS, net::ERR_FAILED);

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::CONNECT_ERROR, socket_->error_state());
}

// Test connection error - timeout
TEST_F(MockCastSocketTest, TestConnectTcpTimeoutError) {
  CreateCastSocketSecure();
  socket_->SetupTcpConnectUnresponsive();
  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  EXPECT_CALL(*observer_, OnError(_, ChannelError::CONNECT_TIMEOUT));
  socket_->AddObserver(observer_.get());
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CONNECTING, socket_->ready_state());
  EXPECT_EQ(ChannelError::NONE, socket_->error_state());
  socket_->TriggerTimeout();
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::CONNECT_TIMEOUT, socket_->error_state());
}

// Test connection error - TCP socket returns timeout
TEST_F(MockCastSocketTest, TestConnectTcpSocketTimeoutError) {
  CreateCastSocketSecure();
  socket_->SetupTcpConnect(net::SYNCHRONOUS, net::ERR_CONNECTION_TIMED_OUT);
  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  EXPECT_CALL(*observer_, OnError(_, ChannelError::CONNECT_TIMEOUT));
  socket_->AddObserver(observer_.get());
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::CONNECT_TIMEOUT, socket_->error_state());
  EXPECT_EQ(net::ERR_CONNECTION_TIMED_OUT,
            logger_->GetLastError(socket_->id()).net_return_value);
}

// Test connection error - SSL connect fails (async)
TEST_F(MockCastSocketTest, TestConnectSslConnectErrorAsync) {
  CreateCastSocketSecure();

  socket_->SetupTcpConnect(net::SYNCHRONOUS, net::OK);
  socket_->SetupSslConnect(net::SYNCHRONOUS, net::ERR_FAILED);

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::AUTHENTICATION_ERROR, socket_->error_state());
}

// Test connection error - SSL connect fails (sync)
TEST_F(MockCastSocketTest, TestConnectSslConnectErrorSync) {
  CreateCastSocketSecure();

  socket_->SetupTcpConnect(net::SYNCHRONOUS, net::OK);
  socket_->SetupSslConnect(net::SYNCHRONOUS, net::ERR_FAILED);

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::AUTHENTICATION_ERROR, socket_->error_state());
  EXPECT_EQ(net::ERR_FAILED,
            logger_->GetLastError(socket_->id()).net_return_value);
}

// Test connection error - SSL connect times out (sync)
TEST_F(MockCastSocketTest, TestConnectSslConnectTimeoutSync) {
  CreateCastSocketSecure();

  socket_->SetupTcpConnect(net::SYNCHRONOUS, net::OK);
  socket_->SetupSslConnect(net::SYNCHRONOUS, net::ERR_CONNECTION_TIMED_OUT);

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::CONNECT_TIMEOUT, socket_->error_state());
  EXPECT_EQ(net::ERR_CONNECTION_TIMED_OUT,
            logger_->GetLastError(socket_->id()).net_return_value);
}

// Test connection error - SSL connect times out (async)
TEST_F(MockCastSocketTest, TestConnectSslConnectTimeoutAsync) {
  CreateCastSocketSecure();

  socket_->SetupTcpConnect(net::ASYNC, net::OK);
  socket_->SetupSslConnect(net::ASYNC, net::ERR_CONNECTION_TIMED_OUT);

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::CONNECT_TIMEOUT, socket_->error_state());
}

// Test connection error - challenge send fails
TEST_F(MockCastSocketTest, TestConnectChallengeSendError) {
  CreateCastSocketSecure();
  socket_->SetupMockTransport();

  socket_->SetupTcpConnect(net::SYNCHRONOUS, net::OK);
  socket_->SetupSslConnect(net::SYNCHRONOUS, net::OK);
  EXPECT_CALL(*socket_->GetMockTransport(),
              SendMessage(EqualsProto(CreateAuthChallenge()), _, _))
      .WillOnce(PostCompletionCallbackTask<1>(net::ERR_CONNECTION_RESET));

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::CAST_SOCKET_ERROR, socket_->error_state());
}

// Test connection error - connection is destroyed after the challenge is
// sent, with the async result still lurking in the task queue.
TEST_F(MockCastSocketTest, TestConnectDestroyedAfterChallengeSent) {
  CreateCastSocketSecure();
  socket_->SetupMockTransport();
  socket_->SetupTcpConnect(net::SYNCHRONOUS, net::OK);
  socket_->SetupSslConnect(net::SYNCHRONOUS, net::OK);
  EXPECT_CALL(*socket_->GetMockTransport(),
              SendMessage(EqualsProto(CreateAuthChallenge()), _, _))
      .WillOnce(PostCompletionCallbackTask<1>(net::ERR_CONNECTION_RESET));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  socket_.reset();
  RunPendingTasks();
}

// Test connection error - challenge reply receive fails
TEST_F(MockCastSocketTest, TestConnectChallengeReplyReceiveError) {
  CreateCastSocketSecure();
  socket_->SetupMockTransport();

  socket_->SetupTcpConnect(net::SYNCHRONOUS, net::OK);
  socket_->SetupSslConnect(net::SYNCHRONOUS, net::OK);
  EXPECT_CALL(*socket_->GetMockTransport(),
              SendMessage(EqualsProto(CreateAuthChallenge()), _, _))
      .WillOnce(PostCompletionCallbackTask<1>(net::OK));
  socket_->AddReadResult(net::SYNCHRONOUS, net::ERR_FAILED);
  EXPECT_CALL(*observer_, OnError(_, ChannelError::CAST_SOCKET_ERROR));
  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  EXPECT_CALL(*socket_->GetMockTransport(), Start());
  socket_->AddObserver(observer_.get());
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();
  socket_->GetMockTransport()->current_delegate()->OnError(
      ChannelError::CAST_SOCKET_ERROR);
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::CAST_SOCKET_ERROR, socket_->error_state());
}

TEST_F(MockCastSocketTest, TestConnectChallengeVerificationFails) {
  CreateCastSocketSecure();
  socket_->SetupMockTransport();
  socket_->SetupTcpConnect(net::ASYNC, net::OK);
  socket_->SetupSslConnect(net::ASYNC, net::OK);
  socket_->SetVerifyChallengeResult(false);

  EXPECT_CALL(*observer_, OnError(_, ChannelError::AUTHENTICATION_ERROR));
  CastMessage challenge_proto = CreateAuthChallenge();
  EXPECT_CALL(*socket_->GetMockTransport(),
              SendMessage(EqualsProto(challenge_proto), _, _))
      .WillOnce(PostCompletionCallbackTask<1>(net::OK));
  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  EXPECT_CALL(*socket_->GetMockTransport(), Start());
  socket_->AddObserver(observer_.get());
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();
  socket_->GetMockTransport()->current_delegate()->OnMessage(CreateAuthReply());
  RunPendingTasks();

  EXPECT_EQ(ReadyState::CLOSED, socket_->ready_state());
  EXPECT_EQ(ChannelError::AUTHENTICATION_ERROR, socket_->error_state());
}

// Sends message data through an actual non-mocked CastTransport object,
// testing the two components in integration.
TEST_F(MockCastSocketTest, TestConnectEndToEndWithRealTransportAsync) {
  CreateCastSocketSecure();
  socket_->SetupTcpConnect(net::ASYNC, net::OK);
  socket_->SetupSslConnect(net::ASYNC, net::OK);

  // Set low-level auth challenge expectations.
  CastMessage challenge = CreateAuthChallenge();
  std::string challenge_str;
  EXPECT_TRUE(MessageFramer::Serialize(challenge, &challenge_str));
  socket_->AddWriteResultForData(net::ASYNC, challenge_str);

  // Set low-level auth reply expectations.
  CastMessage reply = CreateAuthReply();
  std::string reply_str;
  EXPECT_TRUE(MessageFramer::Serialize(reply, &reply_str));
  socket_->AddReadResultForData(net::ASYNC, reply_str);
  socket_->AddReadResult(net::ASYNC, net::ERR_IO_PENDING);

  CastMessage test_message = CreateTestMessage();
  std::string test_message_str;
  EXPECT_TRUE(MessageFramer::Serialize(test_message, &test_message_str));
  socket_->AddWriteResultForData(net::ASYNC, test_message_str);

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();
  EXPECT_EQ(ReadyState::OPEN, socket_->ready_state());
  EXPECT_EQ(ChannelError::NONE, socket_->error_state());

  // Send the test message through a real transport object.
  EXPECT_CALL(handler_, OnWriteComplete(net::OK));
  socket_->transport()->SendMessage(
      test_message,
      base::Bind(&CompleteHandler::OnWriteComplete,
                 base::Unretained(&handler_)),
      TRAFFIC_ANNOTATION_FOR_TESTS);
  RunPendingTasks();

  EXPECT_EQ(ReadyState::OPEN, socket_->ready_state());
  EXPECT_EQ(ChannelError::NONE, socket_->error_state());
}

// Same as TestConnectEndToEndWithRealTransportAsync, except synchronous.
TEST_F(MockCastSocketTest, TestConnectEndToEndWithRealTransportSync) {
  CreateCastSocketSecure();
  socket_->SetupTcpConnect(net::SYNCHRONOUS, net::OK);
  socket_->SetupSslConnect(net::SYNCHRONOUS, net::OK);

  // Set low-level auth challenge expectations.
  CastMessage challenge = CreateAuthChallenge();
  std::string challenge_str;
  EXPECT_TRUE(MessageFramer::Serialize(challenge, &challenge_str));
  socket_->AddWriteResultForData(net::SYNCHRONOUS, challenge_str);

  // Set low-level auth reply expectations.
  CastMessage reply = CreateAuthReply();
  std::string reply_str;
  EXPECT_TRUE(MessageFramer::Serialize(reply, &reply_str));
  socket_->AddReadResultForData(net::SYNCHRONOUS, reply_str);
  socket_->AddReadResult(net::ASYNC, net::ERR_IO_PENDING);

  CastMessage test_message = CreateTestMessage();
  std::string test_message_str;
  EXPECT_TRUE(MessageFramer::Serialize(test_message, &test_message_str));
  socket_->AddWriteResultForData(net::SYNCHRONOUS, test_message_str);

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();
  EXPECT_EQ(ReadyState::OPEN, socket_->ready_state());
  EXPECT_EQ(ChannelError::NONE, socket_->error_state());

  // Send the test message through a real transport object.
  EXPECT_CALL(handler_, OnWriteComplete(net::OK));
  socket_->transport()->SendMessage(
      test_message,
      base::Bind(&CompleteHandler::OnWriteComplete,
                 base::Unretained(&handler_)),
      TRAFFIC_ANNOTATION_FOR_TESTS);
  RunPendingTasks();

  EXPECT_EQ(ReadyState::OPEN, socket_->ready_state());
  EXPECT_EQ(ChannelError::NONE, socket_->error_state());
}

TEST_F(MockCastSocketTest, TestObservers) {
  CreateCastSocketSecure();
  // Test AddObserever
  MockCastSocketObserver observer1;
  MockCastSocketObserver observer2;
  socket_->AddObserver(&observer1);
  socket_->AddObserver(&observer1);
  socket_->AddObserver(&observer2);
  socket_->AddObserver(&observer2);

  // Test notify observers
  EXPECT_CALL(observer1, OnError(_, cast_channel::ChannelError::CONNECT_ERROR));
  EXPECT_CALL(observer2, OnError(_, cast_channel::ChannelError::CONNECT_ERROR));
  CastSocketImpl::CastSocketMessageDelegate delegate(socket_.get());
  delegate.OnError(cast_channel::ChannelError::CONNECT_ERROR);
}

TEST_F(MockCastSocketTest, TestOpenChannelConnectingSocket) {
  CreateCastSocketSecure();
  socket_->SetupTcpConnectUnresponsive();
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get())).Times(2);
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  socket_->TriggerTimeout();
  RunPendingTasks();
}

TEST_F(MockCastSocketTest, TestOpenChannelConnectedSocket) {
  CreateCastSocketSecure();
  socket_->SetupTcpConnect(net::ASYNC, net::OK);
  socket_->SetupSslConnect(net::ASYNC, net::OK);

  HandleAuthHandshake();

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
}

TEST_F(MockCastSocketTest, TestOpenChannelClosedSocket) {
  CreateCastSocketSecure();
  socket_->SetupTcpConnect(net::ASYNC, net::ERR_FAILED);

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
  RunPendingTasks();

  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  socket_->Connect(base::Bind(&CompleteHandler::OnConnectComplete,
                              base::Unretained(&handler_)));
}

// Tests connecting through an actual non-mocked CastTransport object and
// non-mocked SSLClientSocket, testing the components in integration.
TEST_F(SslCastSocketTest, TestConnectEndToEndWithRealSSL) {
  CreateSockets();
  ConnectSockets();

  // Set low-level auth challenge expectations.
  CastMessage challenge = CreateAuthChallenge();
  std::string challenge_str;
  EXPECT_TRUE(MessageFramer::Serialize(challenge, &challenge_str));

  int challenge_buffer_length = challenge_str.size();
  scoped_refptr<net::IOBuffer> challenge_buffer(
      new net::IOBuffer(challenge_buffer_length));
  int read = ReadExactLength(challenge_buffer.get(), challenge_buffer_length,
                             server_socket_.get());

  EXPECT_EQ(challenge_buffer_length, read);
  EXPECT_EQ(challenge_str,
            std::string(challenge_buffer->data(), challenge_buffer_length));

  // Set low-level auth reply expectations.
  CastMessage reply = CreateAuthReply();
  std::string reply_str;
  EXPECT_TRUE(MessageFramer::Serialize(reply, &reply_str));

  scoped_refptr<net::StringIOBuffer> reply_buffer(
      new net::StringIOBuffer(reply_str));
  int written = WriteExactLength(reply_buffer.get(), reply_buffer->size(),
                                 server_socket_.get());

  EXPECT_EQ(reply_buffer->size(), written);
  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::OPEN, socket_->ready_state());
  EXPECT_EQ(ChannelError::NONE, socket_->error_state());
}

// Sends message data through an actual non-mocked CastTransport object and
// non-mocked SSLClientSocket, testing the components in integration.
TEST_F(SslCastSocketTest, TestMessageEndToEndWithRealSSL) {
  CreateSockets();
  ConnectSockets();

  // Set low-level auth challenge expectations.
  CastMessage challenge = CreateAuthChallenge();
  std::string challenge_str;
  EXPECT_TRUE(MessageFramer::Serialize(challenge, &challenge_str));

  int challenge_buffer_length = challenge_str.size();
  scoped_refptr<net::IOBuffer> challenge_buffer(
      new net::IOBuffer(challenge_buffer_length));

  int read = ReadExactLength(challenge_buffer.get(), challenge_buffer_length,
                             server_socket_.get());

  EXPECT_EQ(challenge_buffer_length, read);
  EXPECT_EQ(challenge_str,
            std::string(challenge_buffer->data(), challenge_buffer_length));

  // Set low-level auth reply expectations.
  CastMessage reply = CreateAuthReply();
  std::string reply_str;
  EXPECT_TRUE(MessageFramer::Serialize(reply, &reply_str));

  scoped_refptr<net::StringIOBuffer> reply_buffer(
      new net::StringIOBuffer(reply_str));
  int written = WriteExactLength(reply_buffer.get(), reply_buffer->size(),
                                 server_socket_.get());

  EXPECT_EQ(reply_buffer->size(), written);
  EXPECT_CALL(handler_, OnConnectComplete(socket_.get()));
  RunPendingTasks();

  EXPECT_EQ(ReadyState::OPEN, socket_->ready_state());
  EXPECT_EQ(ChannelError::NONE, socket_->error_state());

  // Send a test message through the ssl socket.
  CastMessage test_message = CreateTestMessage();
  std::string test_message_str;
  EXPECT_TRUE(MessageFramer::Serialize(test_message, &test_message_str));

  int test_message_length = test_message_str.size();
  scoped_refptr<net::IOBuffer> test_message_buffer(
      new net::IOBuffer(test_message_length));

  EXPECT_CALL(handler_, OnWriteComplete(net::OK));
  socket_->transport()->SendMessage(
      test_message,
      base::Bind(&CompleteHandler::OnWriteComplete,
                 base::Unretained(&handler_)),
      TRAFFIC_ANNOTATION_FOR_TESTS);
  RunPendingTasks();

  read = ReadExactLength(test_message_buffer.get(), test_message_length,
                         server_socket_.get());

  EXPECT_EQ(test_message_length, read);
  EXPECT_EQ(test_message_str,
            std::string(test_message_buffer->data(), test_message_length));

  EXPECT_EQ(ReadyState::OPEN, socket_->ready_state());
  EXPECT_EQ(ChannelError::NONE, socket_->error_state());
}

}  // namespace cast_channel
