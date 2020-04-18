// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/xmpp_signal_strategy.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/rand_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_checker.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/http/transport_security_state.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_context_getter.h"
#include "remoting/base/buffered_socket_writer.h"
#include "remoting/base/logging.h"
#include "remoting/signaling/signaling_address.h"
#include "remoting/signaling/xmpp_login_handler.h"
#include "remoting/signaling/xmpp_stream_parser.h"
#include "services/network/proxy_resolving_client_socket.h"
#include "services/network/proxy_resolving_client_socket_factory.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"

// Use 50 seconds keep-alive interval, in case routers terminate
// connections that are idle for more than a minute.
const int kKeepAliveIntervalSeconds = 50;

const int kReadBufferSize = 4096;

const int kDefaultXmppPort = 5222;
const int kDefaultHttpsPort = 443;

namespace remoting {

XmppSignalStrategy::XmppServerConfig::XmppServerConfig()
    : port(kDefaultXmppPort), use_tls(true) {
}

XmppSignalStrategy::XmppServerConfig::XmppServerConfig(
    const XmppServerConfig& other) = default;

XmppSignalStrategy::XmppServerConfig::~XmppServerConfig() = default;

class XmppSignalStrategy::Core : public XmppLoginHandler::Delegate {
 public:
  Core(
      net::ClientSocketFactory* socket_factory,
      const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
      const XmppServerConfig& xmpp_server_config);
  ~Core() override;

  void Connect();
  void Disconnect();
  State GetState() const;
  Error GetError() const;
  const SignalingAddress& GetLocalAddress() const;
  void AddListener(Listener* listener);
  void RemoveListener(Listener* listener);
  bool SendStanza(std::unique_ptr<buzz::XmlElement> stanza);

  void SetAuthInfo(const std::string& username,
                   const std::string& auth_token);

 private:
  enum class TlsState {
    // StartTls() hasn't been called. |socket_| is not encrypted.
    NOT_REQUESTED,

    // StartTls() has been called. Waiting for |writer_| to finish writing
    // data before starting TLS.
    WAITING_FOR_FLUSH,

    // TLS has been started, waiting for TLS handshake to finish.
    CONNECTING,

    // TLS is connected.
    CONNECTED,
  };

  void OnSocketConnected(int result);
  void OnTlsConnected(int result);

  void ReadSocket();
  void OnReadResult(int result);
  void HandleReadResult(int result);

  // XmppLoginHandler::Delegate interface.
  void SendMessage(const std::string& message) override;
  void StartTls() override;
  void OnHandshakeDone(const std::string& jid,
                       std::unique_ptr<XmppStreamParser> parser) override;
  void OnLoginHandlerError(SignalStrategy::Error error) override;

  // Callback for BufferedSocketWriter.
  void OnMessageSent();

  // Event handlers for XmppStreamParser.
  void OnStanza(const std::unique_ptr<buzz::XmlElement> stanza);
  void OnParserError();

  void OnNetworkError(int error);

  void SendKeepAlive();

  net::ClientSocketFactory* socket_factory_;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  XmppServerConfig xmpp_server_config_;

  // Used by the |socket_|.
  std::unique_ptr<network::ProxyResolvingClientSocketFactory>
      proxy_resolving_socket_factory_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  std::unique_ptr<net::TransportSecurityState> transport_security_state_;
  std::unique_ptr<net::CTVerifier> cert_transparency_verifier_;
  std::unique_ptr<net::CTPolicyEnforcer> ct_policy_enforcer_;

  std::unique_ptr<net::StreamSocket> socket_;
  std::unique_ptr<BufferedSocketWriter> writer_;
  scoped_refptr<net::IOBuffer> read_buffer_;
  bool read_pending_ = false;

  TlsState tls_state_ = TlsState::NOT_REQUESTED;

  std::unique_ptr<XmppLoginHandler> login_handler_;
  std::unique_ptr<XmppStreamParser> stream_parser_;
  SignalingAddress local_address_;

  Error error_ = OK;

  base::ObserverList<Listener, true> listeners_;

  base::RepeatingTimer keep_alive_timer_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

XmppSignalStrategy::Core::Core(
    net::ClientSocketFactory* socket_factory,
    const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
    const XmppSignalStrategy::XmppServerConfig& xmpp_server_config)
    : socket_factory_(socket_factory),
      request_context_getter_(request_context_getter),
      xmpp_server_config_(xmpp_server_config) {
#if defined(NDEBUG)
  // Non-secure connections are allowed only for debugging.
  CHECK(xmpp_server_config_.use_tls);
#endif
  thread_checker_.DetachFromThread();
}

XmppSignalStrategy::Core::~Core() {
  Disconnect();
}

void XmppSignalStrategy::Core::Connect() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Disconnect first if we are currently connected.
  Disconnect();

