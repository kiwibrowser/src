// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GOOGLE_SEARCH_URL_TRACKER_FACTORY_H_
#define CHROME_BROWSER_GOOGLE_SEARCH_URL_TRACKER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class SearchURLTracker;
class Profile;

// Singleton that owns all SearchURLTrackers and associates them with Profiles.
class SearchURLTrackerFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the SearchURLTracker for |profile|.  This may return NULL for a
  // testing profile.
  static SearchURLTracker* GetForProfile(Profile* profile);

  static SearchURLTrackerFactory* GetInstance();

  // Returns the default factory used to build SearchURLTracker. Can be
  // registered with SetTestingFactory to use a real SearchURLTracker instance
  // for testing.
  static TestingFactoryFunction GetDefaultFactory();

 private:
  friend struct base::DefaultSingletonTraits<SearchURLTrackerFactory>;
  friend class SearchURLTrackerFactoryTest;

  SearchURLTrackerFactory();
  ~SearchURLTrackerFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(SearchURLTrackerFactory);
};

#endif  // CHROME_BROWSER_GOOGLE_SEARCH_URL_TRACKER_FACTORY_H_
