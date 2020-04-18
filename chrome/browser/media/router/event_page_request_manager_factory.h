// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_EVENT_PAGE_REQUEST_MANAGER_FACTORY_H_
#define CHROME_BROWSER_MEDIA_ROUTER_EVENT_PAGE_REQUEST_MANAGER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace media_router {

class EventPageRequestManager;

// A factory that creates EventPageRequestManager for a given BrowserContext.
class EventPageRequestManagerFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static EventPageRequestManager* GetApiForBrowserContext(
      content::BrowserContext* context);

  static EventPageRequestManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<EventPageRequestManagerFactory>;

  EventPageRequestManagerFactory();
  ~EventPageRequestManagerFactory() override;

  // BrowserContextKeyedServiceFactory:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(EventPageRequestManagerFactory);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_EVENT_PAGE_REQUEST_MANAGER_FACTORY_H_
