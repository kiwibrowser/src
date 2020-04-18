// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TAB_DIALOGS_VIEWS_MAC_H_
#define CHROME_BROWSER_UI_COCOA_TAB_DIALOGS_VIEWS_MAC_H_

#include "chrome/browser/ui/cocoa/tab_dialogs_cocoa.h"

// Implementation of TabDialogs interface for toolkit-views dialogs on Mac.
class TabDialogsViewsMac : public TabDialogsCocoa {
 public:
  explicit TabDialogsViewsMac(content::WebContents* contents);
  ~TabDialogsViewsMac() override;

  // TabDialogs:
  void ShowCollectedCookies() override;
  void ShowProfileSigninConfirmation(
      Browser* browser,
      Profile* profile,
      const std::string& username,
      std::unique_ptr<ui::ProfileSigninConfirmationDelegate> delegate) override;
  void ShowManagePasswordsBubble(bool user_action) override;
  void HideManagePasswordsBubble() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabDialogsViewsMac);
};

#endif  // CHROME_BROWSER_UI_COCOA_TAB_DIALOGS_VIEWS_MAC_H_
