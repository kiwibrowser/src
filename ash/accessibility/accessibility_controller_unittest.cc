// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accessibility/accessibility_controller.h"

#include <utility>

#include "ash/accessibility/accessibility_observer.h"
#include "ash/accessibility/test_accessibility_controller_client.h"
#include "ash/magnifier/docked_magnifier_controller.h"
#include "ash/public/cpp/ash_constants.h"
#include "ash/public/cpp/ash_features.h"
#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/config.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/sticky_keys/sticky_keys_controller.h"
#include "ash/test/ash_test_base.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "components/prefs/pref_service.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/message_center/message_center.h"

using message_center::MessageCenter;

namespace ash {

class TestAccessibilityObserver : public AccessibilityObserver {
 public:
  TestAccessibilityObserver() = default;
  ~TestAccessibilityObserver() override = default;

  // AccessibilityObserver:
  void OnAccessibilityStatusChanged() override { ++status_changed_count_; }

  int status_changed_count_ = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestAccessibilityObserver);
};

class AccessibilityControllerTest : public AshTestBase {
 public:
  AccessibilityControllerTest() = default;
  ~AccessibilityControllerTest() override = default;

  void SetUp() override {
    auto power_manager_client =
        std::make_unique<chromeos::FakePowerManagerClient>();
    power_manager_client_ = power_manager_client.get();
    chromeos::DBusThreadManager::GetSetterForTesting()->SetPowerManagerClient(
        std::move(power_manager_client));

    AshTestBase::SetUp();
  }
  void TearDown() override {
    AshTestBase::TearDown();
    chromeos::DBusThreadManager::Shutdown();
  }

 protected:
  // Owned by chromeos::DBusThreadManager.
  chromeos::FakePowerManagerClient* power_manager_client_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(AccessibilityControllerTest);
};

TEST_F(AccessibilityControllerTest, PrefsAreRegistered) {
  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityAutoclickEnabled));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityAutoclickDelayMs));
  EXPECT_TRUE(
      prefs->FindPreference(prefs::kAccessibilityCaretHighlightEnabled));
  EXPECT_TRUE(
      prefs->FindPreference(prefs::kAccessibilityCursorHighlightEnabled));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityDictationEnabled));
  EXPECT_TRUE(
      prefs->FindPreference(prefs::kAccessibilityFocusHighlightEnabled));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityHighContrastEnabled));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityLargeCursorEnabled));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityLargeCursorDipSize));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityMonoAudioEnabled));
  EXPECT_TRUE(
      prefs->FindPreference(prefs::kAccessibilityScreenMagnifierEnabled));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityScreenMagnifierScale));
  EXPECT_TRUE(
      prefs->FindPreference(prefs::kAccessibilitySpokenFeedbackEnabled));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityStickyKeysEnabled));
  EXPECT_TRUE(
      prefs->FindPreference(prefs::kAccessibilityVirtualKeyboardEnabled));
}

