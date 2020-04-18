// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GOOGLE_GOOGLE_LOGO_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_GOOGLE_GOOGLE_LOGO_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ios {
class ChromeBrowserState;
}

class KeyedService;
class GoogleLogoService;

// Singleton that owns all GoogleLogoServices and associates them with
// ChromeBrowserState.
class GoogleLogoServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static GoogleLogoService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

  static GoogleLogoServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<GoogleLogoServiceFactory>;

  GoogleLogoServiceFactory();
  ~GoogleLogoServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(GoogleLogoServiceFactory);
};

#endif  // IOS_CHROME_BROWSER_GOOGLE_GOOGLE_LOGO_SERVICE_FACTORY_H_
