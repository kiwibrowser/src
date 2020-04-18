// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOCK_DEBUG_VIEW_H_
#define ASH_LOGIN_UI_LOCK_DEBUG_VIEW_H_

#include <memory>
#include <string>
#include <vector>

#include "ash/detachable_base/detachable_base_pairing_status.h"
#include "ash/login/login_screen_controller.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class MdTextButton;
}

namespace ash {

class LoginDataDispatcher;
class LockContentsView;

namespace mojom {
enum class TrayActionState;
}

// Contains the debug UI row (ie, add user, toggle PIN buttons).
class LockDebugView : public views::View, public views::ButtonListener {
 public:
  LockDebugView(mojom::TrayActionState initial_note_action_state,
                LoginDataDispatcher* data_dispatcher);
  ~LockDebugView() override;

  // views::View:
  void Layout() override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  LockContentsView* lock() { return lock_; }

 private:
  class DebugDataDispatcherTransformer;
  class DebugLoginDetachableBaseModel;

  // Rebuilds the debug user column which contains per-user actions.
  void RebuildDebugUserColumn();

  // Initializes the detachable base column - used for overriding detachable
  // base pairing state.
  void BuildDetachableBaseColumn();

  // Updates buttons provided in detachable base column, depending on detected
  // detachable base pairing state..
  void UpdateDetachableBaseColumn();

  // Creates a button on the debug row that cannot be focused.
  views::MdTextButton* AddButton(const std::string& text,
                                 bool add_to_debug_row = true);

  LockContentsView* lock_ = nullptr;

  // User column which contains per-user actions.
  views::View* per_user_action_column_ = nullptr;
  std::vector<views::View*> per_user_action_column_toggle_pin_;
  std::vector<views::View*> per_user_action_column_cycle_easy_unlock_state_;
  std::vector<views::View*> per_user_action_column_force_online_sign_in_;
  std::vector<views::View*> per_user_action_column_toggle_auth_enabled_;
  std::vector<views::View*> per_user_action_column_use_detachable_base_;

  // Debug row which contains buttons that affect the entire UI.
  views::View* debug_row_ = nullptr;
  views::MdTextButton* toggle_blur_ = nullptr;
  views::MdTextButton* toggle_note_action_ = nullptr;
  views::MdTextButton* toggle_caps_lock_ = nullptr;
  views::MdTextButton* add_dev_channel_info_ = nullptr;
  views::MdTextButton* add_user_ = nullptr;
  views::MdTextButton* remove_user_ = nullptr;
  views::MdTextButton* toggle_auth_ = nullptr;

  // Cloumn that contains buttons for debuggon detachable base state.
  views::View* detachable_base_column_ = nullptr;
  views::MdTextButton* toggle_debug_detachable_base_ = nullptr;
  views::MdTextButton* cycle_detachable_base_status_ = nullptr;
  views::MdTextButton* cycle_detachable_base_id_ = nullptr;

  // Debug dispatcher and cached data for the UI.
  std::unique_ptr<DebugDataDispatcherTransformer> const debug_data_dispatcher_;
  // Reference to the detachable base model passed to (and owned by) lock_.
  DebugLoginDetachableBaseModel* debug_detachable_base_model_ = nullptr;
  size_t num_users_ = 1u;
  size_t num_dev_channel_info_clicks_ = 0u;
  LoginScreenController::ForceFailAuth force_fail_auth_ =
      LoginScreenController::ForceFailAuth::kOff;

  DISALLOW_COPY_AND_ASSIGN(LockDebugView);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOCK_DEBUG_VIEW_H_
