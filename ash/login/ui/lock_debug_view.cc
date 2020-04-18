// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/lock_debug_view.h"

#include <algorithm>
#include <map>
#include <memory>
#include <utility>

#include "ash/detachable_base/detachable_base_pairing_status.h"
#include "ash/ime/ime_controller.h"
#include "ash/login/login_screen_controller.h"
#include "ash/login/ui/layout_util.h"
#include "ash/login/ui/lock_contents_view.h"
#include "ash/login/ui/lock_screen.h"
#include "ash/login/ui/login_data_dispatcher.h"
#include "ash/login/ui/login_detachable_base_model.h"
#include "ash/login/ui/non_accessible_view.h"
#include "ash/shell.h"
#include "base/optional.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/ime/chromeos/ime_keyboard.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace ash {
namespace {

constexpr const char* kDebugUserNames[] = {
    "Angelina Johnson", "Marcus Cohen", "Chris Wallace",
    "Debbie Craig",     "Stella Wong",  "Stephanie Wade",
};

constexpr const char* kDebugDetachableBases[] = {"Base A", "Base B", "Base C"};

constexpr const char kDebugOsVersion[] =
    "Chromium 64.0.3279.0 (Platform 10146.0.0 dev-channel peppy test)";
constexpr const char kDebugEnterpriseInfo[] = "Asset ID: 1111";
constexpr const char kDebugBluetoothName[] = "Bluetooth adapter";

// Additional state for a user that the debug UI needs to reference.
struct UserMetadata {
  explicit UserMetadata(const mojom::UserInfoPtr& user_info)
      : account_id(user_info->account_id) {}

  AccountId account_id;
  bool enable_pin = false;
  bool enable_click_to_unlock = false;
  bool enable_auth = true;
  mojom::EasyUnlockIconId easy_unlock_id = mojom::EasyUnlockIconId::NONE;

  views::View* view = nullptr;
};

std::string DetachableBasePairingStatusToString(
    DetachableBasePairingStatus pairing_status) {
  switch (pairing_status) {
    case DetachableBasePairingStatus::kNone:
      return "No device";
    case DetachableBasePairingStatus::kAuthenticated:
      return "Authenticated";
    case DetachableBasePairingStatus::kNotAuthenticated:
      return "Not authenticated";
    case DetachableBasePairingStatus::kInvalidDevice:
      return "Invalid device";
  }
  return "Unknown";
}

}  // namespace

