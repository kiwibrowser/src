// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/permission_bubble/permission_bubble_browser_test_util.h"

#include "base/command_line.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/permissions/mock_permission_request.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

TestPermissionBubbleViewDelegate::TestPermissionBubbleViewDelegate()
    : PermissionPrompt::Delegate() {
}

TestPermissionBubbleViewDelegate::~TestPermissionBubbleViewDelegate() {}

const std::vector<PermissionRequest*>&
TestPermissionBubbleViewDelegate::Requests() {
  return requests_;
}

PermissionPrompt::DisplayNameOrOrigin
TestPermissionBubbleViewDelegate::GetDisplayNameOrOrigin() {
  return {base::string16(), false /* is_origin */};
}

PermissionBubbleBrowserTest::PermissionBubbleBrowserTest() {
}

PermissionBubbleBrowserTest::~PermissionBubbleBrowserTest() {
}

void PermissionBubbleBrowserTest::SetUpOnMainThread() {
  ExtensionBrowserTest::SetUpOnMainThread();

  // Add a single permission request.
  requests_.push_back(std::make_unique<MockPermissionRequest>(
      "Request 1", l10n_util::GetStringUTF8(IDS_PERMISSION_ALLOW),
      l10n_util::GetStringUTF8(IDS_PERMISSION_DENY)));

  std::vector<PermissionRequest*> raw_requests;
  raw_requests.push_back(requests_[0].get());
  test_delegate_.set_requests(raw_requests);
}

Browser* PermissionBubbleBrowserTest::OpenExtensionAppWindow() {
  auto* extension =
      LoadExtension(test_data_dir_.AppendASCII("app_with_panel_container/"));
  CHECK(extension);

  AppLaunchParams params(
      browser()->profile(), extension, extensions::LAUNCH_CONTAINER_PANEL,
      WindowOpenDisposition::NEW_WINDOW, extensions::SOURCE_TEST);

  content::WebContents* app_window = OpenApplication(params);
  CHECK(app_window);

  Browser* app_browser = chrome::FindBrowserWithWebContents(app_window);
  CHECK(app_browser);
  CHECK(app_browser->is_app());

  return app_browser;
}

PermissionBubbleKioskBrowserTest::PermissionBubbleKioskBrowserTest() {
}

PermissionBubbleKioskBrowserTest::~PermissionBubbleKioskBrowserTest() {
}

void PermissionBubbleKioskBrowserTest::SetUpCommandLine(
    base::CommandLine* command_line) {
  PermissionBubbleBrowserTest::SetUpCommandLine(command_line);
  command_line->AppendSwitch(switches::kKioskMode);
}
