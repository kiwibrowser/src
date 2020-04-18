// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NOTE_TAKING_CONTROLLER_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_NOTE_TAKING_CONTROLLER_CLIENT_H_

#include "ash/public/interfaces/note_taking_controller.mojom.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/note_taking_helper.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "mojo/public/cpp/bindings/binding.h"

class Profile;

namespace service_manager {
class Connector;
}

namespace chromeos {

class NoteTakingControllerClient
    : public ash::mojom::NoteTakingControllerClient,
      public NoteTakingHelper::Observer,
      public user_manager::UserManager::UserSessionStateObserver,
      public content::NotificationObserver {
 public:
  explicit NoteTakingControllerClient(NoteTakingHelper* helper);
  ~NoteTakingControllerClient() override;

  // ash::mojom::NoteTakingControllerClient:
  void CreateNote() override;

  // chromeos::NoteTakingHelper::Observer:
  void OnAvailableNoteTakingAppsUpdated() override;
  void OnPreferredNoteTakingAppUpdated(Profile* profile) override;

  // user_manager::UserManager::UserSessionStateObserver:
  void ActiveUserChanged(const user_manager::User* active_user) override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  void SetConnectorForTesting(service_manager::Connector* connector) {
    connector_ = connector;
  }

  void SetProfileForTesting(Profile* profile) { SetProfile(profile); }

  void FlushMojoForTesting() {
    if (controller_)
      controller_.FlushForTesting();
  }

 private:
  void SetProfile(Profile* profile);

  // Unowned pointer to the note taking helper.
  NoteTakingHelper* helper_;

  // Unowned pointer to the active profile.
  Profile* profile_ = nullptr;

  ScopedObserver<NoteTakingHelper, NoteTakingHelper::Observer> note_observer_;
  content::NotificationRegistrar registrar_;
  std::unique_ptr<user_manager::ScopedUserSessionStateObserver>
      session_state_observer_;

  mojo::Binding<ash::mojom::NoteTakingControllerClient> binding_;
  service_manager::Connector* connector_ = nullptr;
  ash::mojom::NoteTakingControllerPtr controller_;

  DISALLOW_COPY_AND_ASSIGN(NoteTakingControllerClient);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NOTE_TAKING_CONTROLLER_CLIENT_H_
