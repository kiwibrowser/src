// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DOM_DISTILLER_DOM_DISTILLER_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_DOM_DISTILLER_DOM_DISTILLER_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace dom_distiller {
class DomDistillerService;
}

namespace ios {
class ChromeBrowserState;
}

namespace dom_distiller {

class DomDistillerServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static DomDistillerServiceFactory* GetInstance();
  static DomDistillerService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

 private:
  friend struct base::DefaultSingletonTraits<DomDistillerServiceFactory>;

  DomDistillerServiceFactory();
  ~DomDistillerServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY(DomDistillerServiceFactory);
};

}  // namespace dom_distiller

#endif  // IOS_CHROME_BROWSER_DOM_DISTILLER_DOM_DISTILLER_SERVICE_FACTORY_H_
