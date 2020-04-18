// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_channel/keep_alive_delegate.h"

#include <string>
#include <utility>

#include "components/cast_channel/cast_channel_enum.h"
#include "components/cast_channel/cast_socket.h"
#include "components/cast_channel/logger.h"
#include "components/cast_channel/proto/cast_channel.pb.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace cast_channel {

using ::cast_channel::ChannelError;

KeepAliveDelegate::KeepAliveDelegate(
    CastSocket* socket,
    scoped_refptr<Logger> logger,
    std::unique_ptr<CastTransport::Delegate> inner_delegate,
    base::TimeDelta ping_interval,
    base::TimeDelta liveness_timeout)
    : started_(false),
      socket_(socket),
      logger_(logger),
      inner_delegate_(std::move(inner_delegate)),
      liveness_timeout_(liveness_timeout),
      ping_interval_(ping_interval),
      ping_message_(CreateKeepAlivePingMessage()),
      pong_message_(CreateKeepAlivePongMessage()) {
  DCHECK(ping_interval_ < liveness_timeout_);
  DCHECK(inner_delegate_);
  DCHECK(socket_);
}

KeepAliveDelegate::~KeepAliveDelegate() {}

void KeepAliveDelegate::SetTimersForTest(
    std::unique_ptr<base::Timer> injected_ping_timer,
    std::unique_ptr<base::Timer> injected_liveness_timer) {
  ping_timer_ = std::move(injected_ping_timer);
  liveness_timer_ = std::move(injected_liveness_timer);
}

void KeepAliveDelegate::Start() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!started_);

  DVLOG(1) << "Starting keep-alive timers.";
  DVLOG(1) << "Ping timeout: " << ping_interval_;
  DVLOG(1) << "Liveness timeout: " << liveness_timeout_;

  // Use injected mock timers, if provided.
  if (!ping_timer_) {
    ping_timer_.reset(new base::Timer(true, false));
  }
  if (!liveness_timer_) {
    liveness_timer_.reset(new base::Timer(true, false));
  }

  ping_timer_->Start(
      FROM_HERE, ping_interval_,
      base::BindRepeating(&KeepAliveDelegate::SendKeepAliveMessage,
                          base::Unretained(this), ping_message_,
                          CastMessageType::kPing));
  liveness_timer_->Start(
      FROM_HERE, liveness_timeout_,
      base::BindRepeating(&KeepAliveDelegate::LivenessTimeout,
                          base::Unretained(this)));

  started_ = true;
  inner_delegate_->Start();
}

void KeepAliveDelegate::ResetTimers() {
  DCHECK(started_);
  ping_timer_->Reset();
  liveness_timer_->Reset();
}

void KeepAliveDelegate::SendKeepAliveMessage(const CastMessage& message,
                                             CastMessageType message_type) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DVLOG(2) << "Sending " << CastMessageTypeToString(message_type);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("cast_keep_alive_delegate", R"(
        semantics {
          sender: "Cast Socket Keep Alive Delegate"
          description:
            "A ping/pong message sent periodically to a Cast device to keep "
            "the connection alive."
          trigger:
            "Periodically while a connection to a Cast device is established."
          data:
            "A protobuf message representing a ping/pong message. No user data."
          destination: OTHER
          destination_other:
            "Data will be sent to a Cast device in local network."
        }
        policy {
          cookies_allowed: NO
          setting:
            "This request cannot be disabled, but it would not be sent if user "
            "does not connect to a Cast device."
          policy_exception_justification: "Not implemented."
        })");
  socket_->transport()->SendMessage(
      message,
      base::Bind(&KeepAliveDelegate::SendKeepAliveMessageComplete,
                 base::Unretained(this), message_type),
      traffic_annotation);
}

void KeepAliveDelegate::SendKeepAliveMessageComplete(
    CastMessageType message_type,
    int rv) {
  DVLOG(2) << "Sending " << CastMessageTypeToString(message_type)
           << " complete, rv=" << rv;
  if (rv != net::OK) {
    // An error occurred while sending the ping response.
    DVLOG(1) << "Error sending " << CastMessageTypeToString(message_type);
    logger_->LogSocketEventWithRv(socket_->id(), ChannelEvent::PING_WRITE_ERROR,
                                  rv);
    OnError(ChannelError::CAST_SOCKET_ERROR);
    return;
  }

  if (liveness_timer_)
    liveness_timer_->Reset();
}

void KeepAliveDelegate::LivenessTimeout() {
  OnError(ChannelError::PING_TIMEOUT);
  Stop();
}

// CastTransport::Delegate interface.
void KeepAliveDelegate::OnError(ChannelError error_state) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DVLOG(1) << "KeepAlive::OnError: "
           << ::cast_channel::ChannelErrorToString(error_state);
  inner_delegate_->OnError(error_state);
  Stop();
}

void KeepAliveDelegate::OnMessage(const CastMessage& message) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DVLOG(2) << "KeepAlive::OnMessage : " << message.payload_utf8();

  if (started_)
    ResetTimers();

  // PING and PONG messages are intercepted and handled by KeepAliveDelegate
  // here. All other messages are passed through to |inner_delegate_|.
  CastMessageType payload_type = ParseMessageType(message);
  if (payload_type == CastMessageType::kPing) {
    DVLOG(2) << "Received PING.";
    if (started_)
      SendKeepAliveMessage(pong_message_, CastMessageType::kPong);
  } else if (payload_type == CastMessageType::kPong) {
    DVLOG(2) << "Received PONG.";
  } else {
    inner_delegate_->OnMessage(message);
  }
}

void KeepAliveDelegate::Stop() {
  if (started_) {
    started_ = false;
    ping_timer_->Stop();
    liveness_timer_->Stop();
  }
}

}  // namespace cast_channel
