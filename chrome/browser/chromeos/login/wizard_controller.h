// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_WIZARD_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_WIZARD_CONTROLLER_H_

#include <map>
#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/chromeos/login/enrollment/auto_enrollment_controller.h"
#include "chrome/browser/chromeos/login/screen_manager.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/controller_pairing_screen.h"
#include "chrome/browser/chromeos/login/screens/eula_screen.h"
#include "chrome/browser/chromeos/login/screens/hid_detection_screen.h"
#include "chrome/browser/chromeos/login/screens/host_pairing_screen.h"
#include "chrome/browser/chromeos/login/screens/network_screen.h"
#include "chrome/browser/chromeos/login/screens/reset_screen.h"
#include "chrome/browser/chromeos/policy/enrollment_config.h"

class PrefService;

namespace pairing_chromeos {
class ControllerPairingController;
class HostPairingController;
class SharkConnectionListener;
}  // namespace pairing_chromeos

namespace chromeos {

class ErrorScreen;
struct Geoposition;
class LoginDisplayHost;
class LoginScreenContext;
class OobeUI;
class SimpleGeolocationProvider;
class TimeZoneProvider;
struct TimeZoneResponseData;

// Class that manages control flow between wizard screens. Wizard controller
// interacts with screen controllers to move the user between screens.
class WizardController : public BaseScreenDelegate,
                         public EulaScreen::Delegate,
                         public ControllerPairingScreen::Delegate,
                         public HostPairingScreen::Delegate,
                         public NetworkScreen::Delegate,
                         public HIDDetectionScreen::Delegate {
 public:
  WizardController(LoginDisplayHost* host, OobeUI* oobe_ui);
  ~WizardController() override;

  // Returns the default wizard controller if it has been created.
  static WizardController* default_controller() { return default_controller_; }

  // Whether to skip any screens that may normally be shown after login
  // (registration, Terms of Service, user image selection).
  static bool skip_post_login_screens() { return skip_post_login_screens_; }

  // Whether to skip any prompts that may be normally shown during enrollment.
  static bool skip_enrollment_prompts() { return skip_enrollment_prompts_; }

  // Sets delays to zero. MUST be used only for tests.
  static void SetZeroDelays();

  // If true zero delays have been enabled (for browser tests).
  static bool IsZeroDelayEnabled();

  // Checks whether screen show time should be tracked with UMA.
  static bool IsOOBEStepToTrack(OobeScreen screen_id);

  // Skips any screens that may normally be shown after login (registration,
  // Terms of Service, user image selection).
  static void SkipPostLoginScreensForTesting();

  // Skips any enrollment prompts that may be normally shown.
  static void SkipEnrollmentPromptsForTesting();

  // Returns true if OOBE is operating under the
  // Zero-Touch Hands-Off Enrollment Flow.
  static bool UsingHandsOffEnrollment();

  // Shows the first screen defined by |first_screen| or by default if the
  // parameter is empty.
  void Init(OobeScreen first_screen);

  // Advances to screen defined by |screen| and shows it.
  void AdvanceToScreen(OobeScreen screen);

  // Advances to login/update screen. Should be used in for testing only.
  void SkipToLoginForTesting(const LoginScreenContext& context);
  void SkipToUpdateForTesting();

  // Should be used for testing only.
  pairing_chromeos::SharkConnectionListener*
  GetSharkConnectionListenerForTesting();

  // Skip update, go straight to enrollment after EULA is accepted.
  void SkipUpdateEnrollAfterEula();

  // TODO(antrim) : temporary hack. Should be removed once screen system is
  // reworked at hackaton.
  void EnableUserImageScreenReturnToPreviousHack();

  // Returns a pointer to the current screen or nullptr if there's no such
  // screen.
  BaseScreen* current_screen() const { return current_screen_; }

  // Returns true if the current wizard instance has reached the login screen.
  bool login_screen_started() const { return login_screen_started_; }

  // Returns a given screen. Creates it lazily.
  BaseScreen* GetScreen(OobeScreen screen);

  // Returns the current ScreenManager instance.
  ScreenManager* screen_manager() { return screen_manager_.get(); }

  // Volume percent at which spoken feedback is still audible.
  static const int kMinAudibleOutputVolumePercent;

  // Allocate a given BaseScreen for the given |Screen|. Used by
  // |screen_manager_|.
  BaseScreen* CreateScreen(OobeScreen screen);

  // Set the current screen. For Test use only.
  void SetCurrentScreenForTesting(BaseScreen* screen);

 private:
  // Show specific screen.
  void ShowNetworkScreen();
  void ShowUserImageScreen();
  void ShowEulaScreen();
  void ShowEnrollmentScreen();
  void ShowDemoModeSetupScreen();
  void ShowResetScreen();
  void ShowKioskAutolaunchScreen();
  void ShowEnableDebuggingScreen();
  void ShowKioskEnableScreen();
  void ShowTermsOfServiceScreen();
  void ShowSyncConsentScreen();
  void ShowArcTermsOfServiceScreen();
  void ShowRecommendAppsScreen();
  void ShowWrongHWIDScreen();
  void ShowAutoEnrollmentCheckScreen();
  void ShowSupervisedUserCreationScreen();
  void ShowArcKioskSplashScreen();
  void ShowHIDDetectionScreen();
  void ShowControllerPairingScreen();
  void ShowHostPairingScreen();
  void ShowDeviceDisabledScreen();
  void ShowEncryptionMigrationScreen();
  void ShowVoiceInteractionValuePropScreen();
  void ShowWaitForContainerReadyScreen();
  void ShowUpdateRequiredScreen();

  // Shows images login screen.
  void ShowLoginScreen(const LoginScreenContext& context);

  // Exit handlers:
  void OnHIDDetectionCompleted();
  void OnNetworkConnected();
  void OnConnectionFailed();
  void OnUpdateCompleted();
  void OnEulaAccepted();
  void OnUpdateErrorCheckingForUpdate();
  void OnUpdateErrorUpdating(bool is_critical_update);
  void OnUserImageSelected();
  void OnUserImageSkipped();
  void OnEnrollmentDone();
  void OnDeviceModificationCanceled();
  void OnKioskAutolaunchCanceled();
  void OnKioskAutolaunchConfirmed();
  void OnKioskEnableCompleted();
  void OnWrongHWIDWarningSkipped();
  void OnTermsOfServiceDeclined();
  void OnTermsOfServiceAccepted();
  void OnArcTermsOfServiceSkipped();
  void OnArcTermsOfServiceAccepted();
  void OnRecommendAppsSkipped();
  void OnVoiceInteractionValuePropSkipped();
  void OnVoiceInteractionValuePropAccepted();
  void OnControllerPairingFinished();
  void OnAutoEnrollmentCheckCompleted();
  void OnDemoSetupClosed();
  void OnWaitForContainerReadyFinished();
  void OnOobeFlowFinished();

  // Callback invoked once it has been determined whether the device is disabled
  // or not.
  void OnDeviceDisabledChecked(bool device_disabled);

  // Callback function after setting MetricsReporting.
  void OnChangedMetricsReportingState(bool enabled);

  // Shows update screen and starts update process.
  void InitiateOOBEUpdate();
  void StartOOBEUpdate();

  // Actions that should be done right after EULA is accepted,
  // before update check.
  void PerformPostEulaActions();

  // Actions that should be done right after update stage is finished.
  void PerformOOBECompletedActions();

  // Overridden from BaseScreenDelegate:
  void OnExit(BaseScreen& screen,
              ScreenExitCode exit_code,
              const ::login::ScreenContext* context) override;
  void ShowCurrentScreen() override;
  ErrorScreen* GetErrorScreen() override;
  void ShowErrorScreen() override;
  void HideErrorScreen(BaseScreen* parent_screen) override;

  // Overridden from EulaScreen::Delegate:
  void SetUsageStatisticsReporting(bool val) override;
  bool GetUsageStatisticsReporting() const override;

  // Override from ControllerPairingScreen::Delegate:
  void SetHostNetwork() override;
  void SetHostConfiguration() override;

  // Override from HostPairingScreen::Delegate:
  void ConfigureHostRequested(bool accepted_eula,
                              const std::string& lang,
                              const std::string& timezone,
                              bool send_reports,
                              const std::string& keyboard_layout) override;
  void AddNetworkRequested(const std::string& onc_spec) override;
  void RebootHostRequested() override;

  // Override from NetworkScreen::Delegate:
  void OnEnableDebuggingScreenRequested() override;

  // Override from HIDDetectionScreen::Delegate
  void OnHIDScreenNecessityCheck(bool screen_needed) override;

  // Notification of a change in the state of an accessibility setting.
  void OnAccessibilityStatusChanged(
      const AccessibilityStatusEventDetails& details);

  // Switches from one screen to another.
  void SetCurrentScreen(BaseScreen* screen);

  // Switches from one screen to another with delay before showing. Calling
  // ShowCurrentScreen directly forces screen to be shown immediately.
  void SetCurrentScreenSmooth(BaseScreen* screen, bool use_smoothing);

  // Update the status area visibility for |screen|.
  void UpdateStatusAreaVisibilityForScreen(OobeScreen screen);

  // Changes whether to show the Material Design OOBE or not.
  void SetShowMdOobe(bool show);

  // Launched kiosk app configured for auto-launch.
  void AutoLaunchKioskApp();

  // Called when LocalState is initialized.
  void OnLocalStateInitialized(bool /* succeeded */);

  // Returns local state.
  PrefService* GetLocalState();

  static void set_local_state_for_testing(PrefService* local_state) {
    local_state_for_testing_ = local_state;
  }

  OobeScreen first_screen() const { return first_screen_; }

  // Called when network is UP.
  void StartTimezoneResolve();

  // Creates provider on demand.
  TimeZoneProvider* GetTimezoneProvider();

  // TimeZoneRequest::TimeZoneResponseCallback implementation.
  void OnTimezoneResolved(std::unique_ptr<TimeZoneResponseData> timezone,
                          bool server_error);

  // Called from SimpleGeolocationProvider when location is resolved.
  void OnLocationResolved(const Geoposition& position,
                          bool server_error,
                          const base::TimeDelta elapsed);

  // Returns true if callback has been installed.
  // Returns false if timezone has already been resolved.
  bool SetOnTimeZoneResolvedForTesting(const base::Closure& callback);

  // Returns true if kHostPairingOobe perf has been set. If it's set, launch the
  // pairing remora OOBE from the beginning no matter an eligible controller is
  // detected or not.
  bool IsRemoraPairingOobe() const;

  // Returns true if voice interaction value prop should be shown.
  bool ShouldShowVoiceInteractionValueProp() const;

  // Start voice interaction setup wizard in container
  void StartVoiceInteractionSetupWizard();

  // Starts listening for an incoming shark controller connection, if we are
  // running remora OOBE.
  void MaybeStartListeningForSharkConnection();

  // Called when a connection to controller has been established. Wizard
  // controller takes the ownership of |pairing_controller| after that call.
  void OnSharkConnected(std::unique_ptr<pairing_chromeos::HostPairingController>
                            pairing_controller);

  // Callback functions for AddNetworkRequested().
  void OnSetHostNetworkSuccessful();
  void OnSetHostNetworkFailed(
      const std::string& error_name,
      std::unique_ptr<base::DictionaryValue> error_data);

  // Start the enrollment screen using the config from
  // |prescribed_enrollment_config_|. If |force_interactive| is true,
  // the user will be presented with a manual enrollment screen requiring
  // Gaia credentials. If it is false, the screen may return after trying
  // attestation-based enrollment if appropriate.
  void StartEnrollmentScreen(bool force_interactive);

  // Returns auto enrollment controller (lazily initializes one if it doesn't
  // exist already).
  AutoEnrollmentController* GetAutoEnrollmentController();

  std::unique_ptr<AutoEnrollmentController> auto_enrollment_controller_;
  std::unique_ptr<ScreenManager> screen_manager_;

  // Whether to skip any screens that may normally be shown after login
  // (registration, Terms of Service, user image selection).
  static bool skip_post_login_screens_;

  static bool zero_delay_enabled_;

  static bool skip_enrollment_prompts_;

  // Screen that's currently active.
  BaseScreen* current_screen_ = nullptr;

  // Screen that was active before, or nullptr for login screen.
  BaseScreen* previous_screen_ = nullptr;

// True if running official BUILD.
#if defined(GOOGLE_CHROME_BUILD)
  bool is_official_build_ = true;
#else
  bool is_official_build_ = false;
#endif

  // True if full OOBE flow should be shown.
  bool is_out_of_box_ = false;

  // Value of the screen name that WizardController was started with.
  OobeScreen first_screen_;

  // OOBE/login display host.
  LoginDisplayHost* host_ = nullptr;

  // Default WizardController.
  static WizardController* default_controller_;

  base::OneShotTimer smooth_show_timer_;

  OobeUI* const oobe_ui_;

  // State of Usage stat/error reporting checkbox on EULA screen
  // during wizard lifetime.
  bool usage_statistics_reporting_ = true;

  // If true then update check is cancelled and enrollment is started after
  // EULA is accepted.
  bool skip_update_enroll_after_eula_ = false;

  // The prescribed enrollment configuration for the device.
  policy::EnrollmentConfig prescribed_enrollment_config_;

  // Whether the auto-enrollment check should be retried or the cached result
  // returned if present.
  bool retry_auto_enrollment_check_ = false;

  // Time when the EULA was accepted. Used to measure the duration from the EULA
  // acceptance until the Sign-In screen is displayed.
  base::Time time_eula_accepted_;

  // Time when OOBE was started. Used to measure the total time from boot to
  // user Sign-In completed.
  base::Time time_oobe_started_;

  // Whether OOBE has yet been marked as completed.
  bool oobe_marked_completed_ = false;

  bool login_screen_started_ = false;

  bool is_in_session_oobe_ = false;

  // Indicates that once image selection screen finishes we should return to
  // a previous screen instead of proceeding with usual flow.
  bool user_image_screen_return_to_previous_hack_ = false;

  // Non-owning pointer to local state used for testing.
  static PrefService* local_state_for_testing_;

  FRIEND_TEST_ALL_PREFIXES(EnrollmentScreenTest, TestCancel);
  FRIEND_TEST_ALL_PREFIXES(WizardControllerFlowTest, Accelerators);
  FRIEND_TEST_ALL_PREFIXES(WizardControllerDeviceStateTest,
                           ControlFlowNoForcedReEnrollmentOnFirstBoot);
  friend class WizardControllerBrokenLocalStateTest;
  friend class WizardControllerDeviceStateTest;
  friend class WizardControllerFlowTest;
  friend class WizardControllerOobeResumeTest;
  friend class WizardInProcessBrowserTest;

  std::unique_ptr<AccessibilityStatusSubscription> accessibility_subscription_;

  std::unique_ptr<SimpleGeolocationProvider> geolocation_provider_;
  std::unique_ptr<TimeZoneProvider> timezone_provider_;

  // Pairing controller for shark devices.
  std::unique_ptr<pairing_chromeos::ControllerPairingController>
      shark_controller_;

  // Pairing controller for remora devices.
  std::unique_ptr<pairing_chromeos::HostPairingController> remora_controller_;

  // Maps screen names to last time of their shows.
  std::map<std::string, base::Time> screen_show_times_;

  // Tests check result of timezone resolve.
  bool timezone_resolved_ = false;
  base::Closure on_timezone_resolved_for_testing_;

  // Listens for incoming connection from a shark controller if a regular (not
  // pairing) remora OOBE is active. If connection is established, wizard
  // conroller swithces to a pairing OOBE.
  std::unique_ptr<pairing_chromeos::SharkConnectionListener>
      shark_connection_listener_;

  BaseScreen* hid_screen_ = nullptr;

  base::WeakPtrFactory<WizardController> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WizardController);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_WIZARD_CONTROLLER_H_
