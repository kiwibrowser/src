// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/websocket.h"

#include <inttypes.h>

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/ssl/ssl_info.h"
#include "net/websockets/websocket_channel.h"
#include "net/websockets/websocket_errors.h"
#include "net/websockets/websocket_frame.h"  // for WebSocketFrameHeader::OpCode
#include "net/websockets/websocket_handshake_request_info.h"
#include "net/websockets/websocket_handshake_response_info.h"

namespace network {
namespace {

// Convert a mojom::WebSocketMessageType to a
// net::WebSocketFrameHeader::OpCode
net::WebSocketFrameHeader::OpCode MessageTypeToOpCode(
    mojom::WebSocketMessageType type) {
  DCHECK(type == mojom::WebSocketMessageType::CONTINUATION ||
         type == mojom::WebSocketMessageType::TEXT ||
         type == mojom::WebSocketMessageType::BINARY);
  typedef net::WebSocketFrameHeader::OpCode OpCode;
  // These compile asserts verify that the same underlying values are used for
  // both types, so we can simply cast between them.
  static_assert(
      static_cast<OpCode>(mojom::WebSocketMessageType::CONTINUATION) ==
          net::WebSocketFrameHeader::kOpCodeContinuation,
      "enum values must match for opcode continuation");
  static_assert(static_cast<OpCode>(mojom::WebSocketMessageType::TEXT) ==
                    net::WebSocketFrameHeader::kOpCodeText,
                "enum values must match for opcode text");
  static_assert(static_cast<OpCode>(mojom::WebSocketMessageType::BINARY) ==
                    net::WebSocketFrameHeader::kOpCodeBinary,
                "enum values must match for opcode binary");
  return static_cast<OpCode>(type);
}

mojom::WebSocketMessageType OpCodeToMessageType(
    net::WebSocketFrameHeader::OpCode opCode) {
  DCHECK(opCode == net::WebSocketFrameHeader::kOpCodeContinuation ||
         opCode == net::WebSocketFrameHeader::kOpCodeText ||
         opCode == net::WebSocketFrameHeader::kOpCodeBinary);
  // This cast is guaranteed valid by the static_assert() statements above.
  return static_cast<mojom::WebSocketMessageType>(opCode);
}

}  // namespace

// Implementation of net::WebSocketEventInterface. Receives events from our
// WebSocketChannel object.
class WebSocket::WebSocketEventHandler final
    : public net::WebSocketEventInterface {
 public:
  explicit WebSocketEventHandler(WebSocket* impl);
  ~WebSocketEventHandler() override;

  // net::WebSocketEventInterface implementation

  void OnCreateURLRequest(net::URLRequest* url_request) override;
  void OnAddChannelResponse(const std::string& selected_subprotocol,
                            const std::string& extensions) override;
  void OnDataFrame(bool fin,
                   WebSocketMessageType type,
                   scoped_refptr<net::IOBuffer> buffer,
                   size_t buffer_size) override;
  void OnClosingHandshake() override;
  void OnFlowControl(int64_t quota) override;
  void OnDropChannel(bool was_clean,
                     uint16_t code,
                     const std::string& reason) override;
  void OnFailChannel(const std::string& message) override;
  void OnStartOpeningHandshake(
      std::unique_ptr<net::WebSocketHandshakeRequestInfo> request) override;
  void OnFinishOpeningHandshake(
      std::unique_ptr<net::WebSocketHandshakeResponseInfo> response) override;
  void OnSSLCertificateError(
      std::unique_ptr<net::WebSocketEventInterface::SSLErrorCallbacks>
          callbacks,
      const GURL& url,
      const net::SSLInfo& ssl_info,
      bool fatal) override;

 private:
  WebSocket* const impl_;

  DISALLOW_COPY_AND_ASSIGN(WebSocketEventHandler);
};

WebSocket::WebSocketEventHandler::WebSocketEventHandler(WebSocket* impl)
    : impl_(impl) {
  DVLOG(1) << "WebSocketEventHandler created @"
           << reinterpret_cast<void*>(this);
}

WebSocket::WebSocketEventHandler::~WebSocketEventHandler() {
  DVLOG(1) << "WebSocketEventHandler destroyed @"
           << reinterpret_cast<void*>(this);
}

void WebSocket::WebSocketEventHandler::OnCreateURLRequest(
    net::URLRequest* url_request) {
  impl_->delegate_->OnCreateURLRequest(impl_->child_id_, impl_->frame_id_,
                                       url_request);
}

void WebSocket::WebSocketEventHandler::OnAddChannelResponse(
    const std::string& selected_protocol,
    const std::string& extensions) {
  DVLOG(3) << "WebSocketEventHandler::OnAddChannelResponse @"
           << reinterpret_cast<void*>(this) << " selected_protocol=\""
           << selected_protocol << "\""
           << " extensions=\"" << extensions << "\"";

  impl_->handshake_succeeded_ = true;
  impl_->pending_connection_tracker_.OnCompleteHandshake();

  impl_->client_->OnAddChannelResponse(selected_protocol, extensions);
}

void WebSocket::WebSocketEventHandler::OnDataFrame(
    bool fin,
    net::WebSocketFrameHeader::OpCode type,
    scoped_refptr<net::IOBuffer> buffer,
    size_t buffer_size) {
  DVLOG(3) << "WebSocketEventHandler::OnDataFrame @"
           << reinterpret_cast<void*>(this) << " fin=" << fin
           << " type=" << type << " data is " << buffer_size << " bytes";

  // TODO(darin): Avoid this copy.
  std::vector<uint8_t> data_to_pass(buffer_size);
  if (buffer_size > 0) {
    std::copy(buffer->data(), buffer->data() + buffer_size,
              data_to_pass.begin());
  }

  impl_->client_->OnDataFrame(fin, OpCodeToMessageType(type), data_to_pass);
}

void WebSocket::WebSocketEventHandler::OnClosingHandshake() {
  DVLOG(3) << "WebSocketEventHandler::OnClosingHandshake @"
           << reinterpret_cast<void*>(this);

  impl_->client_->OnClosingHandshake();
}

void WebSocket::WebSocketEventHandler::OnFlowControl(int64_t quota) {
  DVLOG(3) << "WebSocketEventHandler::OnFlowControl @"
           << reinterpret_cast<void*>(this) << " quota=" << quota;

  impl_->client_->OnFlowControl(quota);
}

void WebSocket::WebSocketEventHandler::OnDropChannel(
    bool was_clean,
    uint16_t code,
    const std::string& reason) {
  DVLOG(3) << "WebSocketEventHandler::OnDropChannel @"
           << reinterpret_cast<void*>(this) << " was_clean=" << was_clean
           << " code=" << code << " reason=\"" << reason << "\"";

  impl_->client_->OnDropChannel(was_clean, code, reason);

  // net::WebSocketChannel requires that we delete it at this point.
  impl_->channel_.reset();
}

void WebSocket::WebSocketEventHandler::OnFailChannel(
    const std::string& message) {
  DVLOG(3) << "WebSocketEventHandler::OnFailChannel @"
           << reinterpret_cast<void*>(this) << " message=\"" << message << "\"";

  impl_->client_->OnFailChannel(message);

  // net::WebSocketChannel requires that we delete it at this point.
  impl_->channel_.reset();
}

void WebSocket::WebSocketEventHandler::OnStartOpeningHandshake(
    std::unique_ptr<net::WebSocketHandshakeRequestInfo> request) {
  bool should_send = impl_->delegate_->CanReadRawCookies();

  DVLOG(3) << "WebSocketEventHandler::OnStartOpeningHandshake @"
           << reinterpret_cast<void*>(this) << " should_send=" << should_send;

  if (!should_send)
    return;

  mojom::WebSocketHandshakeRequestPtr request_to_pass(
      mojom::WebSocketHandshakeRequest::New());
  request_to_pass->url.Swap(&request->url);
  net::HttpRequestHeaders::Iterator it(request->headers);
  while (it.GetNext()) {
    mojom::HttpHeaderPtr header(mojom::HttpHeader::New());
    header->name = it.name();
    header->value = it.value();
    request_to_pass->headers.push_back(std::move(header));
  }
  request_to_pass->headers_text =
      base::StringPrintf("GET %s HTTP/1.1\r\n",
                         request_to_pass->url.spec().c_str()) +
      request->headers.ToString();

  impl_->client_->OnStartOpeningHandshake(std::move(request_to_pass));
}

void WebSocket::WebSocketEventHandler::OnFinishOpeningHandshake(
    std::unique_ptr<net::WebSocketHandshakeResponseInfo> response) {
  bool should_send = impl_->delegate_->CanReadRawCookies();

  DVLOG(3) << "WebSocketEventHandler::OnFinishOpeningHandshake "
           << reinterpret_cast<void*>(this) << " should_send=" << should_send;

  if (!should_send)
    return;

  mojom::WebSocketHandshakeResponsePtr response_to_pass(
      mojom::WebSocketHandshakeResponse::New());
  response_to_pass->url.Swap(&response->url);
  response_to_pass->status_code = response->headers->response_code();
  response_to_pass->status_text = response->headers->GetStatusText();
  size_t iter = 0;
  std::string name, value;
  while (response->headers->EnumerateHeaderLines(&iter, &name, &value)) {
    mojom::HttpHeaderPtr header(mojom::HttpHeader::New());
    header->name = name;
    header->value = value;
    response_to_pass->headers.push_back(std::move(header));
  }
  response_to_pass->headers_text =
      net::HttpUtil::ConvertHeadersBackToHTTPResponse(
          response->headers->raw_headers());

  impl_->client_->OnFinishOpeningHandshake(std::move(response_to_pass));
}

void WebSocket::WebSocketEventHandler::OnSSLCertificateError(
    std::unique_ptr<net::WebSocketEventInterface::SSLErrorCallbacks> callbacks,
    const GURL& url,
    const net::SSLInfo& ssl_info,
    bool fatal) {
  DVLOG(3) << "WebSocketEventHandler::OnSSLCertificateError"
           << reinterpret_cast<void*>(this) << " url=" << url.spec()
           << " cert_status=" << ssl_info.cert_status << " fatal=" << fatal;
  impl_->delegate_->OnSSLCertificateError(std::move(callbacks), url,
                                          impl_->child_id_, impl_->frame_id_,
                                          ssl_info, fatal);
}

WebSocket::WebSocket(
    std::unique_ptr<Delegate> delegate,
    mojom::WebSocketRequest request,
    WebSocketThrottler::PendingConnection pending_connection_tracker,
    int child_id,
    int frame_id,
    url::Origin origin,
    base::TimeDelta delay)
    : delegate_(std::move(delegate)),
      binding_(this, std::move(request)),
      pending_connection_tracker_(std::move(pending_connection_tracker)),
      delay_(delay),
      pending_flow_control_quota_(0),
      child_id_(child_id),
      frame_id_(frame_id),
      origin_(std::move(origin)),
      handshake_succeeded_(false),
      weak_ptr_factory_(this) {
  binding_.set_connection_error_handler(
      base::BindOnce(&WebSocket::OnConnectionError, base::Unretained(this)));
}

WebSocket::~WebSocket() {}

void WebSocket::GoAway() {
  StartClosingHandshake(static_cast<uint16_t>(net::kWebSocketErrorGoingAway),
                        "");
}

void WebSocket::AddChannelRequest(
    const GURL& socket_url,
    const std::vector<std::string>& requested_protocols,
    const GURL& site_for_cookies,
    const std::string& user_agent_override,
    mojom::WebSocketClientPtr client) {
  DVLOG(3) << "WebSocket::AddChannelRequest @" << reinterpret_cast<void*>(this)
           << " socket_url=\"" << socket_url << "\" requested_protocols=\""
           << base::JoinString(requested_protocols, ", ") << "\" origin=\""
           << origin_ << "\" site_for_cookies=\"" << site_for_cookies << "\"";

  if (client_ || !client) {
    delegate_->ReportBadMessage(
        Delegate::BadMessageReason::kUnexpectedAddChannelRequest, this);
    return;
  }

  net::HttpRequestHeaders additional_headers;
  if (!user_agent_override.empty() &&
      net::HttpUtil::IsValidHeaderValue(user_agent_override)) {
    additional_headers.SetHeader(net::HttpRequestHeaders::kUserAgent,
                                 user_agent_override);
  }
  client_ = std::move(client);

  DCHECK(!channel_);
  if (delay_ > base::TimeDelta()) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&WebSocket::AddChannel, weak_ptr_factory_.GetWeakPtr(),
                       socket_url, requested_protocols, site_for_cookies,
                       additional_headers),
        delay_);
  } else {
    AddChannel(socket_url, requested_protocols, site_for_cookies,
               additional_headers);
  }
}

