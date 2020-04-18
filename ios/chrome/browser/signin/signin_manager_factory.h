// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_SIGNIN_MANAGER_FACTORY_H_
#define IOS_CHROME_BROWSER_SIGNIN_SIGNIN_MANAGER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class SigninManagerFactoryObserver;
class SigninManager;
class PrefRegistrySimple;

namespace ios {

class ChromeBrowserState;

// Singleton that owns all SigninManagers and associates them with browser
// states.
class SigninManagerFactory : public BrowserStateKeyedServiceFactory {
 public:
  static SigninManager* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static SigninManager* GetForBrowserStateIfExists(
      ios::ChromeBrowserState* browser_state);

  // Returns an instance of the SigninManagerFactory singleton.
  static SigninManagerFactory* GetInstance();

  // Implementation of BrowserStateKeyedServiceFactory (public so tests
  // can call it).
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  // Registers the browser-global prefs used by SigninManager.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Methods to register or remove observers of SigninManager creation/shutdown.
  void AddObserver(SigninManagerFactoryObserver* observer);
  void RemoveObserver(SigninManagerFactoryObserver* observer);

  // Notifies observers of |manager|'s creation. Should be called only by test
  // SigninManager subclasses whose construction does not occur in
  // |BuildServiceInstanceFor()|.
  void NotifyObserversOfSigninManagerCreationForTesting(SigninManager* manager);

 private:
  friend struct base::DefaultSingletonTraits<SigninManagerFactory>;

  SigninManagerFactory();
  ~SigninManagerFactory() override;

  // List of observers. Checks that list is empty on destruction.
  mutable base::ObserverList<SigninManagerFactoryObserver, true> observer_list_;

  // BrowserStateKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  void BrowserStateShutdown(web::BrowserState* context) override;
};
}

#endif  // IOS_CHROME_BROWSER_SIGNIN_SIGNIN_MANAGER_FACTORY_H_
