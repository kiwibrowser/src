// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/preloaded_web_view_factory.h"

#include "chrome/browser/chromeos/login/ui/preloaded_web_view.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace chromeos {

// static
PreloadedWebView* PreloadedWebViewFactory::GetForProfile(Profile* profile) {
  auto* result = static_cast<PreloadedWebView*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
  return result;
}

// static
PreloadedWebViewFactory* PreloadedWebViewFactory::GetInstance() {
  return base::Singleton<PreloadedWebViewFactory>::get();
}

PreloadedWebViewFactory::PreloadedWebViewFactory()
    : BrowserContextKeyedServiceFactory(
          "PreloadedWebViewFactory",
          BrowserContextDependencyManager::GetInstance()) {}

PreloadedWebViewFactory::~PreloadedWebViewFactory() {}

content::BrowserContext* PreloadedWebViewFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Make sure that only the SigninProfile is using a preloaded webview.
  if (Profile::FromBrowserContext(context) != ProfileHelper::GetSigninProfile())
    return nullptr;

  return context;
}

KeyedService* PreloadedWebViewFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new PreloadedWebView(Profile::FromBrowserContext(context));
}

}  // namespace chromeos
