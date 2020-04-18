// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_CONTENT_FACTORY_NAVIGATION_MONITOR_FACTORY_H_
#define COMPONENTS_DOWNLOAD_CONTENT_FACTORY_NAVIGATION_MONITOR_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace download {

class NavigationMonitor;

// Creates the DownloadNavigationMonitor instance.
class NavigationMonitorFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns singleton instance of DownloadServiceFactory.
  static NavigationMonitorFactory* GetInstance();

  // Helper method to create the DownloadNavigationMonitor instance from
  // |context| if it doesn't exist.
  static download::NavigationMonitor* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<NavigationMonitorFactory>;

  NavigationMonitorFactory();
  ~NavigationMonitorFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(NavigationMonitorFactory);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_CONTENT_FACTORY_NAVIGATION_MONITOR_FACTORY_H_
