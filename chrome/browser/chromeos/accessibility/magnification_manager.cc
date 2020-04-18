// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/accessibility/magnification_manager.h"

#include <limits>
#include <memory>

#include "ash/magnifier/magnification_controller.h"
#include "ash/public/cpp/ash_features.h"
#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "ash/shell.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/focused_node_details.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace chromeos {

namespace {
MagnificationManager* g_magnification_manager = nullptr;
}  // namespace

// static
void MagnificationManager::Initialize() {
  CHECK(g_magnification_manager == nullptr);
  g_magnification_manager = new MagnificationManager();
}

// static
void MagnificationManager::Shutdown() {
  CHECK(g_magnification_manager);
  delete g_magnification_manager;
  g_magnification_manager = nullptr;
}

// static
MagnificationManager* MagnificationManager::Get() {
  return g_magnification_manager;
}

bool MagnificationManager::IsMagnifierEnabled() const {
  return fullscreen_magnifier_enabled_;
}

void MagnificationManager::SetMagnifierEnabled(bool enabled) {
  if (!profile_)
    return;

  PrefService* prefs = profile_->GetPrefs();
  prefs->SetBoolean(ash::prefs::kAccessibilityScreenMagnifierEnabled, enabled);
  prefs->CommitPendingWrite();
}

void MagnificationManager::SaveScreenMagnifierScale(double scale) {
  if (!profile_)
    return;

  profile_->GetPrefs()->SetDouble(
      ash::prefs::kAccessibilityScreenMagnifierScale, scale);
}

double MagnificationManager::GetSavedScreenMagnifierScale() const {
  if (!profile_)
    return std::numeric_limits<double>::min();

  return profile_->GetPrefs()->GetDouble(
      ash::prefs::kAccessibilityScreenMagnifierScale);
}

void MagnificationManager::SetProfileForTest(Profile* profile) {
  SetProfile(profile);
}

MagnificationManager::MagnificationManager() {
  registrar_.Add(this, chrome::NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE,
                 content::NotificationService::AllSources());
  registrar_.Add(this, chrome::NOTIFICATION_SESSION_STARTED,
                 content::NotificationService::AllSources());
  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                 content::NotificationService::AllSources());
  // TODO(warx): observe focus changed in page notification when either
  // fullscreen magnifier or docked magnifier is enabled.
  registrar_.Add(this, content::NOTIFICATION_FOCUS_CHANGED_IN_PAGE,
                 content::NotificationService::AllSources());

  // Connect to ash's DockedMagnifierController interface.
  if (ash::features::IsDockedMagnifierEnabled()) {
    content::ServiceManagerConnection::GetForProcess()
        ->GetConnector()
        ->BindInterface(ash::mojom::kServiceName,
                        &docked_magnifier_controller_);
  }
}

MagnificationManager::~MagnificationManager() {
  CHECK(this == g_magnification_manager);
}

void MagnificationManager::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE: {
      // Update |profile_| when entering the login screen.
      Profile* profile = ProfileManager::GetActiveUserProfile();
      if (ProfileHelper::IsSigninProfile(profile))
        SetProfile(profile);
      break;
    }
    case chrome::NOTIFICATION_SESSION_STARTED:
      // Update |profile_| when entering a session.
      SetProfile(ProfileManager::GetActiveUserProfile());

      // Add a session state observer to be able to monitor session changes.
      if (!session_state_observer_.get())
        session_state_observer_.reset(
            new user_manager::ScopedUserSessionStateObserver(this));
      break;
    case chrome::NOTIFICATION_PROFILE_DESTROYED: {
      // Update |profile_| when exiting a session or shutting down.
      Profile* profile = content::Source<Profile>(source).ptr();
      if (profile_ == profile)
        SetProfile(NULL);
      break;
    }
    case content::NOTIFICATION_FOCUS_CHANGED_IN_PAGE: {
      HandleFocusChangedInPage(details);
      break;
    }
  }
}

void MagnificationManager::ActiveUserChanged(
    const user_manager::User* active_user) {
  if (active_user && active_user->is_profile_created())
    SetProfile(ProfileManager::GetActiveUserProfile());
}

