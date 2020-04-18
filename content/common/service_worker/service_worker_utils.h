// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_UTILS_H_
#define CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_UTILS_H_

#include "base/command_line.h"
#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/resource_type.h"
#include "net/http/http_request_headers.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_error_type.mojom.h"
#include "url/gurl.h"

namespace content {

class ServiceWorkerUtils {
 public:
  static bool IsMainResourceType(ResourceType type) {
    return IsResourceTypeFrame(type) || type == RESOURCE_TYPE_SHARED_WORKER;
  }

  // Returns true if |scope| matches |url|.
  CONTENT_EXPORT static bool ScopeMatches(const GURL& scope, const GURL& url);

  // Returns true if the script at |script_url| is allowed to control |scope|
  // according to Service Worker's path restriction policy. If
  // |service_worker_allowed| is not null, it points to the
  // Service-Worker-Allowed header value.
  CONTENT_EXPORT static bool IsPathRestrictionSatisfied(
      const GURL& scope,
      const GURL& script_url,
      const std::string* service_worker_allowed_header_value,
      std::string* error_message);

  static bool ContainsDisallowedCharacter(const GURL& scope,
                                          const GURL& script_url,
                                          std::string* error_message);

  // Returns true if all members of |urls| have the same origin, and
  // OriginCanAccessServiceWorkers is true for this origin.
  // If --disable-web-security is enabled, the same origin check is
  // not performed.
  CONTENT_EXPORT static bool AllOriginsMatchAndCanAccessServiceWorkers(
      const std::vector<GURL>& urls);

  // Returns true if servicified service worker is enabled.
  CONTENT_EXPORT static bool IsServicificationEnabled();

  // Returns true if the |provider_id| was assigned by the browser process.
  static bool IsBrowserAssignedProviderId(int provider_id) {
    return provider_id < kInvalidServiceWorkerProviderId;
  }

  static std::string ErrorTypeToString(
      blink::mojom::ServiceWorkerErrorType error);

  static std::string ClientTypeToString(
      blink::mojom::ServiceWorkerClientType type);

  // Sets |has_range| to true if |headers| specify a single range request, and
  // |offset| and |size| to the range. Returns true on valid input (regardless
  // of |has_range|), and false if there is more than one range or if the bounds
  // overflow.
  static bool ExtractSinglePartHttpRange(const net::HttpRequestHeaders& headers,
                                         bool* has_range_out,
                                         uint64_t* offset_out,
                                         uint64_t* size_out);

  static bool ShouldBypassCacheDueToUpdateViaCache(
      bool is_main_script,
      blink::mojom::ServiceWorkerUpdateViaCache cache_mode);
};

class CONTENT_EXPORT LongestScopeMatcher {
 public:
  explicit LongestScopeMatcher(const GURL& url) : url_(url) {}
  virtual ~LongestScopeMatcher() {}

  // Returns true if |scope| matches |url_| longer than |match_|.
  bool MatchLongest(const GURL& scope);

 private:
  const GURL url_;
  GURL match_;

  DISALLOW_COPY_AND_ASSIGN(LongestScopeMatcher);
};

}  // namespace content

#endif  // CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_UTILS_H_
