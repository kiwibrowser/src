// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/arc_shelf_spinner_item_controller.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/ash/launcher/shelf_spinner_controller.h"

ArcShelfSpinnerItemController::ArcShelfSpinnerItemController(
    const std::string& arc_app_id,
    int event_flags,
    int64_t display_id)
    : ShelfSpinnerItemController(arc_app_id),
      event_flags_(event_flags),
      display_id_(display_id) {
  arc::ArcSessionManager* arc_session_manager = arc::ArcSessionManager::Get();
  // arc::ArcSessionManager might not be set in tests.
  if (arc_session_manager)
    arc_session_manager->AddObserver(this);
}

ArcShelfSpinnerItemController::~ArcShelfSpinnerItemController() {
  if (observed_profile_)
    ArcAppListPrefs::Get(observed_profile_)->RemoveObserver(this);
  arc::ArcSessionManager* arc_session_manager = arc::ArcSessionManager::Get();
  // arc::ArcSessionManager may be released first.
  if (arc_session_manager)
    arc_session_manager->RemoveObserver(this);
}

void ArcShelfSpinnerItemController::SetHost(
    const base::WeakPtr<ShelfSpinnerController>& controller) {
  DCHECK(!observed_profile_);
  observed_profile_ = controller->OwnerProfile();
  ArcAppListPrefs::Get(observed_profile_)->AddObserver(this);

  ShelfSpinnerItemController::SetHost(controller);
}

void ArcShelfSpinnerItemController::OnAppReadyChanged(
    const std::string& changed_app_id,
    bool ready) {
  if (!ready || app_id() != changed_app_id)
    return;

  // Close() destroys this object, so start launching the app first.
  arc::LaunchApp(observed_profile_, changed_app_id, event_flags_, display_id_);
  Close();
}

void ArcShelfSpinnerItemController::OnAppRemoved(
    const std::string& removed_app_id) {
  Close();
}

void ArcShelfSpinnerItemController::OnArcPlayStoreEnabledChanged(bool enabled) {
  if (enabled)
    return;
  // If ARC was disabled, remove the deferred launch request.
  Close();
}
