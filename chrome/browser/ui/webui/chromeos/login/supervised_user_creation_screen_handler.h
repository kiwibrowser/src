// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_SUPERVISED_USER_CREATION_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_SUPERVISED_USER_CREATION_SCREEN_HANDLER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/chromeos/login/users/default_user_image/default_user_images.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"
#include "content/public/browser/web_ui.h"

namespace base {
class ListValue;
}

namespace chromeos {

class SupervisedUserCreationScreenHandler : public BaseScreenHandler {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    // This method is called, when view is being destroyed. Note, if Delegate
    // is destroyed earlier then it has to call SetDelegate(nullptr).
    virtual void OnViewDestroyed(SupervisedUserCreationScreenHandler* view) = 0;

    // Starts supervised user creation flow, with manager identified by
    // |manager_id| and |manager_password|.
    virtual void AuthenticateManager(const AccountId& manager_account_id,
                                     const std::string& manager_password) = 0;

    // Starts supervised user creation flow, with supervised user that would
    // have |display_name| and authenticated by the |supervised_user_password|.
    virtual void CreateSupervisedUser(
        const base::string16& display_name,
        const std::string& supervised_user_password) = 0;

    // Look up if user with name |display_name| already exist and can be
    // imported. Returns user ID in |out_id|. Returns true if user was found,
    // false otherwise.
    virtual bool FindUserByDisplayName(const base::string16& display_name,
                                       std::string *out_id) const = 0;

    // Starts supervised user import flow for user identified with |user_id|.
    virtual void ImportSupervisedUser(const std::string& user_id) = 0;
    // Starts supervised user import flow for user identified with |user_id| and
    // additional |password|.
    virtual void ImportSupervisedUserWithPassword(
        const std::string& user_id, const std::string& password) = 0;

    virtual void AbortFlow() = 0;
    virtual void FinishFlow() = 0;
    virtual void HideFlow() = 0;

    virtual void OnPhotoTaken(const std::string& raw_data) = 0;
    virtual void OnImageSelected(const std::string& image_type,
                                 const std::string& image_url) = 0;
    virtual void OnImageAccepted() = 0;
    virtual void OnPageSelected(const std::string& page) = 0;
  };

  SupervisedUserCreationScreenHandler();
  ~SupervisedUserCreationScreenHandler() override;

  virtual void Show();
  virtual void Hide();
  virtual void SetDelegate(Delegate* delegate);

  void ShowManagerPasswordError();

  void ShowIntroPage();
  void ShowUsernamePage();

  // Shows progress or error message close in the button area. |is_progress| is
  // true for progress messages and false for error messages.
  void ShowStatusMessage(bool is_progress, const base::string16& message);
  void ShowTutorialPage();

  void ShowErrorPage(const base::string16& title,
                     const base::string16& message,
                     const base::string16& button_text);

  // Navigates to specified page.
  void ShowPage(const std::string& page);

  void SetCameraPresent(bool enabled);

  void ShowExistingSupervisedUsers(const base::ListValue* users);

  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void Initialize() override;

  // WebUIMessageHandler implementation:
  void RegisterMessages() override;

 private:
  // WebUI message handlers.
  void HandleCheckSupervisedUserName(const base::string16& name);

  void HandleManagerSelected(const AccountId& manager_id);
  void HandleImportUserSelected(const AccountId& account_id);

  void HandleFinishLocalSupervisedUserCreation();
  void HandleAbortLocalSupervisedUserCreation();
  void HandleHideLocalSupervisedUserCreation();
  void HandleRetryLocalSupervisedUserCreation(const base::ListValue* args);
  void HandleCurrentSupervisedUserPage(const std::string& current_page);

  void HandleAuthenticateManager(const AccountId& manager_account_id,
                                 const std::string& manager_password);
  void HandleCreateSupervisedUser(const base::string16& new_raw_user_name,
                               const std::string& new_user_password);
  void HandleImportSupervisedUser(const AccountId& account_id);
  void HandleImportSupervisedUserWithPassword(const AccountId& account_id,
                                              const std::string& password);

  void HandleGetImages();
  void HandlePhotoTaken(const std::string& image_url);
  void HandleTakePhoto();
  void HandleDiscardPhoto();
  void HandleSelectImage(const std::string& image_url,
                         const std::string& image_type);

  void UpdateText(const std::string& element_id, const base::string16& text);

  Delegate* delegate_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SupervisedUserCreationScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_SUPERVISED_USER_CREATION_SCREEN_HANDLER_H_
