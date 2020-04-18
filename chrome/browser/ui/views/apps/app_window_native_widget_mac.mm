// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/apps/app_window_native_widget_mac.h"

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/apps/titlebar_background_view.h"
#import "chrome/browser/ui/views/frame/native_widget_mac_frameless_nswindow.h"
#include "extensions/browser/app_window/native_app_window.h"
#import "ui/base/cocoa/window_size_constants.h"

AppWindowNativeWidgetMac::AppWindowNativeWidgetMac(
    views::Widget* widget,
    extensions::NativeAppWindow* native_app_window)
    : NativeWidgetMac(widget), native_app_window_(native_app_window) {
}

AppWindowNativeWidgetMac::~AppWindowNativeWidgetMac() {
}

NativeWidgetMacNSWindow* AppWindowNativeWidgetMac::CreateNSWindow(
    const views::Widget::InitParams& params) {
  // If the window has a native or colored frame, use the same NSWindow as
  // NativeWidgetMac.
  if (!native_app_window_->IsFrameless()) {
    NativeWidgetMacNSWindow* ns_window =
        NativeWidgetMac::CreateNSWindow(params);
    if (native_app_window_->HasFrameColor()) {
      [TitlebarBackgroundView
          addToNSWindow:ns_window
            activeColor:native_app_window_->ActiveFrameColor()
          inactiveColor:native_app_window_->InactiveFrameColor()];
    }
    return ns_window;
  }

  NSUInteger style_mask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskMiniaturizable |
                          NSWindowStyleMaskResizable;
  if (@available(macOS 10.10, *))
    style_mask |= NSWindowStyleMaskFullSizeContentView;
  else
    NOTREACHED();
  return [[[NativeWidgetMacFramelessNSWindow alloc]
      initWithContentRect:ui::kWindowSizeDeterminedLater
                styleMask:style_mask
                  backing:NSBackingStoreBuffered
                    defer:NO] autorelease];
}
