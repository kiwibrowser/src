// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MANIFEST_MANIFEST_UMA_UTIL_H_
#define CONTENT_RENDERER_MANIFEST_MANIFEST_UMA_UTIL_H_

namespace blink {
struct Manifest;
}

namespace content {

class ManifestUmaUtil {
 public:
  enum FetchFailureReason {
    FETCH_EMPTY_URL = 0,
    FETCH_FROM_UNIQUE_ORIGIN,
    FETCH_UNSPECIFIED_REASON
  };

  // Record that the Manifest was successfully parsed. If it is an empty
  // Manifest, it will recorded as so and nothing will happen. Otherwise, the
  // presence of each properties will be recorded.
  static void ParseSucceeded(const blink::Manifest& manifest);

  // Record that the Manifest parsing failed.
  static void ParseFailed();

  // Record that the Manifest fetching succeeded.
  static void FetchSucceeded();

  // Record that the Manifest fetching failed and takes the |reason| why it
  // failed.
  static void FetchFailed(FetchFailureReason reason);
};

} // namespace content

#endif  // CONTENT_RENDERER_MANIFEST_MANIFEST_UMA_UTIL_H_
