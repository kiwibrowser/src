// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/webapk_install_service_factory.h"

#include "chrome/browser/android/webapk/webapk_install_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
WebApkInstallServiceFactory* WebApkInstallServiceFactory::GetInstance() {
  return base::Singleton<WebApkInstallServiceFactory>::get();
}

// static
WebApkInstallService* WebApkInstallServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<WebApkInstallService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

WebApkInstallServiceFactory::WebApkInstallServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WebApkInstallService",
          BrowserContextDependencyManager::GetInstance()) {}

WebApkInstallServiceFactory::~WebApkInstallServiceFactory() {}

KeyedService* WebApkInstallServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new WebApkInstallService(context);
}

content::BrowserContext* WebApkInstallServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
