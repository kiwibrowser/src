// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/xmpp_login_handler.h"

#include <utility>

#include "base/base64.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "remoting/signaling/xmpp_stream_parser.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"

#ifdef SendMessage
#undef SendMessage
#endif

#ifdef ERROR
#undef ERROR
#endif

namespace remoting {

char kTestUsername[] = "testUsername@gmail.com";
char kTestToken[] = "testToken";

class XmppLoginHandlerTest : public testing::Test,
                             public XmppLoginHandler::Delegate {
 public:
  XmppLoginHandlerTest()
      : start_tls_called_(false), error_(SignalStrategy::OK) {}

  void TearDown() override {
    login_handler_.reset();
    parser_.reset();
    base::RunLoop().RunUntilIdle();
  }

  void SendMessage(const std::string& message) override {
    sent_data_ += message;
    if (delete_login_handler_from_delegate_)
      login_handler_.reset();
  }

  void StartTls() override {
    start_tls_called_ = true;
    if (delete_login_handler_from_delegate_)
      login_handler_.reset();
  }

  void OnHandshakeDone(const std::string& jid,
                       std::unique_ptr<XmppStreamParser> parser) override {
    jid_ = jid;
    parser_ = std::move(parser);
    if (delete_login_handler_from_delegate_)
      login_handler_.reset();
  }

  void OnLoginHandlerError(SignalStrategy::Error error) override {
    EXPECT_NE(error, SignalStrategy::OK);
    error_ = error;
    if (delete_login_handler_from_delegate_)
      login_handler_.reset();
  }

 protected:
  void HandshakeBase();

  base::MessageLoop message_loop_;

  std::unique_ptr<XmppLoginHandler> login_handler_;
  std::string sent_data_;
  bool start_tls_called_;
  std::string jid_;
  std::unique_ptr<XmppStreamParser> parser_;
  SignalStrategy::Error error_;
  bool delete_login_handler_from_delegate_ = false;
};

void XmppLoginHandlerTest::HandshakeBase() {
  login_handler_.reset(
      new XmppLoginHandler("google.com", kTestUsername, kTestToken,
                           XmppLoginHandler::TlsMode::WITHOUT_HANDSHAKE, this));
  login_handler_->Start();
  EXPECT_TRUE(start_tls_called_);

  login_handler_->OnTlsStarted();
  std::string cookie;
  base::Base64Encode(
      std::string("\0", 1) + kTestUsername + std::string("\0", 1) + kTestToken,
      &cookie);
  EXPECT_EQ(
      sent_data_,
      "<stream:stream to=\"google.com\" version=\"1.0\" "
          "xmlns=\"jabber:client\" "
          "xmlns:stream=\"http://etherx.jabber.org/streams\">"
      "<auth xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" mechanism=\"X-OAUTH2\" "
          "auth:service=\"oauth2\" auth:allow-generated-jid=\"true\" "
          "auth:client-uses-full-bind-result=\"true\" "
          "auth:allow-non-google-login=\"true\" "
          "xmlns:auth=\"http://www.google.com/talk/protocol/auth\">" + cookie +
      "</auth>");
  sent_data_.clear();

  login_handler_->OnDataReceived(
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
}

TEST_F(XmppLoginHandlerTest, SuccessfulAuth) {
  HandshakeBase();

  login_handler_->OnDataReceived(
      "<success xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"/>");
  EXPECT_EQ(
      sent_data_,
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
        "</iq>");
  sent_data_.clear();

  // |login_handler_| will call OnHandshakeDone() which will delete
  // |login_handler_|.
  delete_login_handler_from_delegate_ = true;

  login_handler_->OnDataReceived(
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

  EXPECT_EQ(jid_, std::string(kTestUsername) + "/chromoting52B4920E");
  EXPECT_TRUE(parser_);
  EXPECT_FALSE(login_handler_);
}

TEST_F(XmppLoginHandlerTest, StartTlsHandshake) {
  login_handler_.reset(
      new XmppLoginHandler("google.com", kTestUsername, kTestToken,
                           XmppLoginHandler::TlsMode::WITH_HANDSHAKE, this));
  login_handler_->Start();
  EXPECT_FALSE(start_tls_called_);

  EXPECT_EQ(sent_data_,
            "<stream:stream to=\"google.com\" version=\"1.0\" "
            "xmlns=\"jabber:client\" "
            "xmlns:stream=\"http://etherx.jabber.org/streams\">"
            "<starttls xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\"/>");
  sent_data_.clear();

  login_handler_->OnDataReceived(
      "<stream:stream from=\"google.com\" id=\"78A87C70559EF28A\" "
          "version=\"1.0\" "
          "xmlns:stream=\"http://etherx.jabber.org/streams\" "
          "xmlns=\"jabber:client\">"
        "<stream:features>"
          "<starttls xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\">"
            "<required/>"
          "</starttls>"
          "<mechanisms xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\">"
            "<mechanism>X-OAUTH2</mechanism>"
            "<mechanism>X-GOOGLE-TOKEN</mechanism>"
          "</mechanisms>"
        "</stream:features>");

  login_handler_->OnDataReceived(
      "<proceed xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\"/>");
  EXPECT_TRUE(start_tls_called_);
}

TEST_F(XmppLoginHandlerTest, AuthError) {
  HandshakeBase();

  login_handler_->OnDataReceived(
      "<failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\">"
      "<not-authorized/></failure>");
  EXPECT_EQ(error_, SignalStrategy::AUTHENTICATION_FAILED);
}

TEST_F(XmppLoginHandlerTest, NoTls) {
  login_handler_.reset(
      new XmppLoginHandler("google.com", kTestUsername, kTestToken,
                           XmppLoginHandler::TlsMode::NO_TLS, this));
  login_handler_->Start();

  EXPECT_FALSE(start_tls_called_);
  std::string cookie;
  base::Base64Encode(
      std::string("\0", 1) + kTestUsername + std::string("\0", 1) + kTestToken,
      &cookie);
  EXPECT_EQ(
      sent_data_,
      "<stream:stream to=\"google.com\" version=\"1.0\" "
          "xmlns=\"jabber:client\" "
          "xmlns:stream=\"http://etherx.jabber.org/streams\">"
      "<auth xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" mechanism=\"X-OAUTH2\" "
          "auth:service=\"oauth2\" auth:allow-generated-jid=\"true\" "
          "auth:client-uses-full-bind-result=\"true\" "
          "auth:allow-non-google-login=\"true\" "
          "xmlns:auth=\"http://www.google.com/talk/protocol/auth\">" + cookie +
      "</auth>");
}

TEST_F(XmppLoginHandlerTest, StreamParseError) {
  HandshakeBase();
  delete_login_handler_from_delegate_ = true;
  login_handler_->OnDataReceived("BAD DATA");
  EXPECT_EQ(error_, SignalStrategy::PROTOCOL_ERROR);
}

// Verify that LoginHandler doesn't crash when destroyed from
// Delegate::SendMessage().
TEST_F(XmppLoginHandlerTest, DeleteInSendMessage) {
  login_handler_.reset(
      new XmppLoginHandler("google.com", kTestUsername, kTestToken,
                           XmppLoginHandler::TlsMode::WITHOUT_HANDSHAKE, this));
  login_handler_->Start();
  EXPECT_TRUE(start_tls_called_);

  delete_login_handler_from_delegate_ = true;
  login_handler_->OnTlsStarted();
  EXPECT_FALSE(login_handler_);
}

// Verify that LoginHandler doesn't crash when destroyed from
// Delegate::StartTls().
TEST_F(XmppLoginHandlerTest, DeleteInStartTls) {
  login_handler_.reset(
      new XmppLoginHandler("google.com", kTestUsername, kTestToken,
                           XmppLoginHandler::TlsMode::WITHOUT_HANDSHAKE, this));
  delete_login_handler_from_delegate_ = true;
  login_handler_->Start();
  EXPECT_TRUE(start_tls_called_);
  EXPECT_FALSE(login_handler_);
}

}  // namespace remoting
