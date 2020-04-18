// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_CORS_CORS_STATUS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_CORS_CORS_STATUS_H_

namespace blink {

enum class CORSStatus {
  kUnknown,        // Status not determined - not supposed to be seen by users.
  kNotApplicable,  // E.g. for main resources or if not in fetch mode CORS.

  // Response not handled by service worker:
  kSameOrigin,  // Request was same origin.
  kSuccessful,  // Request was cross origin and CORS checks passed.
  kFailed,      // Request was cross origin and CORS checks failed.

  // Response handled by service worker:
  kServiceWorkerSuccessful,  // ResponseType other than opaque (including
                             // error).
  kServiceWorkerOpaque,      // ResponseType was opaque.
};

}  // namespace blink

#endif
