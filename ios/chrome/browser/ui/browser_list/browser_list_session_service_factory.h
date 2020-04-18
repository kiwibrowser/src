// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_SESSION_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_SESSION_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace ios {
class ChromeBrowserState;
}

class BrowserListSessionService;

// BrowserListSessionServiceFactory attaches BrowserListSessionService to
// ChromeBrowserState.
class BrowserListSessionServiceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  static BrowserListSessionService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static BrowserListSessionServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BrowserListSessionServiceFactory>;

  BrowserListSessionServiceFactory();
  ~BrowserListSessionServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(BrowserListSessionServiceFactory);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_SESSION_SERVICE_FACTORY_H_
