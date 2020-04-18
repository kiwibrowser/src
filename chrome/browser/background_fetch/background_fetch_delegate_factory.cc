// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_fetch/background_fetch_delegate_factory.h"

#include "chrome/browser/background_fetch/background_fetch_delegate_impl.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/background_fetch_delegate.h"

// static
BackgroundFetchDelegateImpl* BackgroundFetchDelegateFactory::GetForProfile(
    Profile* profile) {
  return static_cast<BackgroundFetchDelegateImpl*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
BackgroundFetchDelegateFactory* BackgroundFetchDelegateFactory::GetInstance() {
  return base::Singleton<BackgroundFetchDelegateFactory>::get();
}

BackgroundFetchDelegateFactory::BackgroundFetchDelegateFactory()
    : BrowserContextKeyedServiceFactory(
          "BackgroundFetchService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(DownloadServiceFactory::GetInstance());
}

BackgroundFetchDelegateFactory::~BackgroundFetchDelegateFactory() {}

KeyedService* BackgroundFetchDelegateFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BackgroundFetchDelegateImpl(Profile::FromBrowserContext(context));
}
