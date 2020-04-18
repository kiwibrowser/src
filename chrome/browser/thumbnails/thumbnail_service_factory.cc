// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/thumbnails/thumbnail_service_factory.h"

#include "base/logging.h"
#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/thumbnails/thumbnail_service.h"
#include "chrome/browser/thumbnails/thumbnail_service_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

using thumbnails::ThumbnailService;
using thumbnails::ThumbnailServiceImpl;

ThumbnailServiceFactory::ThumbnailServiceFactory()
    : RefcountedBrowserContextKeyedServiceFactory(
          "ThumbnailService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(TopSitesFactory::GetInstance());
}

ThumbnailServiceFactory::~ThumbnailServiceFactory() {
}

// static
scoped_refptr<ThumbnailService> ThumbnailServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<ThumbnailService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true).get());
}

// static
ThumbnailServiceFactory* ThumbnailServiceFactory::GetInstance() {
  return base::Singleton<ThumbnailServiceFactory>::get();
}

scoped_refptr<RefcountedKeyedService>
ThumbnailServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return scoped_refptr<RefcountedKeyedService>(
      new ThumbnailServiceImpl(static_cast<Profile*>(profile)));
}
