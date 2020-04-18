// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/content/previews_io_data.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/metrics/histogram.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/time/default_clock.h"
#include "components/previews/content/previews_ui_service.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_opt_out_store.h"
#include "components/previews/core/previews_switches.h"
#include "components/previews/core/previews_user_data.h"
#include "net/base/load_flags.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "url/gurl.h"

namespace previews {

namespace {

void LogPreviewsEligibilityReason(PreviewsEligibilityReason status,
                                  PreviewsType type) {
  int32_t max_limit = static_cast<int32_t>(PreviewsEligibilityReason::LAST);
  base::LinearHistogram::FactoryGet(
      base::StringPrintf("Previews.EligibilityReason.%s",
                         GetStringNameForType(type).c_str()),
      1, max_limit, max_limit + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag)
      ->Add(static_cast<int>(status));
}

bool AllowedOnReload(PreviewsType type) {
  switch (type) {
    // These types return new content on refresh.
    case PreviewsType::LITE_PAGE:
    case PreviewsType::LOFI:
    case PreviewsType::AMP_REDIRECTION:
    case PreviewsType::NOSCRIPT:
      return true;
    // Loading these types will always be stale when refreshed.
    case PreviewsType::OFFLINE:
      return false;
    case PreviewsType::NONE:
    case PreviewsType::UNSPECIFIED:
    case PreviewsType::LAST:
      break;
  }
  NOTREACHED();
  return false;
}

bool IsServerWhitelistedType(PreviewsType type) {
  switch (type) {
    // These types check server whitelist, if available.
    case PreviewsType::NOSCRIPT:
      return true;
    case PreviewsType::OFFLINE:
    case PreviewsType::LITE_PAGE:
    case PreviewsType::LOFI:
    case PreviewsType::AMP_REDIRECTION:
      return false;
    case PreviewsType::NONE:
    case PreviewsType::UNSPECIFIED:
    case PreviewsType::LAST:
      break;
  }
  NOTREACHED();
  return false;
}

bool IsPreviewsBlacklistIgnoredViaFlag() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kIgnorePreviewsBlacklist);
}

}  // namespace

PreviewsIOData::PreviewsIOData(
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : blacklist_ignored_(IsPreviewsBlacklistIgnoredViaFlag()),
      ui_task_runner_(ui_task_runner),
      io_task_runner_(io_task_runner),
      page_id_(1u),
      weak_factory_(this) {}

PreviewsIOData::~PreviewsIOData() {}

void PreviewsIOData::Initialize(
    base::WeakPtr<PreviewsUIService> previews_ui_service,
    std::unique_ptr<PreviewsOptOutStore> previews_opt_out_store,
    std::unique_ptr<PreviewsOptimizationGuide> previews_opt_guide,
    const PreviewsIsEnabledCallback& is_enabled_callback) {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  is_enabled_callback_ = is_enabled_callback;
  previews_ui_service_ = previews_ui_service;
  previews_opt_guide_ = std::move(previews_opt_guide);

  // Set up the IO thread portion of |this|.
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&PreviewsIOData::InitializeOnIOThread,
                                base::Unretained(this),
                                std::move(previews_opt_out_store)));
}

void PreviewsIOData::OnNewBlacklistedHost(const std::string& host,
                                          base::Time time) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(FROM_HERE,
                            base::Bind(&PreviewsUIService::OnNewBlacklistedHost,
                                       previews_ui_service_, host, time));
}

void PreviewsIOData::OnUserBlacklistedStatusChange(bool blacklisted) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&PreviewsUIService::OnUserBlacklistedStatusChange,
                            previews_ui_service_, blacklisted));
}

void PreviewsIOData::OnBlacklistCleared(base::Time time) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(FROM_HERE,
                            base::Bind(&PreviewsUIService::OnBlacklistCleared,
                                       previews_ui_service_, time));
}

void PreviewsIOData::InitializeOnIOThread(
    std::unique_ptr<PreviewsOptOutStore> previews_opt_out_store) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  previews_black_list_.reset(
      new PreviewsBlackList(std::move(previews_opt_out_store),
                            base::DefaultClock::GetInstance(), this));
  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&PreviewsUIService::SetIOData, previews_ui_service_,
                            weak_factory_.GetWeakPtr()));
}

void PreviewsIOData::SetPreviewsBlacklistForTesting(
    std::unique_ptr<PreviewsBlackList> previews_back_list) {
  previews_black_list_ = std::move(previews_back_list);
}

void PreviewsIOData::LogPreviewNavigation(const GURL& url,
                                          bool opt_out,
                                          PreviewsType type,
                                          base::Time time,
                                          uint64_t page_id) const {
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&PreviewsUIService::LogPreviewNavigation,
                     previews_ui_service_, url, type, opt_out, time, page_id));
}

