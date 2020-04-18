// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/xmpp_signal_strategy.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "net/socket/socket_test_util.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"

namespace remoting {

namespace {

class XmppSocketDataProvider : public net::SocketDataProvider {
 public:
  net::MockRead OnRead() override {
    return net::MockRead(net::ASYNC, net::ERR_IO_PENDING);
  }

  net::MockWriteResult OnWrite(const std::string& data) override {
    if (write_error_ != net::OK)
      return net::MockWriteResult(net::SYNCHRONOUS, write_error_);

    written_data_.append(data);

    if (use_async_write_) {
      pending_write_size_ = data.size();
      return net::MockWriteResult(net::ASYNC, net::ERR_IO_PENDING);
    }

    return net::MockWriteResult(net::SYNCHRONOUS, data.size());
  }

  void Reset() override {}

  bool AllReadDataConsumed() const override {
    return true;
  }

  bool AllWriteDataConsumed() const override {
    return true;
  }

  void ReceiveData(const std::string& text) {
    socket()->OnReadComplete(
        net::MockRead(net::ASYNC, text.data(), text.size()));
  }

  void Close() {
    ReceiveData(std::string());
  }

  void SimulateAsyncReadError() {
    socket()->OnReadComplete(
        net::MockRead(net::ASYNC, net::ERR_CONNECTION_RESET));
  }

  std::string GetAndClearWrittenData() {
    std::string data;
    data.swap(written_data_);
    return data;
  }

  void set_use_async_write(bool use_async_write) {
    use_async_write_ = use_async_write;
  }

  void set_write_error(net::Error error) {
    write_error_ = error;
  }

  void CompletePendingWrite() {
    socket()->OnWriteComplete(pending_write_size_);
  }

 private:
  std::string written_data_;
  bool use_async_write_ = false;
  int pending_write_size_ = 0;
  net::Error write_error_ = net::OK;
};

class MockClientSocketFactory : public net::MockClientSocketFactory {
 public:
  std::unique_ptr<net::SSLClientSocket> CreateSSLClientSocket(
      std::unique_ptr<net::ClientSocketHandle> transport_socket,
      const net::HostPortPair& host_and_port,
      const net::SSLConfig& ssl_config,
      const net::SSLClientSocketContext& context) override {
    ssl_socket_created_ = true;
    return net::MockClientSocketFactory::CreateSSLClientSocket(
        std::move(transport_socket), host_and_port, ssl_config, context);
  }

  bool ssl_socket_created() const { return ssl_socket_created_; }

