// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_SHARED_CHROME_BROWSER_UI_OMNIBOX_LOCATION_BAR_CONTROLLER_H_
#define IOS_SHARED_CHROME_BROWSER_UI_OMNIBOX_LOCATION_BAR_CONTROLLER_H_

#include "ios/chrome/browser/ui/omnibox/web_omnibox_edit_controller.h"

class OmniboxView;

// C++ object that wraps an OmniboxViewIOS and serves as its
// OmniboxEditController.  LocationBarController bridges between the edit view
// and the rest of the browser and manages text field decorations (location
// icon, security icon, etc.).
class LocationBarController : public WebOmniboxEditController {
 public:
  LocationBarController();
  ~LocationBarController() override;

  // Called when toolbar state is updated.
  virtual void OnToolbarUpdated() = 0;

  // Resign omnibox first responder and end edit view editing.
  virtual void HideKeyboardAndEndEditing() = 0;

  // Tells the omnibox if it can show the hint text or not.
  virtual void SetShouldShowHintText(bool show_hint_text) = 0;

  // Returns a pointer to the text entry view.
  virtual const OmniboxView* GetLocationEntry() const = 0;
  virtual OmniboxView* GetLocationEntry() = 0;

  // True if the omnibox text field is showing a placeholder image in its left
  // view while it's collapsed (i.e. not in editing mode).
  virtual bool IsShowingPlaceholderWhileCollapsed() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(LocationBarController);
};

#endif  // IOS_SHARED_CHROME_BROWSER_UI_OMNIBOX_LOCATION_BAR_CONTROLLER_H_
