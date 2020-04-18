// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accessibility/accessibility_controller.h"

#include <memory>
#include <utility>

#include "ash/accessibility/accessibility_highlight_controller.h"
#include "ash/accessibility/accessibility_observer.h"
#include "ash/accessibility/accessibility_panel_layout_manager.h"
#include "ash/autoclick/autoclick_controller.h"
#include "ash/components/autoclick/public/mojom/autoclick.mojom.h"
#include "ash/high_contrast/high_contrast_controller.h"
#include "ash/policy/policy_recommendation_restorer.h"
#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/config.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/session/session_observer.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/sticky_keys/sticky_keys_controller.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/power/backlights_forced_off_setter.h"
#include "ash/system/power/scoped_backlights_forced_off.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
#include "base/strings/string16.h"
#include "chromeos/audio/cras_audio_handler.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "mash/public/mojom/launchable.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/accessibility_manager.mojom.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/aura/window.h"
#include "ui/base/cursor/cursor_type.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notifier_id.h"

using session_manager::SessionState;

namespace ash {
namespace {

constexpr char kNotificationId[] = "chrome://settings/accessibility";
constexpr char kNotifierAccessibility[] = "ash.accessibility";

// TODO(warx): Signin screen has more controllable accessibility prefs. We may
// want to expand this to a complete list. If so, merge this with
// |kCopiedOnSigninAccessibilityPrefs|.
constexpr const char* const kA11yPrefsForRecommendedValueOnSignin[]{
    prefs::kAccessibilityLargeCursorEnabled,
    prefs::kAccessibilityHighContrastEnabled,
    prefs::kAccessibilityScreenMagnifierEnabled,
    prefs::kAccessibilitySpokenFeedbackEnabled,
    prefs::kAccessibilityVirtualKeyboardEnabled,
};

// List of accessibility prefs that are to be copied (if changed by the user) on
// signin screen profile to a newly created user profile or a guest session.
constexpr const char* const kCopiedOnSigninAccessibilityPrefs[]{
    prefs::kAccessibilityAutoclickDelayMs,
    prefs::kAccessibilityAutoclickEnabled,
    prefs::kAccessibilityCaretHighlightEnabled,
    prefs::kAccessibilityCursorHighlightEnabled,
    prefs::kAccessibilityDictationEnabled,
    prefs::kAccessibilityFocusHighlightEnabled,
    prefs::kAccessibilityHighContrastEnabled,
    prefs::kAccessibilityLargeCursorEnabled,
    prefs::kAccessibilityMonoAudioEnabled,
    prefs::kAccessibilityScreenMagnifierEnabled,
    prefs::kAccessibilityScreenMagnifierScale,
    prefs::kAccessibilitySelectToSpeakEnabled,
    prefs::kAccessibilitySpokenFeedbackEnabled,
    prefs::kAccessibilityStickyKeysEnabled,
    prefs::kAccessibilitySwitchAccessEnabled,
    prefs::kAccessibilityVirtualKeyboardEnabled,
    prefs::kDockedMagnifierEnabled,
    prefs::kDockedMagnifierScale,
};

// Returns true if |pref_service| is the one used for the signin screen.
bool IsSigninPrefService(PrefService* pref_service) {
  const PrefService* signin_pref_service =
      Shell::Get()->session_controller()->GetSigninScreenPrefService();
  DCHECK(signin_pref_service);
  return pref_service == signin_pref_service;
}

// Returns true if the current session is the guest session.
bool IsCurrentSessionGuest() {
  const base::Optional<user_manager::UserType> user_type =
      Shell::Get()->session_controller()->GetUserType();
  return user_type && *user_type == user_manager::USER_TYPE_GUEST;
}

bool IsUserFirstLogin() {
  return Shell::Get()->session_controller()->IsUserFirstLogin();
}

// The copying of any modified accessibility prefs on the signin prefs happens
// when the |previous_pref_service| is of the signin profile, and the
// |current_pref_service| is of a newly created profile first logged in, or if
// the current session is the guest session.
bool ShouldCopySigninPrefs(PrefService* previous_pref_service,
                           PrefService* current_pref_service) {
  DCHECK(previous_pref_service);
  if (IsUserFirstLogin() && IsSigninPrefService(previous_pref_service) &&
      !IsSigninPrefService(current_pref_service)) {
    // If the user set a pref value on the login screen and is now starting a
    // session with a new profile, copy the pref value to the profile.
    return true;
  }

  if (IsCurrentSessionGuest()) {
    // Guest sessions don't have their own prefs, so always copy.
    return true;
  }

  return false;
}

// On a user's first login into a device, any a11y features enabled/disabled
// by the user on the login screen are enabled/disabled in the user's profile.
// This function copies settings from the signin prefs into the user's prefs
// when it detects a login with a newly created profile.
void CopySigninPrefsIfNeeded(PrefService* previous_pref_service,
                             PrefService* current_pref_service) {
  DCHECK(current_pref_service);
  if (!ShouldCopySigninPrefs(previous_pref_service, current_pref_service))
    return;

  PrefService* signin_prefs =
      Shell::Get()->session_controller()->GetSigninScreenPrefService();
  DCHECK(signin_prefs);
  for (const auto* pref_path : kCopiedOnSigninAccessibilityPrefs) {
    const PrefService::Preference* pref =
        signin_prefs->FindPreference(pref_path);

    // Ignore if the pref has not been set by the user.
    if (!pref || !pref->IsUserControlled())
      continue;

    // Copy the pref value from the signin profile.
    const base::Value* value_on_login = pref->GetValue();
    current_pref_service->Set(pref_path, *value_on_login);
  }
}

// Used to indicate which accessibility notification should be shown.
enum class A11yNotificationType {
  // No accessibility notification.
  kNone,
  // Shown when spoken feedback is set enabled with A11Y_NOTIFICATION_SHOW.
  kSpokenFeedbackEnabled,
  // Shown when braille display is connected while spoken feedback is enabled.
  kBrailleDisplayConnected,
  // Shown when braille display is connected while spoken feedback is not
  // enabled yet. Note: in this case braille display connected would enable
  // spoken feeback.
  kSpokenFeedbackBrailleEnabled,
};

// Returns notification icon based on the A11yNotificationType.
const gfx::VectorIcon& GetNotificationIcon(A11yNotificationType type) {
  if (type == A11yNotificationType::kSpokenFeedbackBrailleEnabled)
    return kNotificationAccessibilityIcon;
  if (type == A11yNotificationType::kBrailleDisplayConnected)
    return kNotificationAccessibilityBrailleIcon;
  return kNotificationChromevoxIcon;
}

void ShowAccessibilityNotification(A11yNotificationType type) {
  message_center::MessageCenter* message_center =
      message_center::MessageCenter::Get();
  message_center->RemoveNotification(kNotificationId, false /* by_user */);

  if (type == A11yNotificationType::kNone)
    return;

  base::string16 text;
  base::string16 title;
  if (type == A11yNotificationType::kBrailleDisplayConnected) {
    text = l10n_util::GetStringUTF16(
        IDS_ASH_STATUS_TRAY_BRAILLE_DISPLAY_CONNECTED);
  } else {
    bool is_tablet = Shell::Get()
                         ->tablet_mode_controller()
                         ->IsTabletModeWindowManagerEnabled();

    title = l10n_util::GetStringUTF16(
        type == A11yNotificationType::kSpokenFeedbackBrailleEnabled
            ? IDS_ASH_STATUS_TRAY_SPOKEN_FEEDBACK_BRAILLE_ENABLED_TITLE
            : IDS_ASH_STATUS_TRAY_SPOKEN_FEEDBACK_ENABLED_TITLE);
    text = l10n_util::GetStringUTF16(
        is_tablet ? IDS_ASH_STATUS_TRAY_SPOKEN_FEEDBACK_ENABLED_TABLET
                  : IDS_ASH_STATUS_TRAY_SPOKEN_FEEDBACK_ENABLED);
  }
  message_center::RichNotificationData options;
  options.should_make_spoken_feedback_for_popup_updates = false;
  std::unique_ptr<message_center::Notification> notification =
      message_center::Notification::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, kNotificationId, title,
          text, gfx::Image(), base::string16(), GURL(),
          message_center::NotifierId(
              message_center::NotifierId::SYSTEM_COMPONENT,
              kNotifierAccessibility),
          options, nullptr, GetNotificationIcon(type),
          message_center::SystemNotificationWarningLevel::NORMAL);
  notification->set_priority(message_center::SYSTEM_PRIORITY);
  message_center->AddNotification(std::move(notification));
}

}  // namespace

