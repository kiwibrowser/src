// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/global_error_bubble_controller.h"

#include "base/logging.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#import "chrome/browser/ui/browser.h"
#import "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/app_menu/app_menu_controller.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/global_error/global_error.h"
#include "chrome/browser/ui/global_error/global_error_bubble_view_base.h"
#include "chrome/browser/ui/global_error/global_error_service.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "components/search_engines/util.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#import "ui/base/cocoa/a11y_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"

using base::SysUTF16ToNSString;

namespace {

const CGFloat kParagraphSpacing = 6;

ToolbarController* ToolbarControllerForBrowser(Browser* browser) {
  NSWindow* parent_window = browser->window()->GetNativeWindow();
  BrowserWindowController* bwc =
      [BrowserWindowController browserWindowControllerForWindow:parent_window];
  return [bwc toolbarController];
}

} // namespace

namespace GlobalErrorBubbleControllerInternal {

// This is the bridge to the C++ GlobalErrorBubbleViewBase object.
class Bridge : public GlobalErrorBubbleViewBase {
 public:
  Bridge(GlobalErrorBubbleController* controller) : controller_(controller) {
  }

 private:
  void CloseBubbleView() override { [controller_ close]; }

  GlobalErrorBubbleController* controller_;  // Weak, owns this.
};

}  // namespace GlobalErrorBubbleControllerInternal

@implementation GlobalErrorBubbleController

+ (GlobalErrorBubbleViewBase*)showForBrowser:(Browser*)browser
    error:(const base::WeakPtr<GlobalErrorWithStandardBubble>&)error {
  NSView* appMenuButton = [ToolbarControllerForBrowser(browser) appMenuButton];
  NSPoint offset = NSMakePoint(
      NSMidX([appMenuButton bounds]),
      app_menu_controller::kAppMenuBubblePointOffsetY);

  // The bubble will be automatically deleted when the window is closed.
  GlobalErrorBubbleController* bubble = [[GlobalErrorBubbleController alloc]
      initWithWindowNibPath:@"GlobalErrorBubble"
             relativeToView:appMenuButton
                     offset:offset];
  bubble->error_ = error;
  bubble->bridge_.reset(new GlobalErrorBubbleControllerInternal::Bridge(
      bubble));
  bubble->browser_ = browser;
  [bubble showWindow:nil];

  return bubble->bridge_.get();
}

- (void)awakeFromNib {
  [super awakeFromNib];

  DCHECK(error_);

  gfx::Image image = error_->GetBubbleViewIcon();
  DCHECK(!image.IsEmpty());
  [iconView_ setImage:image.ToNSImage()];
  // So far, none of these icons have useful descriptions (they only specify
  // "image"). Hide them from the accessibility order for voice over. If any
  // new bubbles use this for an informational icon, we can add a new method
  // to the GlobalErrorWithStandardBubble class.
  ui::a11y_util::HideImageFromAccessibilityOrder(iconView_);

  [title_ setStringValue:SysUTF16ToNSString(error_->GetBubbleViewTitle())];
  std::vector<base::string16> messages = error_->GetBubbleViewMessages();
  base::string16 message = base::JoinString(messages, base::ASCIIToUTF16("\n"));

  base::scoped_nsobject<NSMutableAttributedString> messageValue(
      [[NSMutableAttributedString alloc]
          initWithString:SysUTF16ToNSString(message)]);
  base::scoped_nsobject<NSMutableParagraphStyle> style(
      [[NSMutableParagraphStyle alloc] init]);
  [style setParagraphSpacing:kParagraphSpacing];
  [messageValue addAttribute:NSParagraphStyleAttributeName
                       value:style
                       range:NSMakeRange(0, [messageValue length])];

  [message_ setAttributedStringValue:messageValue];

  [acceptButton_ setTitle:
      SysUTF16ToNSString(error_->GetBubbleViewAcceptButtonLabel())];
  base::string16 cancelLabel = error_->GetBubbleViewCancelButtonLabel();
  if (cancelLabel.empty())
    [cancelButton_ setHidden:YES];
  else
    [cancelButton_ setTitle:SysUTF16ToNSString(cancelLabel)];

  // First make sure that the window is wide enough to accommodate the buttons.
  NSRect frame = [[self window] frame];
  [layoutTweaker_ tweakUI:buttonContainer_];
  CGFloat delta =  NSWidth([buttonContainer_ frame]) - NSWidth(frame);
  if (delta > 0) {
    frame.size.width += delta;
    [[self window] setFrame:frame display:NO];
  }

  // Adapt window height to bottom buttons. Do this before all other layouting.
  NSArray* views = [NSArray arrayWithObjects:
      title_, message_, buttonContainer_, nil];
  NSSize ds = NSMakeSize(0, cocoa_l10n_util::VerticallyReflowGroup(views));
  ds = [[self bubble] convertSize:ds toView:nil];

  frame.origin.y -= ds.height;
  frame.size.height += ds.height;
  [[self window] setFrame:frame display:YES];
}

- (void)showWindow:(id)sender {
  BrowserWindowController* bwc = [BrowserWindowController
      browserWindowControllerForWindow:[self parentWindow]];
  [bwc lockToolbarVisibilityForOwner:self withAnimation:NO];
  [super showWindow:sender];
}

- (void)close {
  if (error_)
    error_->BubbleViewDidClose(browser_);
  bridge_.reset();
  BrowserWindowController* bwc = [BrowserWindowController
      browserWindowControllerForWindow:[self parentWindow]];
  [bwc releaseToolbarVisibilityForOwner:self withAnimation:YES];
  [super close];
}

- (IBAction)onAccept:(id)sender {
  if (error_)
    error_->BubbleViewAcceptButtonPressed(browser_);
  [self close];
}

- (IBAction)onCancel:(id)sender {
  if (error_)
    error_->BubbleViewCancelButtonPressed(browser_);
  [self close];
}

@end

GlobalErrorBubbleViewBase* GlobalErrorBubbleViewBase::ShowStandardBubbleView(
    Browser* browser,
    const base::WeakPtr<GlobalErrorWithStandardBubble>& error) {
  if (!chrome::ShowAllDialogsWithViewsToolkit())
    return [GlobalErrorBubbleController showForBrowser:browser error:error];

  NSPoint ns_point = [ToolbarControllerForBrowser(browser) appMenuBubblePoint];
  return ShowViewsGlobalErrorBubbleOnCocoaBrowser(ns_point, browser, error);
}
