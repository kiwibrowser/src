// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_utils.h"

#include <sstream>
#include <string>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/numerics/safe_math.h"
#include "base/strings/string_util.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/origin_util.h"
#include "net/http/http_byte_range.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/features.h"

namespace content {

namespace {

bool PathContainsDisallowedCharacter(const GURL& url) {
  std::string path = url.path();
  DCHECK(base::IsStringUTF8(path));

  // We should avoid these escaped characters in the path component because
  // these can be handled differently depending on server implementation.
  if (path.find("%2f") != std::string::npos ||
      path.find("%2F") != std::string::npos) {
    return true;
  }
  if (path.find("%5c") != std::string::npos ||
      path.find("%5C") != std::string::npos) {
    return true;
  }
  return false;
}

}  // namespace

// static
bool ServiceWorkerUtils::ScopeMatches(const GURL& scope, const GURL& url) {
  DCHECK(!scope.has_ref());
  return base::StartsWith(url.spec(), scope.spec(),
                          base::CompareCase::SENSITIVE);
}

// static
bool ServiceWorkerUtils::IsPathRestrictionSatisfied(
    const GURL& scope,
    const GURL& script_url,
    const std::string* service_worker_allowed_header_value,
    std::string* error_message) {
  DCHECK(scope.is_valid());
  DCHECK(!scope.has_ref());
  DCHECK(script_url.is_valid());
  DCHECK(!script_url.has_ref());
  DCHECK(error_message);

  if (ContainsDisallowedCharacter(scope, script_url, error_message))
    return false;

  std::string max_scope_string;
  if (service_worker_allowed_header_value) {
    GURL max_scope = script_url.Resolve(*service_worker_allowed_header_value);
    if (!max_scope.is_valid()) {
      *error_message = "An invalid Service-Worker-Allowed header value ('";
      error_message->append(*service_worker_allowed_header_value);
      error_message->append("') was received when fetching the script.");
      return false;
    }
    max_scope_string = max_scope.path();
  } else {
    max_scope_string = script_url.GetWithoutFilename().path();
  }

  std::string scope_string = scope.path();
  if (!base::StartsWith(scope_string, max_scope_string,
                        base::CompareCase::SENSITIVE)) {
    *error_message = "The path of the provided scope ('";
    error_message->append(scope_string);
    error_message->append("') is not under the max scope allowed (");
    if (service_worker_allowed_header_value)
      error_message->append("set by Service-Worker-Allowed: ");
    error_message->append("'");
    error_message->append(max_scope_string);
    error_message->append(
        "'). Adjust the scope, move the Service Worker script, or use the "
        "Service-Worker-Allowed HTTP header to allow the scope.");
    return false;
  }
  return true;
}

// static
bool ServiceWorkerUtils::ContainsDisallowedCharacter(
    const GURL& scope,
    const GURL& script_url,
    std::string* error_message) {
  if (PathContainsDisallowedCharacter(scope) ||
      PathContainsDisallowedCharacter(script_url)) {
    *error_message = "The provided scope ('";
    error_message->append(scope.spec());
    error_message->append("') or scriptURL ('");
    error_message->append(script_url.spec());
    error_message->append("') includes a disallowed escape character.");
    return true;
  }
  return false;
}

// static
bool ServiceWorkerUtils::AllOriginsMatchAndCanAccessServiceWorkers(
    const std::vector<GURL>& urls) {
  // (A) Check if all origins can access service worker. Every URL must be
  // checked despite the same-origin check below in (B), because GetOrigin()
  // uses the inner URL for filesystem URLs so that https://foo/ and
  // filesystem:https://foo/ are considered equal, but filesystem URLs cannot
  // access service worker.
  for (const GURL& url : urls) {
    if (!OriginCanAccessServiceWorkers(url))
      return false;
  }

  // (B) Check if all origins are equal. Cross-origin access is permitted when
  // --disable-web-security is set.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableWebSecurity)) {
    return true;
  }
  const GURL& first = urls.front();
  for (const GURL& url : urls) {
    if (first.GetOrigin() != url.GetOrigin())
      return false;
  }
  return true;
}

// static
bool ServiceWorkerUtils::IsServicificationEnabled() {
  return base::FeatureList::IsEnabled(network::features::kNetworkService) ||
         base::FeatureList::IsEnabled(features::kServiceWorkerServicification);
}

// static
std::string ServiceWorkerUtils::ErrorTypeToString(
    blink::mojom::ServiceWorkerErrorType error) {
  std::ostringstream oss;
  oss << error;
  return oss.str();
}

// static
std::string ServiceWorkerUtils::ClientTypeToString(
    blink::mojom::ServiceWorkerClientType type) {
  std::ostringstream oss;
  oss << type;
  return oss.str();
}

bool ServiceWorkerUtils::ExtractSinglePartHttpRange(
    const net::HttpRequestHeaders& headers,
    bool* has_range_out,
    uint64_t* offset_out,
    uint64_t* length_out) {
  std::string range_header;
  *has_range_out = false;
  if (!headers.GetHeader(net::HttpRequestHeaders::kRange, &range_header))
    return true;

  std::vector<net::HttpByteRange> ranges;
  if (!net::HttpUtil::ParseRangeHeader(range_header, &ranges))
    return true;

  // Multi-part (or invalid) ranges are not supported.
  if (ranges.size() != 1)
    return false;

  // Safely parse the single range to our more-sane output format.
  *has_range_out = true;
  const net::HttpByteRange& byte_range = ranges[0];
  if (byte_range.first_byte_position() < 0)
    return false;
  // Allow the range [0, -1] to be valid and specify the entire range.
  if (byte_range.first_byte_position() == 0 &&
      byte_range.last_byte_position() == -1) {
    *has_range_out = false;
    return true;
  }
  if (byte_range.last_byte_position() < 0)
    return false;

  uint64_t first_byte_position =
      static_cast<uint64_t>(byte_range.first_byte_position());
  uint64_t last_byte_position =
      static_cast<uint64_t>(byte_range.last_byte_position());

  base::CheckedNumeric<uint64_t> length = last_byte_position;
  length -= first_byte_position;
  length += 1;

  if (!length.IsValid())
    return false;

  *offset_out = static_cast<uint64_t>(byte_range.first_byte_position());
  *length_out = length.ValueOrDie();
  return true;
}

bool ServiceWorkerUtils::ShouldBypassCacheDueToUpdateViaCache(
    bool is_main_script,
    blink::mojom::ServiceWorkerUpdateViaCache cache_mode) {
  switch (cache_mode) {
    case blink::mojom::ServiceWorkerUpdateViaCache::kImports:
      return is_main_script;
    case blink::mojom::ServiceWorkerUpdateViaCache::kNone:
      return true;
    case blink::mojom::ServiceWorkerUpdateViaCache::kAll:
      return false;
  }
  NOTREACHED() << static_cast<int>(cache_mode);
  return false;
}

bool LongestScopeMatcher::MatchLongest(const GURL& scope) {
  if (!ServiceWorkerUtils::ScopeMatches(scope, url_))
    return false;
  if (match_.is_empty() || match_.spec().size() < scope.spec().size()) {
    match_ = scope;
    return true;
  }
  return false;
}

}  // namespace content
