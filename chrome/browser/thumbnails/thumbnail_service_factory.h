// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_THUMBNAILS_THUMBNAIL_SERVICE_FACTORY_H_
#define CHROME_BROWSER_THUMBNAILS_THUMBNAIL_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/refcounted_browser_context_keyed_service_factory.h"

class Profile;

namespace thumbnails {
class ThumbnailService;
}

class ThumbnailServiceFactory
    : public RefcountedBrowserContextKeyedServiceFactory {
 public:
  // Returns an instance of ThumbnailService associated with this profile
  // (creating one if none exists). Returns NULL if this profile cannot have a
  // ThumbnailService (for example, if |profile| is incognito).
  // Depending on the settings, the implementation of the service interface
  // can be provided either by TopSites (stored in the profile itself) or
  // be an instance of a real RefcountedKeyedService
  // implementation.
  static scoped_refptr<thumbnails::ThumbnailService> GetForProfile(
      Profile* profile);

  static ThumbnailServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ThumbnailServiceFactory>;

  ThumbnailServiceFactory();
  ~ThumbnailServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  scoped_refptr<RefcountedKeyedService> BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;

  DISALLOW_COPY_AND_ASSIGN(ThumbnailServiceFactory);
};

#endif  // CHROME_BROWSER_THUMBNAILS_THUMBNAIL_SERVICE_FACTORY_H_
