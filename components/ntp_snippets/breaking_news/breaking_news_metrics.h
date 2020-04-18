// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_METRICS_H_
#define COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_METRICS_H_

#include "base/optional.h"
#include "base/time/time.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/ntp_snippets/status.h"

namespace ntp_snippets {
namespace metrics {

void OnSubscriptionRequestCompleted(const Status& status);
void OnUnsubscriptionRequestCompleted(const Status& status);

// Received message action. This enum is used in a UMA histogram, therefore,
// don't remove or reorder elements, only add new ones at the end (before
// COUNT), and keep in sync with ContentSuggestionsBreakingNewsMessageAction in
// enums.xml.
enum class ReceivedMessageAction {
  NO_ACTION = 0,
  INVALID_ACTION = 1,
  PUSH_BY_VALUE = 2,
  PUSH_TO_REFRESH = 3,
  // Insert new values here!
  COUNT
};

void OnMessageReceived(ReceivedMessageAction action);

void OnTokenRetrieved(instance_id::InstanceID::Result result);

// |time_since_last_validation| can be absent for the first validation.
// |was_token_valid_before_validation| may be absent if it is not known (e.g. an
// error happened and a token was not received).
void OnTokenValidationAttempted(
    const base::Optional<base::TimeDelta>& time_since_last_validation,
    const base::Optional<bool>& was_token_valid_before_validation);

}  // namespace metrics
}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_METRICS_H_
