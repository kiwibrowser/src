// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/extensions/extension_popup_views_mac.h"

#import <AppKit/AppKit.h>

#include "chrome/browser/extensions/extension_view_host.h"
#import "chrome/browser/ui/cocoa/bubble_anchor_helper_views.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

ExtensionPopupViewsMac::~ExtensionPopupViewsMac() {
  // ObjC id collides with views::View::id().
  for (::id token in observer_tokens_.get())
    [[NSNotificationCenter defaultCenter] removeObserver:token];
}

// static
ExtensionPopupViewsMac* ExtensionPopupViewsMac::ShowPopup(
    std::unique_ptr<extensions::ExtensionViewHost> host,
    gfx::NativeWindow parent_window,
    const gfx::Point& anchor_point,
    ExtensionPopup::ShowAction show_action) {
  // We can't use std::make_unique here as the constructor is private.
  std::unique_ptr<ExtensionPopupViewsMac> popup_owned(
      new ExtensionPopupViewsMac(std::move(host), anchor_point, show_action));
  auto* popup = popup_owned.get();
  popup->set_parent_window([parent_window contentView]);
  views::BubbleDialogDelegateView::CreateBubble(popup_owned.release());

  KeepBubbleAnchored(popup);

  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  // ObjC id collides with views::View::id().
  ::id token = [center addObserverForName:NSWindowDidBecomeKeyNotification
                                   object:parent_window
                                    queue:nil
                               usingBlock:^(NSNotification* notification) {
                                 popup->CloseUnlessUnderInspection();
                               }];
  [popup->observer_tokens_ addObject:token];
  return popup;
}

ExtensionPopupViewsMac::ExtensionPopupViewsMac(
    std::unique_ptr<extensions::ExtensionViewHost> host,
    const gfx::Point& anchor_point,
    ExtensionPopup::ShowAction show_action)
    : ExtensionPopup(host.release(),
                     nullptr,
                     views::BubbleBorder::TOP_RIGHT /* views flips for RTL. */,
                     show_action),
      observer_tokens_([[NSMutableArray alloc] init]) {
  SetAnchorRect(gfx::Rect(anchor_point, gfx::Size()));
}
