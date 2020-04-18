// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_STARTUP_SESSION_CRASHED_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_UI_STARTUP_SESSION_CRASHED_INFOBAR_DELEGATE_H_

#include "base/macros.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

class Browser;
class Profile;

// A delegate for the InfoBar shown when the previous session has crashed.
//
// TODO: remove this class once mac supports SessionCrashedBubble.
// http://crbug.com/653966.
class SessionCrashedInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  // If |browser| is not incognito, creates a session crashed infobar and
  // delegate and adds the infobar to the InfoBarService for |browser|.
  static void Create(Browser* browser);

 private:
  explicit SessionCrashedInfoBarDelegate(Profile* profile);
  ~SessionCrashedInfoBarDelegate() override;

  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  base::string16 GetMessageText() const override;
  int GetButtons() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;

  bool accepted_;
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(SessionCrashedInfoBarDelegate);
};

#endif  // CHROME_BROWSER_UI_STARTUP_SESSION_CRASHED_INFOBAR_DELEGATE_H_
