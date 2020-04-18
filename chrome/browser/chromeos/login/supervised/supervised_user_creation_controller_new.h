// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SUPERVISED_SUPERVISED_USER_CREATION_CONTROLLER_NEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SUPERVISED_SUPERVISED_USER_CREATION_CONTROLLER_NEW_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_creation_controller.h"
#include "chromeos/login/auth/extended_authenticator.h"
#include "components/account_id/account_id.h"

class Profile;

namespace chromeos {

// Supervised user creation process:
// 0. Manager is logged in
// 1. Generate ID for new supervised user
// 2. Start "transaction" in Local State.
// 3, Generate keys for user : master key, salt, encryption and signature keys.
// 4. Create local cryptohome (errors could arise)
// 5. Create user in cloud (errors could arise)
// 6. Store cloud token in cryptohome (actually, error could arise).
// 7. Mark "transaction" as completed.
// 8. End manager session.
class SupervisedUserCreationControllerNew
    : public SupervisedUserCreationController,
      public ExtendedAuthenticator::NewAuthStatusConsumer {
 public:
  // All UI initialization is deferred till Init() call.
  // |Consumer| is not owned by controller, and it is expected that it wouldn't
  // be deleted before SupervisedUserCreationControllerNew.
  SupervisedUserCreationControllerNew(StatusConsumer* consumer,
                                      const AccountId& manager_id);
  ~SupervisedUserCreationControllerNew() override;

  // Returns the current supervised user controller if it has been created.
  static SupervisedUserCreationControllerNew* current_controller() {
    return current_controller_;
  }

  void SetManagerProfile(Profile* manager_profile) override;
  Profile* GetManagerProfile() override;

  void CancelCreation() override;
  void FinishCreation() override;
  std::string GetSupervisedUserId() override;

 private:
  enum Stage {
    // Just initial stage.
    STAGE_INITIAL,

    // Creation attempt is recoreded to allow cleanup in case of failure.
    TRANSACTION_STARTED,
    // Different keys are generated and public ones are stored in LocalState.
    KEYS_GENERATED,
    // Home directory is created with all necessary passwords.
    CRYPTOHOME_CREATED,
    // All user-related information is confirmed to exist on server.
    DASHBOARD_CREATED,
    // Managed user's sync token is written.
    TOKEN_WRITTEN,
    // Managed user is succesfully created.
    TRANSACTION_COMMITTED,
    // Some error happened while creating supervised user.
    STAGE_ERROR,
  };

  // Indicates if we create new user, or import an existing one.
  enum CreationType {
    NEW_USER,
    USER_IMPORT_OLD,
    USER_IMPORT_NEW,
  };

  // Contains information necessary for new user creation.
  struct UserCreationContext {
    UserCreationContext();
    ~UserCreationContext();

    base::string16 display_name;
    int avatar_index;

    AccountId manager_id = EmptyAccountId();

    std::string local_user_id;  // Used to identify cryptohome.
    std::string sync_user_id;   // Used to identify user in manager's sync data.

    // Keys:
    std::string master_key;       // Random string
    std::string signature_key;    // 256 bit HMAC key
    std::string encryption_key;   // 256 bit HMAC key
    std::string salted_password;  // Hash(salt + Hash(password))

    std::string password;

    std::string salted_master_key;  // Hash(system salt + master key)
    std::string mount_hash;

    std::string token;

    CreationType creation_type;

    base::DictionaryValue password_data;

    Profile* manager_profile;
  };

  // ExtendedAuthenticator::NewAuthStatusConsumer overrides.
  void OnAuthenticationFailure(ExtendedAuthenticator::AuthState error) override;

  // Guard timer callback.
  void CreationTimedOut();

  // Completion callback for StoreSupervisedUserFiles method.
  // Called on the UI thread.
  void OnSupervisedUserFilesStored(bool success);

  // Pointer to the current instance of the controller to be used by
  // automation tests.
  static SupervisedUserCreationControllerNew* current_controller_;

  // Current stage of user creation.
  Stage stage_;

  // Authenticator used for user creation.
  scoped_refptr<ExtendedAuthenticator> authenticator_;

  // Creation context. Not null while creating new LMU.
  std::unique_ptr<UserCreationContext> creation_context_;

  // Timer for showing warning if creation process takes too long.
  base::OneShotTimer timeout_timer_;

  // Factory of callbacks.
  base::WeakPtrFactory<SupervisedUserCreationControllerNew> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SupervisedUserCreationControllerNew);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SUPERVISED_SUPERVISED_USER_CREATION_CONTROLLER_NEW_H_