// Applies a series of user-defined transformations to a |LoginDataDispatcher|
// instance; this is used for debugging and development. The debug overlay uses
// this class to change what data is exposed to the UI.
class LockDebugView::DebugDataDispatcherTransformer
    : public LoginDataDispatcher::Observer {
 public:
  DebugDataDispatcherTransformer(
      mojom::TrayActionState initial_lock_screen_note_state,
      LoginDataDispatcher* dispatcher)
      : root_dispatcher_(dispatcher),
        lock_screen_note_state_(initial_lock_screen_note_state) {
    root_dispatcher_->AddObserver(this);
  }
  ~DebugDataDispatcherTransformer() override {
    root_dispatcher_->RemoveObserver(this);
  }

  LoginDataDispatcher* debug_dispatcher() { return &debug_dispatcher_; }

  // Changes the number of displayed users to |count|.
  void SetUserCount(int count) {
    DCHECK(!root_users_.empty());

    count = std::max(count, 1);

    // Trim any extra debug users.
    if (debug_users_.size() > size_t{count})
      debug_users_.erase(debug_users_.begin() + count, debug_users_.end());

    // Build |users|, add any new users to |debug_users|.
    std::vector<mojom::LoginUserInfoPtr> users;
    for (size_t i = 0; i < size_t{count}; ++i) {
      const mojom::LoginUserInfoPtr& root_user =
          root_users_[i % root_users_.size()];
      users.push_back(root_user->Clone());
      if (i >= root_users_.size()) {
        users[i]->basic_user_info->account_id = AccountId::FromUserEmailGaiaId(
            users[i]->basic_user_info->account_id.GetUserEmail() +
                std::to_string(i),
            users[i]->basic_user_info->account_id.GetGaiaId() +
                std::to_string(i));
      }
      if (i >= debug_users_.size())
        debug_users_.push_back(UserMetadata(users[i]->basic_user_info));
    }

    // Set debug user names. Useful for the stub user, which does not have a
    // name set.
    for (size_t i = 0; i < users.size(); ++i)
      users[i]->basic_user_info->display_name =
          kDebugUserNames[i % arraysize(kDebugUserNames)];

    // User notification resets PIN state.
    for (UserMetadata& user : debug_users_)
      user.enable_pin = false;

    debug_dispatcher_.NotifyUsers(users);
  }

  const AccountId& GetAccountIdForUserIndex(size_t user_index) {
    DCHECK(user_index >= 0 && user_index < debug_users_.size());
    UserMetadata* debug_user = &debug_users_[user_index];
    return debug_user->account_id;
  }

  // Activates or deactivates PIN for the user at |user_index|.
  void TogglePinStateForUserIndex(size_t user_index) {
    DCHECK(user_index >= 0 && user_index < debug_users_.size());
    UserMetadata* debug_user = &debug_users_[user_index];
    debug_user->enable_pin = !debug_user->enable_pin;
    debug_dispatcher_.SetPinEnabledForUser(debug_user->account_id,
                                           debug_user->enable_pin);
  }

  // Enables click to auth for the user at |user_index|.
  void CycleEasyUnlockForUserIndex(size_t user_index) {
    DCHECK(user_index >= 0 && user_index < debug_users_.size());
    UserMetadata* debug_user = &debug_users_[user_index];

    // EasyUnlockIconId state transition.
    auto get_next_id = [](mojom::EasyUnlockIconId id) {
      switch (id) {
        case mojom::EasyUnlockIconId::NONE:
          return mojom::EasyUnlockIconId::SPINNER;
        case mojom::EasyUnlockIconId::SPINNER:
          return mojom::EasyUnlockIconId::LOCKED;
        case mojom::EasyUnlockIconId::LOCKED:
          return mojom::EasyUnlockIconId::LOCKED_TO_BE_ACTIVATED;
        case mojom::EasyUnlockIconId::LOCKED_TO_BE_ACTIVATED:
          return mojom::EasyUnlockIconId::LOCKED_WITH_PROXIMITY_HINT;
        case mojom::EasyUnlockIconId::LOCKED_WITH_PROXIMITY_HINT:
          return mojom::EasyUnlockIconId::HARDLOCKED;
        case mojom::EasyUnlockIconId::HARDLOCKED:
          return mojom::EasyUnlockIconId::UNLOCKED;
        case mojom::EasyUnlockIconId::UNLOCKED:
          return mojom::EasyUnlockIconId::NONE;
      }
      return mojom::EasyUnlockIconId::NONE;
    };
    debug_user->easy_unlock_id = get_next_id(debug_user->easy_unlock_id);

    // Enable/disable click to unlock.
    debug_user->enable_click_to_unlock =
        debug_user->easy_unlock_id == mojom::EasyUnlockIconId::UNLOCKED;

    // Prepare icon that we will show.
    auto icon = mojom::EasyUnlockIconOptions::New();
    icon->icon = debug_user->easy_unlock_id;
    if (icon->icon == mojom::EasyUnlockIconId::SPINNER) {
      icon->aria_label = base::ASCIIToUTF16("Icon is spinning");
    } else if (icon->icon == mojom::EasyUnlockIconId::LOCKED ||
               icon->icon == mojom::EasyUnlockIconId::LOCKED_TO_BE_ACTIVATED) {
      icon->autoshow_tooltip = true;
      icon->tooltip = base::ASCIIToUTF16(
          "This is a long message to trigger overflow. This should show up "
          "automatically. icon_id=" +
          std::to_string(static_cast<int>(icon->icon)));
    } else {
      icon->tooltip =
          base::ASCIIToUTF16("This should not show up automatically.");
    }

    // Show icon and enable/disable click to unlock.
    debug_dispatcher_.ShowEasyUnlockIcon(debug_user->account_id, icon);
    debug_dispatcher_.SetClickToUnlockEnabledForUser(
        debug_user->account_id, debug_user->enable_click_to_unlock);
  }

  // Force online sign-in for the user at |user_index|.
  void ForceOnlineSignInForUserIndex(size_t user_index) {
    DCHECK(user_index >= 0 && user_index < debug_users_.size());
    debug_dispatcher_.SetForceOnlineSignInForUser(
        debug_users_[user_index].account_id);
  }

  // Toggle the unlock allowed state for the user at |user_index|.
  void ToggleAuthEnabledForUserIndex(size_t user_index) {
    DCHECK(user_index >= 0 && user_index < debug_users_.size());
    UserMetadata& user = debug_users_[user_index];
    user.enable_auth = !user.enable_auth;
    debug_dispatcher_.SetAuthEnabledForUser(
        user.account_id, user.enable_auth,
        base::Time::Now() + base::TimeDelta::FromHours(user_index) +
            base::TimeDelta::FromHours(8));
  }

  void ToggleLockScreenNoteButton() {
    if (lock_screen_note_state_ == mojom::TrayActionState::kAvailable) {
      lock_screen_note_state_ = mojom::TrayActionState::kNotAvailable;
    } else {
      lock_screen_note_state_ = mojom::TrayActionState::kAvailable;
    }

    debug_dispatcher_.SetLockScreenNoteState(lock_screen_note_state_);
  }

  void AddLockScreenDevChannelInfo(const std::string& os_version,
                                   const std::string& enterprise_info,
                                   const std::string& bluetooth_name) {
    debug_dispatcher_.SetDevChannelInfo(os_version, enterprise_info,
                                        bluetooth_name);
  }

  // LoginDataDispatcher::Observer:
  void OnUsersChanged(
      const std::vector<mojom::LoginUserInfoPtr>& users) override {
    // Update root_users_ to new source data.
    root_users_.clear();
    for (auto& user : users)
      root_users_.push_back(user->Clone());

    // Rebuild debug users using new source data.
    SetUserCount(debug_users_.size());
  }
  void OnPinEnabledForUserChanged(const AccountId& user,
                                  bool enabled) override {
    // Forward notification only if the user is currently being shown.
    for (size_t i = 0u; i < debug_users_.size(); ++i) {
      if (debug_users_[i].account_id == user) {
        debug_users_[i].enable_pin = enabled;
        debug_dispatcher_.SetPinEnabledForUser(user, enabled);
        break;
      }
    }
  }
  void OnClickToUnlockEnabledForUserChanged(const AccountId& user,
                                            bool enabled) override {
    // Forward notification only if the user is currently being shown.
    for (size_t i = 0u; i < debug_users_.size(); ++i) {
      if (debug_users_[i].account_id == user) {
        debug_users_[i].enable_click_to_unlock = enabled;
        debug_dispatcher_.SetClickToUnlockEnabledForUser(user, enabled);
        break;
      }
    }
  }
  void OnLockScreenNoteStateChanged(mojom::TrayActionState state) override {
    lock_screen_note_state_ = state;
    debug_dispatcher_.SetLockScreenNoteState(state);
  }
  void OnShowEasyUnlockIcon(
      const AccountId& user,
      const mojom::EasyUnlockIconOptionsPtr& icon) override {
    debug_dispatcher_.ShowEasyUnlockIcon(user, icon);
  }
  void OnDetachableBasePairingStatusChanged(
      DetachableBasePairingStatus pairing_status) override {
    debug_dispatcher_.SetDetachableBasePairingStatus(pairing_status);
  }

  void OnPublicSessionKeyboardLayoutsChanged(
      const AccountId& account_id,
      const std::string& locale,
      const std::vector<mojom::InputMethodItemPtr>& keyboard_layouts) override {
    debug_dispatcher_.SetPublicSessionKeyboardLayouts(account_id, locale,
                                                      keyboard_layouts);
  }

 private:
  // The debug overlay UI takes ground-truth data from |root_dispatcher_|,
  // applies a series of transformations to it, and exposes it to the UI via
  // |debug_dispatcher_|.
  LoginDataDispatcher* root_dispatcher_;  // Unowned.
  LoginDataDispatcher debug_dispatcher_;

  // Original set of users from |root_dispatcher_|.
  std::vector<mojom::LoginUserInfoPtr> root_users_;

  // Metadata for users that the UI is displaying.
  std::vector<UserMetadata> debug_users_;

  // The current lock screen note action state.
  mojom::TrayActionState lock_screen_note_state_;

  DISALLOW_COPY_AND_ASSIGN(DebugDataDispatcherTransformer);
};

