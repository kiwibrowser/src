// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/manifest/manifest_uma_util.h"

#include "base/metrics/histogram_macros.h"
#include "third_party/blink/public/common/manifest/manifest.h"

namespace content {

namespace {

static const char kUMANameParseSuccess[] = "Manifest.ParseSuccess";
static const char kUMANameFetchResult[] = "Manifest.FetchResult";

// Enum for UMA purposes, make sure you update histograms.xml if you add new
// result types. Never delete or reorder an entry; only add new entries
// immediately before MANIFEST_FETCH_RESULT_TYPE_COUNT.
enum ManifestFetchResultType {
  MANIFEST_FETCH_SUCCESS = 0,
  MANIFEST_FETCH_ERROR_EMPTY_URL = 1,
  MANIFEST_FETCH_ERROR_UNSPECIFIED = 2,
  MANIFEST_FETCH_ERROR_FROM_UNIQUE_ORIGIN = 3,

  // Must stay at the end.
  MANIFEST_FETCH_RESULT_TYPE_COUNT
};

} // anonymous namespace

void ManifestUmaUtil::ParseSucceeded(const blink::Manifest& manifest) {
  UMA_HISTOGRAM_BOOLEAN(kUMANameParseSuccess, true);
  UMA_HISTOGRAM_BOOLEAN("Manifest.IsEmpty", manifest.IsEmpty());
  if (manifest.IsEmpty())
    return;

  UMA_HISTOGRAM_BOOLEAN("Manifest.HasProperty.name", !manifest.name.is_null());
  UMA_HISTOGRAM_BOOLEAN("Manifest.HasProperty.short_name",
      !manifest.short_name.is_null());
  UMA_HISTOGRAM_BOOLEAN("Manifest.HasProperty.start_url",
      !manifest.start_url.is_empty());
  UMA_HISTOGRAM_BOOLEAN("Manifest.HasProperty.display",
                        manifest.display != blink::kWebDisplayModeUndefined);
  UMA_HISTOGRAM_BOOLEAN(
      "Manifest.HasProperty.orientation",
      manifest.orientation != blink::kWebScreenOrientationLockDefault);
  UMA_HISTOGRAM_BOOLEAN("Manifest.HasProperty.icons", !manifest.icons.empty());
  UMA_HISTOGRAM_BOOLEAN("Manifest.HasProperty.share_target",
                        manifest.share_target.has_value());
  UMA_HISTOGRAM_BOOLEAN("Manifest.HasProperty.gcm_sender_id",
      !manifest.gcm_sender_id.is_null());
}

void ManifestUmaUtil::ParseFailed() {
  UMA_HISTOGRAM_BOOLEAN(kUMANameParseSuccess, false);
}

void ManifestUmaUtil::FetchSucceeded() {
  UMA_HISTOGRAM_ENUMERATION(kUMANameFetchResult,
                            MANIFEST_FETCH_SUCCESS,
                            MANIFEST_FETCH_RESULT_TYPE_COUNT);
}

void ManifestUmaUtil::FetchFailed(FetchFailureReason reason) {
  ManifestFetchResultType fetch_result_type = MANIFEST_FETCH_RESULT_TYPE_COUNT;
  switch (reason) {
    case FETCH_EMPTY_URL:
      fetch_result_type = MANIFEST_FETCH_ERROR_EMPTY_URL;
      break;
    case FETCH_FROM_UNIQUE_ORIGIN:
      fetch_result_type = MANIFEST_FETCH_ERROR_FROM_UNIQUE_ORIGIN;
      break;
    case FETCH_UNSPECIFIED_REASON:
      fetch_result_type = MANIFEST_FETCH_ERROR_UNSPECIFIED;
      break;
  }
  DCHECK_NE(fetch_result_type, MANIFEST_FETCH_RESULT_TYPE_COUNT);

  UMA_HISTOGRAM_ENUMERATION(kUMANameFetchResult,
                            fetch_result_type,
                            MANIFEST_FETCH_RESULT_TYPE_COUNT);
}

} // namespace content
