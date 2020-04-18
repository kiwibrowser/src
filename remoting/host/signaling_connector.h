// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_SIGNALING_CONNECTOR_H_
#define REMOTING_HOST_SIGNALING_CONNECTOR_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "net/base/network_change_notifier.h"
#include "remoting/base/oauth_token_getter.h"
#include "remoting/signaling/xmpp_signal_strategy.h"

namespace remoting {

class DnsBlackholeChecker;

// SignalingConnector listens for SignalStrategy status notifications
// and attempts to keep it connected when possible. When signalling is
// not connected it keeps trying to reconnect it until it is
// connected. It limits connection attempt rate using exponential
// backoff. It also monitors network state and reconnects signalling
// whenever connection type changes or IP address changes.
class SignalingConnector
    : public SignalStrategy::Listener,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  // The |auth_failed_callback| is called when authentication fails.
  SignalingConnector(XmppSignalStrategy* signal_strategy,
                     std::unique_ptr<DnsBlackholeChecker> dns_blackhole_checker,
                     OAuthTokenGetter* oauth_token_getter,
                     const base::Closure& auth_failed_callback);
  ~SignalingConnector() override;

  // OAuthTokenGetter callback.
  void OnAccessToken(OAuthTokenGetter::Status status,
                     const std::string& user_email,
                     const std::string& access_token);

  // SignalStrategy::Listener interface.
  void OnSignalStrategyStateChange(SignalStrategy::State state) override;
  bool OnSignalStrategyIncomingStanza(const buzz::XmlElement* stanza) override;

  // NetworkChangeNotifier::NetworkChangeObserver interface.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

 private:
  void OnNetworkError();
  void ScheduleTryReconnect();
  void ResetAndTryReconnect();
  void TryReconnect();
  void OnDnsBlackholeCheckerDone(bool allow);

  XmppSignalStrategy* signal_strategy_;
  base::Closure auth_failed_callback_;
  std::unique_ptr<DnsBlackholeChecker> dns_blackhole_checker_;

  OAuthTokenGetter* oauth_token_getter_;

  // Number of times we tried to connect without success.
  int reconnect_attempts_;

  base::OneShotTimer timer_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<SignalingConnector> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SignalingConnector);
};

}  // namespace remoting

#endif  // REMOTING_HOST_SIGNALING_CONNECTOR_H_