void PreviewsIOData::LogPreviewDecisionMade(
    PreviewsEligibilityReason reason,
    const GURL& url,
    base::Time time,
    PreviewsType type,
    std::vector<PreviewsEligibilityReason>&& passed_reasons,
    uint64_t page_id) const {
  LogPreviewsEligibilityReason(reason, type);
  ui_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&PreviewsUIService::LogPreviewDecisionMade,
                                previews_ui_service_, reason, url, time, type,
                                std::move(passed_reasons), page_id));
}

void PreviewsIOData::AddPreviewNavigation(const GURL& url,
                                          bool opt_out,
                                          PreviewsType type,
                                          uint64_t page_id) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  base::Time time =
      previews_black_list_->AddPreviewNavigation(url, opt_out, type);
  LogPreviewNavigation(url, opt_out, type, time, page_id);
}

void PreviewsIOData::ClearBlackList(base::Time begin_time,
                                    base::Time end_time) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  previews_black_list_->ClearBlackList(begin_time, end_time);
}

void PreviewsIOData::SetIgnorePreviewsBlacklistDecision(bool ignored) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  blacklist_ignored_ = ignored;
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&PreviewsUIService::OnIgnoreBlacklistDecisionStatusChanged,
                 previews_ui_service_, blacklist_ignored_));
}

bool PreviewsIOData::ShouldAllowPreview(const net::URLRequest& request,
                                        PreviewsType type) const {
  return ShouldAllowPreviewAtECT(request, type,
                                 params::GetECTThresholdForPreview(type),
                                 std::vector<std::string>());
}

bool PreviewsIOData::ShouldAllowPreviewAtECT(
    const net::URLRequest& request,
    PreviewsType type,
    net::EffectiveConnectionType effective_connection_type_threshold,
    const std::vector<std::string>& host_blacklist_from_server) const {
  if (!previews::params::ArePreviewsAllowed()) {
    return false;
  }

  if (!request.url().has_host() || !PreviewsUserData::GetData(request)) {
    // Don't capture UMA on this case, as it is not important and can happen
    // when navigating to files on disk, etc.
    return false;
  }

  std::vector<PreviewsEligibilityReason> passed_reasons;
  uint64_t page_id = PreviewsUserData::GetData(request)->page_id();
  if (is_enabled_callback_.is_null() || !previews_black_list_) {
    LogPreviewDecisionMade(PreviewsEligibilityReason::BLACKLIST_UNAVAILABLE,
                           request.url(), base::Time::Now(), type,
                           std::move(passed_reasons), page_id);
    return false;
  }
  passed_reasons.push_back(PreviewsEligibilityReason::BLACKLIST_UNAVAILABLE);

  if (!is_enabled_callback_.Run(type))
    return false;

  if (!blacklist_ignored_) {
    // The blacklist will disallow certain hosts for periods of time based on
    // user's opting out of the preview.
    PreviewsEligibilityReason status = previews_black_list_->IsLoadedAndAllowed(
        request.url(), type, &passed_reasons);
    if (status != PreviewsEligibilityReason::ALLOWED) {
      LogPreviewDecisionMade(status, request.url(), base::Time::Now(), type,
                             std::move(passed_reasons), page_id);
      return false;
    }
  }

  if (effective_connection_type_threshold !=
      net::EFFECTIVE_CONNECTION_TYPE_LAST) {
    net::NetworkQualityEstimator* network_quality_estimator =
        request.context()->network_quality_estimator();
    const net::EffectiveConnectionType observed_effective_connection_type =
        network_quality_estimator
            ? network_quality_estimator->GetEffectiveConnectionType()
            : net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN;
    // Network quality estimator may sometimes return effective connection type
    // as offline when the Android APIs incorrectly return device connectivity
    // as null. See https://crbug.com/838969. So, we do not trigger previews
    // when |observed_effective_connection_type| is
    // net::EFFECTIVE_CONNECTION_TYPE_OFFLINE.
    if (observed_effective_connection_type <=
        net::EFFECTIVE_CONNECTION_TYPE_OFFLINE) {
      LogPreviewDecisionMade(
          PreviewsEligibilityReason::NETWORK_QUALITY_UNAVAILABLE, request.url(),
          base::Time::Now(), type, std::move(passed_reasons), page_id);
      return false;
    }
    passed_reasons.push_back(
        PreviewsEligibilityReason::NETWORK_QUALITY_UNAVAILABLE);

    if (observed_effective_connection_type >
        effective_connection_type_threshold) {
      LogPreviewDecisionMade(PreviewsEligibilityReason::NETWORK_NOT_SLOW,
                             request.url(), base::Time::Now(), type,
                             std::move(passed_reasons), page_id);
      return false;
    }
    passed_reasons.push_back(PreviewsEligibilityReason::NETWORK_NOT_SLOW);
  }

  // LOAD_VALIDATE_CACHE or LOAD_BYPASS_CACHE mean the user reloaded the page.
  // If this is a query for offline previews, reloads should be disallowed.
  if (!AllowedOnReload(type) &&
      request.load_flags() &
          (net::LOAD_VALIDATE_CACHE | net::LOAD_BYPASS_CACHE)) {
    LogPreviewDecisionMade(PreviewsEligibilityReason::RELOAD_DISALLOWED,
                           request.url(), base::Time::Now(), type,
                           std::move(passed_reasons), page_id);
    return false;
  }
  passed_reasons.push_back(PreviewsEligibilityReason::RELOAD_DISALLOWED);

  // Check provided blacklist, if any. This type of blacklist was added for
  // Finch provided blacklist for Client LoFi.
  if (std::find(host_blacklist_from_server.begin(),
                host_blacklist_from_server.end(), request.url().host_piece()) !=
      host_blacklist_from_server.end()) {
    LogPreviewDecisionMade(
        PreviewsEligibilityReason::HOST_BLACKLISTED_BY_SERVER, request.url(),
        base::Time::Now(), type, std::move(passed_reasons), page_id);
    return false;
  }
  passed_reasons.push_back(
      PreviewsEligibilityReason::HOST_BLACKLISTED_BY_SERVER);

  // Check whitelist from the server, if provided.
  if (IsServerWhitelistedType(type)) {
    if (params::IsOptimizationHintsEnabled()) {
      // Optimization hints are configured, so require whitelist match.
      PreviewsEligibilityReason status =
          IsPreviewAllowedByOptmizationHints(request, type, &passed_reasons);
      if (status != PreviewsEligibilityReason::ALLOWED) {
        LogPreviewDecisionMade(status, request.url(), base::Time::Now(), type,
                               std::move(passed_reasons), page_id);
        return false;
      }
    } else {
      // Since server optimization guidance not configured, allow the preview
      // but with qualified eligibility reason.
      LogPreviewDecisionMade(
          PreviewsEligibilityReason::ALLOWED_WITHOUT_OPTIMIZATION_HINTS,
          request.url(), base::Time::Now(), type, std::move(passed_reasons),
          page_id);
      return true;
    }
  }

  LogPreviewDecisionMade(PreviewsEligibilityReason::ALLOWED, request.url(),
                         base::Time::Now(), type, std::move(passed_reasons),
                         page_id);
  return true;
}

