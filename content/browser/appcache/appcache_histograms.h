// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_HISTOGRAMS_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_HISTOGRAMS_H_

#include "base/macros.h"
#include "content/browser/appcache/appcache_update_job.h"

namespace base {
class TimeDelta;
}

namespace url {
class Origin;
}

namespace content {

class AppCacheHistograms {
 public:
  enum InitResultType {
    INIT_OK, SQL_DATABASE_ERROR, DISK_CACHE_ERROR,
    NUM_INIT_RESULT_TYPES
  };
  static void CountInitResult(InitResultType init_result);
  static void CountReinitAttempt(bool repeated_attempt);
  static void CountCorruptionDetected();
  static void CountUpdateJobResult(AppCacheUpdateJob::ResultType result,
                                   const url::Origin& origin_url);
  enum CheckResponseResultType {
    RESPONSE_OK, MANIFEST_OUT_OF_DATE, RESPONSE_OUT_OF_DATE, ENTRY_NOT_FOUND,
    READ_HEADERS_ERROR, READ_DATA_ERROR, UNEXPECTED_DATA_SIZE, CHECK_CANCELED,
    NUM_CHECK_RESPONSE_RESULT_TYPES
  };
  static void CountCheckResponseResult(CheckResponseResultType result);
  static void CountResponseRetrieval(bool success,
                                     bool is_main_resource,
                                     const url::Origin& origin_url);
  static void LogUpdateFailureStats(const url::Origin& origin_url,
                                    int percent_complete,
                                    bool was_making_progress,
                                    bool off_origin_resource_failure);
  static void AddTaskQueueTimeSample(const base::TimeDelta& duration);
  static void AddTaskRunTimeSample(const base::TimeDelta& duration);
  static void AddCompletionQueueTimeSample(const base::TimeDelta& duration);
  static void AddCompletionRunTimeSample(const base::TimeDelta& duration);
  static void AddNetworkJobStartDelaySample(const base::TimeDelta& duration);
  static void AddErrorJobStartDelaySample(const base::TimeDelta& duration);
  static void AddAppCacheJobStartDelaySample(const base::TimeDelta& duration);
  static void AddMissingManifestEntrySample();

  enum MissingManifestCallsiteType {
    CALLSITE_0, CALLSITE_1, CALLSITE_2, CALLSITE_3,
    NUM_MISSING_MANIFEST_CALLSITE_TYPES
  };
  static void AddMissingManifestDetectedAtCallsite(
      MissingManifestCallsiteType type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AppCacheHistograms);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_HISTOGRAMS_H_
