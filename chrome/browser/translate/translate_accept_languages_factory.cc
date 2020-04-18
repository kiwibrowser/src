// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/translate/translate_accept_languages_factory.h"

#include "base/macros.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_service.h"
#include "components/translate/core/browser/translate_accept_languages.h"

namespace {

// TranslateAcceptLanguagesService is a thin container for
// TranslateAcceptLanguages to enable associating it with a BrowserContext.
class TranslateAcceptLanguagesService : public KeyedService {
 public:
  explicit TranslateAcceptLanguagesService(PrefService* prefs);
  ~TranslateAcceptLanguagesService() override;

  // Returns the associated TranslateAcceptLanguages.
  translate::TranslateAcceptLanguages& accept_languages() {
    return accept_languages_;
  }

 private:
  translate::TranslateAcceptLanguages accept_languages_;
  DISALLOW_COPY_AND_ASSIGN(TranslateAcceptLanguagesService);
};

TranslateAcceptLanguagesService::TranslateAcceptLanguagesService(
    PrefService* prefs)
    : accept_languages_(prefs, prefs::kAcceptLanguages) {}

TranslateAcceptLanguagesService::~TranslateAcceptLanguagesService() {}

}  // namespace

// static
TranslateAcceptLanguagesFactory*
TranslateAcceptLanguagesFactory::GetInstance() {
  return base::Singleton<TranslateAcceptLanguagesFactory>::get();
}

// static
translate::TranslateAcceptLanguages*
TranslateAcceptLanguagesFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  TranslateAcceptLanguagesService* service =
      static_cast<TranslateAcceptLanguagesService*>(
          GetInstance()->GetServiceForBrowserContext(context, true));
  return &service->accept_languages();
}

TranslateAcceptLanguagesFactory::TranslateAcceptLanguagesFactory()
    : BrowserContextKeyedServiceFactory(
          "TranslateAcceptLanguagesService",
          BrowserContextDependencyManager::GetInstance()) {}

TranslateAcceptLanguagesFactory::~TranslateAcceptLanguagesFactory() {}

KeyedService* TranslateAcceptLanguagesFactory::BuildServiceInstanceFor(
    content::BrowserContext* browser_context) const {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  return new TranslateAcceptLanguagesService(profile->GetPrefs());
}

content::BrowserContext*
TranslateAcceptLanguagesFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
