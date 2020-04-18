// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/language/language_model_factory.h"

#include "base/feature_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/language/content/browser/geo_language_model.h"
#include "components/language/content/browser/geo_language_provider.h"
#include "components/language/core/browser/baseline_language_model.h"
#include "components/language/core/browser/heuristic_language_model.h"
#include "components/language/core/browser/pref_names.h"
#include "components/language/core/common/language_experiments.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

// static
LanguageModelFactory* LanguageModelFactory::GetInstance() {
  return base::Singleton<LanguageModelFactory>::get();
}

// static
language::LanguageModel* LanguageModelFactory::GetForBrowserContext(
    content::BrowserContext* const browser_context) {
  return static_cast<language::LanguageModel*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

LanguageModelFactory::LanguageModelFactory()
    : BrowserContextKeyedServiceFactory(
          "LanguageModel",
          BrowserContextDependencyManager::GetInstance()) {}

LanguageModelFactory::~LanguageModelFactory() {}

KeyedService* LanguageModelFactory::BuildServiceInstanceFor(
    content::BrowserContext* const browser_context) const {
  language::OverrideLanguageModel override_model_mode =
      language::GetOverrideLanguageModel();
  Profile* const profile = Profile::FromBrowserContext(browser_context);

  if (override_model_mode == language::OverrideLanguageModel::HEURISTIC) {
    return new language::HeuristicLanguageModel(
        profile->GetPrefs(), g_browser_process->GetApplicationLocale(),
        prefs::kAcceptLanguages, language::prefs::kUserLanguageProfile);
  }

  if (override_model_mode == language::OverrideLanguageModel::GEO) {
    return new language::GeoLanguageModel(
        language::GeoLanguageProvider::GetInstance());
  }

  return new language::BaselineLanguageModel(
      profile->GetPrefs(), g_browser_process->GetApplicationLocale(),
      prefs::kAcceptLanguages);
}

content::BrowserContext* LanguageModelFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Use the original profile's language model even in Incognito mode.
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

void LanguageModelFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* const registry) {
  if (base::FeatureList::IsEnabled(language::kUseHeuristicLanguageModel)) {
    registry->RegisterDictionaryPref(
        language::prefs::kUserLanguageProfile,
        user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  }
}