void MagnificationManager::SetProfile(Profile* profile) {
  pref_change_registrar_.reset();

  if (profile) {
    // TODO(yoshiki): Move following code to PrefHandler.
    pref_change_registrar_.reset(new PrefChangeRegistrar);
    pref_change_registrar_->Init(profile->GetPrefs());
    pref_change_registrar_->Add(
        ash::prefs::kAccessibilityScreenMagnifierEnabled,
        base::BindRepeating(&MagnificationManager::UpdateMagnifierFromPrefs,
                            base::Unretained(this)));
    pref_change_registrar_->Add(
        ash::prefs::kAccessibilityScreenMagnifierCenterFocus,
        base::BindRepeating(&MagnificationManager::UpdateMagnifierFromPrefs,
                            base::Unretained(this)));
    pref_change_registrar_->Add(
        ash::prefs::kAccessibilityScreenMagnifierScale,
        base::BindRepeating(&MagnificationManager::UpdateMagnifierFromPrefs,
                            base::Unretained(this)));
  }

  profile_ = profile;
  UpdateMagnifierFromPrefs();
}

void MagnificationManager::SetMagnifierEnabledInternal(bool enabled) {
  // This method may be invoked even when the other magnifier settings (e.g.
  // type or scale) are changed, so we need to call magnification controller
  // even if |enabled| is unchanged. Only if |enabled| is false and the
  // magnifier is already disabled, we are sure that we don't need to reflect
  // the new settings right now because the magnifier keeps disabled.
  if (!enabled && !fullscreen_magnifier_enabled_)
    return;

  fullscreen_magnifier_enabled_ = enabled;

  ash::Shell::Get()->magnification_controller()->SetEnabled(enabled);
}

void MagnificationManager::SetMagnifierKeepFocusCenteredInternal(
    bool keep_focus_centered) {
  if (keep_focus_centered_ == keep_focus_centered)
    return;

  keep_focus_centered_ = keep_focus_centered;

  ash::Shell::Get()->magnification_controller()->SetKeepFocusCentered(
      keep_focus_centered_);
}

void MagnificationManager::SetMagnifierScaleInternal(double scale) {
  if (scale_ == scale)
    return;

  scale_ = scale;

  ash::Shell::Get()->magnification_controller()->SetScale(scale_,
                                                          false /* animate */);
}

void MagnificationManager::UpdateMagnifierFromPrefs() {
  if (!profile_)
    return;

  PrefService* prefs = profile_->GetPrefs();
  const bool enabled =
      prefs->GetBoolean(ash::prefs::kAccessibilityScreenMagnifierEnabled);
  const bool keep_focus_centered =
      prefs->GetBoolean(ash::prefs::kAccessibilityScreenMagnifierCenterFocus);
  const double scale =
      prefs->GetDouble(ash::prefs::kAccessibilityScreenMagnifierScale);

  if (!enabled) {
    SetMagnifierEnabledInternal(enabled);
    SetMagnifierKeepFocusCenteredInternal(keep_focus_centered);
    SetMagnifierScaleInternal(scale);
  } else {
    SetMagnifierScaleInternal(scale);
    SetMagnifierKeepFocusCenteredInternal(keep_focus_centered);
    SetMagnifierEnabledInternal(enabled);
  }

  AccessibilityStatusEventDetails details(ACCESSIBILITY_TOGGLE_SCREEN_MAGNIFIER,
                                          fullscreen_magnifier_enabled_);

  if (!AccessibilityManager::Get())
    return;
  AccessibilityManager::Get()->NotifyAccessibilityStatusChanged(details);
  if (ash::Shell::Get())
    ash::Shell::Get()->UpdateCursorCompositingEnabled();
}

void MagnificationManager::HandleFocusChangedInPage(
    const content::NotificationDetails& details) {
  const bool docked_magnifier_enabled =
      ash::features::IsDockedMagnifierEnabled() && profile_ &&
      profile_->GetPrefs()->GetBoolean(ash::prefs::kDockedMagnifierEnabled);
  if (!fullscreen_magnifier_enabled_ && !docked_magnifier_enabled)
    return;

  content::FocusedNodeDetails* node_details =
      content::Details<content::FocusedNodeDetails>(details).ptr();
  // Ash uses the InputMethod of the window tree host to observe text input
  // caret bounds changes, which works for both the native UI as well as
  // webpages. We don't need to notify it of editable nodes in this case.
  if (node_details->is_editable_node)
    return;

  const gfx::Rect& bounds_in_screen = node_details->node_bounds_in_screen;
  if (bounds_in_screen.IsEmpty())
    return;

  // Fullscreen magnifier and docked magnifier are mutually exclusive.
  if (fullscreen_magnifier_enabled_) {
    ash::Shell::Get()->magnification_controller()->HandleFocusedNodeChanged(
        node_details->is_editable_node, node_details->node_bounds_in_screen);
    return;
  }
  DCHECK(docked_magnifier_enabled);
  // Called when docked magnifier feature is enabled to avoid unnecessary
  // mojo IPC to ash.
  docked_magnifier_controller_->CenterOnPoint(bounds_in_screen.CenterPoint());
}

}  // namespace chromeos
