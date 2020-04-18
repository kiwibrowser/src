// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_channel/cast_socket.h"

#include <stdlib.h>
#include <string.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/format_macros.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/sys_byteorder.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/cast_channel/cast_auth_util.h"
#include "components/cast_channel/cast_framer.h"
#include "components/cast_channel/cast_message_util.h"
#include "components/cast_channel/cast_transport.h"
#include "components/cast_channel/keep_alive_delegate.h"
#include "components/cast_channel/logger.h"
#include "components/cast_channel/proto/cast_channel.pb.h"
#include "net/base/address_list.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/cert/x509_certificate.h"
#include "net/http/transport_security_state.h"
#include "net/log/net_log.h"
#include "net/log/net_log_source_type.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/stream_socket.h"
#include "net/socket/tcp_client_socket.h"
#include "net/socket/transport_client_socket.h"
#include "net/ssl/ssl_config_service.h"
#include "net/ssl/ssl_info.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

// Helper for logging data with remote host IP and authentication state.
// Assumes |ip_endpoint_| of type net::IPEndPoint and |channel_auth_| of enum
// type ChannelAuthType are available in the current scope.
#define CONNECTION_INFO()                                             \
  "[" << open_params_.ip_endpoint.ToString() << ", auth=SSL_VERIFIED" \
      << "] "
#define VLOG_WITH_CONNECTION(level) VLOG(level) << CONNECTION_INFO()
#define LOG_WITH_CONNECTION(level) LOG(level) << CONNECTION_INFO()

namespace cast_channel {
namespace {

bool IsTerminalState(ConnectionState state) {
  return state == ConnectionState::FINISHED ||
         state == ConnectionState::CONNECT_ERROR ||
         state == ConnectionState::TIMEOUT;
}

// Cert verifier which blindly accepts all certificates, regardless of validity.
class FakeCertVerifier : public net::CertVerifier {
 public:
  FakeCertVerifier() {}
  ~FakeCertVerifier() override {}

