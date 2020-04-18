// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_driver_switches.h"

namespace switches {

// Allows overriding the deferred init fallback timeout.
const char kSyncDeferredStartupTimeoutSeconds[] =
    "sync-deferred-startup-timeout-seconds";

// Enables deferring sync backend initialization until user initiated changes
// occur.
const char kSyncDisableDeferredStartup[] = "sync-disable-deferred-startup";

// Enables feature to avoid unnecessary GetUpdate requests.
const char kSyncEnableGetUpdateAvoidance[] = "sync-enable-get-update-avoidance";

// Overrides the default server used for profile sync.
const char kSyncServiceURL[] = "sync-url";

// This flag causes sync to retry very quickly (see polling_constants.h) the
// when it encounters an error, as the first step towards exponential backoff.
const char kSyncShortInitialRetryOverride[] =
    "sync-short-initial-retry-override";

// This flag significantly shortens the delay between nudge cycles. Its primary
// purpose is to speed up integration tests. The normal delay allows coalescing
// and prevention of server overload, so don't use this unless you're really
// sure
// that it's what you want.
const char kSyncShortNudgeDelayForTest[] = "sync-short-nudge-delay-for-test";

// Enables clearing of sync data when a user enables passphrase encryption.
const base::Feature kSyncClearDataOnPassphraseEncryption{
    "ClearSyncDataOnPassphraseEncryption", base::FEATURE_DISABLED_BY_DEFAULT};

// Gates registration and construction of user events machinery. Enabled by
// default as each use case should have their own gating feature as well.
const base::Feature kSyncUserEvents{"SyncUserEvents",
                                    base::FEATURE_ENABLED_BY_DEFAULT};

// Gates emission of FieldTrial events.
const base::Feature kSyncUserFieldTrialEvents{"SyncUserFieldTrialEvents",
                                              base::FEATURE_ENABLED_BY_DEFAULT};

// Gates emission of UserConsent events.
const base::Feature kSyncUserConsentEvents{"SyncUserConsentEvents",
                                           base::FEATURE_ENABLED_BY_DEFAULT};

// Gates registration for user language detection events.
const base::Feature kSyncUserLanguageDetectionEvents{
    "SyncUserLanguageDetectionEvents", base::FEATURE_DISABLED_BY_DEFAULT};

// Gates registration for user translation events.
const base::Feature kSyncUserTranslationEvents{
    "SyncUserTranslationEvents", base::FEATURE_DISABLED_BY_DEFAULT};

// Enable USS implementation of Bookmarks datatype.
const base::Feature kSyncUSSBookmarks{"SyncUSSBookmarks",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

// Enable USS implementation of sessions.
const base::Feature kSyncUSSSessions{"SyncUSSSessions",
                                     base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace switches
