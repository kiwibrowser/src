// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_SUBSCRIPTION_MANAGER_H_
#define COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_SUBSCRIPTION_MANAGER_H_

#include <string>

#include "components/version_info/version_info.h"
#include "url/gurl.h"

namespace ntp_snippets {

// Returns the appropriate API endpoint for subscribing for push updates, in
// consideration of the channel and field trial parameters.
GURL GetPushUpdatesSubscriptionEndpoint(version_info::Channel channel);

// Returns the appropriate API endpoint for unsubscribing for push updates, in
// consideration of the channel and field trial parameters.
GURL GetPushUpdatesUnsubscriptionEndpoint(version_info::Channel channel);

// Handles subscription to content suggestions server for push updates (e.g. via
// GCM).
class SubscriptionManager {
 public:
  virtual ~SubscriptionManager() = default;

  virtual void Subscribe(const std::string& token) = 0;
  virtual void Unsubscribe() = 0;
  virtual bool IsSubscribed() = 0;

  virtual void Resubscribe(const std::string& new_token) = 0;

  // Checks if some data that has been used when subscribing has changed. For
  // example, the user has signed in.
  virtual bool NeedsToResubscribe() = 0;
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_SUBSCRIPTION_MANAGER_H_
