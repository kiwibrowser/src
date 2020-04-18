// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_IOS_BROWSER_MERGE_SESSION_OBSERVER_BRIDGE_H_
#define COMPONENTS_SIGNIN_IOS_BROWSER_MERGE_SESSION_OBSERVER_BRIDGE_H_

#import <Foundation/Foundation.h>

#include <string>

#include "base/macros.h"
#include "components/signin/core/browser/gaia_cookie_manager_service.h"

class GoogleServiceAuthError;

@protocol MergeSessionObserverBridgeDelegate

// Informs the delegate that the merge session operation for |account_id| has
// finished. If there was an error, it will be described in |error|.
- (void)onMergeSessionCompleted:(const std::string&)account_id
                          error:(const GoogleServiceAuthError&)error;

@end

// C++ class to monitor merge session status in Objective C type.
class MergeSessionObserverBridge : public GaiaCookieManagerService::Observer {
 public:
  MergeSessionObserverBridge(id<MergeSessionObserverBridgeDelegate> delegate,
                             GaiaCookieManagerService* cookie_manager_service);
  ~MergeSessionObserverBridge() override;

  void OnAddAccountToCookieCompleted(
      const std::string& account_id,
      const GoogleServiceAuthError& error) override;

 private:
  __weak id<MergeSessionObserverBridgeDelegate> delegate_;
  GaiaCookieManagerService* cookie_manager_service_;

  DISALLOW_COPY_AND_ASSIGN(MergeSessionObserverBridge);
};

#endif  // COMPONENTS_SIGNIN_IOS_BROWSER_MERGE_SESSION_OBSERVER_BRIDGE_H_