  error_ = OK;

  for (auto& observer : listeners_)
    observer.OnSignalStrategyStateChange(CONNECTING);

  if (!proxy_resolving_socket_factory_) {
    proxy_resolving_socket_factory_ =
        std::make_unique<network::ProxyResolvingClientSocketFactory>(
            socket_factory_, request_context_getter_->GetURLRequestContext());
  }
  socket_ = proxy_resolving_socket_factory_->CreateSocket(
      net::SSLConfig(),
      GURL("https://" +
           net::HostPortPair(xmpp_server_config_.host, xmpp_server_config_.port)
               .ToString()),
      false /*use_tls*/);

  int result = socket_->Connect(base::Bind(
      &Core::OnSocketConnected, base::Unretained(this)));

  keep_alive_timer_.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(kKeepAliveIntervalSeconds),
      base::Bind(&Core::SendKeepAlive, base::Unretained(this)));

  if (result != net::ERR_IO_PENDING)
    OnSocketConnected(result);
}

void XmppSignalStrategy::Core::Disconnect() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (socket_) {
    login_handler_.reset();
    stream_parser_.reset();
    writer_.reset();
    socket_.reset();
    tls_state_ = TlsState::NOT_REQUESTED;
    read_pending_ = false;

    for (auto& observer : listeners_)
      observer.OnSignalStrategyStateChange(DISCONNECTED);
  }
}

SignalStrategy::State XmppSignalStrategy::Core::GetState() const {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (stream_parser_) {
    DCHECK(socket_);
    return CONNECTED;
  } else if (socket_) {
    return CONNECTING;
  } else {
    return DISCONNECTED;
  }
}

SignalStrategy::Error XmppSignalStrategy::Core::GetError() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return error_;
}

const SignalingAddress& XmppSignalStrategy::Core::GetLocalAddress() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return local_address_;
}

void XmppSignalStrategy::Core::AddListener(Listener* listener) {
  DCHECK(thread_checker_.CalledOnValidThread());
  listeners_.AddObserver(listener);
}

void XmppSignalStrategy::Core::RemoveListener(Listener* listener) {
  DCHECK(thread_checker_.CalledOnValidThread());
  listeners_.RemoveObserver(listener);
}

bool XmppSignalStrategy::Core::SendStanza(
    std::unique_ptr<buzz::XmlElement> stanza) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!stream_parser_) {
    VLOG(0) << "Dropping signalling message because XMPP is not connected.";
    return false;
  }

  HOST_LOG << "Sending outgoing stanza:\n"
           << stanza->Str()
           << "\n=========================================================";
  SendMessage(stanza->Str());

  // Return false if the SendMessage() call above resulted in the SignalStrategy
  // being disconnected.
  return stream_parser_ != nullptr;
}

void XmppSignalStrategy::Core::SetAuthInfo(const std::string& username,
                                           const std::string& auth_token) {
  DCHECK(thread_checker_.CalledOnValidThread());
  xmpp_server_config_.username = username;
  xmpp_server_config_.auth_token = auth_token;
}

void XmppSignalStrategy::Core::SendMessage(const std::string& message) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(tls_state_ == TlsState::NOT_REQUESTED ||
         tls_state_ == TlsState::CONNECTED);

  scoped_refptr<net::IOBufferWithSize> buffer =
      new net::IOBufferWithSize(message.size());
  memcpy(buffer->data(), message.data(), message.size());

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("xmpp_signal_strategy", R"(
        semantics {
          sender: "Xmpp Signal Strategy"
           description:
            "This request is used for setting up the ICE connection between "
            "the client and the host for Chrome Remote Desktop."
          trigger:
            "Initiating a Chrome Remote Desktop connection."
          data: "No user data."
          destination: OTHER
          destination_other:
            "The Chrome Remote Desktop client/host that user is connecting to."
        }
        policy {
          cookies_allowed: NO
          setting:
            "This request cannot be stopped in settings, but will not be sent "
            "if user does not use Chrome Remote Desktop."
          policy_exception_justification:
            "Not implemented. 'RemoteAccessHostClientDomainList' and "
            "'RemoteAccessHostDomainList' policies can limit the domains to "
            "which a connection can be made, but they cannot be used to block "
            "the request to all domains. Please refer to help desk for other "
            "approaches to manage this feature."
        })");
  writer_->Write(buffer,
                 base::Bind(&Core::OnMessageSent, base::Unretained(this)),
                 traffic_annotation);
}

