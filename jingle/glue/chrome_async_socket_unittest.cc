// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/glue/chrome_async_socket.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "base/containers/circular_deque.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_pump_default.h"
#include "base/run_loop.h"
#include "jingle/glue/resolving_client_socket_factory.h"
#include "net/base/address_list.h"
#include "net/base/ip_address.h"
#include "net/base/net_errors.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/http/transport_security_state.h"
#include "net/log/net_log_source.h"
#include "net/socket/socket_test_util.h"
#include "net/socket/ssl_client_socket.h"
#include "net/ssl/ssl_config_service.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/webrtc/rtc_base/ipaddress.h"
#include "third_party/webrtc/rtc_base/sigslot.h"
#include "third_party/webrtc/rtc_base/socketaddress.h"

namespace jingle_glue {

namespace {

// Data provider that handles reads/writes for ChromeAsyncSocket.
class AsyncSocketDataProvider : public net::SocketDataProvider {
 public:
  AsyncSocketDataProvider() : has_pending_read_(false) {}

  ~AsyncSocketDataProvider() override {
    EXPECT_TRUE(writes_.empty());
    EXPECT_TRUE(reads_.empty());
  }

  // If there's no read, sets the "has pending read" flag.  Otherwise,
  // pops the next read.
  net::MockRead OnRead() override {
    if (reads_.empty()) {
      DCHECK(!has_pending_read_);
      has_pending_read_ = true;
      const net::MockRead pending_read(net::SYNCHRONOUS, net::ERR_IO_PENDING);
      return pending_read;
    }
    net::MockRead mock_read = reads_.front();
    reads_.pop_front();
    return mock_read;
  }

  // Simply pops the next write and, if applicable, compares it to
  // |data|.
  net::MockWriteResult OnWrite(const std::string& data) override {
    DCHECK(!writes_.empty());
    net::MockWrite mock_write = writes_.front();
    writes_.pop_front();
    if (mock_write.result != net::OK) {
      return net::MockWriteResult(mock_write.mode, mock_write.result);
    }
    std::string expected_data(mock_write.data, mock_write.data_len);
    EXPECT_EQ(expected_data, data);
    if (expected_data != data) {
      return net::MockWriteResult(net::SYNCHRONOUS, net::ERR_UNEXPECTED);
    }
    return net::MockWriteResult(mock_write.mode, data.size());
  }

  // We ignore resets so we can pre-load the socket data provider with
  // read/write events.
  void Reset() override {}

  // If there is a pending read, completes it with the given read.
  // Otherwise, queues up the given read.
  void AddRead(const net::MockRead& mock_read) {
    DCHECK_NE(mock_read.result, net::ERR_IO_PENDING);
    if (has_pending_read_) {
      socket()->OnReadComplete(mock_read);
      has_pending_read_ = false;
      return;
    }
    reads_.push_back(mock_read);
  }

  // Simply queues up the given write.
  void AddWrite(const net::MockWrite& mock_write) {
    writes_.push_back(mock_write);
  }

  bool AllReadDataConsumed() const override {
    return reads_.empty();
  }

  bool AllWriteDataConsumed() const override {
    return writes_.empty();
  }

 private:
  base::circular_deque<net::MockRead> reads_;
  bool has_pending_read_;

  base::circular_deque<net::MockWrite> writes_;

  DISALLOW_COPY_AND_ASSIGN(AsyncSocketDataProvider);
};

class MockXmppClientSocketFactory : public ResolvingClientSocketFactory {
 public:
  MockXmppClientSocketFactory(
      net::ClientSocketFactory* mock_client_socket_factory,
      const net::AddressList& address_list)
          : mock_client_socket_factory_(mock_client_socket_factory),
            address_list_(address_list),
            cert_verifier_(new net::MockCertVerifier),
            transport_security_state_(new net::TransportSecurityState) {
  }

  // ResolvingClientSocketFactory implementation.
  std::unique_ptr<net::StreamSocket> CreateTransportClientSocket(
      const net::HostPortPair& host_and_port) override {
    return mock_client_socket_factory_->CreateTransportClientSocket(
        address_list_, NULL, NULL, net::NetLogSource());
  }

  std::unique_ptr<net::SSLClientSocket> CreateSSLClientSocket(
      std::unique_ptr<net::ClientSocketHandle> transport_socket,
      const net::HostPortPair& host_and_port) override {
    net::SSLClientSocketContext context;
    context.cert_verifier = cert_verifier_.get();
    context.transport_security_state = transport_security_state_.get();
    return mock_client_socket_factory_->CreateSSLClientSocket(
        std::move(transport_socket), host_and_port, ssl_config_, context);
  }

