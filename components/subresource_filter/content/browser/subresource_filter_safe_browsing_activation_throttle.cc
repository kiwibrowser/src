// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/subresource_filter_safe_browsing_activation_throttle.h"

#include <sstream>
#include <utility>
#include <vector>

#include "base/metrics/histogram_macros.h"
#include "base/timer/timer.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "components/subresource_filter/content/browser/content_activation_list_utils.h"
#include "components/subresource_filter/content/browser/content_subresource_filter_driver_factory.h"
#include "components/subresource_filter/content/browser/subresource_filter_client.h"
#include "components/subresource_filter/content/browser/subresource_filter_observer_manager.h"
#include "components/subresource_filter/content/browser/subresource_filter_safe_browsing_client.h"
#include "components/ukm/ukm_source.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace subresource_filter {

SubresourceFilterSafeBrowsingActivationThrottle::
    SubresourceFilterSafeBrowsingActivationThrottle(
        content::NavigationHandle* handle,
        SubresourceFilterClient* client,
        scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
        scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager>
            database_manager)
    : NavigationThrottle(handle),
      io_task_runner_(std::move(io_task_runner)),
      database_client_(new SubresourceFilterSafeBrowsingClient(
                           std::move(database_manager),
                           AsWeakPtr(),
                           io_task_runner_,
                           base::ThreadTaskRunnerHandle::Get()),
                       base::OnTaskRunnerDeleter(io_task_runner_)),
      client_(client) {
  DCHECK(handle->IsInMainFrame());

  CheckCurrentUrl();
  DCHECK(!check_results_.empty());
}

SubresourceFilterSafeBrowsingActivationThrottle::
    ~SubresourceFilterSafeBrowsingActivationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