TEST_F(AccessibilityControllerTest, SetAutoclickEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsAutoclickEnabled());

  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);
  EXPECT_EQ(0, observer.status_changed_count_);

  controller->SetAutoclickEnabled(true);
  EXPECT_TRUE(controller->IsAutoclickEnabled());
  EXPECT_EQ(1, observer.status_changed_count_);

  controller->SetAutoclickEnabled(false);
  EXPECT_FALSE(controller->IsAutoclickEnabled());
  EXPECT_EQ(2, observer.status_changed_count_);

  controller->RemoveObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetCaretHighlightEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsCaretHighlightEnabled());

  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);
  EXPECT_EQ(0, observer.status_changed_count_);

  controller->SetCaretHighlightEnabled(true);
  EXPECT_TRUE(controller->IsCaretHighlightEnabled());
  EXPECT_EQ(1, observer.status_changed_count_);

  controller->SetCaretHighlightEnabled(false);
  EXPECT_FALSE(controller->IsCaretHighlightEnabled());
  EXPECT_EQ(2, observer.status_changed_count_);

  controller->RemoveObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetCursorHighlightEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsCursorHighlightEnabled());

  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);
  EXPECT_EQ(0, observer.status_changed_count_);

  controller->SetCursorHighlightEnabled(true);
  EXPECT_TRUE(controller->IsCursorHighlightEnabled());
  EXPECT_EQ(1, observer.status_changed_count_);

  controller->SetCursorHighlightEnabled(false);
  EXPECT_FALSE(controller->IsCursorHighlightEnabled());
  EXPECT_EQ(2, observer.status_changed_count_);

  controller->RemoveObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetFocusHighlightEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsFocusHighlightEnabled());

  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);
  EXPECT_EQ(0, observer.status_changed_count_);

  controller->SetFocusHighlightEnabled(true);
  EXPECT_TRUE(controller->IsFocusHighlightEnabled());
  EXPECT_EQ(1, observer.status_changed_count_);

  controller->SetFocusHighlightEnabled(false);
  EXPECT_FALSE(controller->IsFocusHighlightEnabled());
  EXPECT_EQ(2, observer.status_changed_count_);

  controller->RemoveObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetHighContrastEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsHighContrastEnabled());

  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);
  EXPECT_EQ(0, observer.status_changed_count_);

  controller->SetHighContrastEnabled(true);
  EXPECT_TRUE(controller->IsHighContrastEnabled());
  EXPECT_EQ(1, observer.status_changed_count_);

  controller->SetHighContrastEnabled(false);
  EXPECT_FALSE(controller->IsHighContrastEnabled());
  EXPECT_EQ(2, observer.status_changed_count_);

  controller->RemoveObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetLargeCursorEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsLargeCursorEnabled());

  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);
  EXPECT_EQ(0, observer.status_changed_count_);

  controller->SetLargeCursorEnabled(true);
  EXPECT_TRUE(controller->IsLargeCursorEnabled());
  EXPECT_EQ(1, observer.status_changed_count_);

  controller->SetLargeCursorEnabled(false);
  EXPECT_FALSE(controller->IsLargeCursorEnabled());
  EXPECT_EQ(2, observer.status_changed_count_);

  controller->RemoveObserver(&observer);
}

TEST_F(AccessibilityControllerTest, DisableLargeCursorResetsSize) {
  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  EXPECT_EQ(kDefaultLargeCursorSize,
            prefs->GetInteger(prefs::kAccessibilityLargeCursorDipSize));

  // Simulate using chrome settings webui to turn on large cursor and set a
  // custom size.
  prefs->SetBoolean(prefs::kAccessibilityLargeCursorEnabled, true);
  prefs->SetInteger(prefs::kAccessibilityLargeCursorDipSize, 48);

  // Turning off large cursor resets the size to the default.
  prefs->SetBoolean(prefs::kAccessibilityLargeCursorEnabled, false);
  EXPECT_EQ(kDefaultLargeCursorSize,
            prefs->GetInteger(prefs::kAccessibilityLargeCursorDipSize));
}

