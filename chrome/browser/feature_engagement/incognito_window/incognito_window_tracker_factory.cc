// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/incognito_window/incognito_window_tracker_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/feature_engagement/incognito_window/incognito_window_tracker.h"
#include "chrome/browser/feature_engagement/session_duration_updater.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/browser_context.h"

namespace feature_engagement {

// static
IncognitoWindowTrackerFactory* IncognitoWindowTrackerFactory::GetInstance() {
  return base::Singleton<IncognitoWindowTrackerFactory>::get();
}

IncognitoWindowTracker* IncognitoWindowTrackerFactory::GetForProfile(
    Profile* profile) {
  return static_cast<IncognitoWindowTracker*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

IncognitoWindowTrackerFactory::IncognitoWindowTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "IncognitoWindowTracker",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(TrackerFactory::GetInstance());
}

IncognitoWindowTrackerFactory::~IncognitoWindowTrackerFactory() = default;

KeyedService* IncognitoWindowTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new IncognitoWindowTracker(Profile::FromBrowserContext(context));
}

content::BrowserContext* IncognitoWindowTrackerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool IncognitoWindowTrackerFactory::ServiceIsCreatedWithBrowserContext() const {
  // Start IncognitoWindowTracker early so the incognito window in-product help
  // starts tracking.
  return true;
}

bool IncognitoWindowTrackerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace feature_engagement
