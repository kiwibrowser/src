// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/cast/tray_cast_test_api.h"

#include "ash/system/tray/system_tray.h"
#include "ui/views/view.h"

namespace ash {

TrayCastTestAPI::TrayCastTestAPI(TrayCast* tray_cast) : tray_cast_(tray_cast) {}

TrayCastTestAPI::~TrayCastTestAPI() = default;

bool TrayCastTestAPI::IsTrayInitialized() const {
  return tray_cast_->default_ != nullptr;
}

bool TrayCastTestAPI::IsTrayVisible() const {
  return IsViewDrawn(TrayCast::TRAY_VIEW);
}

bool TrayCastTestAPI::IsTrayCastViewVisible() const {
  return IsViewDrawn(TrayCast::CAST_VIEW);
}

bool TrayCastTestAPI::IsTraySelectViewVisible() const {
  return IsViewDrawn(TrayCast::SELECT_VIEW);
}

std::string TrayCastTestAPI::GetDisplayedCastId() const {
  return tray_cast_->GetDisplayedCastId();
}

void TrayCastTestAPI::StartCast(const std::string& receiver_id) {
  return tray_cast_->StartCastForTest(receiver_id);
}

void TrayCastTestAPI::StopCast() {
  return tray_cast_->StopCastForTest();
}

void TrayCastTestAPI::OnCastingSessionStartedOrStopped(bool is_casting) {
  tray_cast_->OnCastingSessionStartedOrStopped(is_casting);
}

bool TrayCastTestAPI::IsViewDrawn(TrayCast::ChildViewId id) const {
  const views::View* view = tray_cast_->GetDefaultView()->GetViewByID(id);
  return view != nullptr && view->IsDrawn();
}

}  // namespace ash