void XmppSignalStrategy::Core::StartTls() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(login_handler_);
  DCHECK(tls_state_ == TlsState::NOT_REQUESTED ||
         tls_state_ == TlsState::WAITING_FOR_FLUSH);

  if (writer_->has_data_pending()) {
    tls_state_ = TlsState::WAITING_FOR_FLUSH;
    return;
  }

  tls_state_ = TlsState::CONNECTING;

  // Reset the writer so we don't try to write to the raw socket anymore.
  writer_.reset();

  DCHECK(!read_pending_);

  std::unique_ptr<net::ClientSocketHandle> socket_handle(
      new net::ClientSocketHandle());
  socket_handle->SetSocket(std::move(socket_));

  cert_verifier_ = net::CertVerifier::CreateDefault();
  transport_security_state_.reset(new net::TransportSecurityState());
  cert_transparency_verifier_.reset(new net::MultiLogCTVerifier());
  ct_policy_enforcer_.reset(new net::DefaultCTPolicyEnforcer());
  net::SSLClientSocketContext context;
  context.cert_verifier = cert_verifier_.get();
  context.transport_security_state = transport_security_state_.get();
  context.cert_transparency_verifier = cert_transparency_verifier_.get();
  context.ct_policy_enforcer = ct_policy_enforcer_.get();

  socket_ = socket_factory_->CreateSSLClientSocket(
      std::move(socket_handle),
      net::HostPortPair(xmpp_server_config_.host, kDefaultHttpsPort),
      net::SSLConfig(), context);

  int result = socket_->Connect(
      base::Bind(&Core::OnTlsConnected, base::Unretained(this)));
  if (result != net::ERR_IO_PENDING)
    OnTlsConnected(result);
}

void XmppSignalStrategy::Core::OnHandshakeDone(
    const std::string& jid,
    std::unique_ptr<XmppStreamParser> parser) {
  DCHECK(thread_checker_.CalledOnValidThread());

  local_address_ = SignalingAddress(jid);
  stream_parser_ = std::move(parser);
  stream_parser_->SetCallbacks(
      base::Bind(&Core::OnStanza, base::Unretained(this)),
      base::Bind(&Core::OnParserError, base::Unretained(this)));

  // Don't need |login_handler_| anymore.
  login_handler_.reset();

  for (auto& observer : listeners_)
    observer.OnSignalStrategyStateChange(CONNECTED);
}

void XmppSignalStrategy::Core::OnLoginHandlerError(
    SignalStrategy::Error error) {
  DCHECK(thread_checker_.CalledOnValidThread());

  error_ = error;
  Disconnect();
}

void XmppSignalStrategy::Core::OnMessageSent() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (tls_state_ == TlsState::WAITING_FOR_FLUSH &&
      !writer_->has_data_pending()) {
    StartTls();
  }
}

void XmppSignalStrategy::Core::OnStanza(
    const std::unique_ptr<buzz::XmlElement> stanza) {
  DCHECK(thread_checker_.CalledOnValidThread());

  HOST_LOG << "Received incoming stanza:\n"
           << stanza->Str()
           << "\n=========================================================";

  for (auto& listener : listeners_) {
    if (listener.OnSignalStrategyIncomingStanza(stanza.get()))
      return;
  }
}

void XmppSignalStrategy::Core::OnParserError() {
  DCHECK(thread_checker_.CalledOnValidThread());

  error_ = NETWORK_ERROR;
  Disconnect();
}

void XmppSignalStrategy::Core::OnSocketConnected(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (result != net::OK) {
    OnNetworkError(result);
    return;
  }

  writer_ = BufferedSocketWriter::CreateForSocket(
      socket_.get(), base::Bind(&Core::OnNetworkError, base::Unretained(this)));

  XmppLoginHandler::TlsMode tls_mode;
  if (xmpp_server_config_.use_tls) {
    tls_mode = (xmpp_server_config_.port == kDefaultXmppPort)
                   ? XmppLoginHandler::TlsMode::WITH_HANDSHAKE
                   : XmppLoginHandler::TlsMode::WITHOUT_HANDSHAKE;
  } else {
    tls_mode = XmppLoginHandler::TlsMode::NO_TLS;
  }

  // The server name is passed as to attribute in the <stream>. When connecting
  // to talk.google.com it affects the certificate the server will use for TLS:
  // talk.google.com uses gmail certificate when specified server is gmail.com
  // or googlemail.com and google.com cert otherwise. In the same time it
  // doesn't accept talk.google.com as target server. Here we use google.com
  // server name when authenticating to talk.google.com. This ensures that the
  // server will use google.com cert which will be accepted by the TLS
  // implementation in Chrome (TLS API doesn't allow specifying domain other
  // than the one that was passed to connect()).
  std::string server = xmpp_server_config_.host;
  if (server == "talk.google.com")
    server = "google.com";

  login_handler_.reset(
      new XmppLoginHandler(server, xmpp_server_config_.username,
                           xmpp_server_config_.auth_token, tls_mode, this));
  login_handler_->Start();

  ReadSocket();
}

