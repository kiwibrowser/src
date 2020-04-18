// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media_router/media_router_ui_service_factory.h"

#include "chrome/browser/media/router/media_router_factory.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model_factory.h"
#include "chrome/browser/ui/webui/media_router/media_router_ui_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

using content::BrowserContext;

namespace media_router {

// static
MediaRouterUIService* MediaRouterUIServiceFactory::GetForBrowserContext(
    BrowserContext* context) {
  DCHECK(context);
  return static_cast<MediaRouterUIService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
MediaRouterUIServiceFactory* MediaRouterUIServiceFactory::GetInstance() {
  return base::Singleton<MediaRouterUIServiceFactory>::get();
}

MediaRouterUIServiceFactory::MediaRouterUIServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "MediaRouterUIService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(MediaRouterFactory::GetInstance());
  // MediaRouterUIService owns a MediaRouterActionController that depends on
  // ToolbarActionsModel.
  DependsOn(ToolbarActionsModelFactory::GetInstance());
}

MediaRouterUIServiceFactory::~MediaRouterUIServiceFactory() {}

BrowserContext* MediaRouterUIServiceFactory::GetBrowserContextToUse(
    BrowserContext* context) const {
  return context;
}

KeyedService* MediaRouterUIServiceFactory::BuildServiceInstanceFor(
    BrowserContext* context) const {
  return MediaRouterEnabled(context)
             ? new MediaRouterUIService(Profile::FromBrowserContext(context))
             : nullptr;
}

bool MediaRouterUIServiceFactory::ServiceIsCreatedWithBrowserContext() const {
#if !defined(OS_ANDROID)
  return true;
#else
  return false;
#endif
}

bool MediaRouterUIServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace media_router