AccessibilityController::AccessibilityController(
    service_manager::Connector* connector)
    : connector_(connector),
      autoclick_delay_(AutoclickController::GetDefaultAutoclickDelay()) {
  Shell::Get()->session_controller()->AddObserver(this);
}

AccessibilityController::~AccessibilityController() {
  Shell::Get()->session_controller()->RemoveObserver(this);
}

// static
void AccessibilityController::RegisterProfilePrefs(PrefRegistrySimple* registry,
                                                   bool for_test) {
  if (for_test) {
    // In tests there is no remote pref service. Make ash own the prefs.
    registry->RegisterBooleanPref(prefs::kAccessibilityAutoclickEnabled, false);
    registry->RegisterIntegerPref(
        prefs::kAccessibilityAutoclickDelayMs,
        static_cast<int>(
            AutoclickController::GetDefaultAutoclickDelay().InMilliseconds()));
    registry->RegisterBooleanPref(prefs::kAccessibilityCaretHighlightEnabled,
                                  false);
    registry->RegisterBooleanPref(prefs::kAccessibilityCursorHighlightEnabled,
                                  false);
    registry->RegisterBooleanPref(prefs::kAccessibilityDictationEnabled, false);
    registry->RegisterBooleanPref(prefs::kAccessibilityFocusHighlightEnabled,
                                  false);
    registry->RegisterBooleanPref(prefs::kAccessibilityHighContrastEnabled,
                                  false);
    registry->RegisterBooleanPref(prefs::kAccessibilityLargeCursorEnabled,
                                  false);
    registry->RegisterIntegerPref(prefs::kAccessibilityLargeCursorDipSize,
                                  kDefaultLargeCursorSize);
    registry->RegisterBooleanPref(prefs::kAccessibilityMonoAudioEnabled, false);
    registry->RegisterBooleanPref(prefs::kAccessibilityScreenMagnifierEnabled,
                                  false);
    registry->RegisterDoublePref(prefs::kAccessibilityScreenMagnifierScale,
                                 1.0);
    registry->RegisterBooleanPref(prefs::kAccessibilitySpokenFeedbackEnabled,
                                  false);
    registry->RegisterBooleanPref(prefs::kAccessibilitySelectToSpeakEnabled,
                                  false);
    registry->RegisterBooleanPref(prefs::kAccessibilityStickyKeysEnabled,
                                  false);
    registry->RegisterBooleanPref(prefs::kAccessibilityVirtualKeyboardEnabled,
                                  false);
    return;
  }

  // In production the prefs are owned by chrome.
  // TODO(jamescook): Move ownership to ash.
  registry->RegisterForeignPref(prefs::kAccessibilityAutoclickEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityAutoclickDelayMs);
  registry->RegisterForeignPref(prefs::kAccessibilityCaretHighlightEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityCursorHighlightEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityDictationEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityFocusHighlightEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityHighContrastEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityLargeCursorEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityLargeCursorDipSize);
  registry->RegisterForeignPref(prefs::kAccessibilityMonoAudioEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityScreenMagnifierEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityScreenMagnifierScale);
  registry->RegisterForeignPref(prefs::kAccessibilitySpokenFeedbackEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilitySelectToSpeakEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityStickyKeysEnabled);
  registry->RegisterForeignPref(prefs::kAccessibilityVirtualKeyboardEnabled);
}

