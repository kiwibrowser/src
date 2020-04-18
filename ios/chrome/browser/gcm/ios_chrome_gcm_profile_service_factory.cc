// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/gcm/ios_chrome_gcm_profile_service_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "components/gcm_driver/gcm_client_factory.h"
#include "components/gcm_driver/gcm_profile_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/core/browser/signin_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/oauth2_token_service_factory.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/web/public/web_thread.h"

// static
gcm::GCMProfileService* IOSChromeGCMProfileServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<gcm::GCMProfileService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
IOSChromeGCMProfileServiceFactory*
IOSChromeGCMProfileServiceFactory::GetInstance() {
  return base::Singleton<IOSChromeGCMProfileServiceFactory>::get();
}

// static
std::string IOSChromeGCMProfileServiceFactory::GetProductCategoryForSubtypes() {
#if defined(GOOGLE_CHROME_BUILD)
  return "com.chrome.ios";
#else
  return "org.chromium.ios";
#endif
}

IOSChromeGCMProfileServiceFactory::IOSChromeGCMProfileServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "GCMProfileService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(ios::SigninManagerFactory::GetInstance());
  DependsOn(OAuth2TokenServiceFactory::GetInstance());
}

IOSChromeGCMProfileServiceFactory::~IOSChromeGCMProfileServiceFactory() {}

std::unique_ptr<KeyedService>
IOSChromeGCMProfileServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  DCHECK(!context->IsOffTheRecord());

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner(
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}));
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<gcm::GCMProfileService>(
      browser_state->GetPrefs(), browser_state->GetStatePath(),
      browser_state->GetRequestContext(), ::GetChannel(),
      GetProductCategoryForSubtypes(),
      ios::SigninManagerFactory::GetForBrowserState(browser_state),
      OAuth2TokenServiceFactory::GetForBrowserState(browser_state),
      base::WrapUnique(new gcm::GCMClientFactory),
      web::WebThread::GetTaskRunnerForThread(web::WebThread::UI),
      web::WebThread::GetTaskRunnerForThread(web::WebThread::IO),
      blocking_task_runner);
}
