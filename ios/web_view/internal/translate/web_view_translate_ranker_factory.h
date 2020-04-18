// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_TRANSLATE_WEB_VIEW_TRANSLATE_RANKER_FACTORY_H_
#define IOS_WEB_VIEW_INTERNAL_TRANSLATE_WEB_VIEW_TRANSLATE_RANKER_FACTORY_H_

#include <memory>
#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace translate {
class TranslateRanker;
}

namespace ios_web_view {

class WebViewBrowserState;

// TranslateRankerFactory is a way to associate a
// TranslateRanker instance to a BrowserState.
class WebViewTranslateRankerFactory : public BrowserStateKeyedServiceFactory {
 public:
  static translate::TranslateRanker* GetForBrowserState(
      WebViewBrowserState* browser_state);
  static WebViewTranslateRankerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<WebViewTranslateRankerFactory>;

  WebViewTranslateRankerFactory();
  ~WebViewTranslateRankerFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(WebViewTranslateRankerFactory);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_TRANSLATE_WEB_VIEW_TRANSLATE_RANKER_FACTORY_H_
