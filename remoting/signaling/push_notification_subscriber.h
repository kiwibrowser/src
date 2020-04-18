// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_GCD_NOTIFICATION_SUBSCRIBER_H_
#define REMOTING_HOST_GCD_NOTIFICATION_SUBSCRIBER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "remoting/signaling/signal_strategy.h"

namespace remoting {

class IqSender;
class IqRequest;

// An object that subscribes to push notifications using an XMPP
// channel.  The notifications themselves are ignored, but creating a
// subscription is necessary, e.g., for GCD to see a device as online.
class PushNotificationSubscriber : public SignalStrategy::Listener {
 public:
  struct Subscription {
    Subscription();
    ~Subscription();

    std::string channel;
    std::string from;
  };

  typedef std::vector<Subscription> SubscriptionList;

  PushNotificationSubscriber(SignalStrategy* signal_strategy,
                             const SubscriptionList& subscriptions);
  ~PushNotificationSubscriber() override;

 private:
  // SignalStrategy::Listener interface.
  void OnSignalStrategyStateChange(SignalStrategy::State state) override;
  bool OnSignalStrategyIncomingStanza(const buzz::XmlElement* stanza) override;

  void Subscribe(const Subscription& subscription);
  void OnSubscriptionResult(IqRequest* request,
                            const buzz::XmlElement* response);

  SignalStrategy* signal_strategy_;
  SubscriptionList subscriptions_;
  std::unique_ptr<IqSender> iq_sender_;
  std::unique_ptr<IqRequest> iq_request_;

  DISALLOW_COPY_AND_ASSIGN(PushNotificationSubscriber);
};

}  // namespace remoting

#endif  // REMOTING_HOST_GCD_NOTIFICATION_SUBSCRIBER_H_