 private:
  std::unique_ptr<net::ClientSocketFactory> mock_client_socket_factory_;
  net::AddressList address_list_;
  net::SSLConfig ssl_config_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  std::unique_ptr<net::TransportSecurityState> transport_security_state_;
};

class ChromeAsyncSocketTest
    : public testing::Test,
      public sigslot::has_slots<> {
 protected:
  ChromeAsyncSocketTest()
      : ssl_socket_data_provider_(net::ASYNC, net::OK),
        addr_("localhost", 35) {
    // GTest death tests by default execute in a fork()ed but not exec()ed
    // process. On macOS, a CoreFoundation-backed MessageLoop will exit with a
    // __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__
    // when called. Use the threadsafe mode to avoid this problem.
    testing::GTEST_FLAG(death_test_style) = "threadsafe";
  }

  ~ChromeAsyncSocketTest() override {}

  void SetUp() override {
    std::unique_ptr<net::MockClientSocketFactory> mock_client_socket_factory(
        new net::MockClientSocketFactory());
    mock_client_socket_factory->AddSocketDataProvider(
        &async_socket_data_provider_);
    mock_client_socket_factory->AddSSLSocketDataProvider(
        &ssl_socket_data_provider_);

    // Fake DNS resolution for |addr_| and pass it to the factory.
    const net::AddressList address_list = net::AddressList::CreateFromIPAddress(
        net::IPAddress::IPv4Localhost(), addr_.port());
    std::unique_ptr<MockXmppClientSocketFactory>
        mock_xmpp_client_socket_factory(new MockXmppClientSocketFactory(
            mock_client_socket_factory.release(), address_list));
    chrome_async_socket_.reset(
        new ChromeAsyncSocket(mock_xmpp_client_socket_factory.release(), 14, 20,
                              TRAFFIC_ANNOTATION_FOR_TESTS)),

        chrome_async_socket_->SignalConnected.connect(
            this, &ChromeAsyncSocketTest::OnConnect);
    chrome_async_socket_->SignalSSLConnected.connect(
        this, &ChromeAsyncSocketTest::OnSSLConnect);
    chrome_async_socket_->SignalClosed.connect(
        this, &ChromeAsyncSocketTest::OnClose);
    chrome_async_socket_->SignalRead.connect(
        this, &ChromeAsyncSocketTest::OnRead);
    chrome_async_socket_->SignalError.connect(
        this, &ChromeAsyncSocketTest::OnError);
  }

  void TearDown() override {
    // Run any tasks that we forgot to pump.
    base::RunLoop().RunUntilIdle();
    ExpectClosed();
    ExpectNoSignal();
    chrome_async_socket_.reset();
  }

  enum Signal {
    SIGNAL_CONNECT,
    SIGNAL_SSL_CONNECT,
    SIGNAL_CLOSE,
    SIGNAL_READ,
    SIGNAL_ERROR,
  };

  // Helper struct that records the state at the time of a signal.

  struct SignalSocketState {
    SignalSocketState()
        : signal(SIGNAL_ERROR),
          state(ChromeAsyncSocket::STATE_CLOSED),
          error(ChromeAsyncSocket::ERROR_NONE),
          net_error(net::OK) {}

    SignalSocketState(
        Signal signal,
        ChromeAsyncSocket::State state,
        ChromeAsyncSocket::Error error,
        net::Error net_error)
        : signal(signal),
          state(state),
          error(error),
          net_error(net_error) {}

    bool IsEqual(const SignalSocketState& other) const {
      return
          (signal == other.signal) &&
          (state == other.state) &&
          (error == other.error) &&
          (net_error == other.net_error);
    }

    static SignalSocketState FromAsyncSocket(
        Signal signal,
        buzz::AsyncSocket* async_socket) {
      return SignalSocketState(signal,
                               async_socket->state(),
                               async_socket->error(),
                               static_cast<net::Error>(
                                   async_socket->GetError()));
    }

    static SignalSocketState NoError(
        Signal signal, buzz::AsyncSocket::State state) {
        return SignalSocketState(signal, state,
                                 buzz::AsyncSocket::ERROR_NONE,
                                 net::OK);
    }

    Signal signal;
    ChromeAsyncSocket::State state;
    ChromeAsyncSocket::Error error;
    net::Error net_error;
  };

  void CaptureSocketState(Signal signal) {
    signal_socket_states_.push_back(
        SignalSocketState::FromAsyncSocket(
            signal, chrome_async_socket_.get()));
  }

  void OnConnect() {
    CaptureSocketState(SIGNAL_CONNECT);
  }

  void OnSSLConnect() {
    CaptureSocketState(SIGNAL_SSL_CONNECT);
  }

  void OnClose() {
    CaptureSocketState(SIGNAL_CLOSE);
  }

  void OnRead() {
    CaptureSocketState(SIGNAL_READ);
  }

  void OnError() {
    ADD_FAILURE();
  }

  // State expect functions.

  void ExpectState(ChromeAsyncSocket::State state,
                   ChromeAsyncSocket::Error error,
                   net::Error net_error) {
    EXPECT_EQ(state, chrome_async_socket_->state());
    EXPECT_EQ(error, chrome_async_socket_->error());
    EXPECT_EQ(net_error, chrome_async_socket_->GetError());
  }

  void ExpectNonErrorState(ChromeAsyncSocket::State state) {
    ExpectState(state, ChromeAsyncSocket::ERROR_NONE, net::OK);
  }

  void ExpectErrorState(ChromeAsyncSocket::State state,
                        ChromeAsyncSocket::Error error) {
    ExpectState(state, error, net::OK);
  }

  void ExpectClosed() {
    ExpectNonErrorState(ChromeAsyncSocket::STATE_CLOSED);
  }

  // Signal expect functions.

  void ExpectNoSignal() {
    if (!signal_socket_states_.empty()) {
      ADD_FAILURE() << signal_socket_states_.front().signal;
    }
  }

  void ExpectSignalSocketState(
      SignalSocketState expected_signal_socket_state) {
    if (signal_socket_states_.empty()) {
      ADD_FAILURE() << expected_signal_socket_state.signal;
      return;
    }
    EXPECT_TRUE(expected_signal_socket_state.IsEqual(
        signal_socket_states_.front()))
        << signal_socket_states_.front().signal;
    signal_socket_states_.pop_front();
  }

  void ExpectReadSignal() {
    ExpectSignalSocketState(
        SignalSocketState::NoError(
            SIGNAL_READ, ChromeAsyncSocket::STATE_OPEN));
  }

  void ExpectSSLConnectSignal() {
    ExpectSignalSocketState(
        SignalSocketState::NoError(SIGNAL_SSL_CONNECT,
                                   ChromeAsyncSocket::STATE_TLS_OPEN));
  }

  void ExpectSSLReadSignal() {
    ExpectSignalSocketState(
        SignalSocketState::NoError(
            SIGNAL_READ, ChromeAsyncSocket::STATE_TLS_OPEN));
  }

  // Open/close utility functions.

  void DoOpenClosed() {
    ExpectClosed();
    async_socket_data_provider_.set_connect_data(
        net::MockConnect(net::SYNCHRONOUS, net::OK));
    EXPECT_TRUE(chrome_async_socket_->Connect(addr_));
    ExpectNonErrorState(ChromeAsyncSocket::STATE_CONNECTING);

    base::RunLoop().RunUntilIdle();
    // We may not necessarily be open; may have been other events
    // queued up.
    ExpectSignalSocketState(
        SignalSocketState::NoError(
            SIGNAL_CONNECT, ChromeAsyncSocket::STATE_OPEN));
  }

  void DoCloseOpened(SignalSocketState expected_signal_socket_state) {
    // We may be in an error state, so just compare state().
    EXPECT_EQ(ChromeAsyncSocket::STATE_OPEN, chrome_async_socket_->state());
    EXPECT_TRUE(chrome_async_socket_->Close());
    ExpectSignalSocketState(expected_signal_socket_state);
    ExpectNonErrorState(ChromeAsyncSocket::STATE_CLOSED);
  }

  void DoCloseOpenedNoError() {
    DoCloseOpened(
        SignalSocketState::NoError(
            SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED));
  }

  void DoSSLOpenClosed() {
    const char kDummyData[] = "dummy_data";
    async_socket_data_provider_.AddRead(net::MockRead(kDummyData));
    DoOpenClosed();
    ExpectReadSignal();
    EXPECT_EQ(kDummyData, DrainRead(1));

    EXPECT_TRUE(chrome_async_socket_->StartTls("fakedomain.com"));
    base::RunLoop().RunUntilIdle();
    ExpectSSLConnectSignal();
    ExpectNoSignal();
    ExpectNonErrorState(ChromeAsyncSocket::STATE_TLS_OPEN);
  }

  void DoSSLCloseOpened(SignalSocketState expected_signal_socket_state) {
    // We may be in an error state, so just compare state().
    EXPECT_EQ(ChromeAsyncSocket::STATE_TLS_OPEN,
              chrome_async_socket_->state());
    EXPECT_TRUE(chrome_async_socket_->Close());
    ExpectSignalSocketState(expected_signal_socket_state);
    ExpectNonErrorState(ChromeAsyncSocket::STATE_CLOSED);
  }

  void DoSSLCloseOpenedNoError() {
    DoSSLCloseOpened(
        SignalSocketState::NoError(
            SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED));
  }

  // Read utility fucntions.

  std::string DrainRead(size_t buf_size) {
    std::string read;
    std::unique_ptr<char[]> buf(new char[buf_size]);
    size_t len_read;
    while (true) {
      bool success =
          chrome_async_socket_->Read(buf.get(), buf_size, &len_read);
      if (!success) {
        ADD_FAILURE();
        break;
      }
      if (len_read == 0U) {
        break;
      }
      read.append(buf.get(), len_read);
    }
    return read;
  }

  // ChromeAsyncSocket expects a message loop.
  base::MessageLoop message_loop_;

  AsyncSocketDataProvider async_socket_data_provider_;
  net::SSLSocketDataProvider ssl_socket_data_provider_;

  std::unique_ptr<ChromeAsyncSocket> chrome_async_socket_;
  base::circular_deque<SignalSocketState> signal_socket_states_;
  const rtc::SocketAddress addr_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeAsyncSocketTest);
};

TEST_F(ChromeAsyncSocketTest, InitialState) {
  ExpectClosed();
  ExpectNoSignal();
}

TEST_F(ChromeAsyncSocketTest, EmptyClose) {
  ExpectClosed();
  EXPECT_TRUE(chrome_async_socket_->Close());
  ExpectClosed();
}

TEST_F(ChromeAsyncSocketTest, ImmediateConnectAndClose) {
  DoOpenClosed();

  ExpectNonErrorState(ChromeAsyncSocket::STATE_OPEN);

  DoCloseOpenedNoError();
}

// After this, no need to test immediate successful connect and
// Close() so thoroughly.

TEST_F(ChromeAsyncSocketTest, DoubleClose) {
  DoOpenClosed();

  EXPECT_TRUE(chrome_async_socket_->Close());
  ExpectClosed();
  ExpectSignalSocketState(
      SignalSocketState::NoError(
          SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED));

  EXPECT_TRUE(chrome_async_socket_->Close());
  ExpectClosed();
}

TEST_F(ChromeAsyncSocketTest, NoHostnameConnect) {
  rtc::IPAddress ip_address;
  EXPECT_TRUE(rtc::IPFromString("127.0.0.1", &ip_address));
  const rtc::SocketAddress no_hostname_addr(ip_address, addr_.port());
  EXPECT_FALSE(chrome_async_socket_->Connect(no_hostname_addr));
  ExpectErrorState(ChromeAsyncSocket::STATE_CLOSED,
                   ChromeAsyncSocket::ERROR_DNS);

  EXPECT_TRUE(chrome_async_socket_->Close());
  ExpectClosed();
}

TEST_F(ChromeAsyncSocketTest, ZeroPortConnect) {
  const rtc::SocketAddress zero_port_addr(addr_.hostname(), 0);
  EXPECT_FALSE(chrome_async_socket_->Connect(zero_port_addr));
  ExpectErrorState(ChromeAsyncSocket::STATE_CLOSED,
                   ChromeAsyncSocket::ERROR_DNS);

  EXPECT_TRUE(chrome_async_socket_->Close());
  ExpectClosed();
}

TEST_F(ChromeAsyncSocketTest, DoubleConnect) {
  EXPECT_DEBUG_DEATH({
    DoOpenClosed();

    EXPECT_FALSE(chrome_async_socket_->Connect(addr_));
    ExpectErrorState(ChromeAsyncSocket::STATE_OPEN,
                     ChromeAsyncSocket::ERROR_WRONGSTATE);

    DoCloseOpened(
        SignalSocketState(SIGNAL_CLOSE,
                          ChromeAsyncSocket::STATE_CLOSED,
                          ChromeAsyncSocket::ERROR_WRONGSTATE,
                          net::OK));
  }, "non-closed socket");
}

TEST_F(ChromeAsyncSocketTest, ImmediateConnectCloseBeforeRead) {
  DoOpenClosed();

  EXPECT_TRUE(chrome_async_socket_->Close());
  ExpectClosed();
  ExpectSignalSocketState(
      SignalSocketState::NoError(
          SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED));

  base::RunLoop().RunUntilIdle();
}

TEST_F(ChromeAsyncSocketTest, HangingConnect) {
  EXPECT_TRUE(chrome_async_socket_->Connect(addr_));
  ExpectNonErrorState(ChromeAsyncSocket::STATE_CONNECTING);
  ExpectNoSignal();

  EXPECT_TRUE(chrome_async_socket_->Close());
  ExpectClosed();
  ExpectSignalSocketState(
      SignalSocketState::NoError(
          SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED));
}

TEST_F(ChromeAsyncSocketTest, PendingConnect) {
  async_socket_data_provider_.set_connect_data(
      net::MockConnect(net::ASYNC, net::OK));
  EXPECT_TRUE(chrome_async_socket_->Connect(addr_));
  ExpectNonErrorState(ChromeAsyncSocket::STATE_CONNECTING);
  ExpectNoSignal();

  base::RunLoop().RunUntilIdle();
  ExpectNonErrorState(ChromeAsyncSocket::STATE_OPEN);
  ExpectSignalSocketState(
      SignalSocketState::NoError(
          SIGNAL_CONNECT, ChromeAsyncSocket::STATE_OPEN));
  ExpectNoSignal();

  base::RunLoop().RunUntilIdle();

  DoCloseOpenedNoError();
}

// After this no need to test successful pending connect so
// thoroughly.

TEST_F(ChromeAsyncSocketTest, PendingConnectCloseBeforeRead) {
  async_socket_data_provider_.set_connect_data(
      net::MockConnect(net::ASYNC, net::OK));
  EXPECT_TRUE(chrome_async_socket_->Connect(addr_));

  base::RunLoop().RunUntilIdle();
  ExpectSignalSocketState(
      SignalSocketState::NoError(
          SIGNAL_CONNECT, ChromeAsyncSocket::STATE_OPEN));

  DoCloseOpenedNoError();

  base::RunLoop().RunUntilIdle();
}

TEST_F(ChromeAsyncSocketTest, PendingConnectError) {
  async_socket_data_provider_.set_connect_data(
      net::MockConnect(net::ASYNC, net::ERR_TIMED_OUT));
  EXPECT_TRUE(chrome_async_socket_->Connect(addr_));

  base::RunLoop().RunUntilIdle();

  ExpectSignalSocketState(
      SignalSocketState(
          SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED,
          ChromeAsyncSocket::ERROR_WINSOCK, net::ERR_TIMED_OUT));
}

// After this we can assume Connect() and Close() work as expected.

TEST_F(ChromeAsyncSocketTest, EmptyRead) {
  DoOpenClosed();

  char buf[4096];
  size_t len_read = 10000U;
  EXPECT_TRUE(chrome_async_socket_->Read(buf, sizeof(buf), &len_read));
  EXPECT_EQ(0U, len_read);

  DoCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, WrongRead) {
  EXPECT_DEBUG_DEATH({
    async_socket_data_provider_.set_connect_data(
        net::MockConnect(net::ASYNC, net::OK));
    EXPECT_TRUE(chrome_async_socket_->Connect(addr_));
    ExpectNonErrorState(ChromeAsyncSocket::STATE_CONNECTING);
    ExpectNoSignal();

    char buf[4096];
    size_t len_read;
    EXPECT_FALSE(chrome_async_socket_->Read(buf, sizeof(buf), &len_read));
    ExpectErrorState(ChromeAsyncSocket::STATE_CONNECTING,
                     ChromeAsyncSocket::ERROR_WRONGSTATE);
    EXPECT_TRUE(chrome_async_socket_->Close());
    ExpectSignalSocketState(
        SignalSocketState(
            SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED,
            ChromeAsyncSocket::ERROR_WRONGSTATE, net::OK));
  }, "non-open");
}

TEST_F(ChromeAsyncSocketTest, WrongReadClosed) {
  char buf[4096];
  size_t len_read;
  EXPECT_FALSE(chrome_async_socket_->Read(buf, sizeof(buf), &len_read));
  ExpectErrorState(ChromeAsyncSocket::STATE_CLOSED,
                   ChromeAsyncSocket::ERROR_WRONGSTATE);
  EXPECT_TRUE(chrome_async_socket_->Close());
}

const char kReadData[] = "mydatatoread";

TEST_F(ChromeAsyncSocketTest, Read) {
  async_socket_data_provider_.AddRead(net::MockRead(kReadData));
  DoOpenClosed();

  ExpectReadSignal();
  ExpectNoSignal();

  EXPECT_EQ(kReadData, DrainRead(1));

  base::RunLoop().RunUntilIdle();

  DoCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, ReadTwice) {
  async_socket_data_provider_.AddRead(net::MockRead(kReadData));
  DoOpenClosed();

  ExpectReadSignal();
  ExpectNoSignal();

  EXPECT_EQ(kReadData, DrainRead(1));

  base::RunLoop().RunUntilIdle();

  const char kReadData2[] = "mydatatoread2";
  async_socket_data_provider_.AddRead(net::MockRead(kReadData2));

  ExpectReadSignal();
  ExpectNoSignal();

  EXPECT_EQ(kReadData2, DrainRead(1));

  DoCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, ReadError) {
  async_socket_data_provider_.AddRead(net::MockRead(kReadData));
  DoOpenClosed();

  ExpectReadSignal();
  ExpectNoSignal();

  EXPECT_EQ(kReadData, DrainRead(1));

  base::RunLoop().RunUntilIdle();

  async_socket_data_provider_.AddRead(
      net::MockRead(net::SYNCHRONOUS, net::ERR_TIMED_OUT));

  ExpectSignalSocketState(
      SignalSocketState(
          SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED,
          ChromeAsyncSocket::ERROR_WINSOCK, net::ERR_TIMED_OUT));
}

TEST_F(ChromeAsyncSocketTest, ReadEmpty) {
  async_socket_data_provider_.AddRead(net::MockRead(""));
  DoOpenClosed();

  ExpectSignalSocketState(
      SignalSocketState::NoError(
          SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED));
}

TEST_F(ChromeAsyncSocketTest, PendingRead) {
  DoOpenClosed();

  ExpectNoSignal();

  async_socket_data_provider_.AddRead(net::MockRead(kReadData));

  ExpectSignalSocketState(
      SignalSocketState::NoError(
          SIGNAL_READ, ChromeAsyncSocket::STATE_OPEN));
  ExpectNoSignal();

  EXPECT_EQ(kReadData, DrainRead(1));

  base::RunLoop().RunUntilIdle();

  DoCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, PendingEmptyRead) {
  DoOpenClosed();

  ExpectNoSignal();

  async_socket_data_provider_.AddRead(net::MockRead(""));

  ExpectSignalSocketState(
      SignalSocketState::NoError(
          SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED));
}

TEST_F(ChromeAsyncSocketTest, PendingReadError) {
  DoOpenClosed();

  ExpectNoSignal();

  async_socket_data_provider_.AddRead(
      net::MockRead(net::ASYNC, net::ERR_TIMED_OUT));

  ExpectSignalSocketState(
      SignalSocketState(
          SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED,
          ChromeAsyncSocket::ERROR_WINSOCK, net::ERR_TIMED_OUT));
}

// After this we can assume non-SSL Read() works as expected.

TEST_F(ChromeAsyncSocketTest, WrongWrite) {
  EXPECT_DEBUG_DEATH({
    std::string data("foo");
    EXPECT_FALSE(chrome_async_socket_->Write(data.data(), data.size()));
    ExpectErrorState(ChromeAsyncSocket::STATE_CLOSED,
                     ChromeAsyncSocket::ERROR_WRONGSTATE);
    EXPECT_TRUE(chrome_async_socket_->Close());
  }, "non-open");
}

const char kWriteData[] = "mydatatowrite";

TEST_F(ChromeAsyncSocketTest, SyncWrite) {
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::SYNCHRONOUS, kWriteData, 3));
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::SYNCHRONOUS, kWriteData + 3, 5));
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::SYNCHRONOUS,
                     kWriteData + 8, arraysize(kWriteData) - 8));
  DoOpenClosed();

  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData, 3));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData + 3, 5));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData + 8,
                                          arraysize(kWriteData) - 8));
  base::RunLoop().RunUntilIdle();

  ExpectNoSignal();

  DoCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, AsyncWrite) {
  DoOpenClosed();

  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::ASYNC, kWriteData, 3));
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::ASYNC, kWriteData + 3, 5));
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::ASYNC, kWriteData + 8, arraysize(kWriteData) - 8));

  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData, 3));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData + 3, 5));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData + 8,
                                          arraysize(kWriteData) - 8));
  base::RunLoop().RunUntilIdle();

  ExpectNoSignal();

  DoCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, AsyncWriteError) {
  DoOpenClosed();

  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::ASYNC, kWriteData, 3));
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::ASYNC, kWriteData + 3, 5));
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::ASYNC, net::ERR_TIMED_OUT));

  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData, 3));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData + 3, 5));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData + 8,
                                          arraysize(kWriteData) - 8));
  base::RunLoop().RunUntilIdle();

  ExpectSignalSocketState(
      SignalSocketState(
          SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED,
          ChromeAsyncSocket::ERROR_WINSOCK, net::ERR_TIMED_OUT));
}

