// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TRANSLATE_TRANSLATE_RANKER_FACTORY_H_
#define IOS_CHROME_BROWSER_TRANSLATE_TRANSLATE_RANKER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace ios {
class ChromeBrowserState;
}

namespace translate {

class TranslateRanker;

// TranslateRankerFactory is a way to associate a TranslateRanker instance to
// a BrowserState.
class TranslateRankerFactory : public BrowserStateKeyedServiceFactory {
 public:
  static translate::TranslateRanker* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static TranslateRankerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<TranslateRankerFactory>;

  TranslateRankerFactory();
  ~TranslateRankerFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(TranslateRankerFactory);
};

}  // namespace translate

#endif  // IOS_CHROME_BROWSER_TRANSLATE_TRANSLATE_RANKER_FACTORY_H_
