// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/event_page_request_manager_factory.h"

#include "chrome/browser/media/router/event_page_request_manager.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/process_manager_factory.h"

using content::BrowserContext;

namespace media_router {

// static
EventPageRequestManager*
EventPageRequestManagerFactory::GetApiForBrowserContext(
    BrowserContext* context) {
  DCHECK(context);
  // GetServiceForBrowserContext returns a KeyedService hence the static_cast<>
  // to return a pointer to EventPageRequestManager.
  return static_cast<EventPageRequestManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
EventPageRequestManagerFactory* EventPageRequestManagerFactory::GetInstance() {
  return base::Singleton<EventPageRequestManagerFactory>::get();
}

EventPageRequestManagerFactory::EventPageRequestManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "EventPageRequestManager",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(extensions::ProcessManagerFactory::GetInstance());
}

EventPageRequestManagerFactory::~EventPageRequestManagerFactory() = default;

content::BrowserContext* EventPageRequestManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

KeyedService* EventPageRequestManagerFactory::BuildServiceInstanceFor(
    BrowserContext* context) const {
  return new EventPageRequestManager(context);
}

}  // namespace media_router
