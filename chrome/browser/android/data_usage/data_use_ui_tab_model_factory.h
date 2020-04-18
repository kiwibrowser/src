// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_UI_TAB_MODEL_FACTORY_H_
#define CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_UI_TAB_MODEL_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace content {
class BrowserContext;
}

namespace android {

class DataUseUITabModel;

class DataUseUITabModelFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns a singleton instance of DataUseUITabModelFactory.
  static DataUseUITabModelFactory* GetInstance();

  // Returns the DataUseUITabModel associated with |context|.
  static DataUseUITabModel* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<DataUseUITabModelFactory>;

  DataUseUITabModelFactory();
  ~DataUseUITabModelFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* browser_context) const override;

  DISALLOW_COPY_AND_ASSIGN(DataUseUITabModelFactory);
};

}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_UI_TAB_MODEL_FACTORY_H_