void WebSocket::SendFrame(bool fin,
                          mojom::WebSocketMessageType type,
                          const std::vector<uint8_t>& data) {
  DVLOG(3) << "WebSocket::SendFrame @" << reinterpret_cast<void*>(this)
           << " fin=" << fin << " type=" << type << " data is " << data.size()
           << " bytes";

  if (!channel_) {
    // The client should not be sending us frames until after we've informed
    // it that the channel has been opened (OnAddChannelResponse).
    if (handshake_succeeded_) {
      DVLOG(1) << "Dropping frame sent to closed websocket";
    } else {
      delegate_->ReportBadMessage(
          Delegate::BadMessageReason::kUnexpectedSendFrame, this);
    }
    return;
  }

  // TODO(darin): Avoid this copy.
  scoped_refptr<net::IOBuffer> data_to_pass(new net::IOBuffer(data.size()));
  std::copy(data.begin(), data.end(), data_to_pass->data());

  channel_->SendFrame(fin, MessageTypeToOpCode(type), std::move(data_to_pass),
                      data.size());
}

void WebSocket::SendFlowControl(int64_t quota) {
  DVLOG(3) << "WebSocket::OnFlowControl @" << reinterpret_cast<void*>(this)
           << " quota=" << quota;

  if (!channel_) {
    // WebSocketChannel is not yet created due to the delay introduced by
    // per-renderer WebSocket throttling.
    // SendFlowControl() is called after WebSocketChannel is created.
    pending_flow_control_quota_ += quota;
    return;
  }

  ignore_result(channel_->SendFlowControl(quota));
}

