// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SUPERVISED_SUPERVISED_USER_CREATION_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SUPERVISED_SUPERVISED_USER_CREATION_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/values.h"

class Profile;

namespace chromeos {

class SupervisedUserCreationController {
 public:
  // This constant is used to indicate that user does not have one of default
  // avatars: either they have no chromeos avatar at all, or they have an
  // external image as an avatar.
  static const int kDummyAvatarIndex;

  enum ErrorCode {
    NO_ERROR,
    CRYPTOHOME_NO_MOUNT,
    CRYPTOHOME_FAILED_MOUNT,
    CRYPTOHOME_FAILED_TPM,
    CLOUD_SERVER_ERROR,
    TOKEN_WRITE_FAILED,
  };

  class StatusConsumer {
   public:
    virtual ~StatusConsumer();

    virtual void OnCreationError(ErrorCode code) = 0;
    virtual void OnLongCreationWarning() = 0;
    virtual void OnCreationTimeout() = 0;
    virtual void OnCreationSuccess() = 0;
  };

  // All UI initialization is deferred till Init() call.
  // |Consumer| is not owned by controller, and it is expected that it wouldn't
  // be deleted before SupervisedUserCreationController.
  explicit SupervisedUserCreationController(StatusConsumer* consumer);
  virtual ~SupervisedUserCreationController();

  // Returns the current supervised user controller if it has been created.
  static SupervisedUserCreationController* current_controller() {
    return current_controller_;
  }

  virtual void SetManagerProfile(Profile* manager_profile) = 0;
  virtual Profile* GetManagerProfile() = 0;
  virtual void CancelCreation() = 0;
  virtual void FinishCreation() = 0;
  virtual std::string GetSupervisedUserId() = 0;

 protected:
  // Pointer to the current instance of the controller to be used by
  // automation tests.
  static SupervisedUserCreationController* current_controller_;

  StatusConsumer* consumer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SupervisedUserCreationController);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SUPERVISED_SUPERVISED_USER_CREATION_CONTROLLER_H_
