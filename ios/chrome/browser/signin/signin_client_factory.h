// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_SIGNIN_CLIENT_FACTORY_H_
#define IOS_CHROME_BROWSER_SIGNIN_SIGNIN_CLIENT_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class SigninClient;

namespace ios {
class ChromeBrowserState;
}

// Singleton that owns all SigninErrorControllers and associates them with
// ios::ChromeBrowserState.
class SigninClientFactory : public BrowserStateKeyedServiceFactory {
 public:
  static SigninClient* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static SigninClientFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<SigninClientFactory>;

  SigninClientFactory();
  ~SigninClientFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(SigninClientFactory);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_SIGNIN_CLIENT_FACTORY_H_
