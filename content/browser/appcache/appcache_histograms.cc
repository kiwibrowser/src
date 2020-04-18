// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_histograms.h"

#include "base/metrics/histogram_macros.h"
#include "content/public/common/origin_util.h"
#include "url/origin.h"

namespace content {

static std::string OriginToCustomHistogramSuffix(const url::Origin& origin) {
  if (origin.host() == "docs.google.com")
    return ".Docs";
  return std::string();
}

void AppCacheHistograms::CountInitResult(InitResultType init_result) {
  UMA_HISTOGRAM_ENUMERATION(
       "appcache.InitResult",
       init_result, NUM_INIT_RESULT_TYPES);
}

void AppCacheHistograms::CountReinitAttempt(bool repeated_attempt) {
  UMA_HISTOGRAM_BOOLEAN("appcache.ReinitAttempt", repeated_attempt);
}

void AppCacheHistograms::CountCorruptionDetected() {
  UMA_HISTOGRAM_BOOLEAN("appcache.CorruptionDetected", true);
}

void AppCacheHistograms::CountUpdateJobResult(
    AppCacheUpdateJob::ResultType result,
    const url::Origin& origin) {
  UMA_HISTOGRAM_ENUMERATION(
       "appcache.UpdateJobResult",
       result, AppCacheUpdateJob::NUM_UPDATE_JOB_RESULT_TYPES);

  const std::string suffix = OriginToCustomHistogramSuffix(origin);
  if (!suffix.empty()) {
    base::LinearHistogram::FactoryGet(
        "appcache.UpdateJobResult" + suffix,
        1,
        AppCacheUpdateJob::NUM_UPDATE_JOB_RESULT_TYPES,
        AppCacheUpdateJob::NUM_UPDATE_JOB_RESULT_TYPES + 1,
        base::HistogramBase::kUmaTargetedHistogramFlag)->Add(result);
  }
}

void AppCacheHistograms::CountCheckResponseResult(
    CheckResponseResultType result) {
  UMA_HISTOGRAM_ENUMERATION(
       "appcache.CheckResponseResult",
       result, NUM_CHECK_RESPONSE_RESULT_TYPES);
}

void AppCacheHistograms::CountResponseRetrieval(bool success,
                                                bool is_main_resource,
                                                const url::Origin& origin) {
  std::string label;
  if (is_main_resource) {
    label = "appcache.MainResourceResponseRetrieval";
    UMA_HISTOGRAM_BOOLEAN(label, success);

    // Also count HTTP vs HTTPS appcache usage.
    UMA_HISTOGRAM_BOOLEAN("appcache.MainPageLoad",
                          IsOriginSecure(origin.GetURL()));
  } else {
    label = "appcache.SubResourceResponseRetrieval";
    UMA_HISTOGRAM_BOOLEAN(label, success);
  }
  const std::string suffix = OriginToCustomHistogramSuffix(origin);
  if (!suffix.empty()) {
    base::BooleanHistogram::FactoryGet(
        label + suffix,
        base::HistogramBase::kUmaTargetedHistogramFlag)->Add(success);
  }
}

void AppCacheHistograms::LogUpdateFailureStats(
    const url::Origin& origin,
    int percent_complete,
    bool was_stalled,
    bool was_off_origin_resource_failure) {
  const std::string suffix = OriginToCustomHistogramSuffix(origin);

  std::string label = "appcache.UpdateProgressAtPointOfFaliure";
  UMA_HISTOGRAM_PERCENTAGE(label, percent_complete);
  if (!suffix.empty()) {
    base::LinearHistogram::FactoryGet(
        label + suffix,
        1, 101, 102,
        base::HistogramBase::kUmaTargetedHistogramFlag)->Add(percent_complete);
  }

  label = "appcache.UpdateWasStalledAtPointOfFailure";
  UMA_HISTOGRAM_BOOLEAN(label, was_stalled);
  if (!suffix.empty()) {
    base::BooleanHistogram::FactoryGet(
        label + suffix,
        base::HistogramBase::kUmaTargetedHistogramFlag)->Add(was_stalled);
  }

  label = "appcache.UpdateWasOffOriginAtPointOfFailure";
  UMA_HISTOGRAM_BOOLEAN(label, was_off_origin_resource_failure);
  if (!suffix.empty()) {
    base::BooleanHistogram::FactoryGet(
        label + suffix,
        base::HistogramBase::kUmaTargetedHistogramFlag)->Add(
            was_off_origin_resource_failure);
  }
}

void AppCacheHistograms::AddTaskQueueTimeSample(
    const base::TimeDelta& duration) {
  UMA_HISTOGRAM_TIMES("appcache.TaskQueueTime", duration);
}

void AppCacheHistograms::AddTaskRunTimeSample(
    const base::TimeDelta& duration) {
  UMA_HISTOGRAM_TIMES("appcache.TaskRunTime", duration);
}

void AppCacheHistograms::AddCompletionQueueTimeSample(
    const base::TimeDelta& duration) {
  UMA_HISTOGRAM_TIMES("appcache.CompletionQueueTime", duration);
}

void AppCacheHistograms::AddCompletionRunTimeSample(
    const base::TimeDelta& duration) {
  UMA_HISTOGRAM_TIMES("appcache.CompletionRunTime", duration);
}

void AppCacheHistograms::AddNetworkJobStartDelaySample(
    const base::TimeDelta& duration) {
  UMA_HISTOGRAM_TIMES("appcache.JobStartDelay.Network", duration);
}

void AppCacheHistograms::AddErrorJobStartDelaySample(
    const base::TimeDelta& duration) {
  UMA_HISTOGRAM_TIMES("appcache.JobStartDelay.Error", duration);
}

void AppCacheHistograms::AddAppCacheJobStartDelaySample(
    const base::TimeDelta& duration) {
  UMA_HISTOGRAM_TIMES("appcache.JobStartDelay.AppCache", duration);
}

void AppCacheHistograms::AddMissingManifestEntrySample() {
  UMA_HISTOGRAM_BOOLEAN("appcache.MissingManifestEntry", true);
}

void AppCacheHistograms::AddMissingManifestDetectedAtCallsite(
    MissingManifestCallsiteType callsite) {
  UMA_HISTOGRAM_ENUMERATION(
       "appcache.MissingManifestDetectedAtCallsite",
       callsite, NUM_MISSING_MANIFEST_CALLSITE_TYPES);
}

}  // namespace content