SubresourceFilterSafeBrowsingActivationThrottle::WillRedirectRequest() {
  CheckCurrentUrl();
  return PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
SubresourceFilterSafeBrowsingActivationThrottle::WillProcessResponse() {
  // No need to defer the navigation if the check already happened.
  if (HasFinishedAllSafeBrowsingChecks()) {
    NotifyResult();
    return PROCEED;
  }
  CHECK(!deferring_);
  deferring_ = true;
  defer_time_ = base::TimeTicks::Now();
  return DEFER;
}

const char*
SubresourceFilterSafeBrowsingActivationThrottle::GetNameForLogging() {
  return "SubresourceFilterSafeBrowsingActivationThrottle";
}

void SubresourceFilterSafeBrowsingActivationThrottle::OnCheckUrlResultOnUI(
    const SubresourceFilterSafeBrowsingClient::CheckResult& result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  size_t request_id = result.request_id;
  DCHECK_LT(request_id, check_results_.size());
  DCHECK_LT(request_id, check_start_times_.size());

  auto& stored_result = check_results_.at(request_id);
  CHECK(!stored_result.finished);
  stored_result = result;

  UMA_HISTOGRAM_TIMES("SubresourceFilter.SafeBrowsing.TotalCheckTime",
                      base::TimeTicks::Now() - check_start_times_[request_id]);
  if (deferring_ && HasFinishedAllSafeBrowsingChecks()) {
    NotifyResult();

    deferring_ = false;
    Resume();
  }
}

void SubresourceFilterSafeBrowsingActivationThrottle::CheckCurrentUrl() {
  DCHECK(database_client_);
  check_start_times_.push_back(base::TimeTicks::Now());
  check_results_.emplace_back();
  size_t id = check_results_.size() - 1;
  io_task_runner_->PostTask(
      FROM_HERE, base::Bind(&SubresourceFilterSafeBrowsingClient::CheckUrlOnIO,
                            base::Unretained(database_client_.get()),
                            navigation_handle()->GetURL(), id));
}

void SubresourceFilterSafeBrowsingActivationThrottle::NotifyResult() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("loading"),
               "SubresourceFilterSafeBrowsingActivationThrottle::NotifyResult");
  // Compute the matched list and notify observers of the check result.
  DCHECK(!check_results_.empty());
  ActivationList matched_list = ActivationList::NONE;
  bool warning = false;
  const auto& check_result = check_results_.back();
  DCHECK(check_result.finished);
  matched_list = GetListForThreatTypeAndMetadata(
      check_result.threat_type, check_result.threat_metadata, &warning);
  SubresourceFilterObserverManager::FromWebContents(
      navigation_handle()->GetWebContents())
      ->NotifySafeBrowsingCheckComplete(navigation_handle(),
                                        check_result.threat_type,
                                        check_result.threat_metadata);

  Configuration matched_configuration;
  ActivationDecision activation_decision = ActivationDecision::UNKNOWN;
  if (client_->ForceActivationInCurrentWebContents()) {
    TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("loading"), "ActivationForced");
    activation_decision = ActivationDecision::ACTIVATED;
    matched_configuration = Configuration::MakeForForcedActivation();
  } else {
    base::Optional<Configuration> config =
        GetHighestPriorityConfiguration(matched_list);
    if (config.has_value()) {
      matched_configuration = config.value();
    }
    activation_decision = GetActivationDecision(config);
  }
  DCHECK_NE(activation_decision, ActivationDecision::UNKNOWN);

  // Check for whitelisted status last, so that the client gets an accurate
  // indication of whether there would be activation otherwise.
  // Note that the client is responsible for noticing if we're forcing
  // activation.
  bool whitelisted = client_->OnPageActivationComputed(
      navigation_handle(),
      !warning && matched_configuration.activation_options.activation_level ==
                      ActivationLevel::ENABLED);

  // Only reset the activation decision reason if we would have activated.
  if (whitelisted && activation_decision == ActivationDecision::ACTIVATED) {
    TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("loading"), "ActivationWhitelisted");
    activation_decision = ActivationDecision::URL_WHITELISTED;
    matched_configuration = Configuration();
  }

  LogMetricsOnChecksComplete(
      matched_list, activation_decision,
      matched_configuration.activation_options.activation_level);

  auto* driver_factory = ContentSubresourceFilterDriverFactory::FromWebContents(
      navigation_handle()->GetWebContents());
  DCHECK(driver_factory);
  driver_factory->NotifyPageActivationComputed(
      navigation_handle(), activation_decision, matched_configuration, warning);
}

void SubresourceFilterSafeBrowsingActivationThrottle::
    LogMetricsOnChecksComplete(ActivationList matched_list,
                               ActivationDecision decision,
                               ActivationLevel level) const {
  DCHECK(HasFinishedAllSafeBrowsingChecks());

  base::TimeDelta delay = defer_time_.is_null()
                              ? base::TimeDelta::FromMilliseconds(0)
                              : base::TimeTicks::Now() - defer_time_;
  UMA_HISTOGRAM_TIMES("SubresourceFilter.PageLoad.SafeBrowsingDelay", delay);

  // Log a histogram for the delay we would have introduced if the throttle only
  // speculatively checks URLs on WillStartRequest. This is only different from
  // the actual delay if there was at least one redirect.
  base::TimeDelta no_redirect_speculation_delay =
      check_results_.size() > 1 ? check_results_.back().check_time : delay;
  UMA_HISTOGRAM_TIMES(
      "SubresourceFilter.PageLoad.SafeBrowsingDelay.NoRedirectSpeculation",
      no_redirect_speculation_delay);

  ukm::SourceId source_id = ukm::ConvertToSourceId(
      navigation_handle()->GetNavigationId(), ukm::SourceIdType::NAVIGATION_ID);
  ukm::builders::SubresourceFilter builder(source_id);
  builder.SetActivationDecision(static_cast<int64_t>(decision));
  if (level == subresource_filter::ActivationLevel::DRYRUN) {
    DCHECK_EQ(subresource_filter::ActivationDecision::ACTIVATED, decision);
    builder.SetDryRun(true);
  }
  builder.Record(ukm::UkmRecorder::Get());

  UMA_HISTOGRAM_ENUMERATION("SubresourceFilter.PageLoad.ActivationDecision",
                            decision,
                            ActivationDecision::ACTIVATION_DECISION_MAX);
  UMA_HISTOGRAM_ENUMERATION("SubresourceFilter.PageLoad.ActivationList",
                            matched_list,
                            static_cast<int>(ActivationList::LAST) + 1);
}

