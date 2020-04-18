// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin/merge_session_navigation_throttle.h"

#include "chrome/browser/chromeos/login/signin/merge_session_load_page.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"

// static
std::unique_ptr<content::NavigationThrottle>
MergeSessionNavigationThrottle::Create(content::NavigationHandle* handle) {
  return std::unique_ptr<content::NavigationThrottle>(
      new MergeSessionNavigationThrottle(handle));
}

MergeSessionNavigationThrottle::MergeSessionNavigationThrottle(
    content::NavigationHandle* handle)
    : NavigationThrottle(handle), weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

MergeSessionNavigationThrottle::~MergeSessionNavigationThrottle() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

content::NavigationThrottle::ThrottleCheckResult
MergeSessionNavigationThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!merge_session_throttling_utils::ShouldDelayUrl(
          navigation_handle()->GetURL()) ||
      !merge_session_throttling_utils::ShouldDelayRequestForWebContents(
          navigation_handle()->GetWebContents())) {
    return content::NavigationThrottle::PROCEED;
  }

  // The MergeSessionLoadPage will be deleted by the interstitial page once it
  // is closed.
  (new chromeos::MergeSessionLoadPage(
       navigation_handle()->GetWebContents(), navigation_handle()->GetURL(),
       base::Bind(&MergeSessionNavigationThrottle::OnBlockingPageComplete,
                  weak_factory_.GetWeakPtr())))
      ->Show();
  return content::NavigationThrottle::DEFER;
}

content::NavigationThrottle::ThrottleCheckResult
MergeSessionNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

const char* MergeSessionNavigationThrottle::GetNameForLogging() {
  return "MergeSessionNavigationThrottle";
}

void MergeSessionNavigationThrottle::OnBlockingPageComplete() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  Resume();
}