TEST_F(AccessibilityControllerTest, SetMonoAudioEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsMonoAudioEnabled());

  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);
  EXPECT_EQ(0, observer.status_changed_count_);

  controller->SetMonoAudioEnabled(true);
  EXPECT_TRUE(controller->IsMonoAudioEnabled());
  EXPECT_EQ(1, observer.status_changed_count_);

  controller->SetMonoAudioEnabled(false);
  EXPECT_FALSE(controller->IsMonoAudioEnabled());
  EXPECT_EQ(2, observer.status_changed_count_);

  controller->RemoveObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetSpokenFeedbackEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsSpokenFeedbackEnabled());

  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);
  EXPECT_EQ(0, observer.status_changed_count_);

  controller->SetSpokenFeedbackEnabled(true, A11Y_NOTIFICATION_SHOW);
  EXPECT_TRUE(controller->IsSpokenFeedbackEnabled());
  EXPECT_EQ(1, observer.status_changed_count_);

  controller->SetSpokenFeedbackEnabled(false, A11Y_NOTIFICATION_NONE);
  EXPECT_FALSE(controller->IsSpokenFeedbackEnabled());
  EXPECT_EQ(2, observer.status_changed_count_);

  controller->RemoveObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetStickyKeysEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsStickyKeysEnabled());

  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);
  EXPECT_EQ(0, observer.status_changed_count_);

  StickyKeysController* sticky_keys_controller =
      Shell::Get()->sticky_keys_controller();
  controller->SetStickyKeysEnabled(true);
  EXPECT_TRUE(sticky_keys_controller->enabled_for_test());
  EXPECT_TRUE(controller->IsStickyKeysEnabled());
  EXPECT_EQ(1, observer.status_changed_count_);

  controller->SetStickyKeysEnabled(false);
  EXPECT_FALSE(sticky_keys_controller->enabled_for_test());
  EXPECT_FALSE(controller->IsStickyKeysEnabled());
  EXPECT_EQ(2, observer.status_changed_count_);

  controller->RemoveObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetVirtualKeyboardEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsVirtualKeyboardEnabled());

  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);
  EXPECT_EQ(0, observer.status_changed_count_);

  controller->SetVirtualKeyboardEnabled(true);
  EXPECT_TRUE(keyboard::GetAccessibilityKeyboardEnabled());
  EXPECT_TRUE(controller->IsVirtualKeyboardEnabled());
  EXPECT_EQ(1, observer.status_changed_count_);

  controller->SetVirtualKeyboardEnabled(false);
  EXPECT_FALSE(keyboard::GetAccessibilityKeyboardEnabled());
  EXPECT_FALSE(controller->IsVirtualKeyboardEnabled());
  EXPECT_EQ(2, observer.status_changed_count_);

  controller->RemoveObserver(&observer);
}

// Tests that ash's controller gets shutdown sound duration properly from
// remote client.
TEST_F(AccessibilityControllerTest, GetShutdownSoundDuration) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  TestAccessibilityControllerClient client;
  controller->SetClient(client.CreateInterfacePtrAndBind());

  base::TimeDelta sound_duration;
  controller->PlayShutdownSound(base::BindOnce(
      [](base::TimeDelta* dst, base::TimeDelta duration) { *dst = duration; },
      base::Unretained(&sound_duration)));
  controller->FlushMojoForTest();
  EXPECT_EQ(TestAccessibilityControllerClient::kShutdownSoundDuration,
            sound_duration);
}

// Tests that ash's controller gets should toggle spoken feedback via touch
// properly from remote client.
TEST_F(AccessibilityControllerTest, GetShouldToggleSpokenFeedbackViaTouch) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  TestAccessibilityControllerClient client;
  controller->SetClient(client.CreateInterfacePtrAndBind());

  bool should_toggle = false;
  controller->ShouldToggleSpokenFeedbackViaTouch(base::BindOnce(
      [](bool* dst, bool should_toggle) { *dst = should_toggle; },
      base::Unretained(&should_toggle)));
  controller->FlushMojoForTest();
  // Expects true which is passed by |client|.
  EXPECT_TRUE(should_toggle);
}

TEST_F(AccessibilityControllerTest, SetDarkenScreen) {
  ASSERT_FALSE(power_manager_client_->backlights_forced_off());

  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  controller->SetDarkenScreen(true);
  EXPECT_TRUE(power_manager_client_->backlights_forced_off());

  controller->SetDarkenScreen(false);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
}

