// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_POPUP_VIEWS_MAC_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_POPUP_VIEWS_MAC_H_

#import <Foundation/Foundation.h>

#include <memory>

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "chrome/browser/ui/views/extensions/extension_popup.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/bubble/bubble_border.h"

// Bridges NSWindow and the Anchor View expected by ExtensionPopup.
class ExtensionPopupViewsMac : public ExtensionPopup {
 public:
  ~ExtensionPopupViewsMac() override;

  // Create and show a popup with the given |host| anchored at |anchor_point| in
  // screen coordinates. The |parent_window| serves as the parent to the popup
  // and is used to handle dismissing the popup on activation and lifetime
  // events. |show_action| controls the dismissal of the popup with respect to
  // dev tools. The actual display of the popup is delayed until the page
  // contents finish loading in order to minimize UI flashing and resizing.
  static ExtensionPopupViewsMac* ShowPopup(
      std::unique_ptr<extensions::ExtensionViewHost> host,
      gfx::NativeWindow parent_window,
      const gfx::Point& anchor_point,
      ExtensionPopup::ShowAction show_action);

 private:
  ExtensionPopupViewsMac(std::unique_ptr<extensions::ExtensionViewHost> host,
                         const gfx::Point& anchor_point,
                         ExtensionPopup::ShowAction show_action);

  base::scoped_nsobject<NSMutableArray> observer_tokens_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionPopupViewsMac);
};

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_POPUP_VIEWS_MAC_H_
