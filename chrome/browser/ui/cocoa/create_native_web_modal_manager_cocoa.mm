// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/web_contents_modal_dialog_manager_views_mac.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"

namespace web_modal {

SingleWebContentsDialogManager*
WebContentsModalDialogManager::CreateNativeWebModalManager(
    gfx::NativeWindow dialog,
    web_modal::SingleWebContentsDialogManagerDelegate* delegate) {
  return new SingleWebContentsDialogManagerViewsMac(dialog, delegate);
}

}  // namespace web_modal