// In-memory wrapper around LoginDetachableBaseModel used by lock UI.
// It provides, methods to override the detachable base pairing state seen by
// the UI.
class LockDebugView::DebugLoginDetachableBaseModel
    : public LoginDetachableBaseModel {
 public:
  static constexpr int kNullBaseId = -1;

  explicit DebugLoginDetachableBaseModel(LoginDataDispatcher* data_dispatcher)
      : data_dispatcher_(data_dispatcher) {}
  ~DebugLoginDetachableBaseModel() override = default;

  bool debugging_pairing_state() const { return pairing_status_.has_value(); }

  // Calculates the pairing status to which the model should be changed when
  // button for cycling detachable base pairing statuses is clicked.
  DetachableBasePairingStatus NextPairingStatus() const {
    if (!pairing_status_.has_value())
      return DetachableBasePairingStatus::kNone;

    switch (*pairing_status_) {
      case DetachableBasePairingStatus::kNone:
        return DetachableBasePairingStatus::kAuthenticated;
      case DetachableBasePairingStatus::kAuthenticated:
        return DetachableBasePairingStatus::kNotAuthenticated;
      case DetachableBasePairingStatus::kNotAuthenticated:
        return DetachableBasePairingStatus::kInvalidDevice;
      case DetachableBasePairingStatus::kInvalidDevice:
        return DetachableBasePairingStatus::kNone;
    }

    return DetachableBasePairingStatus::kNone;
  }

  // Calculates the debugging detachable base ID that should become the paired
  // base in the model when the button for cycling paired bases is clicked.
  int NextBaseId() const {
    return (base_id_ + 1) % arraysize(kDebugDetachableBases);
  }

  // Gets the descripting text for currently paired base, if any.
  std::string BaseButtonText() const {
    if (base_id_ < 0)
      return "No base";
    return kDebugDetachableBases[base_id_];
  }

  // Sets the model's pairing state - base pairing status, and the currently
  // paired base ID. ID should be an index in |kDebugDetachableBases| array, and
  // it should be set if pairing status is kAuthenticated. The base ID is
  // ignored if pairing state is different than kAuthenticated.
  void SetPairingState(DetachableBasePairingStatus pairing_status,
                       int base_id) {
    pairing_status_ = pairing_status;
    if (pairing_status == DetachableBasePairingStatus::kAuthenticated) {
      CHECK_GE(base_id, 0);
      CHECK_LT(base_id, static_cast<int>(arraysize(kDebugDetachableBases)));
      base_id_ = base_id;
    } else {
      base_id_ = kNullBaseId;
    }

    data_dispatcher_->SetDetachableBasePairingStatus(pairing_status);
  }

  // Marks the paired base (as seen by the model) as the user's last used base.
  // No-op if the current pairing status is different than kAuthenticated.
  void SetBaseLastUsedForUser(const AccountId& account_id) {
    if (GetPairingStatus() != DetachableBasePairingStatus::kAuthenticated)
      return;
    DCHECK_GE(base_id_, 0);

    last_used_bases_[account_id] = base_id_;
    data_dispatcher_->SetDetachableBasePairingStatus(*pairing_status_);
  }

  // Clears all in-memory pairing state.
  void ClearDebugPairingState() {
    pairing_status_ = base::nullopt;
    base_id_ = kNullBaseId;
    last_used_bases_.clear();

    data_dispatcher_->SetDetachableBasePairingStatus(
        DetachableBasePairingStatus::kNone);
  }

  // LoginDetachableBaseModel:
  DetachableBasePairingStatus GetPairingStatus() override {
    if (!pairing_status_.has_value())
      return DetachableBasePairingStatus::kNone;
    return *pairing_status_;
  }
  bool PairedBaseMatchesLastUsedByUser(
      const mojom::UserInfo& user_info) override {
    if (GetPairingStatus() != DetachableBasePairingStatus::kAuthenticated)
      return false;

    if (last_used_bases_.count(user_info.account_id) == 0)
      return true;
    return last_used_bases_[user_info.account_id] == base_id_;
  }
  bool SetPairedBaseAsLastUsedByUser(
      const mojom::UserInfo& user_info) override {
    if (GetPairingStatus() != DetachableBasePairingStatus::kAuthenticated)
      return false;

    last_used_bases_[user_info.account_id] = base_id_;
    return true;
  }

 private:
  LoginDataDispatcher* data_dispatcher_;

  // In-memory detachable base pairing state.
  base::Optional<DetachableBasePairingStatus> pairing_status_;
  int base_id_ = kNullBaseId;
  // Maps user account to the last used detachable base ID (base ID being the
  // base's index in kDebugDetachableBases array).
  std::map<AccountId, int> last_used_bases_;

  DISALLOW_COPY_AND_ASSIGN(DebugLoginDetachableBaseModel);
};