  int Verify(const RequestParams& params,
             net::CRLSet*,
             net::CertVerifyResult* verify_result,
             const net::CompletionCallback&,
             std::unique_ptr<Request>*,
             const net::NetLogWithSource&) override {
    verify_result->Reset();
    verify_result->verified_cert = params.certificate();
    return net::OK;
  }
};

}  // namespace

CastSocketImpl::CastSocketImpl(const CastSocketOpenParams& open_params,
                               const scoped_refptr<Logger>& logger)
    : CastSocketImpl(open_params, logger, AuthContext::Create()) {}

CastSocketImpl::CastSocketImpl(const CastSocketOpenParams& open_params,
                               const scoped_refptr<Logger>& logger,
                               const AuthContext& auth_context)
    : channel_id_(0),
      open_params_(open_params),
      logger_(logger),
      auth_context_(auth_context),
      connect_timeout_timer_(new base::OneShotTimer),
      is_canceled_(false),
      audio_only_(false),
      connect_state_(ConnectionState::START_CONNECT),
      error_state_(ChannelError::NONE),
      ready_state_(ReadyState::NONE),
      auth_delegate_(nullptr) {
  DCHECK(open_params.ip_endpoint.address().IsValid());
  if (open_params_.net_log) {
    net_log_source_.type = net::NetLogSourceType::SOCKET;
    net_log_source_.id = open_params_.net_log->NextID();
  }
}

CastSocketImpl::~CastSocketImpl() {
  // Ensure that resources are freed but do not run pending callbacks that
  // would result in re-entrancy.
  CloseInternal();

  error_state_ = ChannelError::UNKNOWN;
  for (auto& connect_callback : connect_callbacks_)
    std::move(connect_callback).Run(this);
  connect_callbacks_.clear();
}

ReadyState CastSocketImpl::ready_state() const {
  return ready_state_;
}

ChannelError CastSocketImpl::error_state() const {
  return error_state_;
}

const net::IPEndPoint& CastSocketImpl::ip_endpoint() const {
  return open_params_.ip_endpoint;
}

int CastSocketImpl::id() const {
  return channel_id_;
}

void CastSocketImpl::set_id(int id) {
  channel_id_ = id;
}

bool CastSocketImpl::keep_alive() const {
  return open_params_.liveness_timeout > base::TimeDelta();
}

bool CastSocketImpl::audio_only() const {
  return audio_only_;
}

std::unique_ptr<net::TransportClientSocket> CastSocketImpl::CreateTcpSocket() {
  net::AddressList addresses(open_params_.ip_endpoint);
  return std::unique_ptr<net::TCPClientSocket>(new net::TCPClientSocket(
      addresses, nullptr, open_params_.net_log, net_log_source_));
  // Options cannot be set on the TCPClientSocket yet, because the
  // underlying platform socket will not be created until Bind()
  // or Connect() is called.
}

std::unique_ptr<net::SSLClientSocket> CastSocketImpl::CreateSslSocket(
    std::unique_ptr<net::StreamSocket> socket) {
  net::SSLConfig ssl_config;
  cert_verifier_ = base::WrapUnique(new FakeCertVerifier);
  transport_security_state_.reset(new net::TransportSecurityState);
  cert_transparency_verifier_.reset(new net::MultiLogCTVerifier());
  ct_policy_enforcer_.reset(new net::DefaultCTPolicyEnforcer());

  // Note that |context| fields remain owned by CastSocketImpl.
  net::SSLClientSocketContext context;
  context.cert_verifier = cert_verifier_.get();
  context.transport_security_state = transport_security_state_.get();
  context.cert_transparency_verifier = cert_transparency_verifier_.get();
  context.ct_policy_enforcer = ct_policy_enforcer_.get();

  std::unique_ptr<net::ClientSocketHandle> connection(
      new net::ClientSocketHandle);
  connection->SetSocket(std::move(socket));
  net::HostPortPair host_and_port =
      net::HostPortPair::FromIPEndPoint(open_params_.ip_endpoint);

  return net::ClientSocketFactory::GetDefaultFactory()->CreateSSLClientSocket(
      std::move(connection), host_and_port, ssl_config, context);
}

scoped_refptr<net::X509Certificate> CastSocketImpl::ExtractPeerCert() {
  net::SSLInfo ssl_info;
  if (!socket_->GetSSLInfo(&ssl_info) || !ssl_info.cert)
    return nullptr;

  return ssl_info.cert;
}

bool CastSocketImpl::VerifyChannelPolicy(const AuthResult& result) {
  audio_only_ = (result.channel_policies & AuthResult::POLICY_AUDIO_ONLY) != 0;
  if (audio_only_ && (open_params_.device_capabilities &
                      CastDeviceCapability::VIDEO_OUT) != 0) {
    LOG_WITH_CONNECTION(ERROR)
        << "Audio only channel policy enforced for video out capable device";
    return false;
  }
  return true;
}

bool CastSocketImpl::VerifyChallengeReply() {
  DCHECK(peer_cert_);
  AuthResult result =
      AuthenticateChallengeReply(*challenge_reply_, *peer_cert_, auth_context_);
  logger_->LogSocketChallengeReplyEvent(channel_id_, result);
  if (result.success()) {
    VLOG(1) << result.error_message;
    if (!VerifyChannelPolicy(result)) {
      return false;
    }
  }
  return result.success();
}

void CastSocketImpl::SetTransportForTesting(
    std::unique_ptr<CastTransport> transport) {
  transport_ = std::move(transport);
}

void CastSocketImpl::Connect(OnOpenCallback callback) {
  switch (ready_state_) {
    case ReadyState::NONE:
      connect_callbacks_.push_back(std::move(callback));
      Connect();
      break;
    case ReadyState::CONNECTING:
      connect_callbacks_.push_back(std::move(callback));
      break;
    case ReadyState::OPEN:
      error_state_ = ChannelError::NONE;
      std::move(callback).Run(this);
      break;
    case ReadyState::CLOSED:
      error_state_ = ChannelError::CONNECT_ERROR;
      std::move(callback).Run(this);
      break;
    default:
      NOTREACHED() << "Unknown ReadyState: "
                   << ReadyStateToString(ready_state_);
  }
}

void CastSocketImpl::Connect() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG_WITH_CONNECTION(1) << "Connect readyState = "
                          << ReadyStateToString(ready_state_);
  DCHECK_EQ(ReadyState::NONE, ready_state_);
  DCHECK_EQ(ConnectionState::START_CONNECT, connect_state_);

