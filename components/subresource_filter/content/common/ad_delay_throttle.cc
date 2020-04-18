// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/common/ad_delay_throttle.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "components/subresource_filter/core/common/common_features.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace subresource_filter {

namespace {

void LogSecureInfo(AdDelayThrottle::SecureInfo info) {
  UMA_HISTOGRAM_ENUMERATION("SubresourceFilter.AdDelay.SecureInfo", info);
}

class InsecureCondition : public AdDelayThrottle::DeferCondition {
 public:
  InsecureCondition(base::TimeDelta delay,
                    AdDelayThrottle::MetadataProvider* provider)
      : DeferCondition(delay, provider) {}
  ~InsecureCondition() override {
    if (provider()->IsAdRequest()) {
      LogSecureInfo(was_condition_ever_satisfied()
                        ? AdDelayThrottle::SecureInfo::kInsecureAd
                        : AdDelayThrottle::SecureInfo::kSecureAd);
    } else {
      LogSecureInfo(was_condition_ever_satisfied()
                        ? AdDelayThrottle::SecureInfo::kInsecureNonAd
                        : AdDelayThrottle::SecureInfo::kSecureNonAd);
    }
  }

 private:
  // DeferCondition:
  bool IsConditionSatisfied(const GURL& url) override {
    // Note: this should probably be using content::IsOriginSecure which
    // accounts for things like whitelisted origins, localhost, etc. This isn't
    // used here because that function is quite expensive for insecure schemes,
    // involving many allocations and string scans.
    return url.SchemeIs(url::kHttpScheme);
  }
};

class NonIsolatedCondition : public AdDelayThrottle::DeferCondition {
 public:
  NonIsolatedCondition(base::TimeDelta delay,
                       AdDelayThrottle::MetadataProvider* provider)
      : DeferCondition(delay, provider) {}
  ~NonIsolatedCondition() override {
    if (provider()->IsAdRequest()) {
      UMA_HISTOGRAM_ENUMERATION(
          "SubresourceFilter.AdDelay.IsolatedInfo",
          was_condition_ever_satisfied()
              ? AdDelayThrottle::IsolatedInfo::kNonIsolatedAd
              : AdDelayThrottle::IsolatedInfo::kIsolatedAd);
    }
  }

 private:
  // DeferCondition:
  bool IsConditionSatisfied(const GURL& url) override {
    return provider()->RequestIsInNonIsolatedSubframe();
  }
};

};  // namespace

AdDelayThrottle::DeferCondition::DeferCondition(
    base::TimeDelta delay,
    AdDelayThrottle::MetadataProvider* provider)
    : delay_(delay), provider_(provider) {
  DCHECK(provider);
}
AdDelayThrottle::DeferCondition::~DeferCondition() = default;

bool AdDelayThrottle::DeferCondition::ShouldDefer(const GURL& url) {
  if (was_condition_applied_) {
    DCHECK(was_condition_ever_satisfied_);
    return false;
  }
  was_condition_ever_satisfied_ |= IsConditionSatisfied(url);
  return was_condition_ever_satisfied_;
}

// The request will be deferred. Returns the amount of time to defer.
base::TimeDelta AdDelayThrottle::DeferCondition::OnReadyToDefer() {
  DCHECK(!was_condition_applied_);
  DCHECK(was_condition_ever_satisfied_);
  was_condition_applied_ = true;
  return delay_;
}

constexpr base::TimeDelta AdDelayThrottle::kDefaultDelay;

AdDelayThrottle::Factory::Factory()
    : insecure_delay_(base::TimeDelta::FromMilliseconds(
          base::GetFieldTrialParamByFeatureAsInt(
              kDelayUnsafeAds,
              kInsecureDelayParam,
              kDefaultDelay.InMilliseconds()))),
      non_isolated_delay_(base::TimeDelta::FromMilliseconds(
          base::GetFieldTrialParamByFeatureAsInt(
              kDelayUnsafeAds,
              kNonIsolatedDelayParam,
              kDefaultDelay.InMilliseconds()))),
      delay_enabled_(base::FeatureList::IsEnabled(kAdTagging) &&
                     base::FeatureList::IsEnabled(kDelayUnsafeAds)) {}

AdDelayThrottle::Factory::~Factory() = default;

std::unique_ptr<AdDelayThrottle> AdDelayThrottle::Factory::MaybeCreate(
    std::unique_ptr<AdDelayThrottle::MetadataProvider> provider) const {
  DCHECK(provider);
  return base::WrapUnique(new AdDelayThrottle(std::move(provider), this));
}

AdDelayThrottle::~AdDelayThrottle() {
  if (!expected_delay_.is_zero()) {
    UMA_HISTOGRAM_TIMES("SubresourceFilter.AdDelay.Delay", actual_delay_);
    UMA_HISTOGRAM_TIMES("SubresourceFilter.AdDelay.Delay.Expected",
                        expected_delay_);
    UMA_HISTOGRAM_TIMES("SubresourceFilter.AdDelay.Delay.Queuing",
                        actual_delay_ - expected_delay_);
  }
}

void AdDelayThrottle::DetachFromCurrentSequence() {
  // The throttle is moving to another thread. Ensure this is done before any
  // weak pointers are created.
  DCHECK(!weak_factory_.HasWeakPtrs());
}

void AdDelayThrottle::WillStartRequest(network::ResourceRequest* request,
                                       bool* defer) {
  *defer = MaybeDefer(request->url);
}

void AdDelayThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseHead& response_head,
    bool* defer) {
  // Note: some MetadataProviders may not be able to distinguish requests that
  // are only tagged as ads after a redirect.
  *defer = MaybeDefer(redirect_info.new_url);
}

bool AdDelayThrottle::MaybeDefer(const GURL& url) {
  // Check for condition matching before checking if the feature is enabled, to
  // ensure metrics can be reported.
  std::vector<DeferCondition*> matched_conditions;
  for (auto& condition : defer_conditions_) {
    if (condition->ShouldDefer(url))
      matched_conditions.push_back(condition.get());
  }

  if (!delay_enabled_ || matched_conditions.empty() ||
      !provider_->IsAdRequest()) {
    return false;
  }

  base::TimeDelta delay;
  for (DeferCondition* condition : matched_conditions) {
    delay += condition->OnReadyToDefer();
  }
  // TODO(csharrison): Consider logging to the console here that Chrome
  // delayed this request.
  expected_delay_ += delay;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&AdDelayThrottle::Resume, weak_factory_.GetWeakPtr(),
                     tick_clock_->NowTicks()),
      delay);
  return true;
}

void AdDelayThrottle::Resume(base::TimeTicks defer_start) {
  actual_delay_ += tick_clock_->NowTicks() - defer_start;
  delegate_->Resume();
}

AdDelayThrottle::AdDelayThrottle(std::unique_ptr<MetadataProvider> provider,
                                 const AdDelayThrottle::Factory* factory)
    : provider_(std::move(provider)),
      tick_clock_(base::DefaultTickClock::GetInstance()),
      delay_enabled_(factory->delay_enabled()),
      weak_factory_(this) {
  defer_conditions_.emplace_back(std::make_unique<InsecureCondition>(
      factory->insecure_delay(), provider_.get()));
  defer_conditions_.emplace_back(std::make_unique<NonIsolatedCondition>(
      factory->non_isolated_delay(), provider_.get()));
}

}  // namespace subresource_filter
