// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/supervised/tray_supervised_user.h"

#include <utility>

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/system/supervised/supervised_icon_string.h"
#include "ash/system/tray/label_tray_view.h"
#include "ash/system/tray/tray_constants.h"
#include "base/callback.h"
#include "base/logging.h"
#include "ui/gfx/paint_vector_icon.h"

namespace ash {

TraySupervisedUser::TraySupervisedUser(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_SUPERVISED_USER) {}

TraySupervisedUser::~TraySupervisedUser() = default;

views::View* TraySupervisedUser::CreateDefaultView(LoginStatus status) {
  if (!Shell::Get()->session_controller()->IsUserSupervised())
    return nullptr;

  LabelTrayView* tray_view =
      new LabelTrayView(nullptr, GetSupervisedUserIcon());
  // The message almost never changes during a session, so we compute it when
  // the menu is shown. We don't update it while the menu is open.
  tray_view->SetMessage(GetSupervisedUserMessage());
  return tray_view;
}

}  // namespace ash