  delegate_ = std::make_unique<CastSocketMessageDelegate>(this);

  SetReadyState(ReadyState::CONNECTING);
  SetConnectState(ConnectionState::TCP_CONNECT);

  // Set up connection timeout.
  if (open_params_.connect_timeout.InMicroseconds() > 0) {
    DCHECK(connect_timeout_callback_.IsCancelled());
    connect_timeout_callback_.Reset(
        base::Bind(&CastSocketImpl::OnConnectTimeout, base::Unretained(this)));
    GetTimer()->Start(FROM_HERE, open_params_.connect_timeout,
                      connect_timeout_callback_.callback());
  }

  DoConnectLoop(net::OK);
}

CastTransport* CastSocketImpl::transport() const {
  return transport_.get();
}

void CastSocketImpl::AddObserver(Observer* observer) {
  DCHECK(observer);
  if (!observers_.HasObserver(observer))
    observers_.AddObserver(observer);
}

void CastSocketImpl::RemoveObserver(Observer* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void CastSocketImpl::OnConnectTimeout() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Stop all pending connection setup tasks and report back to the client.
  is_canceled_ = true;
  VLOG_WITH_CONNECTION(1) << "Timeout while establishing a connection.";
  SetErrorState(ChannelError::CONNECT_TIMEOUT);
  DoConnectCallback();
}

void CastSocketImpl::ResetConnectLoopCallback() {
  DCHECK(connect_loop_callback_.IsCancelled());
  connect_loop_callback_.Reset(
      base::Bind(&CastSocketImpl::DoConnectLoop, base::Unretained(this)));
}

void CastSocketImpl::PostTaskToStartConnectLoop(int result) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  ResetConnectLoopCallback();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(connect_loop_callback_.callback(), result));
}

// This method performs the state machine transitions for connection flow.
// There are two entry points to this method:
// 1. Connect method: this starts the flow
// 2. Callback from network operations that finish asynchronously.
void CastSocketImpl::DoConnectLoop(int result) {
  connect_loop_callback_.Cancel();
  if (is_canceled_) {
    LOG_WITH_CONNECTION(ERROR) << "CANCELLED - Aborting DoConnectLoop.";
    return;
  }

  // Network operations can either finish synchronously or asynchronously.
  // This method executes the state machine transitions in a loop so that
  // correct state transitions happen even when network operations finish
  // synchronously.
  int rv = result;
  do {
    ConnectionState state = connect_state_;
    connect_state_ = ConnectionState::UNKNOWN;
    switch (state) {
      case ConnectionState::TCP_CONNECT:
        rv = DoTcpConnect();
        break;
      case ConnectionState::TCP_CONNECT_COMPLETE:
        rv = DoTcpConnectComplete(rv);
        break;
      case ConnectionState::SSL_CONNECT:
        DCHECK_EQ(net::OK, rv);
        rv = DoSslConnect();
        break;
      case ConnectionState::SSL_CONNECT_COMPLETE:
        rv = DoSslConnectComplete(rv);
        break;
      case ConnectionState::AUTH_CHALLENGE_SEND:
        rv = DoAuthChallengeSend();
        break;
      case ConnectionState::AUTH_CHALLENGE_SEND_COMPLETE:
        rv = DoAuthChallengeSendComplete(rv);
        break;
      case ConnectionState::AUTH_CHALLENGE_REPLY_COMPLETE:
        rv = DoAuthChallengeReplyComplete(rv);
        DCHECK(IsTerminalState(connect_state_));
        break;
      default:
        NOTREACHED() << "Unknown state in connect flow: " << AsInteger(state);
        SetConnectState(ConnectionState::FINISHED);
        SetErrorState(ChannelError::UNKNOWN);
        DoConnectCallback();
        return;
    }
  } while (rv != net::ERR_IO_PENDING && !IsTerminalState(connect_state_));
  // Exit the state machine if an asynchronous network operation is pending
  // or if the state machine is in the terminal "finished" state.

  if (IsTerminalState(connect_state_)) {
    DCHECK_NE(rv, net::ERR_IO_PENDING);
    GetTimer()->Stop();
    DoConnectCallback();
  } else {
    DCHECK_EQ(rv, net::ERR_IO_PENDING);
  }
}

