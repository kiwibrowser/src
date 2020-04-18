// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_READING_LIST_READING_LIST_DOWNLOAD_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_READING_LIST_READING_LIST_DOWNLOAD_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class ReadingListDownloadService;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace ios {
class ChromeBrowserState;
}

// Singleton that creates the ReadingListDownloadService and associates that
// service with ios::ChromeBrowserState.
class ReadingListDownloadServiceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  static ReadingListDownloadService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static ReadingListDownloadServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ReadingListDownloadServiceFactory>;

  ReadingListDownloadServiceFactory();
  ~ReadingListDownloadServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(ReadingListDownloadServiceFactory);
};

#endif  // IOS_CHROME_BROWSER_READING_LIST_READING_LIST_DOWNLOAD_SERVICE_FACTORY_H_
