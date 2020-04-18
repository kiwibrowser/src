// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gcm/gcm_profile_service_factory.h"
#include <memory>

#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/gcm_driver/gcm_profile_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/browser_thread.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/gcm/gcm_product_util.h"
#include "chrome/common/channel_info.h"
#include "components/gcm_driver/gcm_client_factory.h"
#endif

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
#include "chrome/browser/offline_pages/prefetch/prefetch_service_factory.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_app_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"
#endif

namespace gcm {

// static
GCMProfileService* GCMProfileServiceFactory::GetForProfile(
    content::BrowserContext* profile) {
  // GCM is not supported in incognito mode.
  if (profile->IsOffTheRecord())
    return NULL;

  return static_cast<GCMProfileService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
GCMProfileServiceFactory* GCMProfileServiceFactory::GetInstance() {
  return base::Singleton<GCMProfileServiceFactory>::get();
}

GCMProfileServiceFactory::GCMProfileServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "GCMProfileService",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SigninManagerFactory::GetInstance());
  DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  DependsOn(offline_pages::PrefetchServiceFactory::GetInstance());
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)
}

GCMProfileServiceFactory::~GCMProfileServiceFactory() {
}

KeyedService* GCMProfileServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  DCHECK(!profile->IsOffTheRecord());

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner(
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}));
  std::unique_ptr<GCMProfileService> service = nullptr;
#if defined(OS_ANDROID)
  service = base::WrapUnique(
      new GCMProfileService(profile->GetPath(), blocking_task_runner));
#else
  service = base::WrapUnique(new GCMProfileService(
      profile->GetPrefs(), profile->GetPath(), profile->GetRequestContext(),
      chrome::GetChannel(),
      gcm::GetProductCategoryForSubtypes(profile->GetPrefs()),
      SigninManagerFactory::GetForProfile(profile),
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile),
      std::unique_ptr<GCMClientFactory>(new GCMClientFactory),
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI),
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::IO),
      blocking_task_runner));
#endif
#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  offline_pages::PrefetchService* prefetch_service =
      offline_pages::PrefetchServiceFactory::GetForBrowserContext(context);
  if (prefetch_service != nullptr) {
    offline_pages::PrefetchGCMHandler* prefetch_gcm_handler =
        prefetch_service->GetPrefetchGCMHandler();
    service->driver()->AddAppHandler(prefetch_gcm_handler->GetAppId(),
                                     prefetch_gcm_handler->AsGCMAppHandler());
  }
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)

  return service.release();
}

content::BrowserContext* GCMProfileServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

}  // namespace gcm
