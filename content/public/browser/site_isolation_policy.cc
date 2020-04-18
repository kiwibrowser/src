// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/site_isolation_policy.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/macros.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_split.h"
#include "base/timer/timer.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "url/gurl.h"

namespace content {

// static
bool SiteIsolationPolicy::UseDedicatedProcessesForAllSites() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSitePerProcess)) {
    return true;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableSiteIsolationTrials)) {
    return false;
  }

  // The switches above needs to be checked first, because if the
  // ContentBrowserClient consults a base::Feature, then it will activate the
  // field trial and assigns the client either to a control or an experiment
  // group - such assignment should be final.
  return GetContentClient() &&
         GetContentClient()->browser()->ShouldEnableStrictSiteIsolation();
}

// static
SiteIsolationPolicy::CrossSiteDocumentBlockingEnabledState
SiteIsolationPolicy::IsCrossSiteDocumentBlockingEnabled() {
  // --disable-web-security also disables cross-origin response blocking (CORB).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableWebSecurity)) {
    return XSDB_DISABLED;
  }

  if (base::FeatureList::IsEnabled(
          ::features::kCrossSiteDocumentBlockingAlways)) {
    return XSDB_ENABLED_UNCONDITIONALLY;
  }

  if (base::FeatureList::IsEnabled(
          ::features::kCrossSiteDocumentBlockingIfIsolating)) {
    return XSDB_ENABLED_IF_ISOLATED;
  }

  return XSDB_DISABLED;
}

// static
bool SiteIsolationPolicy::IsTopDocumentIsolationEnabled() {
  // --site-per-process trumps --top-document-isolation.
  if (UseDedicatedProcessesForAllSites())
    return false;

  // The feature needs to be checked last, because checking the feature
  // activates the field trial and assigns the client either to a control or an
  // experiment group - such assignment should be final.
  return base::FeatureList::IsEnabled(::features::kTopDocumentIsolation);
}

// static
bool SiteIsolationPolicy::AreIsolatedOriginsEnabled() {
  // NOTE: Because it is possible for --isolate-origins to be isolating origins
  // at a finer-than-site granularity, we do not suppress --isolate-origins when
  // --site-per-process is also enabled.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kIsolateOrigins)) {
    return true;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableSiteIsolationTrials)) {
    return false;
  }

  // The feature needs to be checked last, because checking the feature
  // activates the field trial and assigns the client either to a control or an
  // experiment group - such assignment should be final.
  return base::FeatureList::IsEnabled(features::kIsolateOrigins);
}

// static
bool SiteIsolationPolicy::IsErrorPageIsolationEnabled(bool in_main_frame) {
  return GetContentClient()->browser()->ShouldIsolateErrorPage(in_main_frame);
}

// static
bool SiteIsolationPolicy::ShouldPdfCompositorBeEnabledForOopifs() {
  // TODO(weili): We only create pdf compositor client and use pdf compositor
  // service when site-per-process or isolate-origins flag/feature is enabled,
  // or top-document-isolation feature is enabled. This may not cover all cases
  // where OOPIF is used such as isolate-extensions, but should be good for
  // feature testing purpose. Eventually, we will remove this check and use pdf
  // compositor service by default for printing.
  return AreIsolatedOriginsEnabled() || IsTopDocumentIsolationEnabled() ||
         UseDedicatedProcessesForAllSites();
}

// static
std::vector<url::Origin>
SiteIsolationPolicy::GetIsolatedOriginsFromEnvironment() {
  std::string cmdline_arg =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kIsolateOrigins);
  if (!cmdline_arg.empty()) {
    std::vector<url::Origin> cmdline_origins =
        ParseIsolatedOrigins(cmdline_arg);
    UMA_HISTOGRAM_COUNTS_1000("SiteIsolation.IsolateOrigins.Size",
                              cmdline_origins.size());
    return cmdline_origins;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableSiteIsolationTrials)) {
    return std::vector<url::Origin>();
  }

  // The feature needs to be checked last, because checking the feature
  // activates the field trial and assigns the client either to a control or an
  // experiment group - such assignment should be final.
  if (base::FeatureList::IsEnabled(features::kIsolateOrigins)) {
    std::string field_trial_arg = base::GetFieldTrialParamValueByFeature(
        features::kIsolateOrigins,
        features::kIsolateOriginsFieldTrialParamName);
    return ParseIsolatedOrigins(field_trial_arg);
  }
  return std::vector<url::Origin>();
}

// static
std::vector<url::Origin> SiteIsolationPolicy::GetIsolatedOrigins() {
  std::vector<url::Origin> from_environment =
      GetIsolatedOriginsFromEnvironment();
  std::vector<url::Origin> from_embedder =
      GetContentClient()->browser()->GetOriginsRequiringDedicatedProcess();

  std::vector<url::Origin> result = std::move(from_environment);
  result.reserve(result.size() + from_embedder.size());
  std::move(from_embedder.begin(), from_embedder.end(),
            std::back_inserter(result));
  return result;
}

// static
std::vector<url::Origin> SiteIsolationPolicy::ParseIsolatedOrigins(
    base::StringPiece arg) {
  std::vector<base::StringPiece> origin_strings = base::SplitStringPiece(
      arg, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  std::vector<url::Origin> origins;
  origins.reserve(origin_strings.size());
  for (const base::StringPiece& origin_string : origin_strings) {
    url::Origin origin = url::Origin::Create(GURL(origin_string));
    if (!origin.unique())
      origins.push_back(origin);
  }
  return origins;
}

// static
void SiteIsolationPolicy::StartRecordingSiteIsolationFlagUsage() {
  RecordSiteIsolationFlagUsage();
  // Record the flag usage metrics every 24 hours.  Even though site isolation
  // flags can't change dynamically at runtime, collecting these stats daily
  // helps determine the overall population of users who run with a given flag
  // on any given day.
  CR_DEFINE_STATIC_LOCAL(base::RepeatingTimer, update_stats_timer, ());
  update_stats_timer.Start(
      FROM_HERE, base::TimeDelta::FromHours(24),
      base::BindRepeating(&SiteIsolationPolicy::RecordSiteIsolationFlagUsage));
}

// static
void SiteIsolationPolicy::RecordSiteIsolationFlagUsage() {
  // For --site-per-process and --isolate-origins, include flags specified on
  // command-line, in chrome://flags, and via enterprise policy (i.e., include
  // switches::kSitePerProcess and switches::kIsolateOrigins).  Exclude these
  // modes being set through field trials (i.e., exclude
  // features::kSitePerProcess and features::IsolateOrigins).
  UMA_HISTOGRAM_BOOLEAN("SiteIsolation.Flags.IsolateOrigins",
                        base::CommandLine::ForCurrentProcess()->HasSwitch(
                            switches::kIsolateOrigins));

  UMA_HISTOGRAM_BOOLEAN("SiteIsolation.Flags.SitePerProcess",
                        base::CommandLine::ForCurrentProcess()->HasSwitch(
                            switches::kSitePerProcess));
}

}  // namespace content
