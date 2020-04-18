// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/ntp_snippets_constants.h"

namespace ntp_snippets {

const base::FilePath::CharType kDatabaseFolder[] =
    FILE_PATH_LITERAL("NTPSnippets");

const char kContentSuggestionsApiScope[] =
    "https://www.googleapis.com/auth/chrome-content-suggestions";

const char kContentSuggestionsServer[] =
    "https://chromecontentsuggestions-pa.googleapis.com/v1/suggestions/fetch";
const char kContentSuggestionsStagingServer[] =
    "https://staging-chromecontentsuggestions-pa.googleapis.com/v1/suggestions/"
    "fetch";
const char kContentSuggestionsAlphaServer[] =
    "https://alpha-chromecontentsuggestions-pa.sandbox.googleapis.com/v1/"
    "suggestions/fetch";

const char kPushUpdatesSubscriptionServer[] =
    "https://chromecontentsuggestions-pa.googleapis.com/v1/suggestions/"
    "subscribe";
const char kPushUpdatesSubscriptionStagingServer[] =
    "https://staging-chromecontentsuggestions-pa.googleapis.com/v1/suggestions/"
    "subscribe";
const char kPushUpdatesSubscriptionAlphaServer[] =
    "https://alpha-chromecontentsuggestions-pa.sandbox.googleapis.com/v1/"
    "suggestions/subscribe";

const char kPushUpdatesUnsubscriptionServer[] =
    "https://chromecontentsuggestions-pa.googleapis.com/v1/suggestions/"
    "unsubscribe";
const char kPushUpdatesUnsubscriptionStagingServer[] =
    "https://staging-chromecontentsuggestions-pa.googleapis.com/v1/suggestions/"
    "unsubscribe";
const char kPushUpdatesUnsubscriptionAlphaServer[] =
    "https://alpha-chromecontentsuggestions-pa.sandbox.googleapis.com/v1/"
    "suggestions/unsubscribe";

}  // namespace ntp_snippets
