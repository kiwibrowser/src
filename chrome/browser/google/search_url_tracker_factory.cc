// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/google/search_url_tracker_factory.h"

#include <utility>

#include "base/feature_list.h"
#include "chrome/browser/google/chrome_search_url_tracker_client.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/google/core/browser/google_pref_names.h"
#include "components/google/core/browser/search_url_tracker.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"

// static
SearchURLTracker* SearchURLTrackerFactory::GetForProfile(Profile* profile) {
  return static_cast<SearchURLTracker*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
SearchURLTrackerFactory* SearchURLTrackerFactory::GetInstance() {
  return base::Singleton<SearchURLTrackerFactory>::get();
}

namespace {

std::unique_ptr<KeyedService> BuildSearchURLTracker(
    content::BrowserContext* context) {
  auto client = std::make_unique<ChromeSearchURLTrackerClient>(
      Profile::FromBrowserContext(context));
  return std::make_unique<SearchURLTracker>(
      std::move(client),
      SearchURLTracker::NORMAL_MODE);
}

}  // namespace

// static
BrowserContextKeyedServiceFactory::TestingFactoryFunction
SearchURLTrackerFactory::GetDefaultFactory() {
  return &BuildSearchURLTracker;
}

SearchURLTrackerFactory::SearchURLTrackerFactory()
    : BrowserContextKeyedServiceFactory(
        "SearchURLTracker",
        BrowserContextDependencyManager::GetInstance()) {
}

SearchURLTrackerFactory::~SearchURLTrackerFactory() {
}

KeyedService* SearchURLTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return BuildSearchURLTracker(context).release();
}

void SearchURLTrackerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* user_prefs) {
  SearchURLTracker::RegisterProfilePrefs(user_prefs);
}

content::BrowserContext* SearchURLTrackerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool SearchURLTrackerFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

bool SearchURLTrackerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
