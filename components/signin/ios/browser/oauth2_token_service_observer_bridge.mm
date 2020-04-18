// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/ios/browser/oauth2_token_service_observer_bridge.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

OAuth2TokenServiceObserverBridge::OAuth2TokenServiceObserverBridge(
    OAuth2TokenService* token_service,
    id<OAuth2TokenServiceObserverBridgeDelegate> delegate)
        : token_service_(token_service),
          delegate_(delegate) {
    DCHECK(token_service_);
    token_service_->AddObserver(this);
}
OAuth2TokenServiceObserverBridge::~OAuth2TokenServiceObserverBridge() {
  token_service_->RemoveObserver(this);
}

void OAuth2TokenServiceObserverBridge::OnRefreshTokenAvailable(
    const std::string& account_id) {
  if ([delegate_ respondsToSelector:@selector(onRefreshTokenAvailable:)]) {
    [delegate_ onRefreshTokenAvailable:account_id];
  }
}

void OAuth2TokenServiceObserverBridge::OnRefreshTokenRevoked(
    const std::string& account_id) {
  if ([delegate_ respondsToSelector:@selector(onRefreshTokenRevoked:)]) {
    [delegate_ onRefreshTokenRevoked:account_id];
  }
}
void OAuth2TokenServiceObserverBridge::OnRefreshTokensLoaded() {
  if ([delegate_ respondsToSelector:@selector(onRefreshTokensLoaded)]) {
    [delegate_ onRefreshTokensLoaded];
  }
}
void OAuth2TokenServiceObserverBridge::OnStartBatchChanges() {
  if ([delegate_ respondsToSelector:@selector(onStartBatchChanges)]) {
    [delegate_ onStartBatchChanges];
  }

}

void OAuth2TokenServiceObserverBridge::OnEndBatchChanges() {
  if ([delegate_ respondsToSelector:@selector(onEndBatchChanges)]) {
    [delegate_ onEndBatchChanges];
  }
}
