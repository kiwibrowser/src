// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_FACTORY_H_
#define CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class BackgroundFetchDelegateImpl;
class Profile;

class BackgroundFetchDelegateFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static BackgroundFetchDelegateImpl* GetForProfile(Profile* profile);
  static BackgroundFetchDelegateFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BackgroundFetchDelegateFactory>;

  BackgroundFetchDelegateFactory();
  ~BackgroundFetchDelegateFactory() override;

  // BrowserContextKeyedBaseFactory methods:
  // TODO(crbug.com/766082): Override GetBrowserContextToUse to handle Incognito
  // mode.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchDelegateFactory);
};

#endif  // CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_FACTORY_H_
