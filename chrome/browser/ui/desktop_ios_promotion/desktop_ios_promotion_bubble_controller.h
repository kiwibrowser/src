// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_BUBBLE_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_controller.h"
#include "chrome/browser/ui/desktop_ios_promotion/sms_service.h"

namespace desktop_ios_promotion {
enum class PromotionEntryPoint;
}

class DesktopIOSPromotionView;

// This class provides data to the Desktop to mobile promotion and control the
// promotion actions.
class DesktopIOSPromotionBubbleController
    : public DesktopIOSPromotionController {
 public:
  // Must be instantiated on the UI thread.
  DesktopIOSPromotionBubbleController(
      Profile* profile,
      DesktopIOSPromotionView* promotion_view,
      desktop_ios_promotion::PromotionEntryPoint entry_point);
  ~DesktopIOSPromotionBubbleController();

  // Called by the view code when "Send SMS" button is clicked by the user.
  void OnSendSMSClicked();

  // Called by the view code when "No Thanks" button is clicked by the user.
  void OnNoThanksClicked();

  // Returns the Recovery phone number, returns empy string if the number is not
  // set.
  std::string GetUsersRecoveryPhoneNumber();

 private:
  // Updates the user's recovery phone number once the sms_service phone query
  // returns a response.
  void OnGotPhoneNumber(SMSService::Request* request,
                        bool success,
                        const std::string& number);

  // Callback that logs the result when sms_service send sms returns a response.
  void OnSendSMS(SMSService::Request* request,
                 bool success,
                 const std::string& number);

  // Service used to send SMS to the user recovery phone number.
  SMSService* sms_service_;
  // User's recovery phone number, this is updated by the sms_service.
  std::string recovery_number_;
  // A Weak pointer to the promotion view.
  DesktopIOSPromotionView* promotion_view_;

  base::WeakPtrFactory<DesktopIOSPromotionBubbleController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DesktopIOSPromotionBubbleController);
};

#endif  // CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_BUBBLE_CONTROLLER_H_
