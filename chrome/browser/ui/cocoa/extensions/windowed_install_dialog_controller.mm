// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/windowed_install_dialog_controller.h"

#include <utility>

#import "base/callback_helpers.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/extensions/extension_install_prompt_show_params.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/extensions/extension_install_view_controller.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/cocoa/window_size_constants.h"

@interface WindowedInstallController
    : NSWindowController<NSWindowDelegate> {
 @private
  base::scoped_nsobject<ExtensionInstallViewController> installViewController_;
  WindowedInstallDialogController* dialogController_;  // Weak. Owns us.
}

@property(readonly, nonatomic) ExtensionInstallViewController* viewController;

- (id)initWithProfile:(Profile*)profile
            navigator:(content::PageNavigator*)navigator
             delegate:(WindowedInstallDialogController*)delegate
               prompt:(std::unique_ptr<ExtensionInstallPrompt::Prompt>)prompt;

@end

WindowedInstallDialogController::WindowedInstallDialogController(
    ExtensionInstallPromptShowParams* show_params,
    const ExtensionInstallPrompt::DoneCallback& done_callback,
    std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt)
    : done_callback_(done_callback) {
  install_controller_.reset([[WindowedInstallController alloc]
      initWithProfile:show_params->profile()
            navigator:show_params->GetParentWebContents()
             delegate:this
               prompt:std::move(prompt)]);
  [[install_controller_ window] makeKeyAndOrderFront:nil];
}

WindowedInstallDialogController::~WindowedInstallDialogController() {
  DCHECK(!install_controller_);
  DCHECK(done_callback_.is_null());
}

void WindowedInstallDialogController::OnWindowClosing() {
  install_controller_.reset();
  if (!done_callback_.is_null()) {
    base::ResetAndReturn(&done_callback_).Run(
        ExtensionInstallPrompt::Result::ABORTED);
  }
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

ExtensionInstallViewController*
WindowedInstallDialogController::GetViewController() {
  return [install_controller_ viewController];
}

void WindowedInstallDialogController::OnOkButtonClicked() {
  base::ResetAndReturn(&done_callback_).Run(
      ExtensionInstallPrompt::Result::ACCEPTED);
  [[install_controller_ window] close];
}

void WindowedInstallDialogController::OnCancelButtonClicked() {
  base::ResetAndReturn(&done_callback_).Run(
      ExtensionInstallPrompt::Result::USER_CANCELED);
  [[install_controller_ window] close];
}

void WindowedInstallDialogController::OnStoreLinkClicked() {
  base::ResetAndReturn(&done_callback_).Run(
      ExtensionInstallPrompt::Result::USER_CANCELED);
  [[install_controller_ window] close];
}

@implementation WindowedInstallController

- (id)initWithProfile:(Profile*)profile
            navigator:(content::PageNavigator*)navigator
             delegate:(WindowedInstallDialogController*)delegate
               prompt:(std::unique_ptr<ExtensionInstallPrompt::Prompt>)prompt {
  base::scoped_nsobject<NSWindow> controlledPanel(
      [[NSPanel alloc] initWithContentRect:ui::kWindowSizeDeterminedLater
                                 styleMask:NSTitledWindowMask
                                   backing:NSBackingStoreBuffered
                                     defer:NO]);
  if ((self = [super initWithWindow:controlledPanel])) {
    dialogController_ = delegate;
    ExtensionInstallPrompt::Prompt* weakPrompt = prompt.get();
    installViewController_.reset([[ExtensionInstallViewController alloc]
        initWithProfile:profile
              navigator:navigator
               delegate:delegate
                 prompt:std::move(prompt)]);
    NSWindow* window = [self window];

    // Ensure the window does not display behind the app launcher window, and is
    // otherwise hard to lose behind other windows (since it is not modal).
    [window setLevel:NSDockWindowLevel];

    // Animate the window when ordered in, the same way as an NSAlert.
    if ([window respondsToSelector:@selector(setAnimationBehavior:)])
      [window setAnimationBehavior:NSWindowAnimationBehaviorAlertPanel];

    [window setTitle:base::SysUTF16ToNSString(weakPrompt->GetDialogTitle())];
    NSRect viewFrame = [[installViewController_ view] frame];
    [window setFrame:[window frameRectForContentRect:viewFrame]
             display:NO];
    [window setContentView:[installViewController_ view]];
    [window setDelegate:self];
    [window center];
  }
  return self;
}

- (ExtensionInstallViewController*)viewController {
  return installViewController_;
}

- (void)windowWillClose:(NSNotification*)notification {
  [[self window] setDelegate:nil];
  dialogController_->OnWindowClosing();
}

@end
