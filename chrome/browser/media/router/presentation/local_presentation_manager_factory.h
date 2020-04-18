// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_LOCAL_PRESENTATION_MANAGER_FACTORY_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_LOCAL_PRESENTATION_MANAGER_FACTORY_H_

#include "base/lazy_instance.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace media_router {

class LocalPresentationManager;

// LocalPresentationManager is shared between a Profile and
// its associated incognito Profiles.
class LocalPresentationManagerFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // If |web_contents| is normal profile, use it as browser context;
  // If |web_contents| is incognito profile, |GetBrowserContextToUse| will
  // redirect incognito profile to original profile, and use original one as
  // browser context.
  static LocalPresentationManager* GetOrCreateForWebContents(
      content::WebContents* web_contents);
  static LocalPresentationManager* GetOrCreateForBrowserContext(
      content::BrowserContext* context);

  // For test use only.
  static LocalPresentationManagerFactory* GetInstanceForTest();

 private:
  friend struct base::LazyInstanceTraitsBase<LocalPresentationManagerFactory>;

  LocalPresentationManagerFactory();
  ~LocalPresentationManagerFactory() override;

  // BrowserContextKeyedServiceFactory interface.
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(LocalPresentationManagerFactory);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_LOCAL_PRESENTATION_MANAGER_FACTORY_H_
