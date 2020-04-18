// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/bookmark/bookmark_tracker_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/feature_engagement/bookmark/bookmark_tracker.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace feature_engagement {

// static
BookmarkTrackerFactory* BookmarkTrackerFactory::GetInstance() {
  return base::Singleton<BookmarkTrackerFactory>::get();
}

BookmarkTracker* BookmarkTrackerFactory::GetForProfile(Profile* profile) {
  return static_cast<BookmarkTracker*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

BookmarkTrackerFactory::BookmarkTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "BookmarkTracker",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(TrackerFactory::GetInstance());
}

BookmarkTrackerFactory::~BookmarkTrackerFactory() = default;

KeyedService* BookmarkTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BookmarkTracker(Profile::FromBrowserContext(context));
}

content::BrowserContext* BookmarkTrackerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool BookmarkTrackerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace feature_engagement
