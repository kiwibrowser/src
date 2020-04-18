// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_INFOBARS_MOCK_CONFIRM_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_UI_COCOA_INFOBARS_MOCK_CONFIRM_INFOBAR_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

class MockConfirmInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  // Called when the dtor of |this| has been entered.
  class Owner {
   public:
    virtual void OnInfoBarDelegateClosed(
        MockConfirmInfoBarDelegate* delegate) = 0;

   protected:
    virtual ~Owner() {}
  };

  explicit MockConfirmInfoBarDelegate(Owner* owner);
  ~MockConfirmInfoBarDelegate() override;

  void set_dont_close_on_action() { closes_on_action_ = false; }
  bool icon_accessed() const { return icon_accessed_; }
  bool message_text_accessed() const { return message_text_accessed_; }
  bool link_text_accessed() const { return link_text_accessed_; }
  bool ok_clicked() const { return ok_clicked_; }
  bool cancel_clicked() const { return cancel_clicked_; }
  bool link_clicked() const { return link_clicked_; }

  static const char kMessage[];

 private:
  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  int GetIconId() const override;
  base::string16 GetMessageText() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;
  base::string16 GetLinkText() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;

  Owner* owner_;
  // Determines whether the infobar closes when an action is taken or not.
  bool closes_on_action_;
  mutable bool icon_accessed_;
  mutable bool message_text_accessed_;
  mutable bool link_text_accessed_;
  bool ok_clicked_;
  bool cancel_clicked_;
  bool link_clicked_;

  DISALLOW_COPY_AND_ASSIGN(MockConfirmInfoBarDelegate);
};

#endif  // CHROME_BROWSER_UI_COCOA_INFOBARS_MOCK_CONFIRM_INFOBAR_DELEGATE_H_
