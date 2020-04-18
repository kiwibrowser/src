// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_UI_PRELOADED_WEB_VIEW_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_UI_PRELOADED_WEB_VIEW_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace chromeos {

class PreloadedWebView;

// Fetches a PreloadedWebView instance for the signin profile.
class PreloadedWebViewFactory : public BrowserContextKeyedServiceFactory {
 public:
  static PreloadedWebView* GetForProfile(Profile* profile);

  static PreloadedWebViewFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<PreloadedWebViewFactory>;

  PreloadedWebViewFactory();
  ~PreloadedWebViewFactory() override;

  // BrowserContextKeyedServiceFactory:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;

  DISALLOW_COPY_AND_ASSIGN(PreloadedWebViewFactory);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_UI_PRELOADED_WEB_VIEW_FACTORY_H_
