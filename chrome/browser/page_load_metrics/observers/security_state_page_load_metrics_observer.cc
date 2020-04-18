// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/security_state_page_load_metrics_observer.h"

#include <cmath>

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/time/time.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/engagement/site_engagement_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "components/security_state/core/security_state.h"
#include "content/public/browser/navigation_handle.h"
#include "services/metrics/public/cpp/metrics_utils.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace {
// Site Engagement score behavior histogram prefixes.
const char kEngagementFinalPrefix[] = "Security.SiteEngagement";
const char kEngagementDeltaPrefix[] = "Security.SiteEngagementDelta";

// Navigation histogram prefixes.
const char kPageEndReasonPrefix[] = "Security.PageEndReason";
const char kTimeOnPagePrefix[] = "Security.TimeOnPage";

// Security level histograms.
const char kSecurityLevelOnCommit[] = "Security.SecurityLevel.OnCommit";
const char kSecurityLevelOnComplete[] = "Security.SecurityLevel.OnComplete";

std::string GetHistogramSuffixForSecurityLevel(
    security_state::SecurityLevel level) {
  switch (level) {
    case security_state::EV_SECURE:
      return "EV_SECURE";
    case security_state::SECURE:
      return "SECURE";
    case security_state::NONE:
      return "NONE";
    case security_state::HTTP_SHOW_WARNING:
      return "HTTP_SHOW_WARNING";
    case security_state::SECURE_WITH_POLICY_INSTALLED_CERT:
      return "SECURE_WITH_POLICY_INSTALLED_CERT";
    case security_state::DANGEROUS:
      return "DANGEROUS";
    default:
      return "OTHER";
  }
}

std::string GetHistogramName(const char* prefix,
                             security_state::SecurityLevel level) {
  return std::string(prefix) + "." + GetHistogramSuffixForSecurityLevel(level);
}
}  // namespace

// static
std::unique_ptr<page_load_metrics::PageLoadMetricsObserver>
SecurityStatePageLoadMetricsObserver::MaybeCreateForProfile(
    content::BrowserContext* profile) {
  // If the site engagement service is not enabled, this observer will not track
  // site engagement metrics, but will still track the security level and
  // navigation related metrics.
  if (!SiteEngagementService::IsEnabled())
    return std::make_unique<SecurityStatePageLoadMetricsObserver>(nullptr);
  auto* engagement_service = SiteEngagementServiceFactory::GetForProfile(
      static_cast<Profile*>(profile));
  return std::make_unique<SecurityStatePageLoadMetricsObserver>(
      engagement_service);
}

// static
std::string
SecurityStatePageLoadMetricsObserver::GetEngagementDeltaHistogramNameForTesting(
    security_state::SecurityLevel level) {
  return GetHistogramName(kEngagementDeltaPrefix, level);
}

// static
std::string
SecurityStatePageLoadMetricsObserver::GetEngagementFinalHistogramNameForTesting(
    security_state::SecurityLevel level) {
  return GetHistogramName(kEngagementFinalPrefix, level);
}

// static
std::string
SecurityStatePageLoadMetricsObserver::GetPageEndReasonHistogramNameForTesting(
    security_state::SecurityLevel level) {
  return GetHistogramName(kPageEndReasonPrefix, level);
}

SecurityStatePageLoadMetricsObserver::SecurityStatePageLoadMetricsObserver(
    SiteEngagementService* engagement_service)
    : content::WebContentsObserver(), engagement_service_(engagement_service) {}

SecurityStatePageLoadMetricsObserver::~SecurityStatePageLoadMetricsObserver() =
    default;

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
SecurityStatePageLoadMetricsObserver::OnStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url,
    bool started_in_foreground) {
  if (started_in_foreground)
    OnShown();
  if (engagement_service_) {
    initial_engagement_score_ =
        engagement_service_->GetScore(navigation_handle->GetURL());
  }
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
SecurityStatePageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  // Only navigations committed to the main frame are monitored.
  DCHECK(navigation_handle->IsInMainFrame());

  source_id_ = source_id;

  content::WebContents* web_contents = navigation_handle->GetWebContents();
  DCHECK(web_contents);
  Observe(web_contents);

  // Gather initial security level after all server redirects have been
  // resolved.
  security_state_tab_helper_ =
      SecurityStateTabHelper::FromWebContents(web_contents);
  security_state::SecurityInfo security_info;
  security_state_tab_helper_->GetSecurityInfo(&security_info);
  initial_security_level_ = security_info.security_level;
  current_security_level_ = initial_security_level_;

  base::UmaHistogramEnumeration(kSecurityLevelOnCommit, initial_security_level_,
                                security_state::SECURITY_LEVEL_COUNT);

  source_id_ = source_id;
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
SecurityStatePageLoadMetricsObserver::OnHidden(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  if (currently_in_foreground_) {
    foreground_time_ += base::TimeTicks::Now() - last_time_shown_;
    currently_in_foreground_ = false;
  }
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
SecurityStatePageLoadMetricsObserver::OnShown() {
  last_time_shown_ = base::TimeTicks::Now();
  currently_in_foreground_ = true;
  return CONTINUE_OBSERVING;
}

void SecurityStatePageLoadMetricsObserver::OnComplete(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  if (!extra_info.did_commit)
    return;

  if (engagement_service_) {
    double final_engagement_score =
        engagement_service_->GetScore(extra_info.url);
    // Round the engagement score down to the closest multiple of 10 to decrease
    // the granularity of the UKM collection.
    int64_t coarse_engagement_score =
        ukm::GetLinearBucketMin(final_engagement_score, 10);

    ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
    ukm::builders::Security_SiteEngagement(source_id_)
        .SetInitialSecurityLevel(initial_security_level_)
        .SetFinalSecurityLevel(current_security_level_)
        .SetScoreDelta(final_engagement_score - initial_engagement_score_)
        .SetScoreFinal(coarse_engagement_score)
        .Record(ukm_recorder);

    // Get the change in Site Engagement score and transform it into the range
    // [0, 100] so it can be logged in an EXACT_LINEAR histogram.
    int delta = std::round(
        (final_engagement_score - initial_engagement_score_ + 100) / 2);
    base::UmaHistogramExactLinear(
        GetHistogramName(kEngagementDeltaPrefix, current_security_level_),
        delta, 100);
    base::UmaHistogramExactLinear(
        GetHistogramName(kEngagementFinalPrefix, current_security_level_),
        final_engagement_score, 100);
  }

  base::UmaHistogramEnumeration(
      GetHistogramName(kPageEndReasonPrefix, current_security_level_),
      extra_info.page_end_reason, page_load_metrics::PAGE_END_REASON_COUNT);
  base::UmaHistogramCustomTimes(
      GetHistogramName(kTimeOnPagePrefix, current_security_level_),
      foreground_time_, base::TimeDelta::FromMilliseconds(1),
      base::TimeDelta::FromHours(1), 100);
  base::UmaHistogramEnumeration(kSecurityLevelOnComplete,
                                current_security_level_,
                                security_state::SECURITY_LEVEL_COUNT);
}

void SecurityStatePageLoadMetricsObserver::DidChangeVisibleSecurityState() {
  if (!security_state_tab_helper_)
    return;

  security_state::SecurityInfo security_info;
  security_state_tab_helper_->GetSecurityInfo(&security_info);
  current_security_level_ = security_info.security_level;
}
