// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/cloud_print_proxy_service_factory.h"

#include "chrome/browser/printing/cloud_print/cloud_print_proxy_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
CloudPrintProxyService* CloudPrintProxyServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<CloudPrintProxyService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

CloudPrintProxyServiceFactory* CloudPrintProxyServiceFactory::GetInstance() {
  return base::Singleton<CloudPrintProxyServiceFactory>::get();
}

CloudPrintProxyServiceFactory::CloudPrintProxyServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "CloudPrintProxyService",
        BrowserContextDependencyManager::GetInstance()) {
}

CloudPrintProxyServiceFactory::~CloudPrintProxyServiceFactory() {
}

KeyedService* CloudPrintProxyServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  CloudPrintProxyService* service =
      new CloudPrintProxyService(static_cast<Profile*>(profile));
  service->Initialize();

  return service;
}

bool CloudPrintProxyServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