TEST_F(ChromeAsyncSocketTest, LargeWrite) {
  EXPECT_DEBUG_DEATH({
    DoOpenClosed();

    std::string large_data(100, 'x');
    EXPECT_FALSE(chrome_async_socket_->Write(large_data.data(),
                                             large_data.size()));
    ExpectState(ChromeAsyncSocket::STATE_OPEN,
                ChromeAsyncSocket::ERROR_WINSOCK,
                net::ERR_INSUFFICIENT_RESOURCES);
    DoCloseOpened(
        SignalSocketState(
            SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED,
            ChromeAsyncSocket::ERROR_WINSOCK,
            net::ERR_INSUFFICIENT_RESOURCES));
    }, "exceed the max write buffer");
}

TEST_F(ChromeAsyncSocketTest, LargeAccumulatedWrite) {
  EXPECT_DEBUG_DEATH({
    DoOpenClosed();

    std::string data(15, 'x');
    EXPECT_TRUE(chrome_async_socket_->Write(data.data(), data.size()));
    EXPECT_FALSE(chrome_async_socket_->Write(data.data(), data.size()));
    ExpectState(ChromeAsyncSocket::STATE_OPEN,
                ChromeAsyncSocket::ERROR_WINSOCK,
                net::ERR_INSUFFICIENT_RESOURCES);
    DoCloseOpened(
        SignalSocketState(
            SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED,
            ChromeAsyncSocket::ERROR_WINSOCK,
            net::ERR_INSUFFICIENT_RESOURCES));
    }, "exceed the max write buffer");
}

