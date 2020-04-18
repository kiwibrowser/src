// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/bookmarks/startup_task_runner_service_factory.h"

#include "base/memory/singleton.h"
#include "base/sequenced_task_runner.h"
#include "components/bookmarks/browser/startup_task_runner_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

namespace ios {

// static
bookmarks::StartupTaskRunnerService*
StartupTaskRunnerServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<bookmarks::StartupTaskRunnerService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
StartupTaskRunnerServiceFactory*
StartupTaskRunnerServiceFactory::GetInstance() {
  return base::Singleton<StartupTaskRunnerServiceFactory>::get();
}

StartupTaskRunnerServiceFactory::StartupTaskRunnerServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "StartupTaskRunnerService",
          BrowserStateDependencyManager::GetInstance()) {
}

StartupTaskRunnerServiceFactory::~StartupTaskRunnerServiceFactory() {
}

std::unique_ptr<KeyedService>
StartupTaskRunnerServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<bookmarks::StartupTaskRunnerService>(
      browser_state->GetIOTaskRunner());
}

}  // namespace ios
