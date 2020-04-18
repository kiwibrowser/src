// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_INFOBARS_TEST_INFOBAR_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_INFOBARS_TEST_INFOBAR_DELEGATE_H_

#include "base/strings/utf_string_conversions.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

// The title of the test infobar.
extern const char kTestInfoBarTitle[];

// An infobar that displays a single line of text and no buttons.
class TestInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  static bool Create(infobars::InfoBarManager* infobar_manager);

  // InfoBarDelegate implementation.
  InfoBarIdentifier GetIdentifier() const override;
  // ConfirmInfoBarDelegate implementation.
  base::string16 GetMessageText() const override;
  int GetButtons() const override;
};

#endif  // IOS_CHROME_BROWSER_UI_INFOBARS_TEST_INFOBAR_DELEGATE_H_
