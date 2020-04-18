/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_URL_ERROR_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_URL_ERROR_H_

#include "base/logging.h"
#include "base/optional.h"
#include "services/network/public/cpp/cors/cors_error_status.h"
#include "third_party/blink/public/platform/web_url.h"

namespace blink {

// TODO(yhirano): Change this to a class.
struct WebURLError {
 public:
  enum class HasCopyInCache {
    kFalse,
    kTrue,
  };
  enum class IsWebSecurityViolation {
    kFalse,
    kTrue,
  };

  WebURLError() = delete;
  // |reason| must not be 0.
  BLINK_PLATFORM_EXPORT WebURLError(int reason, const WebURL&);
  // |reason| must not be 0.
  BLINK_PLATFORM_EXPORT WebURLError(int reason,
                                    int extended_reason,
                                    HasCopyInCache,
                                    IsWebSecurityViolation,
                                    const WebURL&);
  BLINK_PLATFORM_EXPORT WebURLError(const network::CORSErrorStatus&,
                                    HasCopyInCache,
                                    const WebURL&);

  int reason() const { return reason_; }
  int extended_reason() const { return extended_reason_; }
  bool has_copy_in_cache() const { return has_copy_in_cache_; }
  bool is_web_security_violation() const { return is_web_security_violation_; }
  const WebURL& url() const { return url_; }
  const base::Optional<network::CORSErrorStatus> cors_error_status() const {
    return cors_error_status_;
  }

 private:
  // A numeric error code detailing the reason for this error. The value must
  // not be 0.
  int reason_;

  // Additional information based on the reason_.
  int extended_reason_ = 0;

  // A flag showing whether or not we have a (possibly stale) copy of the
  // requested resource in the cache.
  bool has_copy_in_cache_ = false;

  // True if this error is created for a web security violation.
  bool is_web_security_violation_ = false;

  // The url that failed to load.
  WebURL url_;

  // Optional CORS error details.
  base::Optional<network::CORSErrorStatus> cors_error_status_;
};

}  // namespace blink

#endif
