// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CONTENT_COMMON_AD_DELAY_THROTTLE_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CONTENT_COMMON_AD_DELAY_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "content/public/common/url_loader_throttle.h"
#include "services/network/public/cpp/resource_request.h"

class GURL;

namespace base {
class TickClock;
}  // namespace base

namespace subresource_filter {

class DeferCondition;

// This class delays ad requests satisfying certain conditions.
// - The ad is insecure (e.g. uses http).
// TODO(csharrison): Add delays for when the request is in a same-origin iframe.
class AdDelayThrottle : public content::URLLoaderThrottle {
 public:
  static constexpr base::TimeDelta kDefaultDelay =
      base::TimeDelta::FromMilliseconds(50);

  class MetadataProvider {
   public:
    virtual ~MetadataProvider() {}
    virtual bool IsAdRequest() = 0;

    // Whether the request is taking place in a non-isolated (e.g. same-domain)
    // iframe. Should default to false if the isolation status is unknown.
    virtual bool RequestIsInNonIsolatedSubframe() = 0;
  };

  // Mainly used for caching values that we don't want to compute for every
  // resource request.
  class Factory {
   public:
    Factory();
    ~Factory();

    std::unique_ptr<AdDelayThrottle> MaybeCreate(
        std::unique_ptr<MetadataProvider> provider) const;

    base::TimeDelta insecure_delay() const { return insecure_delay_; }
    base::TimeDelta non_isolated_delay() const { return non_isolated_delay_; }
    bool delay_enabled() const { return delay_enabled_; }

   private:
    const base::TimeDelta insecure_delay_;
    const base::TimeDelta non_isolated_delay_;
    const bool delay_enabled_ = false;

    DISALLOW_COPY_AND_ASSIGN(Factory);
  };

  // This enum backs a histogram. Make sure to only append elements, and update
  // enums.xml with new values.
  enum class SecureInfo {
    // Ad that was loaded securely (e.g. using https).
    kSecureAd = 0,

    // Ad that was loaded insecurely (e.g. at least one request through http).
    kInsecureAd = 1,

    kSecureNonAd = 2,
    kInsecureNonAd = 3,

    // Add new elements above kLast.
    kMaxValue = kInsecureNonAd
  };

  // This enum backs a histogram. Make sure to only append elements, and update
  // enums.xml with new values.
  enum class IsolatedInfo {
    // Ad that was loaded isolated from the top-level page (e.g. from a
    // cross-domain iframe).
    kIsolatedAd = 0,

    // Ad loaded from a non-isolated context.
    kNonIsolatedAd = 1,

    // Add new elements above kLast.
    kMaxValue = kNonIsolatedAd
  };
  // The AdDelayThrottle has multiple possible conditions which can cause
  // delays. These conditions will subclass DeferCondition and override
  // IsConditionSatisfied.
  class DeferCondition {
   public:
    DeferCondition(base::TimeDelta delay,
                   AdDelayThrottle::MetadataProvider* provider);
    virtual ~DeferCondition();

    bool ShouldDefer(const GURL& url);

    // The request will be deferred. Returns the amount of time to defer. Should
    // be called at most once after ShouldDefer returns true.
    base::TimeDelta OnReadyToDefer();

    bool was_condition_ever_satisfied() const {
      return was_condition_ever_satisfied_;
    }

    AdDelayThrottle::MetadataProvider* provider() { return provider_; }

   protected:
    virtual bool IsConditionSatisfied(const GURL& url) = 0;

   private:
    base::TimeDelta delay_;

    // Must outlive this object. Will always be non-nullptr.
    AdDelayThrottle::MetadataProvider* provider_;

    bool was_condition_applied_ = false;
    bool was_condition_ever_satisfied_ = false;
    DISALLOW_COPY_AND_ASSIGN(DeferCondition);
  };

  ~AdDelayThrottle() override;

  void set_tick_clock_for_testing(const base::TickClock* tick_clock) {
    tick_clock_ = tick_clock;
  }

 private:
  // content::URLLoaderThrottle:
  void DetachFromCurrentSequence() override;
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           const network::ResourceResponseHead& response_head,
                           bool* defer) override;

  // Returns whether the request to |url| should be deferred.
  bool MaybeDefer(const GURL& url);
  void Resume(base::TimeTicks defer_start);

  AdDelayThrottle(std::unique_ptr<MetadataProvider> provider,
                  const Factory* factory);

  // Will never be nullptr.
  std::unique_ptr<MetadataProvider> provider_;

  // Must be destroyed before |provider_|.
  std::vector<std::unique_ptr<DeferCondition>> defer_conditions_;

  // Must never be nullptr.
  const base::TickClock* tick_clock_;

  // Will be zero if no delay occurs.
  base::TimeDelta expected_delay_;
  base::TimeDelta actual_delay_;

  // Whether to actually delay the request. If set to false, will operate in a
  // dry-run style mode that only logs metrics.
  const bool delay_enabled_ = false;

  base::WeakPtrFactory<AdDelayThrottle> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AdDelayThrottle);
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CONTENT_COMMON_AD_DELAY_THROTTLE_H_
