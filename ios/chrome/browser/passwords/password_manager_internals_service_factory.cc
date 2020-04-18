// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/password_manager_internals_service_factory.h"

#include <memory>
#include <utility>

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/password_manager/core/browser/password_manager_internals_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

namespace ios {

// static
password_manager::PasswordManagerInternalsService*
PasswordManagerInternalsServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<password_manager::PasswordManagerInternalsService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
PasswordManagerInternalsServiceFactory*
PasswordManagerInternalsServiceFactory::GetInstance() {
  return base::Singleton<PasswordManagerInternalsServiceFactory>::get();
}

PasswordManagerInternalsServiceFactory::PasswordManagerInternalsServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "PasswordManagerInternalsService",
          BrowserStateDependencyManager::GetInstance()) {}

PasswordManagerInternalsServiceFactory::
    ~PasswordManagerInternalsServiceFactory() {}

std::unique_ptr<KeyedService>
PasswordManagerInternalsServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return std::make_unique<password_manager::PasswordManagerInternalsService>();
}

}  // namespace ios
