// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/common/webservice_search_provider.h"

#include <stddef.h>

#include <string>

#include "base/callback.h"
#include "base/strings/string_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/app_list/search/common/webservice_cache.h"
#include "chrome/browser/ui/app_list/search/common/webservice_cache_factory.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace app_list {

namespace {

const int kWebserviceQueryThrottleIntrevalInMs = 100;
const size_t kMinimumQueryLength = 3u;

bool IsSuggestPrefEnabled(Profile* profile) {
  return profile && !profile->IsOffTheRecord() && profile->GetPrefs() &&
         profile->GetPrefs()->GetBoolean(prefs::kSearchSuggestEnabled);
}

}  // namespace

WebserviceSearchProvider::WebserviceSearchProvider(Profile* profile)
    : profile_(profile),
      cache_(WebserviceCacheFactory::GetForBrowserContext(profile)),
      use_throttling_(true) {}

WebserviceSearchProvider::~WebserviceSearchProvider() {}

void WebserviceSearchProvider::StartThrottledQuery(
    const base::Closure& start_query) {
  base::TimeDelta interval =
      base::TimeDelta::FromMilliseconds(kWebserviceQueryThrottleIntrevalInMs);
  if (!use_throttling_ || base::Time::Now() - last_keytyped_ > interval) {
    query_throttler_.Stop();
    start_query.Run();
  } else {
    query_throttler_.Start(FROM_HERE, interval, start_query);
  }
  last_keytyped_ = base::Time::Now();
}

bool WebserviceSearchProvider::IsValidQuery(const base::string16& query) {
  // If |query| contains sensitive data, bail out and do not create the place
  // holder "search-web-store" result.
  if (IsSensitiveInput(query) || (query.size() < kMinimumQueryLength) ||
      !IsSuggestPrefEnabled(profile_)) {
    return false;
  }

  return true;
}

// Returns whether or not the user's input string, |query|, might contain any
// sensitive information, based purely on its value and not where it came from.
bool WebserviceSearchProvider::IsSensitiveInput(const base::string16& query) {
  const GURL query_as_url(query);
  if (!query_as_url.is_valid())
    return false;

  // The input can be interpreted as a URL. Check to see if it is potentially
  // sensitive. (Code shamelessly copied from search_provider.cc's
  // IsQuerySuitableForSuggest function.)

  // First we check the scheme: if this looks like a URL with a scheme that is
  // file, we shouldn't send it. Sending such things is a waste of time and a
  // disclosure of potentially private, local data. If the scheme is OK, we
  // still need to check other cases below.
  if (base::LowerCaseEqualsASCII(query_as_url.scheme(), url::kFileScheme))
    return true;

  // Don't send URLs with usernames, queries or refs. Some of these are
  // private, and the Suggest server is unlikely to have any useful results
  // for any of them. Also don't send URLs with ports, as we may initially
  // think that a username + password is a host + port (and we don't want to
  // send usernames/passwords), and even if the port really is a port, the
  // server is once again unlikely to have and useful results.
  if (!query_as_url.username().empty() ||
      !query_as_url.port().empty() ||
      !query_as_url.query().empty() ||
      !query_as_url.ref().empty()) {
    return true;
  }

  // Don't send anything for https except the hostname. Hostnames are OK
  // because they are visible when the TCP connection is established, but the
  // specific path may reveal private information.
  if (base::LowerCaseEqualsASCII(query_as_url.scheme(), url::kHttpsScheme) &&
      !query_as_url.path().empty() && query_as_url.path() != "/") {
    return true;
  }

  return false;
}

}  // namespace app_list
