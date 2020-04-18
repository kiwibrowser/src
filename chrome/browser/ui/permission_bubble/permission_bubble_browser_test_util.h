// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PERMISSION_BUBBLE_PERMISSION_BUBBLE_BROWSER_TEST_UTIL_H_
#define CHROME_BROWSER_UI_PERMISSION_BUBBLE_PERMISSION_BUBBLE_BROWSER_TEST_UTIL_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/permission_bubble/permission_prompt.h"

namespace base {
class CommandLine;
}
class PermissionRequest;
class Browser;

class TestPermissionBubbleViewDelegate : public PermissionPrompt::Delegate {
 public:
  TestPermissionBubbleViewDelegate();
  ~TestPermissionBubbleViewDelegate() override;

  const std::vector<PermissionRequest*>& Requests() override;
  PermissionPrompt::DisplayNameOrOrigin GetDisplayNameOrOrigin() override;

  void Accept() override {}
  void Deny() override {}
  void Closing() override {}

  void set_requests(std::vector<PermissionRequest*> requests) {
    requests_ = requests;
  }

 private:
  std::vector<PermissionRequest*> requests_;

  DISALLOW_COPY_AND_ASSIGN(TestPermissionBubbleViewDelegate);
};

// Use this class to test on a default window or an app window. Inheriting from
// ExtensionBrowserTest allows us to easily load and launch apps, and doesn't
// really add any extra work.
class PermissionBubbleBrowserTest : public extensions::ExtensionBrowserTest {
 public:
  PermissionBubbleBrowserTest();
  ~PermissionBubbleBrowserTest() override;

  void SetUpOnMainThread() override;

  // Opens an app window, and returns the associated browser.
  Browser* OpenExtensionAppWindow();

  PermissionPrompt::Delegate* test_delegate() { return &test_delegate_; }

 private:
  TestPermissionBubbleViewDelegate test_delegate_;
  std::vector<std::unique_ptr<PermissionRequest>> requests_;

  DISALLOW_COPY_AND_ASSIGN(PermissionBubbleBrowserTest);
};

// Use this class to test on a kiosk window.
class PermissionBubbleKioskBrowserTest : public PermissionBubbleBrowserTest {
 public:
  PermissionBubbleKioskBrowserTest();
  ~PermissionBubbleKioskBrowserTest() override;

  void SetUpCommandLine(base::CommandLine* command_line) override;

  DISALLOW_COPY_AND_ASSIGN(PermissionBubbleKioskBrowserTest);
};

#endif  // CHROME_BROWSER_UI_PERMISSION_BUBBLE_PERMISSION_BUBBLE_BROWSER_TEST_UTIL_H_