void AccessibilityController::AddObserver(AccessibilityObserver* observer) {
  observers_.AddObserver(observer);
}

void AccessibilityController::RemoveObserver(AccessibilityObserver* observer) {
  observers_.RemoveObserver(observer);
}

void AccessibilityController::BindRequest(
    mojom::AccessibilityControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void AccessibilityController::SetAutoclickEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilityAutoclickEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsAutoclickEnabled() const {
  return autoclick_enabled_;
}

void AccessibilityController::SetCaretHighlightEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilityCaretHighlightEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsCaretHighlightEnabled() const {
  return caret_highlight_enabled_;
}

void AccessibilityController::SetCursorHighlightEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilityCursorHighlightEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsCursorHighlightEnabled() const {
  return cursor_highlight_enabled_;
}

void AccessibilityController::SetDictationEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kEnableExperimentalAccessibilityFeatures)) {
    return;
  }

  active_user_prefs_->SetBoolean(prefs::kAccessibilityDictationEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsDictationEnabled() const {
  return dictation_enabled_;
}

void AccessibilityController::SetFocusHighlightEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilityFocusHighlightEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsFocusHighlightEnabled() const {
  return focus_highlight_enabled_;
}

void AccessibilityController::SetHighContrastEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilityHighContrastEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsHighContrastEnabled() const {
  return high_contrast_enabled_;
}

void AccessibilityController::SetLargeCursorEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilityLargeCursorEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsLargeCursorEnabled() const {
  return large_cursor_enabled_;
}

void AccessibilityController::SetMonoAudioEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilityMonoAudioEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsMonoAudioEnabled() const {
  return mono_audio_enabled_;
}

void AccessibilityController::SetSpokenFeedbackEnabled(
    bool enabled,
    AccessibilityNotificationVisibility notify) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilitySpokenFeedbackEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();

  A11yNotificationType type = A11yNotificationType::kNone;
  if (enabled && notify == A11Y_NOTIFICATION_SHOW)
    type = A11yNotificationType::kSpokenFeedbackEnabled;
  ShowAccessibilityNotification(type);
}

