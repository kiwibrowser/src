// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_SUGGESTIONS_SUGGESTIONS_SERVICE_FACTORY_H_
#define CHROME_BROWSER_SEARCH_SUGGESTIONS_SUGGESTIONS_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace suggestions {

class SuggestionsService;

// Singleton that owns all SuggestionsServices and associates them with
// Profiles.
class SuggestionsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the SuggestionsService for |profile|.
  static suggestions::SuggestionsService* GetForProfile(Profile* profile);

  static SuggestionsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<SuggestionsServiceFactory>;

  SuggestionsServiceFactory();
  ~SuggestionsServiceFactory() override;

  // Overrides from BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  DISALLOW_COPY_AND_ASSIGN(SuggestionsServiceFactory);
};

}  // namespace suggestions

#endif  // CHROME_BROWSER_SEARCH_SUGGESTIONS_SUGGESTIONS_SERVICE_FACTORY_H_
