// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/perf/windowed_incognito_observer.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"

namespace metrics {

WindowedIncognitoObserver::WindowedIncognitoObserver()
    : incognito_launched_(false) {
  BrowserList::AddObserver(this);
}

WindowedIncognitoObserver::~WindowedIncognitoObserver() {
  BrowserList::RemoveObserver(this);
}

void WindowedIncognitoObserver::OnBrowserAdded(Browser* browser) {
  if (browser->profile()->IsOffTheRecord())
    incognito_launched_ = true;
}

}  // namespace metrics
