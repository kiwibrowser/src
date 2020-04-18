// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_MODEL_FACTORY_H_
#define CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_MODEL_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace offline_pages {

class OfflinePageModel;

// A factory to create one unique OfflinePageModel. Offline pages are not
// supported in incognito, and this class uses default implementation of
// |GetBrowserContextToUse|.
// TODO(fgorski): Add an integration test that ensures incognito users don't
// save or open offline pages.
class OfflinePageModelFactory : public BrowserContextKeyedServiceFactory {
 public:
  static OfflinePageModelFactory* GetInstance();
  static OfflinePageModel* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<OfflinePageModelFactory>;

  OfflinePageModelFactory();
  ~OfflinePageModelFactory() override {}

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(OfflinePageModelFactory);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_MODEL_FACTORY_H_
