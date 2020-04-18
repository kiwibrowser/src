// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process_platform_part_base.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "components/policy/core/browser/browser_policy_connector.h"

BrowserProcessPlatformPartBase::BrowserProcessPlatformPartBase() {
}

BrowserProcessPlatformPartBase::~BrowserProcessPlatformPartBase() {
}

void BrowserProcessPlatformPartBase::PlatformSpecificCommandLineProcessing(
    const base::CommandLine& /* command_line */) {
}

void BrowserProcessPlatformPartBase::StartTearDown() {
}

void BrowserProcessPlatformPartBase::AttemptExit() {
// chrome::CloseAllBrowsers() doesn't link on OS_ANDROID, but it overrides this
// method already.
#if defined(OS_ANDROID)
  NOTREACHED();
#else
  // On most platforms, closing all windows causes the application to exit.
  chrome::CloseAllBrowsers();
#endif
}

void BrowserProcessPlatformPartBase::PreMainMessageLoopRun() {
}

std::unique_ptr<policy::ChromeBrowserPolicyConnector>
BrowserProcessPlatformPartBase::CreateBrowserPolicyConnector() {
  return std::make_unique<policy::ChromeBrowserPolicyConnector>();
}

void BrowserProcessPlatformPartBase::RegisterInProcessServices(
    content::ContentBrowserClient::StaticServiceMap* services,
    content::ServiceManagerConnection* connection) {}
