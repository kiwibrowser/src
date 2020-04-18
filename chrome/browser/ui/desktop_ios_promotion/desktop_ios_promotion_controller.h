// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_CONTROLLER_H_
#define CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/macros.h"

namespace desktop_ios_promotion {
enum class PromotionEntryPoint;
enum class PromotionDismissalReason;
}

class Profile;
class PrefService;

// This class provides data to the Desktop to mobile promotion and control the
// promotion actions.
class DesktopIOSPromotionController {
 public:
  // Must be instantiated on the UI thread.
  DesktopIOSPromotionController(
      Profile* profile,
      desktop_ios_promotion::PromotionEntryPoint entry_point);
  ~DesktopIOSPromotionController();

  // Returns the current promotion entry point.
  desktop_ios_promotion::PromotionEntryPoint entry_point() const {
    return entry_point_;
  }

  desktop_ios_promotion::PromotionDismissalReason dismissal_reason() const {
    return dismissal_reason_;
  }

  // Called by the view code when the promotion is ready to show.
  void OnPromotionShown();

  // Called by the view when link to detailed promo is clicked by the user.
  void OnLearnMoreLinkClicked();

 protected:
  void SetDismissalReason(
      desktop_ios_promotion::PromotionDismissalReason reason);

  PrefService* profile_prefs() { return profile_prefs_; }

 private:
  PrefService* profile_prefs_;
  const desktop_ios_promotion::PromotionEntryPoint entry_point_;

  // Track the action that is responsible for the promotion Dismissal.
  desktop_ios_promotion::PromotionDismissalReason dismissal_reason_;

  DISALLOW_COPY_AND_ASSIGN(DesktopIOSPromotionController);
};

#endif  // CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_CONTROLLER_H_
