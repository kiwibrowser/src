// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/google/google_url_tracker_client_impl.h"

#include "base/logging.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

GoogleURLTrackerClientImpl::GoogleURLTrackerClientImpl(
    ios::ChromeBrowserState* browser_state)
    : browser_state_(browser_state) {
  DCHECK(browser_state_);
}

GoogleURLTrackerClientImpl::~GoogleURLTrackerClientImpl() {
}

bool GoogleURLTrackerClientImpl::IsBackgroundNetworkingEnabled() {
  return true;
}

PrefService* GoogleURLTrackerClientImpl::GetPrefs() {
  return browser_state_->GetPrefs();
}

network::mojom::URLLoaderFactory*
GoogleURLTrackerClientImpl::GetURLLoaderFactory() {
  return browser_state_->GetURLLoaderFactory();
}
