// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INFOBARS_CORE_CONFIRM_INFOBAR_DELEGATE_H_
#define COMPONENTS_INFOBARS_CORE_CONFIRM_INFOBAR_DELEGATE_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/infobars/core/infobar_delegate.h"
#include "components/infobars/core/infobar_manager.h"
#include "ui/gfx/text_constants.h"
#include "url/gurl.h"

namespace infobars {
class InfoBar;
}

// An interface derived from InfoBarDelegate implemented by objects wishing to
// control a ConfirmInfoBar.
class ConfirmInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  enum InfoBarButton {
    BUTTON_NONE   = 0,
    BUTTON_OK     = 1 << 0,
    BUTTON_CANCEL = 1 << 1,
  };

  ~ConfirmInfoBarDelegate() override;

  // Returns the InfoBar type to be displayed for the InfoBar.
  InfoBarAutomationType GetInfoBarAutomationType() const override;

  // Returns the message string to be displayed for the InfoBar.
  virtual base::string16 GetMessageText() const = 0;

  // Returns the elide behavior for the message string.
  // Not supported on Android.
  virtual gfx::ElideBehavior GetMessageElideBehavior() const;

  // Returns the buttons to be shown for this InfoBar.
  virtual int GetButtons() const;

  // Returns the label for the specified button. The default implementation
  // returns "OK" for the OK button and "Cancel" for the Cancel button.
  virtual base::string16 GetButtonLabel(InfoBarButton button) const;

  // Returns whether or not the OK button will trigger a UAC elevation prompt on
  // Windows.
  virtual bool OKButtonTriggersUACPrompt() const;

  // Called when the OK button is pressed. If this function returns true, the
  // infobar is then immediately closed. Subclasses MUST NOT return true if in
  // handling this call something triggers the infobar to begin closing.
  virtual bool Accept();

  // Called when the Cancel button is pressed. If this function returns true,
  // the infobar is then immediately closed. Subclasses MUST NOT return true if
  // in handling this call something triggers the infobar to begin closing.
  virtual bool Cancel();

  // Returns the text of the link to be displayed, if any. Otherwise returns
  // an empty string.
  virtual base::string16 GetLinkText() const;

  // Returns the URL of the link to be displayed.
  virtual GURL GetLinkURL() const;

  // Called when the link (if any) is clicked; if this function returns true,
  // the infobar is then immediately closed. The default implementation opens
  // the URL returned by GetLinkURL(), above, and returns false. Subclasses MUST
  // NOT return true if in handling this call something triggers the infobar to
  // begin closing.
  //
  // The |disposition| specifies how the resulting document should be loaded
  // (based on the event flags present when the link was clicked).
  virtual bool LinkClicked(WindowOpenDisposition disposition);

 protected:
  ConfirmInfoBarDelegate();

 private:
  // InfoBarDelegate:
  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override;
  ConfirmInfoBarDelegate* AsConfirmInfoBarDelegate() override;

  DISALLOW_COPY_AND_ASSIGN(ConfirmInfoBarDelegate);
};

#endif  // COMPONENTS_INFOBARS_CORE_CONFIRM_INFOBAR_DELEGATE_H_