TEST_F(AccessibilityControllerTest, ShowNotificationOnSpokenFeedback) {
  const base::string16 kChromeVoxEnabledTitle =
      base::ASCIIToUTF16("ChromeVox enabled");
  const base::string16 kChromeVoxEnabled =
      base::ASCIIToUTF16("Press Ctrl + Alt + Z to disable spoken feedback.");
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();

  // Enabling spoken feedback should show the notification if specified to show
  // notification.
  controller->SetSpokenFeedbackEnabled(true, A11Y_NOTIFICATION_SHOW);
  message_center::NotificationList::Notifications notifications =
      MessageCenter::Get()->GetVisibleNotifications();
  ASSERT_EQ(1u, notifications.size());
  EXPECT_EQ(kChromeVoxEnabledTitle, (*notifications.begin())->title());
  EXPECT_EQ(kChromeVoxEnabled, (*notifications.begin())->message());

  // Disabling spoken feedback should not show any notification even if
  // specified to show notification.
  controller->SetSpokenFeedbackEnabled(false, A11Y_NOTIFICATION_SHOW);
  notifications = MessageCenter::Get()->GetVisibleNotifications();
  EXPECT_EQ(0u, notifications.size());

  // Enabling spoken feedback but not specified to show notification should not
  // show any notification, for example toggling on tray detailed menu.
  controller->SetSpokenFeedbackEnabled(true, A11Y_NOTIFICATION_NONE);
  notifications = MessageCenter::Get()->GetVisibleNotifications();
  EXPECT_EQ(0u, notifications.size());
}

TEST_F(AccessibilityControllerTest,
       ShowNotificationOnBrailleDisplayStateChanged) {
  const base::string16 kBrailleConnected =
      base::ASCIIToUTF16("Braille display connected.");
  const base::string16 kChromeVoxEnabled =
      base::ASCIIToUTF16("Press Ctrl + Alt + Z to disable spoken feedback.");
  const base::string16 kBrailleConnectedAndChromeVoxEnabledTitle =
      base::ASCIIToUTF16("Braille and ChromeVox are enabled");
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();

  controller->SetSpokenFeedbackEnabled(true, A11Y_NOTIFICATION_SHOW);
  EXPECT_TRUE(controller->IsSpokenFeedbackEnabled());
  // Connecting a braille display when spoken feedback is already enabled
  // should only show the message about the braille display.
  controller->BrailleDisplayStateChanged(true);
  message_center::NotificationList::Notifications notifications =
      MessageCenter::Get()->GetVisibleNotifications();
  ASSERT_EQ(1u, notifications.size());
  EXPECT_EQ(base::string16(), (*notifications.begin())->title());
  EXPECT_EQ(kBrailleConnected, (*notifications.begin())->message());

  // Neither disconnecting a braille display, nor disabling spoken feedback
  // should show any notification.
  controller->BrailleDisplayStateChanged(false);
  EXPECT_TRUE(controller->IsSpokenFeedbackEnabled());
  notifications = MessageCenter::Get()->GetVisibleNotifications();
  EXPECT_EQ(0u, notifications.size());
  controller->SetSpokenFeedbackEnabled(false, A11Y_NOTIFICATION_SHOW);
  notifications = MessageCenter::Get()->GetVisibleNotifications();
  EXPECT_EQ(0u, notifications.size());
  EXPECT_FALSE(controller->IsSpokenFeedbackEnabled());

  // Connecting a braille display should enable spoken feedback and show
  // both messages.
  controller->BrailleDisplayStateChanged(true);
  EXPECT_TRUE(controller->IsSpokenFeedbackEnabled());
  notifications = MessageCenter::Get()->GetVisibleNotifications();
  EXPECT_EQ(kBrailleConnectedAndChromeVoxEnabledTitle,
            (*notifications.begin())->title());
  EXPECT_EQ(kChromeVoxEnabled, (*notifications.begin())->message());
}

TEST_F(AccessibilityControllerTest, SelectToSpeakStateChanges) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  TestAccessibilityObserver observer;
  controller->AddObserver(&observer);

  controller->SetSelectToSpeakState(
      ash::mojom::SelectToSpeakState::kSelectToSpeakStateSelecting);
  EXPECT_EQ(controller->GetSelectToSpeakState(),
            ash::mojom::SelectToSpeakState::kSelectToSpeakStateSelecting);
  EXPECT_EQ(observer.status_changed_count_, 1);

  controller->SetSelectToSpeakState(
      ash::mojom::SelectToSpeakState::kSelectToSpeakStateSpeaking);
  EXPECT_EQ(controller->GetSelectToSpeakState(),
            ash::mojom::SelectToSpeakState::kSelectToSpeakStateSpeaking);
  EXPECT_EQ(observer.status_changed_count_, 2);
}