 private:
  bool ssl_socket_created_ = false;
};

}  // namespace

const char kTestUsername[] = "test_username@example.com";
const char kTestAuthToken[] = "test_auth_token";
const int kDefaultPort = 443;

class XmppSignalStrategyTest : public testing::Test,
                               public SignalStrategy::Listener {
 public:
  XmppSignalStrategyTest() : message_loop_(base::MessageLoop::TYPE_IO) {}

  void SetUp() override {
    request_context_getter_ = new net::TestURLRequestContextGetter(
        message_loop_.task_runner(),
        std::make_unique<net::TestURLRequestContext>());
  }

  void CreateSignalStrategy(int port) {
    XmppSignalStrategy::XmppServerConfig config;
    config.host = "talk.google.com";
    config.port = port;
    config.username = kTestUsername;
    config.auth_token = kTestAuthToken;
    signal_strategy_.reset(new XmppSignalStrategy(
        &client_socket_factory_, request_context_getter_, config));
    signal_strategy_->AddListener(this);
  }

  void TearDown() override {
    signal_strategy_->RemoveListener(this);
    signal_strategy_.reset();
    base::RunLoop().RunUntilIdle();
  }

  void OnSignalStrategyStateChange(SignalStrategy::State state) override {
    state_history_.push_back(state);
  }

  bool OnSignalStrategyIncomingStanza(const buzz::XmlElement* stanza) override {
    received_messages_.push_back(std::make_unique<buzz::XmlElement>(*stanza));
    return true;
  }

  void Connect(bool success);

 protected:
  base::MessageLoop message_loop_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  MockClientSocketFactory client_socket_factory_;
  std::unique_ptr<XmppSocketDataProvider> socket_data_provider_;
  std::unique_ptr<net::SSLSocketDataProvider> ssl_socket_data_provider_;
  std::unique_ptr<XmppSignalStrategy> signal_strategy_;

  std::vector<SignalStrategy::State> state_history_;
  std::vector<std::unique_ptr<buzz::XmlElement>> received_messages_;
};

void XmppSignalStrategyTest::Connect(bool success) {
  EXPECT_EQ(SignalStrategy::DISCONNECTED, signal_strategy_->GetState());
  state_history_.clear();

  socket_data_provider_.reset(new XmppSocketDataProvider());
  socket_data_provider_->set_connect_data(
      net::MockConnect(net::ASYNC, net::OK));
  client_socket_factory_.AddSocketDataProvider(socket_data_provider_.get());

  ssl_socket_data_provider_.reset(
      new net::SSLSocketDataProvider(net::ASYNC, net::OK));
  client_socket_factory_.AddSSLSocketDataProvider(
      ssl_socket_data_provider_.get());

  signal_strategy_->Connect();

  EXPECT_EQ(SignalStrategy::CONNECTING, signal_strategy_->GetState());
  EXPECT_EQ(1U, state_history_.size());
  EXPECT_EQ(SignalStrategy::CONNECTING, state_history_[0]);

  // No data written before TLS.
  EXPECT_EQ("", socket_data_provider_->GetAndClearWrittenData());

  base::RunLoop().RunUntilIdle();

  socket_data_provider_->ReceiveData(
      "<stream:stream from=\"google.com\" id=\"DCDDE5171CB2154A\" "
        "version=\"1.0\" "
        "xmlns:stream=\"http://etherx.jabber.org/streams\" "
        "xmlns=\"jabber:client\">"
      "<stream:features>"
        "<mechanisms xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\">"
          "<mechanism>X-OAUTH2</mechanism>"
          "<mechanism>X-GOOGLE-TOKEN</mechanism>"
          "<mechanism>PLAIN</mechanism>"
        "</mechanisms>"
      "</stream:features>");

  base::RunLoop().RunUntilIdle();

  std::string cookie;
  base::Base64Encode(std::string("\0", 1) + kTestUsername +
                         std::string("\0", 1) + kTestAuthToken,
                     &cookie);
  // Expect auth message.
  EXPECT_EQ(
      "<stream:stream to=\"google.com\" version=\"1.0\" "
          "xmlns=\"jabber:client\" "
          "xmlns:stream=\"http://etherx.jabber.org/streams\">"
      "<auth xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" mechanism=\"X-OAUTH2\" "
          "auth:service=\"oauth2\" auth:allow-generated-jid=\"true\" "
          "auth:client-uses-full-bind-result=\"true\" "
          "auth:allow-non-google-login=\"true\" "
          "xmlns:auth=\"http://www.google.com/talk/protocol/auth\">" + cookie +
      "</auth>", socket_data_provider_->GetAndClearWrittenData());

  if (!success) {
    socket_data_provider_->ReceiveData(
        "<failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\">"
        "<not-authorized/></failure>");
    EXPECT_EQ(2U, state_history_.size());
    EXPECT_EQ(SignalStrategy::DISCONNECTED, state_history_[1]);
    EXPECT_EQ(SignalStrategy::AUTHENTICATION_FAILED,
              signal_strategy_->GetError());
    return;
  }

  socket_data_provider_->ReceiveData(
      "<success xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"/>");

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(
      "<stream:stream to=\"google.com\" version=\"1.0\" "
        "xmlns=\"jabber:client\" "
        "xmlns:stream=\"http://etherx.jabber.org/streams\">"
      "<iq type=\"set\" id=\"0\">"
        "<bind xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\">"
          "<resource>chromoting</resource>"
        "</bind>"
      "</iq>"
      "<iq type=\"set\" id=\"1\">"
        "<session xmlns=\"urn:ietf:params:xml:ns:xmpp-session\"/>"
      "</iq>",
      socket_data_provider_->GetAndClearWrittenData());
  socket_data_provider_->ReceiveData(
      "<stream:stream from=\"google.com\" id=\"104FA10576E2AA80\" "
        "version=\"1.0\" "
        "xmlns:stream=\"http://etherx.jabber.org/streams\" "
        "xmlns=\"jabber:client\">"
      "<stream:features>"
        "<bind xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\"/>"
        "<session xmlns=\"urn:ietf:params:xml:ns:xmpp-session\"/>"
      "</stream:features>"
      "<iq id=\"0\" type=\"result\">"
        "<bind xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\">"
          "<jid>" + std::string(kTestUsername) + "/chromoting52B4920E</jid>"
        "</bind>"
      "</iq>"
      "<iq type=\"result\" id=\"1\"/>");

  EXPECT_EQ(2U, state_history_.size());
  EXPECT_EQ(SignalStrategy::CONNECTED, state_history_[1]);
}

TEST_F(XmppSignalStrategyTest, SendAndReceive) {
  CreateSignalStrategy(kDefaultPort);
  Connect(true);

  EXPECT_TRUE(signal_strategy_->SendStanza(
      std::make_unique<buzz::XmlElement>(buzz::QName(std::string(), "hello"))));
  EXPECT_EQ("<hello/>", socket_data_provider_->GetAndClearWrittenData());

  socket_data_provider_->ReceiveData("<hi xmlns=\"hello\"/>");
  EXPECT_EQ(1U, received_messages_.size());
  EXPECT_EQ("<hi xmlns=\"hello\"/>", received_messages_[0]->Str());
}

TEST_F(XmppSignalStrategyTest, AuthError) {
  CreateSignalStrategy(kDefaultPort);
  Connect(false);
}

TEST_F(XmppSignalStrategyTest, ConnectionClosed) {
  CreateSignalStrategy(kDefaultPort);
  Connect(true);

  socket_data_provider_->Close();

  EXPECT_EQ(3U, state_history_.size());
  EXPECT_EQ(SignalStrategy::DISCONNECTED, state_history_[2]);
  EXPECT_EQ(SignalStrategy::DISCONNECTED, signal_strategy_->GetState());
  EXPECT_EQ(SignalStrategy::OK, signal_strategy_->GetError());

  // Can't send messages anymore.
  EXPECT_FALSE(signal_strategy_->SendStanza(
      std::make_unique<buzz::XmlElement>(buzz::QName(std::string(), "hello"))));

  // Try connecting again.
  Connect(true);
}

TEST_F(XmppSignalStrategyTest, NetworkReadError) {
  CreateSignalStrategy(kDefaultPort);
  Connect(true);

  socket_data_provider_->SimulateAsyncReadError();

  EXPECT_EQ(3U, state_history_.size());
  EXPECT_EQ(SignalStrategy::DISCONNECTED, state_history_[2]);
  EXPECT_EQ(SignalStrategy::NETWORK_ERROR, signal_strategy_->GetError());

  // Can't send messages anymore.
  EXPECT_FALSE(signal_strategy_->SendStanza(
      std::make_unique<buzz::XmlElement>(buzz::QName(std::string(), "hello"))));

  // Try connecting again.
  Connect(true);
}

TEST_F(XmppSignalStrategyTest, NetworkWriteError) {
  CreateSignalStrategy(kDefaultPort);
  Connect(true);

  socket_data_provider_->set_write_error(net::ERR_FAILED);

  // Next SendMessage() will call Write() which will fail.
  EXPECT_FALSE(signal_strategy_->SendStanza(
      std::make_unique<buzz::XmlElement>(buzz::QName(std::string(), "hello"))));

  EXPECT_EQ(3U, state_history_.size());
  EXPECT_EQ(SignalStrategy::DISCONNECTED, state_history_[2]);
  EXPECT_EQ(SignalStrategy::NETWORK_ERROR, signal_strategy_->GetError());

  // Try connecting again.
  Connect(true);
}

TEST_F(XmppSignalStrategyTest, StartTlsWithPendingWrite) {
  // Use port 5222 so that XmppLoginHandler uses starttls/proceed handshake
  // before starting TLS.
  CreateSignalStrategy(5222);

  socket_data_provider_.reset(new XmppSocketDataProvider());
  socket_data_provider_->set_connect_data(
      net::MockConnect(net::SYNCHRONOUS, net::OK));
  client_socket_factory_.AddSocketDataProvider(socket_data_provider_.get());

  ssl_socket_data_provider_.reset(
      new net::SSLSocketDataProvider(net::ASYNC, net::OK));
  client_socket_factory_.AddSSLSocketDataProvider(
      ssl_socket_data_provider_.get());

  // Make sure write is handled asynchronously.
  socket_data_provider_->set_use_async_write(true);

  signal_strategy_->Connect();
  base::RunLoop().RunUntilIdle();

  socket_data_provider_->ReceiveData(
      "<stream:stream from=\"google.com\" id=\"104FA10576E2AA80\" "
          "version=\"1.0\" "
          "xmlns:stream=\"http://etherx.jabber.org/streams\" "
          "xmlns=\"jabber:client\">"
        "<stream:features>"
          "<starttls xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\"/>"
        "</stream:features>"
        "<proceed xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\"/>");

  // Verify that SSL is connected only after write is finished.
  EXPECT_FALSE(client_socket_factory_.ssl_socket_created());
  socket_data_provider_->CompletePendingWrite();
  EXPECT_TRUE(client_socket_factory_.ssl_socket_created());
}


  }  // namespace remoting
