// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GCM_INSTANCE_ID_IOS_CHROME_INSTANCE_ID_PROFILE_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_GCM_INSTANCE_ID_IOS_CHROME_INSTANCE_ID_PROFILE_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace instance_id {
class InstanceIDProfileService;
}

namespace ios {
class ChromeBrowserState;
}

// Singleton that owns all InstanceIDProfileService and associates them with
// ios::ChromeBrowserState.
class IOSChromeInstanceIDProfileServiceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  static instance_id::InstanceIDProfileService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

  static IOSChromeInstanceIDProfileServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      IOSChromeInstanceIDProfileServiceFactory>;

  IOSChromeInstanceIDProfileServiceFactory();
  ~IOSChromeInstanceIDProfileServiceFactory() override;

  // BrowserStateKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(IOSChromeInstanceIDProfileServiceFactory);
};

#endif  // IOS_CHROME_BROWSER_GCM_INSTANCE_ID_IOS_CHROME_INSTANCE_ID_PROFILE_SERVICE_FACTORY_H_
