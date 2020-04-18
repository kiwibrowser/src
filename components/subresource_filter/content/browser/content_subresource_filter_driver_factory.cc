// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/content_subresource_filter_driver_factory.h"

#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "components/subresource_filter/content/browser/navigation_console_logger.h"
#include "components/subresource_filter/content/browser/page_load_statistics.h"
#include "components/subresource_filter/content/browser/subresource_filter_client.h"
#include "components/subresource_filter/content/browser/subresource_filter_observer_manager.h"
#include "components/subresource_filter/core/browser/subresource_filter_constants.h"
#include "components/subresource_filter/core/common/activation_list.h"
#include "components/subresource_filter/core/common/activation_state.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/console_message_level.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    subresource_filter::ContentSubresourceFilterDriverFactory);

namespace subresource_filter {

namespace {

// Returns true with a probability given by |performance_measurement_rate| if
// ThreadTicks is supported, otherwise returns false.
}  // namespace

// static
void ContentSubresourceFilterDriverFactory::CreateForWebContents(
    content::WebContents* web_contents,
    SubresourceFilterClient* client) {
  if (FromWebContents(web_contents))
    return;
  web_contents->SetUserData(
      UserDataKey(), std::make_unique<ContentSubresourceFilterDriverFactory>(
                         web_contents, client));
}

// static
ContentSubresourceFilterDriverFactory::ContentSubresourceFilterDriverFactory(
    content::WebContents* web_contents,
    SubresourceFilterClient* client)
    : content::WebContentsObserver(web_contents),
      client_(client),
      throttle_manager_(
          std::make_unique<ContentSubresourceFilterThrottleManager>(
              this,
              client_->GetRulesetDealer(),
              web_contents)) {}

ContentSubresourceFilterDriverFactory::
    ~ContentSubresourceFilterDriverFactory() {}

void ContentSubresourceFilterDriverFactory::NotifyPageActivationComputed(
    content::NavigationHandle* navigation_handle,
    ActivationDecision activation_decision,
    const Configuration& matched_configuration,
    bool warning) {
  DCHECK(navigation_handle->IsInMainFrame());
  DCHECK(!navigation_handle->IsSameDocument());
  if (navigation_handle->GetNetErrorCode() != net::OK)
    return;

  activation_decision_ = activation_decision;
  matched_configuration_ = matched_configuration;
  DCHECK_NE(activation_decision_, ActivationDecision::UNKNOWN);

  ActivationLevel effective_activation_level =
      matched_configuration_.activation_options.activation_level;

  // ACTIVATION_DISABLED implies DISABLED activation level.
  DCHECK(activation_decision_ != ActivationDecision::ACTIVATION_DISABLED ||
         effective_activation_level == ActivationLevel::DISABLED);

  // Ensure the matched config is in our config list. If it wasn't then this
  // must be a forced activation via devtools.
  bool forced_activation_via_devtools =
      (matched_configuration == Configuration::MakeForForcedActivation());
  DCHECK(activation_decision_ != ActivationDecision::ACTIVATED ||
         HasEnabledConfiguration(matched_configuration) ||
         forced_activation_via_devtools)
      << matched_configuration;

  if (warning && effective_activation_level == ActivationLevel::ENABLED) {
    NavigationConsoleLogger::LogMessageOnCommit(
        navigation_handle, content::CONSOLE_MESSAGE_LEVEL_WARNING,
        kActivationWarningConsoleMessage);

    // Do not disallow enforcement if activated via devtools.
    if (!forced_activation_via_devtools) {
      activation_decision_ = ActivationDecision::ACTIVATION_DISABLED;
      effective_activation_level = ActivationLevel::DISABLED;
    }
  }

  matched_configuration_.activation_options.activation_level =
      effective_activation_level;
  SubresourceFilterObserverManager::FromWebContents(web_contents())
      ->NotifyPageActivationComputed(navigation_handle, activation_decision_,
                                     matched_configuration_.GetActivationState(
                                         effective_activation_level));
}

void ContentSubresourceFilterDriverFactory::OnFirstSubresourceLoadDisallowed() {
  if (matched_configuration_.activation_conditions.forced_activation) {
    UMA_HISTOGRAM_BOOLEAN(
        "SubresourceFilter.PageLoad.ForcedActivation.DisallowedLoad", true);
    return;
  }
  client_->ShowNotification();
}

void ContentSubresourceFilterDriverFactory::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame() &&
      !navigation_handle->IsSameDocument()) {
    activation_decision_ = ActivationDecision::UNKNOWN;
    client_->OnNewNavigationStarted();
  }
}

void ContentSubresourceFilterDriverFactory::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  if (activation_decision_ == ActivationDecision::UNKNOWN) {
    activation_decision_ = ActivationDecision::ACTIVATION_DISABLED;
    matched_configuration_ = Configuration();
  }
}

}  // namespace subresource_filter