void WebSocket::StartClosingHandshake(uint16_t code,
                                      const std::string& reason) {
  DVLOG(3) << "WebSocket::StartClosingHandshake @"
           << reinterpret_cast<void*>(this) << " code=" << code << " reason=\""
           << reason << "\"";

  if (!channel_) {
    // WebSocketChannel is not yet created due to the delay introduced by
    // per-renderer WebSocket throttling.
    if (client_)
      client_->OnDropChannel(false, net::kWebSocketErrorAbnormalClosure, "");
    return;
  }

  ignore_result(channel_->StartClosingHandshake(code, reason));
}

void WebSocket::OnConnectionError() {
  DVLOG(3) << "WebSocket::OnConnectionError @" << reinterpret_cast<void*>(this);

  delegate_->OnLostConnectionToClient(this);
}

void WebSocket::AddChannel(const GURL& socket_url,
                           const std::vector<std::string>& requested_protocols,
                           const GURL& site_for_cookies,
                           const net::HttpRequestHeaders& additional_headers) {
  DVLOG(3) << "WebSocket::AddChannel @" << reinterpret_cast<void*>(this)
           << " socket_url=\"" << socket_url << "\" requested_protocols=\""
           << base::JoinString(requested_protocols, ", ") << "\" origin=\""
           << origin_ << "\" site_for_cookies=\"" << site_for_cookies << "\"";

  DCHECK(!channel_);

  std::unique_ptr<net::WebSocketEventInterface> event_interface(
      new WebSocketEventHandler(this));
  channel_.reset(new net::WebSocketChannel(std::move(event_interface),
                                           delegate_->GetURLRequestContext()));

  int64_t quota = pending_flow_control_quota_;
  pending_flow_control_quota_ = 0;

  channel_->SendAddChannelRequest(socket_url, requested_protocols, origin_,
                                  site_for_cookies, additional_headers);
  if (quota > 0)
    SendFlowControl(quota);
}

}  // namespace network
