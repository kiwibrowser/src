// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/signaling_connector.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/strings/string_util.h"
#include "google_apis/google_api_keys.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "remoting/base/logging.h"
#include "remoting/host/dns_blackhole_checker.h"
#include "remoting/signaling/signaling_address.h"

namespace remoting {

namespace {

// The delay between reconnect attempts will increase exponentially up
// to the maximum specified here.
const int kMaxReconnectDelaySeconds = 10 * 60;

const char* SignalStrategyErrorToString(SignalStrategy::Error error){
  switch(error) {
    case SignalStrategy::OK:
      return "OK";
    case SignalStrategy::AUTHENTICATION_FAILED:
      return "AUTHENTICATION_FAILED";
    case SignalStrategy::NETWORK_ERROR:
      return "NETWORK_ERROR";
    case SignalStrategy::PROTOCOL_ERROR:
      return "PROTOCOL_ERROR";
  }
  NOTREACHED();
  return "";
}

}  // namespace

SignalingConnector::SignalingConnector(
    XmppSignalStrategy* signal_strategy,
    std::unique_ptr<DnsBlackholeChecker> dns_blackhole_checker,
    OAuthTokenGetter* oauth_token_getter,
    const base::Closure& auth_failed_callback)
    : signal_strategy_(signal_strategy),
      auth_failed_callback_(auth_failed_callback),
      dns_blackhole_checker_(std::move(dns_blackhole_checker)),
      oauth_token_getter_(oauth_token_getter),
      reconnect_attempts_(0),
      weak_factory_(this) {
  DCHECK(!auth_failed_callback_.is_null());
  DCHECK(dns_blackhole_checker_.get());
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  signal_strategy_->AddListener(this);
  ScheduleTryReconnect();
}

SignalingConnector::~SignalingConnector() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  signal_strategy_->RemoveListener(this);
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void SignalingConnector::OnSignalStrategyStateChange(
    SignalStrategy::State state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (state == SignalStrategy::CONNECTED) {
    HOST_LOG << "Signaling connected. New JID: "
             << signal_strategy_->GetLocalAddress().jid();
    reconnect_attempts_ = 0;
  } else if (state == SignalStrategy::DISCONNECTED) {
    HOST_LOG << "Signaling disconnected. error="
             << SignalStrategyErrorToString(signal_strategy_->GetError());
    reconnect_attempts_++;

    if (signal_strategy_->GetError() == SignalStrategy::AUTHENTICATION_FAILED)
      oauth_token_getter_->InvalidateCache();

    ScheduleTryReconnect();
  }
}

bool SignalingConnector::OnSignalStrategyIncomingStanza(
    const buzz::XmlElement* stanza) {
  return false;
}

void SignalingConnector::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (type != net::NetworkChangeNotifier::CONNECTION_NONE &&
      signal_strategy_->GetState() == SignalStrategy::DISCONNECTED) {
    HOST_LOG << "Network state changed to online.";
    ResetAndTryReconnect();
  }
}

void SignalingConnector::OnAccessToken(OAuthTokenGetter::Status status,
                                       const std::string& user_email,
                                       const std::string& access_token) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (status == OAuthTokenGetter::AUTH_ERROR) {
    auth_failed_callback_.Run();
    return;
  } else if (status == OAuthTokenGetter::NETWORK_ERROR) {
    OnNetworkError();
    return;
  }

  DCHECK_EQ(status, OAuthTokenGetter::SUCCESS);
  HOST_LOG << "Received user info.";

  signal_strategy_->SetAuthInfo(user_email, access_token);

  // Now that we've refreshed the token and verified that it's for the correct
  // user account, try to connect using the new token.
  DCHECK_EQ(signal_strategy_->GetState(), SignalStrategy::DISCONNECTED);
  signal_strategy_->Connect();
}

void SignalingConnector::OnNetworkError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  reconnect_attempts_++;
  ScheduleTryReconnect();
}

void SignalingConnector::ScheduleTryReconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (timer_.IsRunning() || net::NetworkChangeNotifier::IsOffline())
    return;
  int delay_s = std::min(1 << reconnect_attempts_,
                         kMaxReconnectDelaySeconds);
  timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(delay_s),
               this, &SignalingConnector::TryReconnect);
}

void SignalingConnector::ResetAndTryReconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  signal_strategy_->Disconnect();
  reconnect_attempts_ = 0;
  timer_.Stop();
  ScheduleTryReconnect();
}

void SignalingConnector::TryReconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(dns_blackhole_checker_.get());

  // This will check if this machine is allowed to access the chromoting
  // host talkgadget.
  dns_blackhole_checker_->CheckForDnsBlackhole(
      base::Bind(&SignalingConnector::OnDnsBlackholeCheckerDone,
                 base::Unretained(this)));
}

void SignalingConnector::OnDnsBlackholeCheckerDone(bool allow) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Unable to access the host talkgadget. Don't allow the connection, but
  // schedule a reconnect in case this is a transient problem rather than
  // an outright block.
  if (!allow) {
    reconnect_attempts_++;
    HOST_LOG << "Talkgadget check failed. Scheduling reconnect. Attempt "
              << reconnect_attempts_;
    ScheduleTryReconnect();
    return;
  }

  if (signal_strategy_->GetState() == SignalStrategy::DISCONNECTED) {
    HOST_LOG << "Attempting to connect signaling.";
    oauth_token_getter_->CallWithToken(base::Bind(
        &SignalingConnector::OnAccessToken, weak_factory_.GetWeakPtr()));
  }
}

}  // namespace remoting
