// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_LISTENER_NOTIFICATION_DEFINES_H_
#define JINGLE_NOTIFIER_LISTENER_NOTIFICATION_DEFINES_H_

#include <string>
#include <vector>

namespace notifier {

struct Subscription {
  Subscription();
  ~Subscription();
  bool Equals(const Subscription& other) const;

  // The name of the channel to subscribe to; usually but not always
  // a URL.
  std::string channel;
  // A sender, which could be a domain or a bare JID, from which we
  // will accept pushes.
  std::string from;
};

typedef std::vector<Subscription> SubscriptionList;

bool SubscriptionListsEqual(const SubscriptionList& subscriptions1,
                            const SubscriptionList& subscriptions2);

// A structure representing a <recipient/> block within a push message.
struct Recipient {
  Recipient();
  ~Recipient();
  bool Equals(const Recipient& other) const;

  // The bare jid of the recipient.
  std::string to;
  // User-specific data for the recipient.
  std::string user_specific_data;
};

typedef std::vector<Recipient> RecipientList;

bool RecipientListsEqual(const RecipientList& recipients1,
                         const RecipientList& recipients2);

struct Notification {
  Notification();
  Notification(const Notification& other);
  ~Notification();

  // The channel the notification is coming in on.
  std::string channel;
  // Recipients for this notification (may be empty).
  RecipientList recipients;
  // The notification data payload.
  std::string data;

  bool Equals(const Notification& other) const;
  std::string ToString() const;
};

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_LISTENER_NOTIFICATION_DEFINES_H_