LockDebugView::LockDebugView(mojom::TrayActionState initial_note_action_state,
                             LoginDataDispatcher* data_dispatcher)
    : debug_data_dispatcher_(std::make_unique<DebugDataDispatcherTransformer>(
          initial_note_action_state,
          data_dispatcher)) {
  SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal));

  auto debug_detachable_base_model =
      std::make_unique<DebugLoginDetachableBaseModel>(data_dispatcher);
  debug_detachable_base_model_ = debug_detachable_base_model.get();

  lock_ = new LockContentsView(initial_note_action_state,
                               debug_data_dispatcher_->debug_dispatcher(),
                               std::move(debug_detachable_base_model));
  AddChildView(lock_);

  debug_row_ = new NonAccessibleView();
  debug_row_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal));
  AddChildView(debug_row_);

  per_user_action_column_ = new NonAccessibleView();
  per_user_action_column_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  debug_row_->AddChildView(per_user_action_column_);

  auto* margin = new NonAccessibleView();
  margin->SetPreferredSize(gfx::Size(10, 10));
  debug_row_->AddChildView(margin);

  toggle_blur_ = AddButton("Blur");
  toggle_note_action_ = AddButton("Toggle note action");
  toggle_caps_lock_ = AddButton("Toggle caps lock");
  add_dev_channel_info_ = AddButton("Add dev channel info");
  add_user_ = AddButton("Add user");
  remove_user_ = AddButton("Remove user");
  toggle_auth_ = AddButton("Auth (allowed)");

  RebuildDebugUserColumn();
  BuildDetachableBaseColumn();
}

