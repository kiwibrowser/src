// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/captive_portal/captive_portal_service_factory.h"

#include "chrome/browser/captive_portal/captive_portal_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
CaptivePortalService* CaptivePortalServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<CaptivePortalService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
CaptivePortalServiceFactory* CaptivePortalServiceFactory::GetInstance() {
  return base::Singleton<CaptivePortalServiceFactory>::get();
}

CaptivePortalServiceFactory::CaptivePortalServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "CaptivePortalService",
        BrowserContextDependencyManager::GetInstance()) {
}

CaptivePortalServiceFactory::~CaptivePortalServiceFactory() {
}

KeyedService* CaptivePortalServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new CaptivePortalService(static_cast<Profile*>(profile));
}

content::BrowserContext* CaptivePortalServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
