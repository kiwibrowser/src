// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DOWNLOAD_BROWSER_DOWNLOAD_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_DOWNLOAD_BROWSER_DOWNLOAD_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class BrowserDownloadService;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace web {
class BrowserState;
}  // namespace web

// Singleton that creates BrowserDownloadService and associates that service
// with web::BrowserState.
class BrowserDownloadServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static BrowserDownloadService* GetForBrowserState(
      web::BrowserState* browser_state);
  static BrowserDownloadServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BrowserDownloadServiceFactory>;

  BrowserDownloadServiceFactory();
  ~BrowserDownloadServiceFactory() override;

  // BrowserStateKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsCreatedWithBrowserState() const override;
  web::BrowserState* GetBrowserStateToUse(web::BrowserState*) const override;

  DISALLOW_COPY_AND_ASSIGN(BrowserDownloadServiceFactory);
};

#endif  // IOS_CHROME_BROWSER_DOWNLOAD_BROWSER_DOWNLOAD_SERVICE_FACTORY_H_
