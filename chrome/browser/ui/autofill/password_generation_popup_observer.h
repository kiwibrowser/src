// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_AUTOFILL_PASSWORD_GENERATION_POPUP_OBSERVER_H_
#define CHROME_BROWSER_UI_AUTOFILL_PASSWORD_GENERATION_POPUP_OBSERVER_H_

namespace autofill {

// Observer for PasswordGenerationPopup events. Currently only used for testing.
class PasswordGenerationPopupObserver {
 public:
  virtual void OnPopupShown(bool password_visible) = 0;
  virtual void OnPopupHidden() = 0;
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_AUTOFILL_PASSWORD_GENERATION_POPUP_OBSERVER_H_