bool AccessibilityController::IsSpokenFeedbackEnabled() const {
  return spoken_feedback_enabled_;
}

void AccessibilityController::SetSelectToSpeakEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilitySelectToSpeakEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsSelectToSpeakEnabled() const {
  return select_to_speak_enabled_;
}

void AccessibilityController::RequestSelectToSpeakStateChange() {
  client_->RequestSelectToSpeakStateChange();
}

void AccessibilityController::SetSelectToSpeakState(
    mojom::SelectToSpeakState state) {
  select_to_speak_state_ = state;
  NotifyAccessibilityStatusChanged();
}

mojom::SelectToSpeakState AccessibilityController::GetSelectToSpeakState()
    const {
  return select_to_speak_state_;
}

void AccessibilityController::SetStickyKeysEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilityStickyKeysEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsStickyKeysEnabled() const {
  return sticky_keys_enabled_;
}

void AccessibilityController::SetVirtualKeyboardEnabled(bool enabled) {
  if (!active_user_prefs_)
    return;
  active_user_prefs_->SetBoolean(prefs::kAccessibilityVirtualKeyboardEnabled,
                                 enabled);
  active_user_prefs_->CommitPendingWrite();
}

bool AccessibilityController::IsVirtualKeyboardEnabled() const {
  return virtual_keyboard_enabled_;
}

bool AccessibilityController::IsDictationActive() const {
  return dictation_active_;
}

void AccessibilityController::SetDictationActive(bool is_active) {
  dictation_active_ = is_active;
}

void AccessibilityController::TriggerAccessibilityAlert(
    mojom::AccessibilityAlert alert) {
  if (client_)
    client_->TriggerAccessibilityAlert(alert);
}

void AccessibilityController::PlayEarcon(int32_t sound_key) {
  if (client_)
    client_->PlayEarcon(sound_key);
}

void AccessibilityController::PlayShutdownSound(
    base::OnceCallback<void(base::TimeDelta)> callback) {
  if (client_)
    client_->PlayShutdownSound(std::move(callback));
}

void AccessibilityController::HandleAccessibilityGesture(
    ax::mojom::Gesture gesture) {
  if (client_)
    client_->HandleAccessibilityGesture(gesture);
}

