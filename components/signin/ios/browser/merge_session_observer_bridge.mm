// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/ios/browser/merge_session_observer_bridge.h"

#include "base/logging.h"
#include "google_apis/gaia/google_service_auth_error.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

MergeSessionObserverBridge::MergeSessionObserverBridge(
    id<MergeSessionObserverBridgeDelegate> delegate,
    GaiaCookieManagerService* cookie_manager_service)
    : delegate_(delegate), cookie_manager_service_(cookie_manager_service) {
  DCHECK(delegate);
  DCHECK(cookie_manager_service);
  cookie_manager_service_->AddObserver(this);
}

MergeSessionObserverBridge::~MergeSessionObserverBridge() {
  cookie_manager_service_->RemoveObserver(this);
}

void MergeSessionObserverBridge::OnAddAccountToCookieCompleted(
    const std::string& account_id,
    const GoogleServiceAuthError& error) {
  [delegate_ onMergeSessionCompleted:account_id error:error];
}
