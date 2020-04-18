// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tab_contents/navigation_metrics_recorder.h"

#include "base/feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "components/navigation_metrics/navigation_metrics.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/content_features.h"
#include "google_apis/gaia/gaia_urls.h"
#include "url/gurl.h"
#include "url/origin.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

DEFINE_WEB_CONTENTS_USER_DATA_KEY(NavigationMetricsRecorder);

NavigationMetricsRecorder::NavigationMetricsRecorder(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

NavigationMetricsRecorder::~NavigationMetricsRecorder() {
}

void NavigationMetricsRecorder::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!navigation_handle->HasCommitted())
    return;

  // This needs to go before the IsInMainFrame() check, as we want to register
  // committed navigations to the sign-in URL from both main frames and
  // subframes.
  //
  // TODO(alexmos): See if there's a better place for this check.
  if (url::Origin::Create(navigation_handle->GetURL()) ==
      url::Origin::Create(GaiaUrls::GetInstance()->gaia_url())) {
    RegisterSyntheticSigninIsolationTrial();
  }

  if (!navigation_handle->IsInMainFrame())
    return;

  content::BrowserContext* context = web_contents()->GetBrowserContext();
  content::NavigationEntry* last_committed_entry =
      web_contents()->GetController().GetLastCommittedEntry();

  navigation_metrics::RecordMainFrameNavigation(
      last_committed_entry->GetVirtualURL(),
      navigation_handle->IsSameDocument(), context->IsOffTheRecord());
}

void NavigationMetricsRecorder::RegisterSyntheticSigninIsolationTrial() {
  // For navigations to the sign-in URL, register a synthetic field trial for
  // this client. This will indicate whether the sign-in process isolation is
  // active, and whether or not it was activated manually via a command-line
  // flag.
  //
  // TODO(alexmos): Remove this after the field trial for sign-in isolation.
  if (base::FeatureList::IsEnabled(features::kSignInProcessIsolation)) {
    if (base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
            features::kSignInProcessIsolation.name,
            base::FeatureList::OVERRIDE_ENABLE_FEATURE)) {
      ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
          "SignInProcessIsolationActive", "ForceEnabled");
    } else {
      ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
          "SignInProcessIsolationActive", "Enabled");
    }
  } else {
    if (base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
            ::features::kSignInProcessIsolation.name,
            base::FeatureList::OVERRIDE_DISABLE_FEATURE)) {
      ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
          "SignInProcessIsolationActive", "ForceDisabled");
    } else {
      ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
          "SignInProcessIsolationActive", "Disabled");
    }
  }
}
