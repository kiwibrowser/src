// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/web_state/global_web_state_observer.h"

#include "ios/web/web_state/global_web_state_event_tracker.h"

namespace web {

GlobalWebStateObserver::GlobalWebStateObserver() {
  GlobalWebStateEventTracker::GetInstance()->AddObserver(this);
}

GlobalWebStateObserver::~GlobalWebStateObserver() {
  GlobalWebStateEventTracker::GetInstance()->RemoveObserver(this);
}

}  // namespace web
