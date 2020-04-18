// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_IDENTITY_SERVICE_CREATOR_H_
#define IOS_CHROME_BROWSER_SIGNIN_IDENTITY_SERVICE_CREATOR_H_

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

// Registers an Identity Service factory function associated with
// |browser_state| in the given service map.
void RegisterIdentityServiceForBrowserState(
    ios::ChromeBrowserState* browser_state,
    web::BrowserState::StaticServiceMap* services);

#endif  // IOS_CHROME_BROWSER_SIGNIN_IDENTITY_SERVICE_CREATOR_H_