bool PreviewsIOData::IsURLAllowedForPreview(const net::URLRequest& request,
                                            PreviewsType type) const {
  if (previews_black_list_ && !blacklist_ignored_) {
    std::vector<PreviewsEligibilityReason> passed_reasons;
    // The blacklist will disallow certain hosts for periods of time based on
    // user's opting out of the preview.
    PreviewsEligibilityReason status = previews_black_list_->IsLoadedAndAllowed(
        request.url(), type, &passed_reasons);
    if (status != PreviewsEligibilityReason::ALLOWED) {
      LogPreviewDecisionMade(status, request.url(), base::Time::Now(), type,
                             std::move(passed_reasons),
                             PreviewsUserData::GetData(request)->page_id());
      return false;
    }
  }

  // Check whitelist from the server, if provided.
  if (IsServerWhitelistedType(type)) {
    if (params::IsOptimizationHintsEnabled()) {
      std::vector<PreviewsEligibilityReason> passed_reasons;
      PreviewsEligibilityReason status =
          IsPreviewAllowedByOptmizationHints(request, type, &passed_reasons);
      if (status != PreviewsEligibilityReason::ALLOWED) {
        LogPreviewDecisionMade(status, request.url(), base::Time::Now(), type,
                               std::move(passed_reasons),
                               PreviewsUserData::GetData(request)->page_id());
        return false;
      }
    }
  }
  return true;
}

PreviewsEligibilityReason PreviewsIOData::IsPreviewAllowedByOptmizationHints(
    const net::URLRequest& request,
    PreviewsType type,
    std::vector<PreviewsEligibilityReason>* passed_reasons) const {
  if (!previews_opt_guide_)
    return PreviewsEligibilityReason::ALLOWED;

  // Check optmization guide whitelist.
  if (!previews_opt_guide_->IsWhitelisted(request, type)) {
    return PreviewsEligibilityReason::HOST_NOT_WHITELISTED_BY_SERVER;
  }
  passed_reasons->push_back(
      PreviewsEligibilityReason::HOST_NOT_WHITELISTED_BY_SERVER);

  return PreviewsEligibilityReason::ALLOWED;
}

uint64_t PreviewsIOData::GeneratePageId() {
  return ++page_id_;
}

}  // namespace previews