LockDebugView::~LockDebugView() {
  // Make sure debug_data_dispatcher_ lives longer than LockContentsView so
  // pointer debug_dispatcher_ is always valid for LockContentsView.
  RemoveChildView(lock_);
}

void LockDebugView::Layout() {
  views::View::Layout();
  lock_->SetBoundsRect(GetLocalBounds());
  debug_row_->SetPosition(gfx::Point());
  debug_row_->SizeToPreferredSize();
}

void LockDebugView::ButtonPressed(views::Button* sender,
                                  const ui::Event& event) {
  // Enable or disable wallpaper blur.
  if (sender == toggle_blur_) {
    LockScreen::Get()->ToggleBlurForDebug();
    return;
  }

  // Enable or disable note action.
  if (sender == toggle_note_action_) {
    debug_data_dispatcher_->ToggleLockScreenNoteButton();
    return;
  }

  // Enable or disable caps lock.
  if (sender == toggle_caps_lock_) {
    ImeController* ime_controller = Shell::Get()->ime_controller();
    ime_controller->SetCapsLockEnabled(!ime_controller->IsCapsLockEnabled());
    return;
  }

  // Iteratively adds more info to the dev channel labels to test 7 permutations
  // and then disables the button.
  if (sender == add_dev_channel_info_) {
    DCHECK_LT(num_dev_channel_info_clicks_, 7u);
    ++num_dev_channel_info_clicks_;
    if (num_dev_channel_info_clicks_ == 7u)
      add_dev_channel_info_->SetEnabled(false);

    std::string os_version =
        num_dev_channel_info_clicks_ / 4 ? kDebugOsVersion : "";
    std::string enterprise_info =
        (num_dev_channel_info_clicks_ % 4) / 2 ? kDebugEnterpriseInfo : "";
    std::string bluetooth_name =
        num_dev_channel_info_clicks_ % 2 ? kDebugBluetoothName : "";
    debug_data_dispatcher_->AddLockScreenDevChannelInfo(
        os_version, enterprise_info, bluetooth_name);
    return;
  }

  // Add or remove a user.
  if (sender == add_user_ || sender == remove_user_) {
    if (sender == add_user_)
      ++num_users_;
    else if (sender == remove_user_)
      --num_users_;
    if (num_users_ < 1u)
      num_users_ = 1u;
    debug_data_dispatcher_->SetUserCount(num_users_);
    RebuildDebugUserColumn();
    Layout();
    return;
  }

  // Enable/disable auth. This is useful for testing auth failure scenarios on
  // Linux Desktop builds, where the cryptohome dbus stub accepts all passwords
  // as valid.
  if (sender == toggle_auth_) {
    auto get_next_auth_state = [](LoginScreenController::ForceFailAuth auth) {
      switch (auth) {
        case LoginScreenController::ForceFailAuth::kOff:
          return LoginScreenController::ForceFailAuth::kImmediate;
        case LoginScreenController::ForceFailAuth::kImmediate:
          return LoginScreenController::ForceFailAuth::kDelayed;
        case LoginScreenController::ForceFailAuth::kDelayed:
          return LoginScreenController::ForceFailAuth::kOff;
      }
      NOTREACHED();
      return LoginScreenController::ForceFailAuth::kOff;
    };
    auto get_auth_label = [](LoginScreenController::ForceFailAuth auth) {
      switch (auth) {
        case LoginScreenController::ForceFailAuth::kOff:
          return "Auth (allowed)";
        case LoginScreenController::ForceFailAuth::kImmediate:
          return "Auth (immediate fail)";
        case LoginScreenController::ForceFailAuth::kDelayed:
          return "Auth (delayed fail)";
      }
      NOTREACHED();
      return "Auth (allowed)";
    };
    force_fail_auth_ = get_next_auth_state(force_fail_auth_);
    toggle_auth_->SetText(base::ASCIIToUTF16(get_auth_label(force_fail_auth_)));
    Shell::Get()
        ->login_screen_controller()
        ->set_force_fail_auth_for_debug_overlay(force_fail_auth_);
    return;
  }

  if (sender == toggle_debug_detachable_base_) {
    if (debug_detachable_base_model_->debugging_pairing_state()) {
      debug_detachable_base_model_->ClearDebugPairingState();
      // In authenticated state, per user column has a button to mark the
      // current base as last used for the user - ut should get removed when the
      // detachable base debugging gets disabled.
      RebuildDebugUserColumn();
    } else {
      debug_detachable_base_model_->SetPairingState(
          DetachableBasePairingStatus::kNone,
          DebugLoginDetachableBaseModel::kNullBaseId);
    }
    UpdateDetachableBaseColumn();
    Layout();
    return;
  }

  if (sender == cycle_detachable_base_status_) {
    debug_detachable_base_model_->SetPairingState(
        debug_detachable_base_model_->NextPairingStatus(),
        debug_detachable_base_model_->NextBaseId());
    RebuildDebugUserColumn();
    UpdateDetachableBaseColumn();
    Layout();
    return;
  }

  if (sender == cycle_detachable_base_id_) {
    debug_detachable_base_model_->SetPairingState(
        DetachableBasePairingStatus::kAuthenticated,
        debug_detachable_base_model_->NextBaseId());
    UpdateDetachableBaseColumn();
    Layout();
    return;
  }

  for (size_t i = 0u; i < per_user_action_column_use_detachable_base_.size();
       ++i) {
    if (per_user_action_column_use_detachable_base_[i] == sender) {
      debug_detachable_base_model_->SetBaseLastUsedForUser(
          debug_data_dispatcher_->GetAccountIdForUserIndex(i));
      return;
    }
  }

  // Enable or disable PIN.
  for (size_t i = 0u; i < per_user_action_column_toggle_pin_.size(); ++i) {
    if (per_user_action_column_toggle_pin_[i] == sender)
      debug_data_dispatcher_->TogglePinStateForUserIndex(i);
  }

  // Cycle easy unlock.
  for (size_t i = 0u;
       i < per_user_action_column_cycle_easy_unlock_state_.size(); ++i) {
    if (per_user_action_column_cycle_easy_unlock_state_[i] == sender)
      debug_data_dispatcher_->CycleEasyUnlockForUserIndex(i);
  }

  // Force online sign-in.
  for (size_t i = 0u; i < per_user_action_column_force_online_sign_in_.size();
       ++i) {
    if (per_user_action_column_force_online_sign_in_[i] == sender)
      debug_data_dispatcher_->ForceOnlineSignInForUserIndex(i);
  }

  // Enable or disable auth.
  for (size_t i = 0u; i < per_user_action_column_toggle_auth_enabled_.size();
       ++i) {
    if (per_user_action_column_toggle_auth_enabled_[i] == sender)
      debug_data_dispatcher_->ToggleAuthEnabledForUserIndex(i);
  }
}