int CastSocketImpl::DoTcpConnect() {
  DCHECK(connect_loop_callback_.IsCancelled());
  VLOG_WITH_CONNECTION(1) << "DoTcpConnect";
  SetConnectState(ConnectionState::TCP_CONNECT_COMPLETE);
  tcp_socket_ = CreateTcpSocket();

  int rv = tcp_socket_->Connect(
      base::Bind(&CastSocketImpl::DoConnectLoop, base::Unretained(this)));
  logger_->LogSocketEventWithRv(channel_id_, ChannelEvent::TCP_SOCKET_CONNECT,
                                rv);
  return rv;
}

int CastSocketImpl::DoTcpConnectComplete(int connect_result) {
  VLOG_WITH_CONNECTION(1) << "DoTcpConnectComplete: " << connect_result;
  logger_->LogSocketEventWithRv(
      channel_id_, ChannelEvent::TCP_SOCKET_CONNECT_COMPLETE, connect_result);
  if (connect_result == net::OK) {
    SetConnectState(ConnectionState::SSL_CONNECT);
  } else if (connect_result == net::ERR_CONNECTION_TIMED_OUT) {
    SetConnectState(ConnectionState::FINISHED);
    SetErrorState(ChannelError::CONNECT_TIMEOUT);
  } else {
    SetConnectState(ConnectionState::FINISHED);
    SetErrorState(ChannelError::CONNECT_ERROR);
  }
  return connect_result;
}

int CastSocketImpl::DoSslConnect() {
  DCHECK(connect_loop_callback_.IsCancelled());
  VLOG_WITH_CONNECTION(1) << "DoSslConnect";
  SetConnectState(ConnectionState::SSL_CONNECT_COMPLETE);
  socket_ = CreateSslSocket(std::move(tcp_socket_));

  int rv = socket_->Connect(
      base::Bind(&CastSocketImpl::DoConnectLoop, base::Unretained(this)));
  logger_->LogSocketEventWithRv(channel_id_, ChannelEvent::SSL_SOCKET_CONNECT,
                                rv);
  return rv;
}

int CastSocketImpl::DoSslConnectComplete(int result) {
  logger_->LogSocketEventWithRv(
      channel_id_, ChannelEvent::SSL_SOCKET_CONNECT_COMPLETE, result);
  VLOG_WITH_CONNECTION(1) << "DoSslConnectComplete: " << result;
  if (result == net::OK) {
    peer_cert_ = ExtractPeerCert();

    if (!peer_cert_) {
      LOG_WITH_CONNECTION(WARNING) << "Could not extract peer cert.";
      SetConnectState(ConnectionState::FINISHED);
      SetErrorState(ChannelError::AUTHENTICATION_ERROR);
      return net::ERR_CERT_INVALID;
    }

    // SSL connection succeeded.
    if (!transport_) {
      // Create a channel transport if one wasn't already set (e.g. by test
      // code).
      transport_.reset(new CastTransportImpl(
          this->socket_.get(), channel_id_, open_params_.ip_endpoint, logger_));
    }
    auth_delegate_ = new AuthTransportDelegate(this);
    transport_->SetReadDelegate(base::WrapUnique(auth_delegate_));
    SetConnectState(ConnectionState::AUTH_CHALLENGE_SEND);
  } else if (result == net::ERR_CONNECTION_TIMED_OUT) {
    SetConnectState(ConnectionState::FINISHED);
    SetErrorState(ChannelError::CONNECT_TIMEOUT);
  } else {
    SetConnectState(ConnectionState::FINISHED);
    SetErrorState(ChannelError::AUTHENTICATION_ERROR);
  }
  return result;
}

