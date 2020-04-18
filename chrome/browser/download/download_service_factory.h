// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_SERVICE_FACTORY_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "chrome/browser/download/download_service_factory.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace download {
class DownloadService;
}  // namespace download

// DownloadServiceFactory is the main client class for interaction with the
// download component.
class DownloadServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns singleton instance of DownloadServiceFactory.
  static DownloadServiceFactory* GetInstance();

  // Returns the DownloadService associated with |context|.
  static download::DownloadService* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<DownloadServiceFactory>;

  DownloadServiceFactory();
  ~DownloadServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(DownloadServiceFactory);
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_SERVICE_FACTORY_H_