void LockDebugView::RebuildDebugUserColumn() {
  per_user_action_column_->RemoveAllChildViews(true /*delete_children*/);
  per_user_action_column_toggle_pin_.clear();
  per_user_action_column_cycle_easy_unlock_state_.clear();
  per_user_action_column_force_online_sign_in_.clear();
  per_user_action_column_toggle_auth_enabled_.clear();
  per_user_action_column_use_detachable_base_.clear();

  for (size_t i = 0u; i < num_users_; ++i) {
    auto* row = new NonAccessibleView();
    row->SetLayoutManager(
        std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal));

    views::View* toggle_pin =
        AddButton("Toggle PIN", false /*add_to_debug_row*/);
    per_user_action_column_toggle_pin_.push_back(toggle_pin);
    row->AddChildView(toggle_pin);

    views::View* toggle_click_auth =
        AddButton("Cycle easy unlock", false /*add_to_debug_row*/);
    per_user_action_column_cycle_easy_unlock_state_.push_back(
        toggle_click_auth);
    row->AddChildView(toggle_click_auth);

    views::View* force_online_sign_in =
        AddButton("Force online sign-in", false /*add_to_debug_row*/);
    per_user_action_column_force_online_sign_in_.push_back(
        force_online_sign_in);
    row->AddChildView(force_online_sign_in);

    views::View* toggle_auth_enabled =
        AddButton("Toggle auth enabled", false /*add_to_debug_row*/);
    per_user_action_column_toggle_auth_enabled_.push_back(toggle_auth_enabled);
    row->AddChildView(toggle_auth_enabled);

    if (debug_detachable_base_model_->debugging_pairing_state() &&
        debug_detachable_base_model_->GetPairingStatus() ==
            DetachableBasePairingStatus::kAuthenticated) {
      views::View* use_detachable_base = AddButton("Set base used", false);
      per_user_action_column_use_detachable_base_.push_back(
          use_detachable_base);
      row->AddChildView(use_detachable_base);
    }

    per_user_action_column_->AddChildView(row);
  }
}

