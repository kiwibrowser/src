// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/notifier_reason_util.h"

#include "base/logging.h"

namespace syncer {

InvalidatorState FromNotifierReason(
    notifier::NotificationsDisabledReason reason) {
  switch (reason) {
    case notifier::NO_NOTIFICATION_ERROR:
      return INVALIDATIONS_ENABLED;
    case notifier::TRANSIENT_NOTIFICATION_ERROR:
      return TRANSIENT_INVALIDATION_ERROR;
    case notifier::NOTIFICATION_CREDENTIALS_REJECTED:
      return INVALIDATION_CREDENTIALS_REJECTED;
    default:
      NOTREACHED();
      return DEFAULT_INVALIDATION_ERROR;
  }
}

notifier::NotificationsDisabledReason ToNotifierReasonForTest(
    InvalidatorState state) {
  switch (state) {
    case TRANSIENT_INVALIDATION_ERROR:
      return notifier::TRANSIENT_NOTIFICATION_ERROR;
    case INVALIDATION_CREDENTIALS_REJECTED:
      return notifier::NOTIFICATION_CREDENTIALS_REJECTED;
    case INVALIDATIONS_ENABLED:
      // Fall through.
    default:
      NOTREACHED();
      return notifier::TRANSIENT_NOTIFICATION_ERROR;
  }
}

}  // namespace syncer
