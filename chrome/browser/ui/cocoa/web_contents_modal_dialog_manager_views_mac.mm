// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/web_contents_modal_dialog_manager_views_mac.h"

#import <Cocoa/Cocoa.h>

#import "base/mac/foundation_util.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"
#include "content/public/browser/web_contents.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#import "ui/base/cocoa/constrained_window/constrained_window_animation.h"
#include "ui/views/widget/widget.h"

// A wrapper for a views::Widget dialog to interact with a Cocoa browser's
// ContrainedWindowSheetController. Similar to CustomConstrainedWindowSheet, but
// since Widgets of dialog type animate themselves, and also manage their own
// parenting, there's not much to do except position properly.
@interface WrappedConstrainedWindowSheet : NSObject<ConstrainedWindowSheet> {
 @private
  base::scoped_nsobject<NSWindow> customWindow_;
}
- (id)initWithCustomWindow:(NSWindow*)customWindow;
@end

@implementation WrappedConstrainedWindowSheet

- (id)initWithCustomWindow:(NSWindow*)customWindow {
  if ((self = [super init])) {
    customWindow_.reset([customWindow retain]);
  }
  return self;
}

// ConstrainedWindowSheet implementation.

- (void)showSheetForWindow:(NSWindow*)window {
  // This is only called for the initial show, then calls go to unhideSheet.
  // Since Widget::Show() will be called, just update the position.
  [self updateSheetPosition];
}

- (void)closeSheetWithAnimation:(BOOL)withAnimation {
  // Nothing to do here. Either SingleWebContentsDialogManagerViewsMac::Close()
  // was called or Widget::Close(). Both cases ending up in OnWidgetClosing() to
  // call [ConstrainedWindowSheetController closeSheet:], which calls this.
  // However, the Widget is already closing in those cases.

  // OnWidgetClosing() is also the _only_ trigger. The exception would be
  // -[ConstrainedWindowSheetController onParentWindowWillClose:] which also
  // calls closeSheetWithAnimation:, but a Widget never gets there because
  // WebContentsModalDialogManager::CloseAllDialogs() is triggered from
  // -[BrowserWindowController windowShouldClose:], but the notification that
  // calls onParentWindowWillClose always happens once that has returned YES.

  // So, since onParentWindowWillClose never calls this, we can assert that
  // withAnimation is YES, otherwise there's some code path that might not be
  // catered for.
  DCHECK(withAnimation);
}

- (void)hideSheet {
  // Hide the sheet window by setting the alpha to 0. This technique is used
  // instead of -orderOut: because that may cause a Spaces change or window
  // ordering change.
  [customWindow_ setAlphaValue:0.0];
  // TODO(tapted): Support child windows.
  DCHECK_EQ(0u, [[customWindow_ childWindows] count]);
}

- (void)unhideSheet {
  [customWindow_ setAlphaValue:1.0];
  DCHECK_EQ(0u, [[customWindow_ childWindows] count]);
}

- (void)pulseSheet {
  base::scoped_nsobject<NSAnimation> animation(
      [[ConstrainedWindowAnimationPulse alloc] initWithWindow:customWindow_]);
  [animation startAnimation];
}

- (void)makeSheetKeyAndOrderFront {
  // If the window is not visible, do nothing. Widget::Show() is responsible for
  // showing, and it may want to animate it.
  if ([customWindow_ isVisible])
    [customWindow_ makeKeyAndOrderFront:nil];
}

- (void)updateSheetPosition {
  ConstrainedWindowSheetController* controller =
      [ConstrainedWindowSheetController controllerForSheet:self];
  NSPoint origin = [controller originForSheet:self
                               withWindowSize:[customWindow_ frame].size];
  [customWindow_ setFrameOrigin:origin];
}

- (void)resizeWithNewSize:(NSSize)size {
  // NOOP
}

- (NSWindow*)sheetWindow {
  return customWindow_;
}

@end

