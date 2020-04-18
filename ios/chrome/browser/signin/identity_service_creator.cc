// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/identity_service_creator.h"

#include <memory>

#include "components/signin/core/browser/signin_manager.h"
#include "ios/chrome/browser/signin/account_tracker_service_factory.h"
#include "ios/chrome/browser/signin/oauth2_token_service_factory.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#include "services/identity/identity_service.h"
#include "services/identity/public/mojom/constants.mojom.h"

namespace {

// Creates an instance of the Identity Service for
// |browser_state|, populating it with the appropriate instances of
// its dependencies.
std::unique_ptr<service_manager::Service> CreateIdentityService(
    ios::ChromeBrowserState* browser_state) {
  AccountTrackerService* account_tracker =
      ios::AccountTrackerServiceFactory::GetForBrowserState(browser_state);
  SigninManagerBase* signin_manager =
      ios::SigninManagerFactory::GetForBrowserState(browser_state);
  ProfileOAuth2TokenService* token_service =
      OAuth2TokenServiceFactory::GetForBrowserState(browser_state);
  return std::make_unique<identity::IdentityService>(
      account_tracker, signin_manager, token_service);
}

}  //  namespace

void RegisterIdentityServiceForBrowserState(
    ios::ChromeBrowserState* browser_state,
    web::BrowserState::StaticServiceMap* services) {
  service_manager::EmbeddedServiceInfo identity_service_info;

  // The Identity Service must run on the UI thread.
  identity_service_info.task_runner = base::ThreadTaskRunnerHandle::Get();

  // NOTE: The dependencies of the Identity Service have not yet been created,
  // so it is not possible to bind them here. Instead, bind them at the time
  // of the actual request to create the Identity Service.
  identity_service_info.factory = base::BindRepeating(
      &CreateIdentityService, base::Unretained(browser_state));
  services->insert(
      std::make_pair(identity::mojom::kServiceName, identity_service_info));
}
