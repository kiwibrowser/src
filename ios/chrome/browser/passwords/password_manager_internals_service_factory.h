// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_PASSWORD_MANAGER_INTERNALS_FACTORY_H_
#define IOS_CHROME_BROWSER_PASSWORDS_PASSWORD_MANAGER_INTERNALS_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace password_manager {
class PasswordManagerInternalsService;
}

namespace ios {
class ChromeBrowserState;

// Singleton that owns all PasswordStores and associates them with
// ios::ChromeBrowserState.
class PasswordManagerInternalsServiceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  static password_manager::PasswordManagerInternalsService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

  static PasswordManagerInternalsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      PasswordManagerInternalsServiceFactory>;

  PasswordManagerInternalsServiceFactory();
  ~PasswordManagerInternalsServiceFactory() override;

  // BrowserStateKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerInternalsServiceFactory);
};

}  // namespace ios
#endif  // IOS_CHROME_BROWSER_PASSWORDS_PASSWORD_MANAGER_INTERNALS_FACTORY_H_
