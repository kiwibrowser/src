// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/synced_printers_manager_factory.h"

#include <memory>
#include <utility>

#include "base/debug/dump_without_crashing.h"
#include "chrome/browser/chromeos/printing/printers_sync_bridge.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

namespace chromeos {

namespace {

base::LazyInstance<SyncedPrintersManagerFactory>::DestructorAtExit
    g_printers_manager = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
SyncedPrintersManager* SyncedPrintersManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SyncedPrintersManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
SyncedPrintersManagerFactory* SyncedPrintersManagerFactory::GetInstance() {
  return g_printers_manager.Pointer();
}

content::BrowserContext* SyncedPrintersManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

SyncedPrintersManagerFactory::SyncedPrintersManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "SyncedPrintersManager",
          BrowserContextDependencyManager::GetInstance()) {}

SyncedPrintersManagerFactory::~SyncedPrintersManagerFactory() {}

SyncedPrintersManager* SyncedPrintersManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* browser_context) const {
  Profile* profile = Profile::FromBrowserContext(browser_context);

  syncer::OnceModelTypeStoreFactory store_factory =
      browser_sync::ProfileSyncService::GetModelTypeStoreFactory(
          profile->GetPath());

  std::unique_ptr<PrintersSyncBridge> sync_bridge =
      std::make_unique<PrintersSyncBridge>(
          std::move(store_factory), base::BindRepeating(base::IgnoreResult(
                                        &base::debug::DumpWithoutCrashing)));

  return SyncedPrintersManager::Create(profile, std::move(sync_bridge))
      .release();
}

}  // namespace chromeos
