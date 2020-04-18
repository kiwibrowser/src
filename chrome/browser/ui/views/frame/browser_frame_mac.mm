// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/views/frame/browser_frame_mac.h"

#import "base/mac/foundation_util.h"
#include "chrome/browser/global_keyboard_shortcuts_mac.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#import "chrome/browser/ui/cocoa/browser_window_command_handler.h"
#import "chrome/browser/ui/cocoa/chrome_command_dispatcher_delegate.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"
#import "chrome/browser/ui/views/frame/browser_native_widget_window_mac.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "content/public/browser/native_web_keyboard_event.h"
#import "ui/base/cocoa/window_size_constants.h"

namespace {

bool ShouldHandleKeyboardEvent(const content::NativeWebKeyboardEvent& event) {
  // |event.skip_in_browser| is true when it shouldn't be handled by the browser
  // if it was ignored by the renderer. See http://crbug.com/25000.
  if (event.skip_in_browser)
    return false;

  // Ignore synthesized keyboard events. See http://crbug.com/23221.
  if (event.GetType() == content::NativeWebKeyboardEvent::kChar)
    return false;

  // If the event was not synthesized it should have an os_event.
  DCHECK(event.os_event);

  // Do not fire shortcuts on key up.
  return [event.os_event type] == NSKeyDown;
}

// Returns true if |event| was handled.
bool HandleExtraKeyboardShortcut(NSEvent* event,
                                 Browser* browser,
                                 ChromeCommandDispatcherDelegate* delegate) {
  // Send the event to the menu before sending it to the browser/window
  // shortcut handling, so that if a user configures cmd-left to mean
  // "previous tab", it takes precedence over the built-in "history back"
  // binding.
  if ([[NSApp mainMenu] performKeyEquivalent:event])
    return true;

  // Invoke ChromeCommandDispatcherDelegate for Mac-specific shortcuts that
  // can't be handled by accelerator_table.cc.
  return [delegate
      handleExtraKeyboardShortcut:event
                           window:browser->window()->GetNativeWindow()];
}

}  // namespace

BrowserFrameMac::BrowserFrameMac(BrowserFrame* browser_frame,
                                 BrowserView* browser_view)
    : views::NativeWidgetMac(browser_frame),
      browser_view_(browser_view),
      command_dispatcher_delegate_(
          [[ChromeCommandDispatcherDelegate alloc] init]) {}

BrowserFrameMac::~BrowserFrameMac() {
}

////////////////////////////////////////////////////////////////////////////////
// BrowserFrameMac, views::NativeWidgetMac implementation:

int BrowserFrameMac::SheetPositionY() {
  web_modal::WebContentsModalDialogHost* dialog_host =
      browser_view_->GetWebContentsModalDialogHost();
  NSView* view = dialog_host->GetHostView();
  // Get the position of the host view relative to the window since
  // ModalDialogHost::GetDialogPosition() is relative to the host view.
  int host_view_y =
      [view convertPoint:NSMakePoint(0, NSHeight([view frame])) toView:nil].y;
  return host_view_y - dialog_host->GetDialogPosition(gfx::Size()).y();
}

void BrowserFrameMac::InitNativeWidget(
    const views::Widget::InitParams& params) {
  views::NativeWidgetMac::InitNativeWidget(params);

  [[GetNativeWindow() contentView] setWantsLayer:YES];
}

NativeWidgetMacNSWindow* BrowserFrameMac::CreateNSWindow(
    const views::Widget::InitParams& params) {
  NSUInteger style_mask = NSTitledWindowMask | NSClosableWindowMask |
                          NSMiniaturizableWindowMask | NSResizableWindowMask;

  base::scoped_nsobject<NativeWidgetMacNSWindow> ns_window;
  if (browser_view_->IsBrowserTypeNormal()) {
    if (@available(macOS 10.10, *))
      style_mask |= NSFullSizeContentViewWindowMask;
    ns_window.reset([[BrowserNativeWidgetWindow alloc]
        initWithContentRect:ui::kWindowSizeDeterminedLater
                  styleMask:style_mask
                    backing:NSBackingStoreBuffered
                      defer:NO]);
    // Ensure tabstrip/profile button are visible.
    if (@available(macOS 10.10, *))
      [ns_window setTitlebarAppearsTransparent:YES];
  } else {
    ns_window.reset([[NativeWidgetMacNSWindow alloc]
        initWithContentRect:ui::kWindowSizeDeterminedLater
                  styleMask:style_mask
                    backing:NSBackingStoreBuffered
                      defer:NO]);
  }
  [ns_window setCommandDispatcherDelegate:command_dispatcher_delegate_];
  [ns_window setCommandHandler:[[[BrowserWindowCommandHandler alloc] init]
                                   autorelease]];
  return ns_window.autorelease();
}

void BrowserFrameMac::OnWindowDestroying(NSWindow* window) {
  // Clear delegates set in CreateNSWindow() to prevent objects with a reference
  // to |window| attempting to validate commands by looking for a Browser*.
  NativeWidgetMacNSWindow* ns_window =
      base::mac::ObjCCastStrict<NativeWidgetMacNSWindow>(window);
  [ns_window setCommandHandler:nil];
  [ns_window setCommandDispatcherDelegate:nil];
}

int BrowserFrameMac::GetMinimizeButtonOffset() const {
  NOTIMPLEMENTED();
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// BrowserFrameMac, NativeBrowserFrame implementation:

views::Widget::InitParams BrowserFrameMac::GetWidgetParams() {
  views::Widget::InitParams params;
  params.native_widget = this;
  return params;
}

bool BrowserFrameMac::UseCustomFrame() const {
  return false;
}

bool BrowserFrameMac::UsesNativeSystemMenu() const {
  return true;
}

bool BrowserFrameMac::ShouldSaveWindowPlacement() const {
  return true;
}

void BrowserFrameMac::GetWindowPlacement(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  return NativeWidgetMac::GetWindowPlacement(bounds, show_state);
}

// Mac is special because the user could override the menu shortcuts (see
// comment in HandleExtraKeyboardShortcut), and there's a set of additional
// accelerator tables (handled by ChromeCommandDispatcherDelegate) that couldn't
// be ported to accelerator_table.cc: see global_keyboard_shortcuts_views_mac.mm
bool BrowserFrameMac::PreHandleKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  if (!ShouldHandleKeyboardEvent(event))
    return false;

  // CommandForKeyEvent consults the [NSApp mainMenu] and Mac-specific
  // accelerator tables internally.
  int command_id = CommandForKeyEvent(event.os_event);
  if (command_id == -1)
    return false;

  // Only handle a small list of reserved commands that we don't want to be
  // handled by the renderer.
  Browser* browser = browser_view_->browser();
  if (!browser->command_controller()->IsReservedCommandOrKey(
          command_id, event))
    return false;

  return HandleExtraKeyboardShortcut(event.os_event, browser,
                                     command_dispatcher_delegate_);
}

bool BrowserFrameMac::HandleKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  if (!ShouldHandleKeyboardEvent(event))
    return false;

  return HandleExtraKeyboardShortcut(event.os_event, browser_view_->browser(),
                                     command_dispatcher_delegate_);
}
