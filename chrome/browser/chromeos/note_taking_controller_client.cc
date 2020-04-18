// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/note_taking_controller_client.h"

#include "ash/public/interfaces/constants.mojom.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/connector.h"

namespace chromeos {

NoteTakingControllerClient::NoteTakingControllerClient(NoteTakingHelper* helper)
    : helper_(helper), note_observer_(this), binding_(this) {
  note_observer_.Add(helper_);
  registrar_.Add(this, chrome::NOTIFICATION_SESSION_STARTED,
                 content::NotificationService::AllSources());
  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                 content::NotificationService::AllSources());
}

NoteTakingControllerClient::~NoteTakingControllerClient() = default;

void NoteTakingControllerClient::CreateNote() {
  helper_->LaunchAppForNewNote(profile_, base::FilePath());
}

void NoteTakingControllerClient::OnAvailableNoteTakingAppsUpdated() {
  bool has_app = profile_ && helper_->IsAppAvailable(profile_);

  // No need to rebind the client if already connected.
  if (binding_.is_bound() == has_app)
    return;

  if (binding_.is_bound()) {
    binding_.Close();
  } else {
    // Connector can be overridden for testing.
    if (!connector_) {
      connector_ =
          content::ServiceManagerConnection::GetForProcess()->GetConnector();
    }
    connector_->BindInterface(ash::mojom::kServiceName, &controller_);
    ash::mojom::NoteTakingControllerClientPtr client;
    binding_.Bind(mojo::MakeRequest(&client));
    controller_->SetClient(std::move(client));
  }
}

void NoteTakingControllerClient::OnPreferredNoteTakingAppUpdated(
    Profile* profile) {}

void NoteTakingControllerClient::ActiveUserChanged(
    const user_manager::User* active_user) {
  SetProfile(ProfileHelper::Get()->GetProfileByUser(active_user));
}

void NoteTakingControllerClient::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_SESSION_STARTED:
      // Update |profile_| when entering a session.
      SetProfile(ProfileManager::GetActiveUserProfile());

      // Add a session state observer to be able to monitor session changes.
      if (!session_state_observer_.get()) {
        session_state_observer_.reset(
            new user_manager::ScopedUserSessionStateObserver(this));
      }
      break;
    case chrome::NOTIFICATION_PROFILE_DESTROYED: {
      // Update |profile_| when exiting a session or shutting down.
      Profile* profile = content::Source<Profile>(source).ptr();
      if (profile_ == profile)
        SetProfile(nullptr);
      break;
    }
  }
}

void NoteTakingControllerClient::SetProfile(Profile* profile) {
  profile_ = profile;
  OnAvailableNoteTakingAppsUpdated();
}

}  // namespace chromeos