int CastSocketImpl::DoAuthChallengeSend() {
  VLOG_WITH_CONNECTION(1) << "DoAuthChallengeSend";
  SetConnectState(ConnectionState::AUTH_CHALLENGE_SEND_COMPLETE);

  CastMessage challenge_message;
  CreateAuthChallengeMessage(&challenge_message, auth_context_);
  VLOG_WITH_CONNECTION(1) << "Sending challenge: "
                          << CastMessageToString(challenge_message);

  ResetConnectLoopCallback();

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("cast_socket", R"(
        semantics {
          sender: "Cast Socket"
          description:
            "An auth challenge request sent to a Cast device as a part of "
            "establishing a connection to it."
          trigger:
            "A new Cast device has been discovered via mDNS in the local "
            "network."
          data:
            "A serialized protobuf message containing the nonce challenge. No "
            "user data."
          destination: OTHER
          destination_other:
            "Data will be sent to a Cast device in local network."
        }
        policy {
          cookies_allowed: NO
          setting:
            "This request cannot be disabled, but it would not be sent if user "
            "does not connect a Cast device to the local network."
          policy_exception_justification: "Not implemented."
        })");
  transport_->SendMessage(challenge_message, connect_loop_callback_.callback(),
                          traffic_annotation);

  // Always return IO_PENDING since the result is always asynchronous.
  return net::ERR_IO_PENDING;
}

int CastSocketImpl::DoAuthChallengeSendComplete(int result) {
  VLOG_WITH_CONNECTION(1) << "DoAuthChallengeSendComplete: " << result;
  if (result < 0) {
    SetConnectState(ConnectionState::CONNECT_ERROR);
    SetErrorState(ChannelError::CAST_SOCKET_ERROR);
    logger_->LogSocketEventWithRv(
        channel_id_, ChannelEvent::SEND_AUTH_CHALLENGE_FAILED, result);
    return result;
  }
  transport_->Start();
  SetConnectState(ConnectionState::AUTH_CHALLENGE_REPLY_COMPLETE);
  return net::ERR_IO_PENDING;
}

CastSocketImpl::AuthTransportDelegate::AuthTransportDelegate(
    CastSocketImpl* socket)
    : socket_(socket), error_state_(ChannelError::NONE) {
  DCHECK(socket);
}

ChannelError CastSocketImpl::AuthTransportDelegate::error_state() const {
  return error_state_;
}

LastError CastSocketImpl::AuthTransportDelegate::last_error() const {
  return last_error_;
}

void CastSocketImpl::AuthTransportDelegate::OnError(ChannelError error_state) {
  error_state_ = error_state;
  socket_->PostTaskToStartConnectLoop(net::ERR_CONNECTION_FAILED);
}

void CastSocketImpl::AuthTransportDelegate::OnMessage(
    const CastMessage& message) {
  if (!IsAuthMessage(message)) {
    error_state_ = ChannelError::TRANSPORT_ERROR;
    socket_->PostTaskToStartConnectLoop(net::ERR_INVALID_RESPONSE);
  } else {
    socket_->challenge_reply_.reset(new CastMessage(message));
    socket_->PostTaskToStartConnectLoop(net::OK);
  }
}

void CastSocketImpl::AuthTransportDelegate::Start() {}

int CastSocketImpl::DoAuthChallengeReplyComplete(int result) {
  VLOG_WITH_CONNECTION(1) << "DoAuthChallengeReplyComplete: " << result;

  if (auth_delegate_->error_state() != ChannelError::NONE) {
    SetErrorState(auth_delegate_->error_state());
    SetConnectState(ConnectionState::CONNECT_ERROR);
    return net::ERR_CONNECTION_FAILED;
  }
  auth_delegate_ = nullptr;

  if (result < 0) {
    SetConnectState(ConnectionState::CONNECT_ERROR);
    return result;
  }

  if (!VerifyChallengeReply()) {
    SetErrorState(ChannelError::AUTHENTICATION_ERROR);
    SetConnectState(ConnectionState::CONNECT_ERROR);
    return net::ERR_CONNECTION_FAILED;
  }
  VLOG_WITH_CONNECTION(1) << "Auth challenge verification succeeded";

  SetConnectState(ConnectionState::FINISHED);
  return net::OK;
}

