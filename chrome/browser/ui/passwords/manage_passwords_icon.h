// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PASSWORDS_MANAGE_PASSWORDS_ICON_H_
#define CHROME_BROWSER_UI_PASSWORDS_MANAGE_PASSWORDS_ICON_H_

#include "base/macros.h"
#include "chrome/browser/ui/passwords/manage_passwords_icon_view.h"
#include "components/password_manager/core/common/password_manager_ui.h"

// Base class for platform-specific password management icon views.
// TODO(estade): remove this class from Cocoa as it has been from Views.
class ManagePasswordsIcon : public ManagePasswordsIconView {
 public:
  // Get/set the icon's state. Implementations of this class must implement
  // SetStateInternal to do reasonable platform-specific things to represent
  // the icon's state to the user.
  void SetState(password_manager::ui::State state) override;
  password_manager::ui::State state() const { return state_; }

 protected:
  // The ID of the text resource that is currently displayed.
  int tooltip_text_id_;

  ManagePasswordsIcon();
  ~ManagePasswordsIcon();

  // Called from SetState() and SetActive() in order to do whatever
  // platform-specific UI work is necessary.
  virtual void UpdateVisibleUI() = 0;

  // Called from SetState() iff the icon's state has changed.
  virtual void OnChangingState() = 0;

 private:
  // Updates the resource IDs in response to state changes.
  void UpdateIDs();

  password_manager::ui::State state_;

  DISALLOW_COPY_AND_ASSIGN(ManagePasswordsIcon);
};

#endif  // CHROME_BROWSER_UI_PASSWORDS_MANAGE_PASSWORDS_ICON_H_
