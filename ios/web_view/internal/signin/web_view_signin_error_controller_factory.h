// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_SIGNIN_ERROR_CONTROLLER_FACTORY_H_
#define IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_SIGNIN_ERROR_CONTROLLER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class SigninErrorController;

namespace ios_web_view {

class WebViewBrowserState;

// Singleton that owns all SigninErrorControllers and associates them with
// a browser state.
class WebViewSigninErrorControllerFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  static SigninErrorController* GetForBrowserState(
      ios_web_view::WebViewBrowserState* browser_state);
  static WebViewSigninErrorControllerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      WebViewSigninErrorControllerFactory>;

  WebViewSigninErrorControllerFactory();
  ~WebViewSigninErrorControllerFactory() override = default;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(WebViewSigninErrorControllerFactory);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_SIGNIN_ERROR_CONTROLLER_FACTORY_H_
