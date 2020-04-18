// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_SINGLE_WEB_CONTENTS_DIALOG_MANAGER_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_SINGLE_WEB_CONTENTS_DIALOG_MANAGER_COCOA_H_

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "components/web_modal/single_web_contents_dialog_manager.h"

class ConstrainedWindowMac;
@protocol ConstrainedWindowSheet;

// Cocoa implementation of web_modal::SingleWebContentsDialogManager for showing
// native Cocoa sheet dialogs (of various kinds).
class SingleWebContentsDialogManagerCocoa
    : public web_modal::SingleWebContentsDialogManager {
 public:
  SingleWebContentsDialogManagerCocoa(
      ConstrainedWindowMac* client,
      id<ConstrainedWindowSheet> sheet,
      web_modal::SingleWebContentsDialogManagerDelegate* delegate);
  ~SingleWebContentsDialogManagerCocoa() override;

  // SingleWebContentsDialogManager overrides.
  void Show() override;
  void Hide() override;
  void Close() override;
  void Focus() override;
  void Pulse() override;
  void HostChanged(web_modal::WebContentsModalDialogHost* new_host) override;
  gfx::NativeWindow dialog() override;

 private:
  // Weak. Legacy DialogManager-style delegate interface that Cocoa uses.
  ConstrainedWindowMac* client_;
  base::scoped_nsprotocol<id<ConstrainedWindowSheet>> sheet_;
  // Weak. Owns this.
  web_modal::SingleWebContentsDialogManagerDelegate* delegate_;
  // Weak. Owned by parent window.
  web_modal::WebContentsModalDialogHost* host_;

  DISALLOW_COPY_AND_ASSIGN(SingleWebContentsDialogManagerCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_SINGLE_WEB_CONTENTS_DIALOG_MANAGER_COCOA_H_
