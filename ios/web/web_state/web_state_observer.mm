// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/web_state/web_state_observer.h"

#include "ios/web/public/load_committed_details.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

WebStateObserver::WebStateObserver() = default;

WebStateObserver::~WebStateObserver() = default;

}  // namespace web