// After this we can assume non-SSL I/O works as expected.

TEST_F(ChromeAsyncSocketTest, HangingSSLConnect) {
  async_socket_data_provider_.AddRead(net::MockRead(kReadData));
  DoOpenClosed();
  ExpectReadSignal();

  EXPECT_TRUE(chrome_async_socket_->StartTls("fakedomain.com"));
  ExpectNoSignal();

  ExpectNonErrorState(ChromeAsyncSocket::STATE_TLS_CONNECTING);
  EXPECT_TRUE(chrome_async_socket_->Close());
  ExpectSignalSocketState(
      SignalSocketState::NoError(SIGNAL_CLOSE,
                                 ChromeAsyncSocket::STATE_CLOSED));
  ExpectNonErrorState(ChromeAsyncSocket::STATE_CLOSED);
}

TEST_F(ChromeAsyncSocketTest, ImmediateSSLConnect) {
  async_socket_data_provider_.AddRead(net::MockRead(kReadData));
  DoOpenClosed();
  ExpectReadSignal();

  EXPECT_TRUE(chrome_async_socket_->StartTls("fakedomain.com"));
  base::RunLoop().RunUntilIdle();
  ExpectSSLConnectSignal();
  ExpectNoSignal();
  ExpectNonErrorState(ChromeAsyncSocket::STATE_TLS_OPEN);

  DoSSLCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, DoubleSSLConnect) {
  EXPECT_DEBUG_DEATH({
    async_socket_data_provider_.AddRead(net::MockRead(kReadData));
    DoOpenClosed();
    ExpectReadSignal();

    EXPECT_TRUE(chrome_async_socket_->StartTls("fakedomain.com"));
    base::RunLoop().RunUntilIdle();
    ExpectSSLConnectSignal();
    ExpectNoSignal();
    ExpectNonErrorState(ChromeAsyncSocket::STATE_TLS_OPEN);

    EXPECT_FALSE(chrome_async_socket_->StartTls("fakedomain.com"));

    DoSSLCloseOpened(
        SignalSocketState(SIGNAL_CLOSE,
                          ChromeAsyncSocket::STATE_CLOSED,
                          ChromeAsyncSocket::ERROR_WRONGSTATE,
                          net::OK));
  }, "wrong state");
}