bool SubresourceFilterSafeBrowsingActivationThrottle::
    HasFinishedAllSafeBrowsingChecks() const {
  for (const auto& check_result : check_results_) {
    if (!check_result.finished) {
      return false;
    }
  }
  return true;
}

base::Optional<Configuration> SubresourceFilterSafeBrowsingActivationThrottle::
    GetHighestPriorityConfiguration(ActivationList matched_list) {
  base::Optional<Configuration> selected_config;

  // If it's http or https, find the best config.
  if (navigation_handle()->GetURL().SchemeIsHTTPOrHTTPS()) {
    const auto& decreasing_configs =
        GetEnabledConfigurations()->configs_by_decreasing_priority();
    const auto selected_config_itr =
        std::find_if(decreasing_configs.begin(), decreasing_configs.end(),
                     [matched_list, this](const Configuration& config) {
                       return DoesMainFrameURLSatisfyActivationConditions(
                           config.activation_conditions, matched_list);
                     });
    if (selected_config_itr != decreasing_configs.end()) {
      selected_config = *selected_config_itr;
    }
  }
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("loading"),
               "SubresourceFilterSafeBrowsingActivationThrottle::"
               "GetHighestPriorityConfiguration",
               "selected_config",
               selected_config.has_value()
                   ? selected_config->ToTracedValue()
                   : std::make_unique<base::trace_event::TracedValue>());
  return selected_config;
}

ActivationDecision
SubresourceFilterSafeBrowsingActivationThrottle::GetActivationDecision(
    const base::Optional<Configuration>& config) {
  if (!config.has_value()) {
    return ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET;
  }

  auto activation_level = config->activation_options.activation_level;
  return activation_level == ActivationLevel::DISABLED
             ? ActivationDecision::ACTIVATION_DISABLED
             : ActivationDecision::ACTIVATED;
}

bool SubresourceFilterSafeBrowsingActivationThrottle::
    DoesMainFrameURLSatisfyActivationConditions(
        const Configuration::ActivationConditions& conditions,
        ActivationList matched_list) const {
  // Avoid copies when tracing disabled.
  auto list_to_string = [](ActivationList activation_list) {
    std::ostringstream matched_list_stream;
    matched_list_stream << activation_list;
    return matched_list_stream.str();
  };
  TRACE_EVENT2(TRACE_DISABLED_BY_DEFAULT("loading"),
               "SubresourceFilterSafeBrowsingActivationThrottle::"
               "DoesMainFrameURLSatisfyActivationConditions",
               "matched_list", list_to_string(matched_list), "conditions",
               conditions.ToTracedValue());
  switch (conditions.activation_scope) {
    case ActivationScope::ALL_SITES:
      return true;
    case ActivationScope::ACTIVATION_LIST:
      if (matched_list == ActivationList::NONE)
        return false;
      if (conditions.activation_list == matched_list)
        return true;

      if (conditions.activation_list == ActivationList::PHISHING_INTERSTITIAL &&
          matched_list == ActivationList::SOCIAL_ENG_ADS_INTERSTITIAL) {
        // Handling special case, where activation on the phishing sites also
        // mean the activation on the sites with social engineering metadata.
        return true;
      }
      return false;
    case ActivationScope::NO_SITES:
      return false;
  }
  NOTREACHED();
  return false;
}

}  //  namespace subresource_filter
