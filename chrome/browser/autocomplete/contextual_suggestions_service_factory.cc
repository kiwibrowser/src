// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/contextual_suggestions_service_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/omnibox/browser/contextual_suggestions_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"

// static
ContextualSuggestionsService*
ContextualSuggestionsServiceFactory::GetForProfile(Profile* profile,
                                                   bool create_if_necessary) {
  return static_cast<ContextualSuggestionsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, create_if_necessary));
}

// static
ContextualSuggestionsServiceFactory*
ContextualSuggestionsServiceFactory::GetInstance() {
  return base::Singleton<ContextualSuggestionsServiceFactory>::get();
}

KeyedService* ContextualSuggestionsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile);
  ProfileOAuth2TokenService* token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  return new ContextualSuggestionsService(signin_manager, token_service,
                                          profile->GetRequestContext());
}

ContextualSuggestionsServiceFactory::ContextualSuggestionsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ContextualSuggestionsService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SigninManagerFactory::GetInstance());
  DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
}

ContextualSuggestionsServiceFactory::~ContextualSuggestionsServiceFactory() {}
