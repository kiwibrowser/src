// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_WEBDATA_SERVICES_WEB_VIEW_WEB_DATA_SERVICE_WRAPPER_FACTORY_H
#define IOS_WEB_VIEW_INTERNAL_WEBDATA_SERVICES_WEB_VIEW_WEB_DATA_SERVICE_WRAPPER_FACTORY_H

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class TokenWebData;
class WebDataServiceWrapper;
enum class ServiceAccessType;

namespace autofill {
class AutofillWebDataService;
}

namespace ios_web_view {

class WebViewBrowserState;

// Singleton that owns all WebDataServiceWrappers and associates them with
// a browser state.
class WebViewWebDataServiceWrapperFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  // Returns the AutofillWebDataService associated with |browser_state|.
  static WebDataServiceWrapper* GetForBrowserState(
      WebViewBrowserState* browser_state,
      ServiceAccessType access_type);
  static WebDataServiceWrapper* GetForBrowserStateIfExists(
      WebViewBrowserState* browser_state,
      ServiceAccessType access_type);

  // Returns the AutofillWebDataService associated with |browser_state|.
  static scoped_refptr<autofill::AutofillWebDataService>
  GetAutofillWebDataForBrowserState(WebViewBrowserState* browser_state,
                                    ServiceAccessType access_type);

  // Returns the TokenWebData associated with |browser_state|.
  static scoped_refptr<TokenWebData> GetTokenWebDataForBrowserState(
      WebViewBrowserState* browser_state,
      ServiceAccessType access_type);

  static WebViewWebDataServiceWrapperFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      WebViewWebDataServiceWrapperFactory>;

  WebViewWebDataServiceWrapperFactory();
  ~WebViewWebDataServiceWrapperFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(WebViewWebDataServiceWrapperFactory);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_WEBDATA_SERVICES_WEB_VIEW_WEB_DATA_SERVICE_WRAPPER_FACTORY_H
