// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_FAKE_SIGNIN_MANAGER_BUILDER_H_
#define IOS_CHROME_BROWSER_SIGNIN_FAKE_SIGNIN_MANAGER_BUILDER_H_

#include <memory>

namespace web {
class BrowserState;
}

class KeyedService;

namespace ios {

// Helper function to be used with KeyedService::SetTestingFactory().
// The returned instance is initialized.
std::unique_ptr<KeyedService> BuildFakeSigninManager(
    web::BrowserState* browser_state);

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_SIGNIN_FAKE_SIGNIN_MANAGER_BUILDER_H_