void AccessibilityController::ToggleDictation() {
  // Do nothing if dictation is not enabled.
  if (!IsDictationEnabled())
    return;

  if (client_) {
    client_->ToggleDictation(base::BindOnce(
        [](AccessibilityController* self, bool is_active) {
          self->SetDictationActive(is_active);
          if (is_active)
            Shell::Get()->OnDictationStarted();
          else
            Shell::Get()->OnDictationEnded();
        },
        base::Unretained(this)));
  }
}

void AccessibilityController::SilenceSpokenFeedback() {
  if (client_)
    client_->SilenceSpokenFeedback();
}

void AccessibilityController::OnTwoFingerTouchStart() {
  if (client_)
    client_->OnTwoFingerTouchStart();
}

void AccessibilityController::OnTwoFingerTouchStop() {
  if (client_)
    client_->OnTwoFingerTouchStop();
}

void AccessibilityController::ShouldToggleSpokenFeedbackViaTouch(
    base::OnceCallback<void(bool)> callback) {
  if (client_)
    client_->ShouldToggleSpokenFeedbackViaTouch(std::move(callback));
}

void AccessibilityController::PlaySpokenFeedbackToggleCountdown(
    int tick_count) {
  if (client_)
    client_->PlaySpokenFeedbackToggleCountdown(tick_count);
}

void AccessibilityController::NotifyAccessibilityStatusChanged() {
  for (auto& observer : observers_)
    observer.OnAccessibilityStatusChanged();
}

void AccessibilityController::SetClient(
    mojom::AccessibilityControllerClientPtr client) {
  client_ = std::move(client);
}

void AccessibilityController::SetDarkenScreen(bool darken) {
  if (darken && !scoped_backlights_forced_off_) {
    scoped_backlights_forced_off_ =
        Shell::Get()->backlights_forced_off_setter()->ForceBacklightsOff();
  } else if (!darken && scoped_backlights_forced_off_) {
    scoped_backlights_forced_off_.reset();
  }
}

void AccessibilityController::BrailleDisplayStateChanged(bool connected) {
  A11yNotificationType type = A11yNotificationType::kNone;
  if (connected && spoken_feedback_enabled_)
    type = A11yNotificationType::kBrailleDisplayConnected;
  else if (connected && !spoken_feedback_enabled_)
    type = A11yNotificationType::kSpokenFeedbackBrailleEnabled;

  if (connected)
    SetSpokenFeedbackEnabled(true, A11Y_NOTIFICATION_NONE);
  NotifyAccessibilityStatusChanged();

  ShowAccessibilityNotification(type);
}

void AccessibilityController::SetFocusHighlightRect(
    const gfx::Rect& bounds_in_screen) {
  if (!accessibility_highlight_controller_)
    return;
  accessibility_highlight_controller_->SetFocusHighlightRect(bounds_in_screen);
}

void AccessibilityController::SetAccessibilityPanelFullscreen(bool fullscreen) {
  // The accessibility panel is only shown on the primary display.
  aura::Window* root = Shell::GetPrimaryRootWindow();
  aura::Window* container =
      Shell::GetContainer(root, kShellWindowId_AccessibilityPanelContainer);
  // TODO(jamescook): Avoid this cast by moving ash::AccessibilityObserver
  // ownership to this class and notifying it on ChromeVox fullscreen updates.
  AccessibilityPanelLayoutManager* layout =
      static_cast<AccessibilityPanelLayoutManager*>(
          container->layout_manager());
  layout->SetPanelFullscreen(fullscreen);
}

void AccessibilityController::OnSigninScreenPrefServiceInitialized(
    PrefService* prefs) {
  // Make |kA11yPrefsForRecommendedValueOnSignin| observing recommended values
  // on signin screen. See PolicyRecommendationRestorer.
  PolicyRecommendationRestorer* policy_recommendation_restorer =
      Shell::Get()->policy_recommendation_restorer();
  for (auto* const pref_name : kA11yPrefsForRecommendedValueOnSignin)
    policy_recommendation_restorer->ObservePref(pref_name);

  // Observe user settings. This must happen after PolicyRecommendationRestorer.
  ObservePrefs(prefs);
}

