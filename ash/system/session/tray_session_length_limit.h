// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_SESSION_TRAY_SESSION_LENGTH_LIMIT_H_
#define ASH_SYSTEM_SESSION_TRAY_SESSION_LENGTH_LIMIT_H_

#include <memory>

#include "ash/system/model/session_length_limit_model.h"
#include "ash/system/tray/system_tray_item.h"
#include "base/macros.h"
#include "base/strings/string16.h"

namespace ash {

class LabelTrayView;

// Adds a countdown timer to the system tray if the session length is limited.
class ASH_EXPORT TraySessionLengthLimit
    : public SystemTrayItem,
      public SessionLengthLimitModel::Observer {
 public:
  explicit TraySessionLengthLimit(SystemTray* system_tray);
  ~TraySessionLengthLimit() override;

  // SystemTrayItem:
  views::View* CreateDefaultView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;

  // SessionLengthLimitModel::Observer:
  void OnSessionLengthLimitUpdated() override;

 private:
  friend class TraySessionLengthLimitTest;

  void UpdateTrayBubbleView() const;

  base::string16 ComposeTrayBubbleMessage() const;

  // Unowned.
  SessionLengthLimitModel* const model_;

  LabelTrayView* tray_bubble_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TraySessionLengthLimit);
};

}  // namespace ash

#endif  // ASH_SYSTEM_SESSION_TRAY_SESSION_LENGTH_LIMIT_H_
