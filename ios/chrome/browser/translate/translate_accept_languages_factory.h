// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TRANSLATE_TRANSLATE_ACCEPT_LANGUAGES_FACTORY_H_
#define IOS_CHROME_BROWSER_TRANSLATE_TRANSLATE_ACCEPT_LANGUAGES_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace translate {
class TranslateAcceptLanguages;
}

namespace ios {
class ChromeBrowserState;
}

// TranslateAcceptLanguagesFactory is a way to associate a
// TranslateAcceptLanguages instance to a BrowserState.
class TranslateAcceptLanguagesFactory : public BrowserStateKeyedServiceFactory {
 public:
  static translate::TranslateAcceptLanguages* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static TranslateAcceptLanguagesFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<TranslateAcceptLanguagesFactory>;

  TranslateAcceptLanguagesFactory();
  ~TranslateAcceptLanguagesFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(TranslateAcceptLanguagesFactory);
};

#endif  // IOS_CHROME_BROWSER_TRANSLATE_TRANSLATE_ACCEPT_LANGUAGES_FACTORY_H_