void AccessibilityController::OnActiveUserPrefServiceChanged(
    PrefService* prefs) {
  // This is guaranteed to be received after
  // OnSigninScreenPrefServiceInitialized() so only copy the signin prefs if
  // needed here.
  CopySigninPrefsIfNeeded(active_user_prefs_, prefs);
  ObservePrefs(prefs);
}

void AccessibilityController::FlushMojoForTest() {
  client_.FlushForTesting();
}

void AccessibilityController::ObservePrefs(PrefService* prefs) {
  DCHECK(prefs);

  active_user_prefs_ = prefs;

  // Watch for pref updates from webui settings and policy.
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(prefs);
  pref_change_registrar_->Add(
      prefs::kAccessibilityAutoclickEnabled,
      base::BindRepeating(&AccessibilityController::UpdateAutoclickFromPref,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityAutoclickDelayMs,
      base::BindRepeating(
          &AccessibilityController::UpdateAutoclickDelayFromPref,
          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityCaretHighlightEnabled,
      base::BindRepeating(
          &AccessibilityController::UpdateCaretHighlightFromPref,
          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityCursorHighlightEnabled,
      base::BindRepeating(
          &AccessibilityController::UpdateCursorHighlightFromPref,
          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityDictationEnabled,
      base::BindRepeating(&AccessibilityController::UpdateDictationFromPref,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityFocusHighlightEnabled,
      base::BindRepeating(
          &AccessibilityController::UpdateFocusHighlightFromPref,
          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityHighContrastEnabled,
      base::BindRepeating(&AccessibilityController::UpdateHighContrastFromPref,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityLargeCursorEnabled,
      base::BindRepeating(&AccessibilityController::UpdateLargeCursorFromPref,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityLargeCursorDipSize,
      base::BindRepeating(&AccessibilityController::UpdateLargeCursorFromPref,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityMonoAudioEnabled,
      base::BindRepeating(&AccessibilityController::UpdateMonoAudioFromPref,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilitySpokenFeedbackEnabled,
      base::BindRepeating(
          &AccessibilityController::UpdateSpokenFeedbackFromPref,
          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilitySelectToSpeakEnabled,
      base::BindRepeating(&AccessibilityController::UpdateSelectToSpeakFromPref,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityStickyKeysEnabled,
      base::BindRepeating(&AccessibilityController::UpdateStickyKeysFromPref,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kAccessibilityVirtualKeyboardEnabled,
      base::BindRepeating(
          &AccessibilityController::UpdateVirtualKeyboardFromPref,
          base::Unretained(this)));

  // Load current state.
  UpdateAutoclickFromPref();
  UpdateAutoclickDelayFromPref();
  UpdateCaretHighlightFromPref();
  UpdateCursorHighlightFromPref();
  UpdateFocusHighlightFromPref();
  UpdateHighContrastFromPref();
  UpdateLargeCursorFromPref();
  UpdateMonoAudioFromPref();
  UpdateSpokenFeedbackFromPref();
  UpdateSelectToSpeakFromPref();
  UpdateStickyKeysFromPref();
  UpdateVirtualKeyboardFromPref();
}

void AccessibilityController::UpdateAutoclickFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled =
      active_user_prefs_->GetBoolean(prefs::kAccessibilityAutoclickEnabled);

  if (autoclick_enabled_ == enabled)
    return;

  autoclick_enabled_ = enabled;

  NotifyAccessibilityStatusChanged();

  if (Shell::GetAshConfig() == Config::MASH) {
    if (!connector_)  // Null in tests.
      return;
    mash::mojom::LaunchablePtr launchable;
    connector_->BindInterface("autoclick_app", &launchable);
    launchable->Launch(mash::mojom::kWindow, mash::mojom::LaunchMode::DEFAULT);
    return;
  }

  Shell::Get()->autoclick_controller()->SetEnabled(enabled);
}

void AccessibilityController::UpdateAutoclickDelayFromPref() {
  DCHECK(active_user_prefs_);
  base::TimeDelta autoclick_delay = base::TimeDelta::FromMilliseconds(int64_t{
      active_user_prefs_->GetInteger(prefs::kAccessibilityAutoclickDelayMs)});

  if (autoclick_delay_ == autoclick_delay)
    return;
  autoclick_delay_ = autoclick_delay;

  if (Shell::GetAshConfig() == Config::MASH) {
    if (!connector_)  // Null in tests.
      return;
    autoclick::mojom::AutoclickControllerPtr autoclick_controller;
    connector_->BindInterface("autoclick_app", &autoclick_controller);
    autoclick_controller->SetAutoclickDelay(autoclick_delay_.InMilliseconds());
    return;
  }

  Shell::Get()->autoclick_controller()->SetAutoclickDelay(autoclick_delay_);
}

void AccessibilityController::UpdateCaretHighlightFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled = active_user_prefs_->GetBoolean(
      prefs::kAccessibilityCaretHighlightEnabled);

  if (caret_highlight_enabled_ == enabled)
    return;

  caret_highlight_enabled_ = enabled;

  NotifyAccessibilityStatusChanged();
  UpdateAccessibilityHighlightingFromPrefs();
}

void AccessibilityController::UpdateCursorHighlightFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled = active_user_prefs_->GetBoolean(
      prefs::kAccessibilityCursorHighlightEnabled);

  if (cursor_highlight_enabled_ == enabled)
    return;

  cursor_highlight_enabled_ = enabled;

  NotifyAccessibilityStatusChanged();
  UpdateAccessibilityHighlightingFromPrefs();
}

void AccessibilityController::UpdateDictationFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled =
      active_user_prefs_->GetBoolean(prefs::kAccessibilityDictationEnabled);

  if (dictation_enabled_ == enabled)
    return;

  dictation_enabled_ = enabled;

  NotifyAccessibilityStatusChanged();
}

void AccessibilityController::UpdateFocusHighlightFromPref() {
  DCHECK(active_user_prefs_);
  bool enabled = active_user_prefs_->GetBoolean(
      prefs::kAccessibilityFocusHighlightEnabled);

  // Focus highlighting can't be on when spoken feedback is on, because
  // ChromeVox does its own focus highlighting.
  if (spoken_feedback_enabled_)
    enabled = false;

  if (focus_highlight_enabled_ == enabled)
    return;

  focus_highlight_enabled_ = enabled;

  NotifyAccessibilityStatusChanged();
  UpdateAccessibilityHighlightingFromPrefs();
}

void AccessibilityController::UpdateHighContrastFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled =
      active_user_prefs_->GetBoolean(prefs::kAccessibilityHighContrastEnabled);

  if (high_contrast_enabled_ == enabled)
    return;

  high_contrast_enabled_ = enabled;

  NotifyAccessibilityStatusChanged();

  // Under mash the UI service (window server) handles high contrast mode.
  if (Shell::GetAshConfig() == Config::MASH) {
    if (!connector_)  // Null in tests.
      return;
    ui::mojom::AccessibilityManagerPtr accessibility_ptr;
    connector_->BindInterface(ui::mojom::kServiceName, &accessibility_ptr);
    accessibility_ptr->SetHighContrastMode(enabled);
    return;
  }

  // Under classic ash high contrast mode is handled internally.
  Shell::Get()->high_contrast_controller()->SetEnabled(enabled);
  Shell::Get()->UpdateCursorCompositingEnabled();
}

void AccessibilityController::UpdateLargeCursorFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled =
      active_user_prefs_->GetBoolean(prefs::kAccessibilityLargeCursorEnabled);
  // Reset large cursor size to the default size when large cursor is disabled.
  if (!enabled)
    active_user_prefs_->ClearPref(prefs::kAccessibilityLargeCursorDipSize);
  const int size =
      active_user_prefs_->GetInteger(prefs::kAccessibilityLargeCursorDipSize);

  if (large_cursor_enabled_ == enabled && large_cursor_size_in_dip_ == size)
    return;

  large_cursor_enabled_ = enabled;
  large_cursor_size_in_dip_ = size;

  NotifyAccessibilityStatusChanged();

  ShellPort::Get()->SetCursorSize(
      large_cursor_enabled_ ? ui::CursorSize::kLarge : ui::CursorSize::kNormal);
  Shell::Get()->SetLargeCursorSizeInDip(large_cursor_size_in_dip_);
  Shell::Get()->UpdateCursorCompositingEnabled();
}

void AccessibilityController::UpdateMonoAudioFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled =
      active_user_prefs_->GetBoolean(prefs::kAccessibilityMonoAudioEnabled);

  if (mono_audio_enabled_ == enabled)
    return;

  mono_audio_enabled_ = enabled;

  NotifyAccessibilityStatusChanged();
  chromeos::CrasAudioHandler::Get()->SetOutputMonoEnabled(enabled);
}

void AccessibilityController::UpdateSpokenFeedbackFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled = active_user_prefs_->GetBoolean(
      prefs::kAccessibilitySpokenFeedbackEnabled);

  if (spoken_feedback_enabled_ == enabled)
    return;

  spoken_feedback_enabled_ = enabled;

  NotifyAccessibilityStatusChanged();

  // TODO(warx): ChromeVox loading/unloading requires browser process started,
  // thus it is still handled on Chrome side.

  // ChromeVox focus highlighting overrides the other focus highlighting.
  UpdateFocusHighlightFromPref();
}

void AccessibilityController::UpdateAccessibilityHighlightingFromPrefs() {
  if (!caret_highlight_enabled_ && !cursor_highlight_enabled_ &&
      !focus_highlight_enabled_) {
    accessibility_highlight_controller_.reset();
    return;
  }

  if (!accessibility_highlight_controller_) {
    accessibility_highlight_controller_ =
        std::make_unique<AccessibilityHighlightController>();
  }

  accessibility_highlight_controller_->HighlightCaret(caret_highlight_enabled_);
  accessibility_highlight_controller_->HighlightCursor(
      cursor_highlight_enabled_);
  accessibility_highlight_controller_->HighlightFocus(focus_highlight_enabled_);
}

void AccessibilityController::UpdateSelectToSpeakFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled =
      active_user_prefs_->GetBoolean(prefs::kAccessibilitySelectToSpeakEnabled);

  if (select_to_speak_enabled_ == enabled)
    return;

  select_to_speak_enabled_ = enabled;
  select_to_speak_state_ =
      mojom::SelectToSpeakState::kSelectToSpeakStateInactive;

  NotifyAccessibilityStatusChanged();
}

void AccessibilityController::UpdateStickyKeysFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled =
      active_user_prefs_->GetBoolean(prefs::kAccessibilityStickyKeysEnabled);

  if (sticky_keys_enabled_ == enabled)
    return;

  sticky_keys_enabled_ = enabled;

  NotifyAccessibilityStatusChanged();

  Shell::Get()->sticky_keys_controller()->Enable(enabled);
}

void AccessibilityController::UpdateVirtualKeyboardFromPref() {
  DCHECK(active_user_prefs_);
  const bool enabled = active_user_prefs_->GetBoolean(
      prefs::kAccessibilityVirtualKeyboardEnabled);

  if (virtual_keyboard_enabled_ == enabled)
    return;

  virtual_keyboard_enabled_ = enabled;

  NotifyAccessibilityStatusChanged();

  keyboard::SetAccessibilityKeyboardEnabled(enabled);

  if (Shell::GetAshConfig() == Config::MASH) {
    // TODO(mash): Support on-screen keyboard. See https://crbug.com/646565.
    NOTIMPLEMENTED();
    return;
  }

  // Note that there are two versions of the on-screen keyboard. A full layout
  // is provided for accessibility, which includes sticky modifier keys to
  // enable typing of hotkeys. A compact version is used in tablet mode to
  // provide a layout with larger keys to facilitate touch typing. In the event
  // that the a11y keyboard is being disabled, an on-screen keyboard might still
  // be enabled and a forced reset is required to pick up the layout change.
  if (keyboard::IsKeyboardEnabled())
    Shell::Get()->CreateKeyboard();
  else
    Shell::Get()->DestroyKeyboard();
}

}  // namespace ash
