// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/budget_service/budget_manager_factory.h"

#include "base/time/default_clock.h"
#include "chrome/browser/budget_service/budget_manager.h"
#include "chrome/browser/engagement/site_engagement_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

// static
BudgetManager* BudgetManagerFactory::GetForProfile(
    content::BrowserContext* profile) {
  return static_cast<BudgetManager*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
BudgetManagerFactory* BudgetManagerFactory::GetInstance() {
  return base::Singleton<BudgetManagerFactory>::get();
}

BudgetManagerFactory::BudgetManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "BudgetManager",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SiteEngagementServiceFactory::GetInstance());
}

content::BrowserContext* BudgetManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

KeyedService* BudgetManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  DCHECK(profile);
  return new BudgetManager(static_cast<Profile*>(profile));
}
