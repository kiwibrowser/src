// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/webui/web_ui_ios_controller_factory_registry.h"

#include <stddef.h>
#include <memory>

#include "base/lazy_instance.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace web {

base::LazyInstance<std::vector<WebUIIOSControllerFactory*>>::DestructorAtExit
    g_factories = LAZY_INSTANCE_INITIALIZER;

void WebUIIOSControllerFactory::RegisterFactory(
    WebUIIOSControllerFactory* factory) {
  g_factories.Pointer()->push_back(factory);
}

WebUIIOSControllerFactoryRegistry*
WebUIIOSControllerFactoryRegistry::GetInstance() {
  return base::Singleton<WebUIIOSControllerFactoryRegistry>::get();
}

std::unique_ptr<WebUIIOSController>
WebUIIOSControllerFactoryRegistry::CreateWebUIIOSControllerForURL(
    WebUIIOS* web_ui,
    const GURL& url) const {
  std::vector<WebUIIOSControllerFactory*>* factories = g_factories.Pointer();
  for (WebUIIOSControllerFactory* factory : *factories) {
    auto controller = factory->CreateWebUIIOSControllerForURL(web_ui, url);
    if (controller)
      return controller;
  }
  return nullptr;
}

WebUIIOSControllerFactoryRegistry::WebUIIOSControllerFactoryRegistry() {
}

WebUIIOSControllerFactoryRegistry::~WebUIIOSControllerFactoryRegistry() {
}

}  // namespace web