SingleWebContentsDialogManagerViewsMac::SingleWebContentsDialogManagerViewsMac(
    NSWindow* dialog,
    web_modal::SingleWebContentsDialogManagerDelegate* delegate)
    : delegate_(delegate), host_(nullptr) {
  sheet_.reset(
      [[WrappedConstrainedWindowSheet alloc] initWithCustomWindow:dialog]);
  widget_ = views::Widget::GetWidgetForNativeWindow(dialog);
  DCHECK(widget_);
  widget_->AddObserver(this);
}

SingleWebContentsDialogManagerViewsMac::
    ~SingleWebContentsDialogManagerViewsMac() {
  DCHECK(!widget_->HasObserver(this));
}

void SingleWebContentsDialogManagerViewsMac::Show() {
  DCHECK(host_);

  NSView* parent_view = host_->GetHostView();
  // Note that simply [parent_view window] for an inactive tab is nil. However,
  // the following should always be non-nil for all WebContents containers.
  NSWindow* parent_window =
      delegate_->GetWebContents()->GetTopLevelNativeWindow();
  // Register with the ConstrainedWindowSheetController. This ensures that, e.g.
  // the NSView that overlays the Cocoa WebContents to intercept clicks is
  // installed and managed.
  [[ConstrainedWindowSheetController controllerForParentWindow:parent_window]
          showSheet:sheet_
      forParentView:parent_view];

  if (!widget_->IsVisible()) {
    if (was_shown_) {
      // Disable animations when switching tabs.
      widget_->SetVisibilityChangedAnimationsEnabled(false);
    }
    widget_->Show();
    widget_->SetVisibilityChangedAnimationsEnabled(true);
    was_shown_ = true;
  }
}

void SingleWebContentsDialogManagerViewsMac::Hide() {
  NSWindow* parent_window =
      delegate_->GetWebContents()->GetTopLevelNativeWindow();
  [[ConstrainedWindowSheetController controllerForParentWindow:parent_window]
      hideSheet:sheet_];

  widget_->Hide();
}

void SingleWebContentsDialogManagerViewsMac::Close() {
  // When the WebContents is destroyed, WebContentsModalDialogManager
  // ::CloseAllDialogs will call this. Close the Widget in the same manner as
  // the dialogs so that codepaths are consistent.
  widget_->SetVisibilityChangedAnimationsEnabled(false);
  widget_->Close();  // Note: Synchronously calls OnWidgetClosing() below.
}

void SingleWebContentsDialogManagerViewsMac::Focus() {
  [sheet_ makeSheetKeyAndOrderFront];
}
void SingleWebContentsDialogManagerViewsMac::Pulse() {
  // Handled by ConstrainedWindowSheetController.
}

void SingleWebContentsDialogManagerViewsMac::HostChanged(
    web_modal::WebContentsModalDialogHost* new_host) {
  // No need to observe the host. For Cocoa, the constrained window controller
  // will reposition the dialog when necessary. The host can also never change.
  // Tabs showing a dialog can not be dragged off a Cocoa browser window.
  // However, closing a tab with a dialog open will set the host back to null.
  DCHECK_NE(!!host_, !!new_host);
  host_ = new_host;
}

gfx::NativeWindow SingleWebContentsDialogManagerViewsMac::dialog() {
  return [sheet_ sheetWindow];
}

// views::WidgetObserver:
void SingleWebContentsDialogManagerViewsMac::OnWidgetClosing(
    views::Widget* widget) {
  DCHECK_EQ(widget, widget_);
  widget->RemoveObserver(this);
  [[ConstrainedWindowSheetController controllerForSheet:sheet_]
      closeSheet:sheet_];
  delegate_->WillClose(dialog());  // Deletes |this|.
}

void SingleWebContentsDialogManagerViewsMac::OnWidgetDestroying(
    views::Widget* widget) {
  // On Mac, this would only be reached if something called -[NSWindow close]
  // on the dialog without going through Widget::Close or CloseNow(). In this
  // case (only), OnWidgetClosing() is skipped, so invoke it here. Note: since
  // dialogs have no titlebar, it "shouldn't" happen, but crashes in
  // https://crbug.com/825809 suggest it can. Possibly this occurs via code
  // injection or a third party tool.
  OnWidgetClosing(widget);  // Deletes |this|.
}