void LockDebugView::BuildDetachableBaseColumn() {
  detachable_base_column_ = new NonAccessibleView();
  detachable_base_column_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  debug_row_->AddChildView(detachable_base_column_);

  UpdateDetachableBaseColumn();
}

void LockDebugView::UpdateDetachableBaseColumn() {
  detachable_base_column_->RemoveAllChildViews(true /*delete_children*/);

  toggle_debug_detachable_base_ = AddButton("Detachable base debugging", false);
  detachable_base_column_->AddChildView(
      login_layout_util::WrapViewForPreferredSize(
          toggle_debug_detachable_base_));

  if (!debug_detachable_base_model_->debugging_pairing_state())
    return;

  const std::string kPairingStatusText =
      std::string("Pairing status : ") +
      DetachableBasePairingStatusToString(
          debug_detachable_base_model_->GetPairingStatus());
  cycle_detachable_base_status_ = AddButton(kPairingStatusText, false);

  detachable_base_column_->AddChildView(
      login_layout_util::WrapViewForPreferredSize(
          cycle_detachable_base_status_));

  cycle_detachable_base_id_ =
      AddButton(debug_detachable_base_model_->BaseButtonText(), false);
  bool base_authenticated = debug_detachable_base_model_->GetPairingStatus() ==
                            DetachableBasePairingStatus::kAuthenticated;
  cycle_detachable_base_id_->SetEnabled(base_authenticated);

  detachable_base_column_->AddChildView(
      login_layout_util::WrapViewForPreferredSize(cycle_detachable_base_id_));
}

views::MdTextButton* LockDebugView::AddButton(const std::string& text,
                                              bool add_to_debug_row) {
  // Creates a button with |text| that cannot be focused.
  auto* button = views::MdTextButton::Create(this, base::ASCIIToUTF16(text));
  button->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  if (add_to_debug_row)
    debug_row_->AddChildView(
        login_layout_util::WrapViewForPreferredSize(button));
  return button;
}

}  // namespace ash