namespace {

enum class TestUserLoginType {
  kNewUser,
  kGuest,
  kExistingUser,
};

class AccessibilityControllerSigninTest
    : public NoSessionAshTestBase,
      public testing::WithParamInterface<TestUserLoginType> {
 public:
  AccessibilityControllerSigninTest() = default;
  ~AccessibilityControllerSigninTest() = default;

  // AshTestBase:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kDockedMagnifier);
    NoSessionAshTestBase::SetUp();
  }

  void SimulateLogin() {
    constexpr char kUserEmail[] = "user1@test.com";
    switch (GetParam()) {
      case TestUserLoginType::kNewUser:
        SimulateNewUserFirstLogin(kUserEmail);
        break;

      case TestUserLoginType::kGuest:
        SimulateGuestLogin();
        break;

      case TestUserLoginType::kExistingUser:
        SimulateUserLogin(kUserEmail);
        break;
    }
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityControllerSigninTest);
};

}  // namespace

INSTANTIATE_TEST_CASE_P(,
                        AccessibilityControllerSigninTest,
                        ::testing::Values(TestUserLoginType::kNewUser,
                                          TestUserLoginType::kGuest,
                                          TestUserLoginType::kExistingUser));

TEST_P(AccessibilityControllerSigninTest, EnableOnLoginScreenAndLogin) {
  constexpr float kMagnifierScale = 4.3f;

  AccessibilityController* accessibility =
      Shell::Get()->accessibility_controller();
  DockedMagnifierController* docked_magnifier =
      Shell::Get()->docked_magnifier_controller();

  SessionController* session = Shell::Get()->session_controller();
  EXPECT_EQ(session_manager::SessionState::LOGIN_PRIMARY,
            session->GetSessionState());
  EXPECT_FALSE(accessibility->IsLargeCursorEnabled());
  EXPECT_FALSE(accessibility->IsSpokenFeedbackEnabled());
  EXPECT_FALSE(accessibility->IsHighContrastEnabled());
  EXPECT_FALSE(accessibility->IsAutoclickEnabled());
  EXPECT_FALSE(accessibility->IsMonoAudioEnabled());
  EXPECT_FALSE(docked_magnifier->GetEnabled());
  using prefs::kAccessibilityLargeCursorEnabled;
  using prefs::kAccessibilitySpokenFeedbackEnabled;
  using prefs::kAccessibilityHighContrastEnabled;
  using prefs::kAccessibilityAutoclickEnabled;
  using prefs::kAccessibilityMonoAudioEnabled;
  using prefs::kDockedMagnifierEnabled;
  PrefService* signin_prefs = session->GetSigninScreenPrefService();
  EXPECT_FALSE(signin_prefs->GetBoolean(kAccessibilityLargeCursorEnabled));
  EXPECT_FALSE(signin_prefs->GetBoolean(kAccessibilitySpokenFeedbackEnabled));
  EXPECT_FALSE(signin_prefs->GetBoolean(kAccessibilityHighContrastEnabled));
  EXPECT_FALSE(signin_prefs->GetBoolean(kAccessibilityAutoclickEnabled));
  EXPECT_FALSE(signin_prefs->GetBoolean(kAccessibilityMonoAudioEnabled));
  EXPECT_FALSE(signin_prefs->GetBoolean(kDockedMagnifierEnabled));

  // Verify that toggling prefs at the signin screen changes the signin setting.
  accessibility->SetLargeCursorEnabled(true);
  accessibility->SetSpokenFeedbackEnabled(true, A11Y_NOTIFICATION_NONE);
  accessibility->SetHighContrastEnabled(true);
  accessibility->SetAutoclickEnabled(true);
  accessibility->SetMonoAudioEnabled(true);
  docked_magnifier->SetEnabled(true);
  docked_magnifier->SetScale(kMagnifierScale);
  // TODO(afakhry): Test the Fullscreen magnifier prefs once the
  // ash::MagnificationController handles all the prefs work itself inside ash
  // without needing magnification manager in Chrome.
  EXPECT_TRUE(accessibility->IsLargeCursorEnabled());
  EXPECT_TRUE(accessibility->IsSpokenFeedbackEnabled());
  EXPECT_TRUE(accessibility->IsHighContrastEnabled());
  EXPECT_TRUE(accessibility->IsAutoclickEnabled());
  EXPECT_TRUE(accessibility->IsMonoAudioEnabled());
  EXPECT_TRUE(docked_magnifier->GetEnabled());
  EXPECT_FLOAT_EQ(kMagnifierScale, docked_magnifier->GetScale());
  EXPECT_TRUE(signin_prefs->GetBoolean(kAccessibilityLargeCursorEnabled));
  EXPECT_TRUE(signin_prefs->GetBoolean(kAccessibilitySpokenFeedbackEnabled));
  EXPECT_TRUE(signin_prefs->GetBoolean(kAccessibilityHighContrastEnabled));
  EXPECT_TRUE(signin_prefs->GetBoolean(kAccessibilityAutoclickEnabled));
  EXPECT_TRUE(signin_prefs->GetBoolean(kAccessibilityMonoAudioEnabled));
  EXPECT_TRUE(signin_prefs->GetBoolean(kDockedMagnifierEnabled));

  SimulateLogin();

  // Verify that prefs values are copied if they should.
  PrefService* user_prefs = session->GetLastActiveUserPrefService();
  EXPECT_NE(signin_prefs, user_prefs);
  const bool should_signin_prefs_be_copied =
      GetParam() == TestUserLoginType::kNewUser ||
      GetParam() == TestUserLoginType::kGuest;
  if (should_signin_prefs_be_copied) {
    EXPECT_TRUE(accessibility->IsLargeCursorEnabled());
    EXPECT_TRUE(accessibility->IsSpokenFeedbackEnabled());
    EXPECT_TRUE(accessibility->IsHighContrastEnabled());
    EXPECT_TRUE(accessibility->IsAutoclickEnabled());
    EXPECT_TRUE(accessibility->IsMonoAudioEnabled());
    EXPECT_TRUE(docked_magnifier->GetEnabled());
    EXPECT_FLOAT_EQ(kMagnifierScale, docked_magnifier->GetScale());
    EXPECT_TRUE(user_prefs->GetBoolean(kAccessibilityLargeCursorEnabled));
    EXPECT_TRUE(user_prefs->GetBoolean(kAccessibilitySpokenFeedbackEnabled));
    EXPECT_TRUE(user_prefs->GetBoolean(kAccessibilityHighContrastEnabled));
    EXPECT_TRUE(user_prefs->GetBoolean(kAccessibilityAutoclickEnabled));
    EXPECT_TRUE(user_prefs->GetBoolean(kAccessibilityMonoAudioEnabled));
    EXPECT_TRUE(user_prefs->GetBoolean(kDockedMagnifierEnabled));
  } else {
    EXPECT_FALSE(accessibility->IsLargeCursorEnabled());
    EXPECT_FALSE(accessibility->IsSpokenFeedbackEnabled());
    EXPECT_FALSE(accessibility->IsHighContrastEnabled());
    EXPECT_FALSE(accessibility->IsAutoclickEnabled());
    EXPECT_FALSE(accessibility->IsMonoAudioEnabled());
    EXPECT_FALSE(docked_magnifier->GetEnabled());
    EXPECT_NE(kMagnifierScale, docked_magnifier->GetScale());
    EXPECT_FALSE(user_prefs->GetBoolean(kAccessibilityLargeCursorEnabled));
    EXPECT_FALSE(user_prefs->GetBoolean(kAccessibilitySpokenFeedbackEnabled));
    EXPECT_FALSE(user_prefs->GetBoolean(kAccessibilityHighContrastEnabled));
    EXPECT_FALSE(user_prefs->GetBoolean(kAccessibilityAutoclickEnabled));
    EXPECT_FALSE(user_prefs->GetBoolean(kAccessibilityMonoAudioEnabled));
    EXPECT_FALSE(user_prefs->GetBoolean(kDockedMagnifierEnabled));
  }
}

}  // namespace ash
