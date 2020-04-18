// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_ACCOUNT_TRACKER_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_SIGNIN_ACCOUNT_TRACKER_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class AccountTrackerService;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ios {

class ChromeBrowserState;

// Singleton that owns all AccountTrackerServices and associates them with
// ios::ChromeBrowserState.
class AccountTrackerServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Returns the instance of AccountTrackerService associated with this
  // browser state (creating one if none exists). Returns nullptr if this
  // browser state cannot have a AccountTrackerService (for example, if
  // |browser_state| is incognito).
  static AccountTrackerService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

  // Returns the instance of the AccountTrackerServiceFactory singleton.
  static AccountTrackerServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AccountTrackerServiceFactory>;

  AccountTrackerServiceFactory();
  ~AccountTrackerServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(AccountTrackerServiceFactory);
};

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_SIGNIN_ACCOUNT_TRACKER_SERVICE_FACTORY_H_
