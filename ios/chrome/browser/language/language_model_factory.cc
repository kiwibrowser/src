// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/language/language_model_factory.h"

#include "base/feature_list.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/language/core/browser/baseline_language_model.h"
#include "components/language/core/browser/heuristic_language_model.h"
#include "components/language/core/browser/language_model.h"
#include "components/language/core/browser/pref_names.h"
#include "components/language/core/common/language_experiments.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/pref_names.h"

// static
LanguageModelFactory* LanguageModelFactory::GetInstance() {
  return base::Singleton<LanguageModelFactory>::get();
}

// static
language::LanguageModel* LanguageModelFactory::GetForBrowserState(
    ios::ChromeBrowserState* const state) {
  return static_cast<language::LanguageModel*>(
      GetInstance()->GetServiceForBrowserState(state, true));
}

LanguageModelFactory::LanguageModelFactory()
    : BrowserStateKeyedServiceFactory(
          "LanguageModel",
          BrowserStateDependencyManager::GetInstance()) {}

LanguageModelFactory::~LanguageModelFactory() {}

std::unique_ptr<KeyedService> LanguageModelFactory::BuildServiceInstanceFor(
    web::BrowserState* const state) const {
  language::OverrideLanguageModel override_model_mode =
      language::GetOverrideLanguageModel();
  ios::ChromeBrowserState* const chrome_state =
      ios::ChromeBrowserState::FromBrowserState(state);

  if (override_model_mode == language::OverrideLanguageModel::HEURISTIC) {
    return std::make_unique<language::HeuristicLanguageModel>(
        chrome_state->GetPrefs(),
        GetApplicationContext()->GetApplicationLocale(),
        prefs::kAcceptLanguages, language::prefs::kUserLanguageProfile);
  }

  // language::OverrideLanguageModel::GEO is not supported on iOS yet.

  return std::make_unique<language::BaselineLanguageModel>(
      chrome_state->GetPrefs(), GetApplicationContext()->GetApplicationLocale(),
      prefs::kAcceptLanguages);
}

web::BrowserState* LanguageModelFactory::GetBrowserStateToUse(
    web::BrowserState* const state) const {
  // Use the original profile's language model even in Incognito mode.
  return GetBrowserStateRedirectedInIncognito(state);
}

void LanguageModelFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* const registry) {
  if (base::FeatureList::IsEnabled(language::kUseHeuristicLanguageModel)) {
    registry->RegisterDictionaryPref(
        language::prefs::kUserLanguageProfile,
        user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  }
}
