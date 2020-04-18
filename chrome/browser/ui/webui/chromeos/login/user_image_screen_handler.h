// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_USER_IMAGE_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_USER_IMAGE_SCREEN_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/login/screens/user_image_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/native_widget_types.h"

namespace chromeos {

// WebUI implementation of UserImageView. It is used to interact with the JS
// page allowing user to select an avatar.
class UserImageScreenHandler : public UserImageView, public BaseScreenHandler {
 public:
  UserImageScreenHandler();
  ~UserImageScreenHandler() override;

 private:
  // BaseScreenHandler implementation:
  void Initialize() override;
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;

  // WebUIMessageHandler implementation:
  void RegisterMessages() override;

  // UserImageView implementation:
  void Bind(UserImageScreen* screen) override;
  void Unbind() override;
  void Show() override;
  void Hide() override;
  void HideCurtain() override;

  // Sends image data to the page.
  void HandleGetImages();

  // Screen ready to be shown.
  void HandleScreenReady();

  // Handles photo taken with WebRTC UI.
  void HandlePhotoTaken(const std::string& image_url);

  // Handles 'discard-photo' button click.
  void HandleDiscardPhoto();

  // Handles clicking on default user image.
  void HandleSelectImage(const std::string& image_type,
                         const std::string& image_url,
                         bool is_user_selection);

  // Called when user accept the image closing the screen.
  void HandleImageAccepted();

  // Called when the user image screen has been loaded and shown.
  void HandleScreenShown();

  UserImageScreen* screen_ = nullptr;

  // Keeps whether screen should be shown right after initialization.
  bool show_on_init_ = false;

  // Keeps whether screen has loaded all default images and redy to be shown.
  bool is_ready_ = false;

  base::Time screen_show_time_;

  DISALLOW_COPY_AND_ASSIGN(UserImageScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_USER_IMAGE_SCREEN_HANDLER_H_