TEST_F(ChromeAsyncSocketTest, FailedSSLConnect) {
  ssl_socket_data_provider_.connect =
      net::MockConnect(net::ASYNC, net::ERR_CERT_COMMON_NAME_INVALID),

  async_socket_data_provider_.AddRead(net::MockRead(kReadData));
  DoOpenClosed();
  ExpectReadSignal();

  EXPECT_TRUE(chrome_async_socket_->StartTls("fakedomain.com"));
  base::RunLoop().RunUntilIdle();
  ExpectSignalSocketState(
      SignalSocketState(
          SIGNAL_CLOSE, ChromeAsyncSocket::STATE_CLOSED,
          ChromeAsyncSocket::ERROR_WINSOCK,
          net::ERR_CERT_COMMON_NAME_INVALID));

  EXPECT_TRUE(chrome_async_socket_->Close());
  ExpectClosed();
}

TEST_F(ChromeAsyncSocketTest, ReadDuringSSLConnecting) {
  async_socket_data_provider_.AddRead(net::MockRead(kReadData));
  DoOpenClosed();
  ExpectReadSignal();
  EXPECT_EQ(kReadData, DrainRead(1));

  EXPECT_TRUE(chrome_async_socket_->StartTls("fakedomain.com"));
  ExpectNoSignal();

  // Shouldn't do anything.
  async_socket_data_provider_.AddRead(net::MockRead(kReadData));

  char buf[4096];
  size_t len_read = 10000U;
  EXPECT_TRUE(chrome_async_socket_->Read(buf, sizeof(buf), &len_read));
  EXPECT_EQ(0U, len_read);

  base::RunLoop().RunUntilIdle();
  ExpectSSLConnectSignal();
  ExpectSSLReadSignal();
  ExpectNoSignal();
  ExpectNonErrorState(ChromeAsyncSocket::STATE_TLS_OPEN);

  len_read = 10000U;
  EXPECT_TRUE(chrome_async_socket_->Read(buf, sizeof(buf), &len_read));
  EXPECT_EQ(kReadData, std::string(buf, len_read));

  DoSSLCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, WriteDuringSSLConnecting) {
  async_socket_data_provider_.AddRead(net::MockRead(kReadData));
  DoOpenClosed();
  ExpectReadSignal();

  EXPECT_TRUE(chrome_async_socket_->StartTls("fakedomain.com"));
  ExpectNoSignal();
  ExpectNonErrorState(ChromeAsyncSocket::STATE_TLS_CONNECTING);

  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::ASYNC, kWriteData, 3));

  // Shouldn't do anything.
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData, 3));

  // TODO(akalin): Figure out how to test that the write happens
  // *after* the SSL connect.

  base::RunLoop().RunUntilIdle();
  ExpectSSLConnectSignal();
  ExpectNoSignal();

  base::RunLoop().RunUntilIdle();

  DoSSLCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, SSLConnectDuringPendingRead) {
  EXPECT_DEBUG_DEATH({
    DoOpenClosed();

    EXPECT_FALSE(chrome_async_socket_->StartTls("fakedomain.com"));

    DoCloseOpened(
        SignalSocketState(SIGNAL_CLOSE,
                          ChromeAsyncSocket::STATE_CLOSED,
                          ChromeAsyncSocket::ERROR_WRONGSTATE,
                          net::OK));
  }, "wrong state");
}

