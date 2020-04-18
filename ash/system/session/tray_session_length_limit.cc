// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/session/tray_session_length_limit.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/model/session_length_limit_model.h"
#include "ash/system/model/system_tray_model.h"
#include "ash/system/tray/label_tray_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/tray_constants.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/view.h"

namespace ash {

TraySessionLengthLimit::TraySessionLengthLimit(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_SESSION_LENGTH_LIMIT),
      model_(Shell::Get()->system_tray_model()->session_length_limit()) {
  model_->AddObserver(this);
  OnSessionLengthLimitUpdated();
}

TraySessionLengthLimit::~TraySessionLengthLimit() {
  model_->RemoveObserver(this);
}

// Add view to tray bubble.
views::View* TraySessionLengthLimit::CreateDefaultView(LoginStatus status) {
  CHECK(!tray_bubble_view_);
  if (model_->limit_state() == SessionLengthLimitModel::LIMIT_NONE)
    return nullptr;
  tray_bubble_view_ = new LabelTrayView(nullptr, kSystemMenuTimerIcon);
  tray_bubble_view_->SetMessage(ComposeTrayBubbleMessage());
  return tray_bubble_view_;
}

// View has been removed from tray bubble.
void TraySessionLengthLimit::OnDefaultViewDestroyed() {
  tray_bubble_view_ = nullptr;
}

void TraySessionLengthLimit::OnSessionLengthLimitUpdated() {
  // Don't show tray item until the user is logged in.
  if (!Shell::Get()->session_controller()->IsActiveUserSessionStarted())
    return;

  UpdateTrayBubbleView();
}

void TraySessionLengthLimit::UpdateTrayBubbleView() const {
  if (!tray_bubble_view_)
    return;
  if (model_->limit_state() == SessionLengthLimitModel::LIMIT_NONE)
    tray_bubble_view_->SetMessage(base::string16());
  else
    tray_bubble_view_->SetMessage(ComposeTrayBubbleMessage());
  tray_bubble_view_->Layout();
}

base::string16 TraySessionLengthLimit::ComposeTrayBubbleMessage() const {
  return l10n_util::GetStringFUTF16(
      IDS_ASH_STATUS_TRAY_BUBBLE_SESSION_LENGTH_LIMIT,
      ui::TimeFormat::Detailed(ui::TimeFormat::FORMAT_DURATION,
                               ui::TimeFormat::LENGTH_LONG, 10,
                               model_->remaining_session_time()));
}

}  // namespace ash
