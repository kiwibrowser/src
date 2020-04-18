// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_SIGNALING_XMPP_LOGIN_HANDLER_H_
#define REMOTING_SIGNALING_XMPP_LOGIN_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "remoting/signaling/signal_strategy.h"

// Undefine SendMessage and ERROR defined in Windows headers.
#ifdef SendMessage
#undef SendMessage
#endif

#ifdef ERROR
#undef ERROR
#endif

namespace remoting {

class XmppStreamParser;

// XmppLoginHandler handles authentication handshake for XmppSignalStrategy. It
// receives incoming data using onDataReceived(), calls Delegate::SendMessage()
// to send outgoing messages and calls Delegate::OnHandshakeDone() after
// authentication is finished successfully or Delegate::OnError() on error.
//
// See RFC3920 for description of XMPP and authentication handshake.
class XmppLoginHandler {
 public:
  class Delegate {
   public:
    Delegate() {}

    // All Delegate methods are allowed to destroy XmppLoginHandler.
    virtual void SendMessage(const std::string& message) = 0;
    virtual void StartTls() = 0;
    virtual void OnHandshakeDone(const std::string& jid,
                                 std::unique_ptr<XmppStreamParser> parser) = 0;
    virtual void OnLoginHandlerError(SignalStrategy::Error error) = 0;

   protected:
    virtual ~Delegate() {}
  };

  enum class TlsMode {
    NO_TLS,
    WITH_HANDSHAKE,
    WITHOUT_HANDSHAKE,
  };

  XmppLoginHandler(const std::string& server,
                   const std::string& username,
                   const std::string& auth_token,
                   TlsMode tls_mode,
                   Delegate* delegate);
  ~XmppLoginHandler();

  void Start();
  void OnDataReceived(const std::string& data);
  void OnTlsStarted();

 private:
  // States the handshake goes through. States are iterated from INIT to DONE
  // sequentially, except for ERROR state which may be accepted at any point.
  //
  // Following messages are sent/received in each state:
  //    INIT
  //      client -> server: Stream header
  //      client -> server: <starttls>
  //    WAIT_STREAM_HEADER
  //      client <- server: Stream header with list of supported features which
  //          should include starttls.
  //    WAIT_STARTTLS_RESPONSE
  //      client <- server: <proceed>
  //    STARTING_TLS
  //      TLS handshake
  //      client -> server: Stream header
  //      client -> server: <auth> message with the OAuth2 token.
  //    WAIT_STREAM_HEADER_AFTER_TLS
  //      client <- server: Stream header with list of supported authentication
  //          methods which is expected to include X-OAUTH2
  //    WAIT_AUTH_RESULT
  //      client <- server: <success> or <failure>
  //      client -> server: Stream header
  //      client -> server: <bind>
  //      client -> server: <iq><session/></iq> to start the session
  //    WAIT_STREAM_HEADER_AFTER_AUTH
  //      client <- server: Stream header with list of features that should
  //         include <bind>.
  //    WAIT_BIND_RESULT
  //      client <- server: <bind> result with JID.
  //    WAIT_SESSION_IQ_RESULT
  //      client <- server: result for <iq><session/></iq>
  //    DONE
  enum class State {
    INIT,
    WAIT_STREAM_HEADER,
    WAIT_STARTTLS_RESPONSE,
    STARTING_TLS,
    WAIT_STREAM_HEADER_AFTER_TLS,
    WAIT_AUTH_RESULT,
    WAIT_STREAM_HEADER_AFTER_AUTH,
    WAIT_BIND_RESULT,
    WAIT_SESSION_IQ_RESULT,
    DONE,
    ERROR,
  };

  // Callbacks for XmppStreamParser.
  void OnStanza(std::unique_ptr<buzz::XmlElement> stanza);
  void OnParserError();

  // Starts authentication handshake in WAIT_STREAM_HEADER_AFTER_TLS state.
  void StartAuthHandshake();

  // Helper used to send stream header.
  void StartStream(const std::string& first_message);

  // Report the |error| to the delegate and changes |state_| to ERROR,
  void OnError(SignalStrategy::Error error);

  std::string server_;
  std::string username_;
  std::string auth_token_;
  TlsMode tls_mode_;
  Delegate* delegate_;

  State state_;

  std::string jid_;

  std::unique_ptr<XmppStreamParser> stream_parser_;

  DISALLOW_COPY_AND_ASSIGN(XmppLoginHandler);
};

}  // namespace remoting

#endif  // REMOTING_SIGNALING_XMPP_LOGIN_HANDLER_H_
