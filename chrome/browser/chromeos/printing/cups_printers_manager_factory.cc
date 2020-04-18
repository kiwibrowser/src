// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/cups_printers_manager_factory.h"

#include "chrome/browser/chromeos/printing/cups_printers_manager.h"
#include "chrome/browser/chromeos/printing/synced_printers_manager_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace chromeos {
namespace {

static base::LazyInstance<CupsPrintersManagerFactory>::Leaky::DestructorAtExit
    g_cups_printers_manager_factory = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
CupsPrintersManagerFactory* CupsPrintersManagerFactory::GetInstance() {
  return g_cups_printers_manager_factory.Pointer();
}

// static
CupsPrintersManager* CupsPrintersManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<CupsPrintersManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

content::BrowserContext* CupsPrintersManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

CupsPrintersManagerFactory::CupsPrintersManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "CupsPrintersManagerFactory",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(chromeos::SyncedPrintersManagerFactory::GetInstance());
}

CupsPrintersManagerFactory::~CupsPrintersManagerFactory() {}

KeyedService* CupsPrintersManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return CupsPrintersManager::Create(Profile::FromBrowserContext(context))
      .release();
}

}  // namespace chromeos
