// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_FACTORY_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace ios {
class ChromeBrowserState;
}

class BrowserList;

// BrowserListFactory attaches BrowserLists to ChromeBrowserStates.
class BrowserListFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Convenience getter that typecasts the value returned to BrowserList.
  static BrowserList* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

  // Getter for singleton instance.
  static BrowserListFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BrowserListFactory>;

  BrowserListFactory();
  ~BrowserListFactory() override;

  // BrowserStateKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BrowserListFactory);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_FACTORY_H_
