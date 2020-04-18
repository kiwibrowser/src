// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DATA_USE_MEASUREMENT_CHROME_DATA_USE_ASCRIBER_SERVICE_FACTORY_H_
#define CHROME_BROWSER_DATA_USE_MEASUREMENT_CHROME_DATA_USE_ASCRIBER_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace data_use_measurement {

class ChromeDataUseAscriberService;

class ChromeDataUseAscriberServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static ChromeDataUseAscriberService* GetForBrowserContext(
      content::BrowserContext* context);

  static ChromeDataUseAscriberServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ChromeDataUseAscriberService>;

  ChromeDataUseAscriberServiceFactory();
  ~ChromeDataUseAscriberServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

 private:
  friend struct base::DefaultSingletonTraits<
      ChromeDataUseAscriberServiceFactory>;

  DISALLOW_COPY_AND_ASSIGN(ChromeDataUseAscriberServiceFactory);
};

}  // namespace data_use_measurement

#endif  // CHROME_BROWSER_DATA_USE_MEASUREMENT_CHROME_DATA_USE_ASCRIBER_SERVICE_FACTORY_H_
