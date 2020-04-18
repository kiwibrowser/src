// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_reporting.h"

#include <string>

#include "base/metrics/histogram_macros.h"
#include "content/browser/indexed_db/indexed_db_leveldb_coding.h"
#include "url/origin.h"

namespace content {
namespace indexed_db {

namespace {

std::string OriginToCustomHistogramSuffix(const url::Origin& origin) {
  if (origin.host() == "docs.google.com")
    return ".Docs";
  return std::string();
}

}  // namespace

void HistogramOpenStatus(IndexedDBBackingStoreOpenResult result,
                         const url::Origin& origin) {
  UMA_HISTOGRAM_ENUMERATION("WebCore.IndexedDB.BackingStore.OpenStatus", result,
                            INDEXED_DB_BACKING_STORE_OPEN_MAX);
  const std::string suffix = OriginToCustomHistogramSuffix(origin);
  // Data from the WebCore.IndexedDB.BackingStore.OpenStatus histogram is used
  // to generate a graph. So as not to alter the meaning of that graph,
  // continue to collect all stats there (above) but also now collect docs stats
  // separately (below).
  if (!suffix.empty()) {
    base::LinearHistogram::FactoryGet(
        "WebCore.IndexedDB.BackingStore.OpenStatus" + suffix, 1,
        INDEXED_DB_BACKING_STORE_OPEN_MAX,
        INDEXED_DB_BACKING_STORE_OPEN_MAX + 1,
        base::HistogramBase::kUmaTargetedHistogramFlag)
        ->Add(result);
  }
}

void ReportInternalError(const char* type,
                         IndexedDBBackingStoreErrorSource location) {
  std::string name;
  name.append("WebCore.IndexedDB.BackingStore.").append(type).append("Error");
  base::Histogram::FactoryGet(name, 1, INTERNAL_ERROR_MAX,
                              INTERNAL_ERROR_MAX + 1,
                              base::HistogramBase::kUmaTargetedHistogramFlag)
      ->Add(location);
}

void ReportSchemaVersion(int version, const url::Origin& origin) {
  UMA_HISTOGRAM_ENUMERATION("WebCore.IndexedDB.SchemaVersion", version,
                            kLatestKnownSchemaVersion + 1);
  const std::string suffix = OriginToCustomHistogramSuffix(origin);
  if (!suffix.empty()) {
    base::LinearHistogram::FactoryGet(
        "WebCore.IndexedDB.SchemaVersion" + suffix, 0,
        indexed_db::kLatestKnownSchemaVersion,
        indexed_db::kLatestKnownSchemaVersion + 1,
        base::HistogramBase::kUmaTargetedHistogramFlag)
        ->Add(version);
  }
}

void ReportV2Schema(bool has_broken_blobs, const url::Origin& origin) {
  UMA_HISTOGRAM_BOOLEAN("WebCore.IndexedDB.SchemaV2HasBlobs", has_broken_blobs);
  const std::string suffix = OriginToCustomHistogramSuffix(origin);
  if (!suffix.empty()) {
    base::BooleanHistogram::FactoryGet(
        "WebCore.IndexedDB.SchemaV2HasBlobs" + suffix,
        base::HistogramBase::kUmaTargetedHistogramFlag)
        ->Add(has_broken_blobs);
  }
}

}  // namespace indexed_db
}  // namespace content