void CastSocketImpl::DoConnectCallback() {
  VLOG(1) << "DoConnectCallback (error_state = "
          << ChannelErrorToString(error_state_) << ")";
  if (connect_callbacks_.empty()) {
    DLOG(FATAL) << "Connection callback invoked multiple times.";
    return;
  }

  if (error_state_ == ChannelError::NONE) {
    SetReadyState(ReadyState::OPEN);
    if (keep_alive()) {
      auto* keep_alive_delegate = new KeepAliveDelegate(
          this, logger_, std::move(delegate_), open_params_.ping_interval,
          open_params_.liveness_timeout);
      delegate_.reset(keep_alive_delegate);
    }
    transport_->SetReadDelegate(std::move(delegate_));
  } else {
    CloseInternal();
  }

  for (auto& connect_callback : connect_callbacks_)
    std::move(connect_callback).Run(this);
  connect_callbacks_.clear();
}

void CastSocketImpl::Close(const net::CompletionCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  CloseInternal();
  // Run this callback last.  It may delete the socket.
  callback.Run(net::OK);
}

void CastSocketImpl::CloseInternal() {
  // TODO(mfoltz): Enforce this when CastChannelAPITest is rewritten to create
  // and free sockets on the same thread.  crbug.com/398242
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (ready_state_ == ReadyState::CLOSED) {
    return;
  }

  VLOG_WITH_CONNECTION(1) << "Close ReadyState = "
                          << ReadyStateToString(ready_state_);
  observers_.Clear();
  delegate_.reset();
  transport_.reset();
  tcp_socket_.reset();
  socket_.reset();
  transport_security_state_.reset();
  if (GetTimer()) {
    GetTimer()->Stop();
  }

  // Cancel callbacks that we queued ourselves to re-enter the connect or read
  // loops.
  connect_loop_callback_.Cancel();
  connect_timeout_callback_.Cancel();
  SetReadyState(ReadyState::CLOSED);
}

base::Timer* CastSocketImpl::GetTimer() {
  return connect_timeout_timer_.get();
}

void CastSocketImpl::SetConnectState(ConnectionState connect_state) {
  if (connect_state_ != connect_state) {
    connect_state_ = connect_state;
  }
}

void CastSocketImpl::SetReadyState(ReadyState ready_state) {
  if (ready_state_ != ready_state)
    ready_state_ = ready_state;
}

void CastSocketImpl::SetErrorState(ChannelError error_state) {
  VLOG_WITH_CONNECTION(1) << "SetErrorState "
                          << ChannelErrorToString(error_state);
  DCHECK_EQ(ChannelError::NONE, error_state_);
  error_state_ = error_state;
  delegate_->OnError(error_state_);
}

CastSocketImpl::CastSocketMessageDelegate::CastSocketMessageDelegate(
    CastSocketImpl* socket)
    : socket_(socket) {
  DCHECK(socket_);
}

CastSocketImpl::CastSocketMessageDelegate::~CastSocketMessageDelegate() {}

// CastTransport::Delegate implementation.
void CastSocketImpl::CastSocketMessageDelegate::OnError(
    ChannelError error_state) {
  for (auto& observer : socket_->observers_)
    observer.OnError(*socket_, error_state);
}

void CastSocketImpl::CastSocketMessageDelegate::OnMessage(
    const CastMessage& message) {
  for (auto& observer : socket_->observers_)
    observer.OnMessage(*socket_, message);
}

void CastSocketImpl::CastSocketMessageDelegate::Start() {}

CastSocketOpenParams::CastSocketOpenParams(const net::IPEndPoint& ip_endpoint,
                                           net::NetLog* net_log,
                                           base::TimeDelta connect_timeout)
    : ip_endpoint(ip_endpoint),
      net_log(net_log),
      connect_timeout(connect_timeout),
      device_capabilities(cast_channel::CastDeviceCapability::NONE) {}

CastSocketOpenParams::CastSocketOpenParams(const net::IPEndPoint& ip_endpoint,
                                           net::NetLog* net_log,
                                           base::TimeDelta connect_timeout,
                                           base::TimeDelta liveness_timeout,
                                           base::TimeDelta ping_interval,
                                           uint64_t device_capabilities)
    : ip_endpoint(ip_endpoint),
      net_log(net_log),
      connect_timeout(connect_timeout),
      liveness_timeout(liveness_timeout),
      ping_interval(ping_interval),
      device_capabilities(device_capabilities) {}

}  // namespace cast_channel
#undef VLOG_WITH_CONNECTION
