// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/files/file_path.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/common/content_paths.h"
#include "content/shell/browser/shell_browser_context.h"
#include "ui/views_content_client/views_content_client.h"
#include "ui/views_content_client/views_content_client_main_parts.h"

// A simple NSApplicationDelegate that provides a basic mainMenu and can
// activate a task when the application has finished loading.
@interface ViewsContentClientAppController : NSObject<NSApplicationDelegate> {
 @private
  base::Closure task_;
}

// Set the task to run after receiving -applicationDidFinishLaunching:.
- (void)setTask:(const base::Closure&)task;

@end

namespace ui {

namespace {

class ViewsContentClientMainPartsMac : public ViewsContentClientMainParts {
 public:
  ViewsContentClientMainPartsMac(
      const content::MainFunctionParams& content_params,
      ViewsContentClient* views_content_client);
  ~ViewsContentClientMainPartsMac() override;

  // content::BrowserMainParts:
  void PreMainMessageLoopRun() override;

 private:
  base::scoped_nsobject<ViewsContentClientAppController> app_controller_;

  DISALLOW_COPY_AND_ASSIGN(ViewsContentClientMainPartsMac);
};

ViewsContentClientMainPartsMac::ViewsContentClientMainPartsMac(
    const content::MainFunctionParams& content_params,
    ViewsContentClient* views_content_client)
    : ViewsContentClientMainParts(content_params, views_content_client) {
  // Cache the child process path to avoid triggering an AssertIOAllowed.
  base::FilePath child_process_exe;
  base::PathService::Get(content::CHILD_PROCESS_EXE, &child_process_exe);

  app_controller_.reset([[ViewsContentClientAppController alloc] init]);
  [[NSApplication sharedApplication] setDelegate:app_controller_];
}

void ViewsContentClientMainPartsMac::PreMainMessageLoopRun() {
  ViewsContentClientMainParts::PreMainMessageLoopRun();

  // On Mac, the task must be deferred to applicationDidFinishLaunching. If not,
  // the widget can activate, but (even if configured) the mainMenu won't be
  // ready to switch over in the OSX UI, so it will look strange.
  NSWindow* window_context = nil;
  [app_controller_ setTask:base::Bind(views_content_client()->task(),
                                      base::Unretained(browser_context()),
                                      base::Unretained(window_context))];
}

ViewsContentClientMainPartsMac::~ViewsContentClientMainPartsMac() {
  [[NSApplication sharedApplication] setDelegate:nil];
}

}  // namespace

// static
ViewsContentClientMainParts* ViewsContentClientMainParts::Create(
    const content::MainFunctionParams& content_params,
    ViewsContentClient* views_content_client) {
  return
      new ViewsContentClientMainPartsMac(content_params, views_content_client);
}

}  // namespace ui

@implementation ViewsContentClientAppController

- (void)setTask:(const base::Closure&)task {
  task_ = task;
}

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  // To get key events, the application needs to have an activation policy.
  // Unbundled apps (i.e. those without an Info.plist) default to
  // NSApplicationActivationPolicyProhibited.
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

  // Create a basic mainMenu object using the executable filename.
  base::scoped_nsobject<NSMenu> mainMenu([[NSMenu alloc] initWithTitle:@""]);
  NSMenuItem* appMenuItem =
      [mainMenu addItemWithTitle:@"" action:NULL keyEquivalent:@""];
  [NSApp setMainMenu:mainMenu];

  base::scoped_nsobject<NSMenu> appMenu([[NSMenu alloc] initWithTitle:@""]);
  NSString* appName = [[NSProcessInfo processInfo] processName];
  // TODO(tapted): Localize "Quit" if this is ever used for a released binary.
  // At the time of writing, ui_strings.grd has "Close" but not "Quit".
  NSString* quitTitle = [@"Quit " stringByAppendingString:appName];
  [appMenu addItemWithTitle:quitTitle
                     action:@selector(terminate:)
              keyEquivalent:@"q"];
  [appMenuItem setSubmenu:appMenu];

  task_.Run();
}

@end