TEST_F(ChromeAsyncSocketTest, SSLConnectDuringPostedWrite) {
  EXPECT_DEBUG_DEATH({
    DoOpenClosed();

    async_socket_data_provider_.AddWrite(
        net::MockWrite(net::ASYNC, kWriteData, 3));
    EXPECT_TRUE(chrome_async_socket_->Write(kWriteData, 3));

    EXPECT_FALSE(chrome_async_socket_->StartTls("fakedomain.com"));

    base::RunLoop().RunUntilIdle();

    DoCloseOpened(
        SignalSocketState(SIGNAL_CLOSE,
                          ChromeAsyncSocket::STATE_CLOSED,
                          ChromeAsyncSocket::ERROR_WRONGSTATE,
                          net::OK));
  }, "wrong state");
}

// After this we can assume SSL connect works as expected.

TEST_F(ChromeAsyncSocketTest, SSLRead) {
  DoSSLOpenClosed();
  async_socket_data_provider_.AddRead(net::MockRead(kReadData));
  base::RunLoop().RunUntilIdle();

  ExpectSSLReadSignal();
  ExpectNoSignal();

  EXPECT_EQ(kReadData, DrainRead(1));

  base::RunLoop().RunUntilIdle();

  DoSSLCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, SSLSyncWrite) {
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::SYNCHRONOUS, kWriteData, 3));
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::SYNCHRONOUS, kWriteData + 3, 5));
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::SYNCHRONOUS,
                     kWriteData + 8, arraysize(kWriteData) - 8));
  DoSSLOpenClosed();

  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData, 3));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData + 3, 5));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData + 8,
                                          arraysize(kWriteData) - 8));
  base::RunLoop().RunUntilIdle();

  ExpectNoSignal();

  DoSSLCloseOpenedNoError();
}

TEST_F(ChromeAsyncSocketTest, SSLAsyncWrite) {
  DoSSLOpenClosed();

  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::ASYNC, kWriteData, 3));
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::ASYNC, kWriteData + 3, 5));
  async_socket_data_provider_.AddWrite(
      net::MockWrite(net::ASYNC, kWriteData + 8, arraysize(kWriteData) - 8));

  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData, 3));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData + 3, 5));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(chrome_async_socket_->Write(kWriteData + 8,
                                          arraysize(kWriteData) - 8));
  base::RunLoop().RunUntilIdle();

  ExpectNoSignal();

  DoSSLCloseOpenedNoError();
}

}  // namespace

}  // namespace jingle_glue