void XmppSignalStrategy::Core::OnTlsConnected(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(tls_state_ == TlsState::CONNECTING);
  tls_state_ = TlsState::CONNECTED;

  if (result != net::OK) {
    OnNetworkError(result);
    return;
  }

  writer_ = BufferedSocketWriter::CreateForSocket(
      socket_.get(), base::Bind(&Core::OnNetworkError, base::Unretained(this)));

  login_handler_->OnTlsStarted();

  ReadSocket();
}

void XmppSignalStrategy::Core::ReadSocket() {
  DCHECK(thread_checker_.CalledOnValidThread());

  while (socket_ && !read_pending_ && (tls_state_ == TlsState::NOT_REQUESTED ||
                                       tls_state_ == TlsState::CONNECTED)) {
    read_buffer_ = new net::IOBuffer(kReadBufferSize);
    int result = socket_->Read(
        read_buffer_.get(), kReadBufferSize,
        base::Bind(&Core::OnReadResult, base::Unretained(this)));
    HandleReadResult(result);
  }
}

void XmppSignalStrategy::Core::OnReadResult(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(read_pending_);
  read_pending_ = false;
  HandleReadResult(result);
  ReadSocket();
}

void XmppSignalStrategy::Core::HandleReadResult(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (result == net::ERR_IO_PENDING) {
    read_pending_ = true;
    return;
  }

  if (result < 0) {
    OnNetworkError(result);
    return;
  }

  if (result == 0) {
    // Connection was closed by the server.
    error_ = OK;
    Disconnect();
    return;
  }

  if (stream_parser_) {
    stream_parser_->AppendData(std::string(read_buffer_->data(), result));
  } else {
    login_handler_->OnDataReceived(std::string(read_buffer_->data(), result));
  }
}

void XmppSignalStrategy::Core::OnNetworkError(int error) {
  DCHECK(thread_checker_.CalledOnValidThread());

  LOG(ERROR) << "XMPP socket error " << error;
  error_ = NETWORK_ERROR;
  Disconnect();
}

void XmppSignalStrategy::Core::SendKeepAlive() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (GetState() == CONNECTED)
    SendMessage(" ");
}

XmppSignalStrategy::XmppSignalStrategy(
    net::ClientSocketFactory* socket_factory,
    const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
    const XmppServerConfig& xmpp_server_config)
    : core_(new Core(socket_factory,
                     request_context_getter,
                     xmpp_server_config)) {
}

XmppSignalStrategy::~XmppSignalStrategy() {
  // All listeners should be removed at this point, so it's safe to detach
  // |core_|.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, core_.release());
}

void XmppSignalStrategy::Connect() {
  core_->Connect();
}

void XmppSignalStrategy::Disconnect() {
  core_->Disconnect();
}

SignalStrategy::State XmppSignalStrategy::GetState() const {
  return core_->GetState();
}

SignalStrategy::Error XmppSignalStrategy::GetError() const {
  return core_->GetError();
}

const SignalingAddress& XmppSignalStrategy::GetLocalAddress() const {
  return core_->GetLocalAddress();
}

void XmppSignalStrategy::AddListener(Listener* listener) {
  core_->AddListener(listener);
}

void XmppSignalStrategy::RemoveListener(Listener* listener) {
  core_->RemoveListener(listener);
}
bool XmppSignalStrategy::SendStanza(std::unique_ptr<buzz::XmlElement> stanza) {
  return core_->SendStanza(std::move(stanza));
}

std::string XmppSignalStrategy::GetNextId() {
  return base::NumberToString(base::RandUint64());
}

void XmppSignalStrategy::SetAuthInfo(const std::string& username,
                                     const std::string& auth_token) {
  core_->SetAuthInfo(username, auth_token);
}

}  // namespace remoting
