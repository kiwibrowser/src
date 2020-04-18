// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/presentation/local_presentation_manager_factory.h"

#include "base/lazy_instance.h"

#include "chrome/browser/media/router/presentation/local_presentation_manager.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/web_contents.h"

namespace media_router {

namespace {

base::LazyInstance<LocalPresentationManagerFactory>::DestructorAtExit
    service_factory = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
LocalPresentationManager*
LocalPresentationManagerFactory::GetOrCreateForWebContents(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  return LocalPresentationManagerFactory::GetOrCreateForBrowserContext(
      web_contents->GetBrowserContext());
}

// static
LocalPresentationManager*
LocalPresentationManagerFactory::GetOrCreateForBrowserContext(
    content::BrowserContext* context) {
  DCHECK(context);
  return static_cast<LocalPresentationManager*>(
      service_factory.Get().GetServiceForBrowserContext(context, true));
}

// static
LocalPresentationManagerFactory*
LocalPresentationManagerFactory::GetInstanceForTest() {
  return &service_factory.Get();
}

LocalPresentationManagerFactory::LocalPresentationManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "LocalPresentationManager",
          BrowserContextDependencyManager::GetInstance()) {}

LocalPresentationManagerFactory::~LocalPresentationManagerFactory() {}

content::BrowserContext*
LocalPresentationManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
KeyedService* LocalPresentationManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new LocalPresentationManager;
}

}  // namespace media_router
