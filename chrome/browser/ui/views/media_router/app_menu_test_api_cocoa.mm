// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/app_menu_test_api.h"

#import <Cocoa/Cocoa.h>

#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/app_menu/app_menu_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/test/run_loop_testing.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "ui/base/ui_features.h"

namespace {

class AppMenuTestApiCocoa : public test::AppMenuTestApi {
 public:
  explicit AppMenuTestApiCocoa(Browser* browser);
  ~AppMenuTestApiCocoa() override;

  // AppMenuTestApi:
  bool IsMenuShowing() override;
  void ShowMenu() override;
  void ExecuteCommand(int command) override;

 private:
  AppMenuController* GetAppMenuController();
  NSButton* GetAppMenuButton();
  ToolbarController* GetToolbarController();

  bool menu_showing_ = false;
  Browser* browser_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AppMenuTestApiCocoa);
};

AppMenuTestApiCocoa::AppMenuTestApiCocoa(Browser* browser)
    : browser_(browser) {}
AppMenuTestApiCocoa::~AppMenuTestApiCocoa() {}

bool AppMenuTestApiCocoa::IsMenuShowing() {
  return menu_showing_;
}

// Note: this class fakes the state of the app menu. In principle, the body
// should be something like this:
//   return [GetAppMenuButton() performClick:nil];
// but doing that starts a nested run loop for the menu, which hangs the rest of
// the test. Instead, keep track of whether the menu is supposed to be showing
// or not. This class can't *actually* show the real menu because of
// https://crbug.com/823495.
// TODO(ellyjones): Use the real menu here.
void AppMenuTestApiCocoa::ShowMenu() {
  menu_showing_ = true;
}

void AppMenuTestApiCocoa::ExecuteCommand(int command) {
  menu_showing_ = false;
  base::scoped_nsobject<NSButton> button([[NSButton alloc] init]);
  [button setTag:command];
  [GetAppMenuController() dispatchAppMenuCommand:button.get()];
  chrome::testing::NSRunLoopRunAllPending();
}

AppMenuController* AppMenuTestApiCocoa::GetAppMenuController() {
  return [GetToolbarController() appMenuController];
}

NSButton* AppMenuTestApiCocoa::GetAppMenuButton() {
  return [GetToolbarController() appMenuButton];
}

ToolbarController* AppMenuTestApiCocoa::GetToolbarController() {
  NSWindow* window = browser_->window()->GetNativeWindow();
  BrowserWindowController* bwc =
      [BrowserWindowController browserWindowControllerForWindow:window];
  return [bwc toolbarController];
}

}  // namespace

namespace test {

std::unique_ptr<AppMenuTestApi> AppMenuTestApi::CreateCocoa(Browser* browser) {
  return std::make_unique<AppMenuTestApiCocoa>(browser);
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
std::unique_ptr<AppMenuTestApi> AppMenuTestApi::Create(Browser* browser) {
  return std::make_unique<AppMenuTestApiCocoa>(browser);
}
#endif  // !MAC_VIEWS_BROWSER
}
