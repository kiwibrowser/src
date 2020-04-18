// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/xmpp_login_handler.h"

#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/logging.h"
#include "remoting/signaling/xmpp_stream_parser.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"

// Undefine SendMessage and ERROR defined in Windows headers.
#ifdef SendMessage
#undef SendMessage
#endif

#ifdef ERROR
#undef ERROR
#endif

namespace remoting {

const char kOAuthMechanism[] = "X-OAUTH2";

buzz::StaticQName kXmppIqName = {"jabber:client", "iq"};

char kXmppBindNs[] = "urn:ietf:params:xml:ns:xmpp-bind";
buzz::StaticQName kXmppBindName = {kXmppBindNs, "bind"};
buzz::StaticQName kXmppJidName = {kXmppBindNs, "jid"};

buzz::StaticQName kJabberFeaturesName = {"http://etherx.jabber.org/streams",
                                         "features"};

char kXmppTlsNs[] = "urn:ietf:params:xml:ns:xmpp-tls";
buzz::StaticQName kStartTlsName = {kXmppTlsNs, "starttls"};
buzz::StaticQName kTlsProceedName = {kXmppTlsNs, "proceed"};

char kXmppSaslNs[] = "urn:ietf:params:xml:ns:xmpp-sasl";
buzz::StaticQName kSaslMechanismsName = {kXmppSaslNs, "mechanisms"};
buzz::StaticQName kSaslMechanismName = {kXmppSaslNs, "mechanism"};
buzz::StaticQName kSaslSuccessName = {kXmppSaslNs, "success"};

XmppLoginHandler::XmppLoginHandler(const std::string& server,
                                   const std::string& username,
                                   const std::string& auth_token,
                                   TlsMode tls_mode,
                                   Delegate* delegate)
    : server_(server),
      username_(username),
      auth_token_(auth_token),
      tls_mode_(tls_mode),
      delegate_(delegate),
      state_(State::INIT) {
}

XmppLoginHandler::~XmppLoginHandler() = default;

void XmppLoginHandler::Start() {
  switch (tls_mode_) {
    case TlsMode::NO_TLS:
      state_ = State::WAIT_STREAM_HEADER_AFTER_TLS;
      StartAuthHandshake();
      break;
    case TlsMode::WITH_HANDSHAKE:
      state_ = State::WAIT_STREAM_HEADER;
      StartStream("<starttls xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\"/>");
      break;
    case TlsMode::WITHOUT_HANDSHAKE:
      // If <starttls> handshake is not required then start TLS right away.
      state_ = State::STARTING_TLS;
      delegate_->StartTls();
      break;
  }
}

void XmppLoginHandler::OnDataReceived(const std::string& data) {
  DCHECK(state_ != State::INIT && state_ != State::DONE &&
         state_ != State::ERROR);
  stream_parser_->AppendData(data);
}

void XmppLoginHandler::OnStanza(std::unique_ptr<buzz::XmlElement> stanza) {
  switch (state_) {
    case State::WAIT_STREAM_HEADER: {
      if (stanza->Name() == kJabberFeaturesName &&
          stanza->FirstNamed(kStartTlsName) != nullptr) {
        state_ = State::WAIT_STARTTLS_RESPONSE;
      } else {
        LOG(ERROR) << "Server doesn't support TLS.";
        OnError(SignalStrategy::PROTOCOL_ERROR);
      }
      break;
    }

    case State::WAIT_STARTTLS_RESPONSE: {
      if (stanza->Name() == kTlsProceedName) {
        state_ = State::STARTING_TLS;
        delegate_->StartTls();
      } else {
        LOG(ERROR) << "Failed to start TLS: " << stanza->Str();
        OnError(SignalStrategy::PROTOCOL_ERROR);
      }
      break;
    }

    case State::WAIT_STREAM_HEADER_AFTER_TLS: {
      buzz::XmlElement* mechanisms_element =
          stanza->FirstNamed(kSaslMechanismsName);
      bool oauth_supported = false;
      if (mechanisms_element) {
        for (buzz::XmlElement* element =
                 mechanisms_element->FirstNamed(kSaslMechanismName);
             element; element = element->NextNamed(kSaslMechanismName)) {
          if (element->BodyText() == kOAuthMechanism) {
            oauth_supported = true;
            break;
          }
        }
      }

      if (!oauth_supported) {
        LOG(ERROR) << kOAuthMechanism
                   << " auth mechanism is not supported by the server.";
        OnError(SignalStrategy::PROTOCOL_ERROR);
        return;
      }

      state_ = State::WAIT_AUTH_RESULT;
      break;
    }

    case State::WAIT_AUTH_RESULT: {
      if (stanza->Name() == kSaslSuccessName) {
        state_ = State::WAIT_STREAM_HEADER_AFTER_AUTH;
        StartStream(
            "<iq type=\"set\" id=\"0\">"
              "<bind xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\">"
                "<resource>chromoting</resource>"
              "</bind>"
            "</iq>"
            "<iq type=\"set\" id=\"1\">"
              "<session xmlns=\"urn:ietf:params:xml:ns:xmpp-session\"/>"
            "</iq>");
      } else {
        OnError(SignalStrategy::AUTHENTICATION_FAILED);
      }
      break;
    }

    case State::WAIT_STREAM_HEADER_AFTER_AUTH:
      if (stanza->Name() == kJabberFeaturesName &&
          stanza->FirstNamed(kXmppBindName) != nullptr) {
        state_ = State::WAIT_BIND_RESULT;
      } else {
        LOG(ERROR) << "Server doesn't support bind after authentication.";
        OnError(SignalStrategy::PROTOCOL_ERROR);
      }
      break;

    case State::WAIT_BIND_RESULT: {
      buzz::XmlElement* bind = stanza->FirstNamed(kXmppBindName);
      buzz::XmlElement* jid = bind ? bind->FirstNamed(kXmppJidName) : nullptr;
      if (stanza->Attr(buzz::QName("", "id")) != "0" ||
          stanza->Attr(buzz::QName("", "type")) != "result" || !jid) {
        LOG(ERROR) << "Received unexpected response to bind: " << stanza->Str();
        OnError(SignalStrategy::PROTOCOL_ERROR);
        return;
      }
      jid_ = jid->BodyText();
      state_ = State::WAIT_SESSION_IQ_RESULT;
      break;
    }

    case State::WAIT_SESSION_IQ_RESULT:
      if (stanza->Name() != kXmppIqName ||
          stanza->Attr(buzz::QName("", "id")) != "1" ||
          stanza->Attr(buzz::QName("", "type")) != "result") {
        LOG(ERROR) << "Failed to start session: " << stanza->Str();
        OnError(SignalStrategy::PROTOCOL_ERROR);
        return;
      }
      state_ = State::DONE;
      delegate_->OnHandshakeDone(jid_, std::move(stream_parser_));
      break;

    default:
      NOTREACHED();
      break;
  }
}

void XmppLoginHandler::OnTlsStarted() {
  DCHECK(state_ == State::STARTING_TLS);
  state_ = State::WAIT_STREAM_HEADER_AFTER_TLS;
  StartAuthHandshake();
}

void XmppLoginHandler::StartAuthHandshake() {
  DCHECK(state_ == State::WAIT_STREAM_HEADER_AFTER_TLS);

  std::string cookie;
  base::Base64Encode(
      std::string("\0", 1) + username_ + std::string("\0", 1) + auth_token_,
      &cookie);
  StartStream(
      "<auth xmlns=\"" + std::string(kXmppSaslNs) + "\" "
             "mechanism=\"" +  "X-OAUTH2" + "\" "
             "auth:service=\"oauth2\" "
             "auth:allow-generated-jid=\"true\" "
             "auth:client-uses-full-bind-result=\"true\" "
             "auth:allow-non-google-login=\"true\" "
             "xmlns:auth=\"http://www.google.com/talk/protocol/auth\">" +
        cookie +
      "</auth>");
};

void XmppLoginHandler::OnParserError() {
  OnError(SignalStrategy::PROTOCOL_ERROR);
}

void XmppLoginHandler::StartStream(const std::string& first_message) {
  stream_parser_.reset(new XmppStreamParser());
  stream_parser_->SetCallbacks(
      base::Bind(&XmppLoginHandler::OnStanza, base::Unretained(this)),
      base::Bind(&XmppLoginHandler::OnParserError, base::Unretained(this)));
  delegate_->SendMessage("<stream:stream to=\"" + server_ +
                         "\" version=\"1.0\" xmlns=\"jabber:client\" "
                         "xmlns:stream=\"http://etherx.jabber.org/streams\">" +
                         first_message);
}

void XmppLoginHandler::OnError(SignalStrategy::Error error) {
  if (state_ != State::ERROR) {
    state_ = State::ERROR;
    delegate_->OnLoginHandlerError(error);
  }
}

}  // namespace remoting
