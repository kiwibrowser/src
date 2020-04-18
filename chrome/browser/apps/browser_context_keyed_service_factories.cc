// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/browser_context_keyed_service_factories.h"

#include "apps/browser_context_keyed_service_factories.h"
#include "chrome/browser/apps/app_load_service_factory.h"
#include "chrome/browser/apps/shortcut_manager_factory.h"
#include "content/public/browser/browser_context.h"

namespace chrome_apps {

void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  apps::EnsureBrowserContextKeyedServiceFactoriesBuilt();
  AppShortcutManagerFactory::GetInstance();
  apps::AppLoadServiceFactory::GetInstance();
}

void NotifyApplicationTerminating(content::BrowserContext* browser_context) {
  apps::NotifyApplicationTerminating(browser_context);
}

}  // namespace chrome_apps
