// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/wizard_controller.h"

#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/json/json_string_value_serializer.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/voice_interaction/arc_voice_interaction_framework_service.h"
#include "chrome/browser/chromeos/customization/customization_document.h"
#include "chrome/browser/chromeos/login/enrollment/auto_enrollment_check_screen.h"
#include "chrome/browser/chromeos/login/enrollment/enrollment_screen.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/chromeos/login/hwid_checker.h"
#include "chrome/browser/chromeos/login/screens/arc_terms_of_service_screen.h"
#include "chrome/browser/chromeos/login/screens/demo_setup_screen.h"
#include "chrome/browser/chromeos/login/screens/device_disabled_screen.h"
#include "chrome/browser/chromeos/login/screens/enable_debugging_screen.h"
#include "chrome/browser/chromeos/login/screens/encryption_migration_screen.h"
#include "chrome/browser/chromeos/login/screens/error_screen.h"
#include "chrome/browser/chromeos/login/screens/eula_screen.h"
#include "chrome/browser/chromeos/login/screens/hid_detection_view.h"
#include "chrome/browser/chromeos/login/screens/kiosk_autolaunch_screen.h"
#include "chrome/browser/chromeos/login/screens/kiosk_enable_screen.h"
#include "chrome/browser/chromeos/login/screens/network_error.h"
#include "chrome/browser/chromeos/login/screens/network_view.h"
#include "chrome/browser/chromeos/login/screens/recommend_apps_screen.h"
#include "chrome/browser/chromeos/login/screens/reset_screen.h"
#include "chrome/browser/chromeos/login/screens/sync_consent_screen.h"
#include "chrome/browser/chromeos/login/screens/terms_of_service_screen.h"
#include "chrome/browser/chromeos/login/screens/update_required_screen.h"
#include "chrome/browser/chromeos/login/screens/update_screen.h"
#include "chrome/browser/chromeos/login/screens/user_image_screen.h"
#include "chrome/browser/chromeos/login/screens/voice_interaction_value_prop_screen.h"
#include "chrome/browser/chromeos/login/screens/wait_for_container_ready_screen.h"
#include "chrome/browser/chromeos/login/screens/wrong_hwid_screen.h"
#include "chrome/browser/chromeos/login/session/user_session_manager.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_creation_screen.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/net/delay_network_call.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/device_cloud_policy_manager_chromeos.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/system/device_disabling_manager.h"
#include "chrome/browser/chromeos/system/timezone_resolver_manager.h"
#include "chrome/browser/chromeos/system/timezone_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "chrome/browser/ui/ash/tablet_mode_client.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chrome/browser/ui/webui/help/help_utils_chromeos.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "chromeos/audio/cras_audio_handler.h"
#include "chromeos/chromeos_constants.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "chromeos/geolocation/simple_geolocation_provider.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/portal_detector/network_portal_detector.h"
#include "chromeos/settings/cros_settings_names.h"
#include "chromeos/settings/cros_settings_provider.h"
#include "chromeos/settings/timezone_settings.h"
#include "chromeos/timezone/timezone_provider.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_prefs.h"
#include "components/arc/arc_util.h"
#include "components/crash/content/app/breakpad_linux.h"
#include "components/pairing/bluetooth_controller_pairing_controller.h"
#include "components/pairing/bluetooth_host_pairing_controller.h"
#include "components/pairing/shark_connection_listener.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/session_manager/core/session_manager.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_types.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/accelerators/accelerator.h"

using content::BrowserThread;

namespace {
// Interval in ms which is used for smooth screen showing.
static int kShowDelayMs = 400;

// Total timezone resolving process timeout.
const unsigned int kResolveTimeZoneTimeoutSeconds = 60;

// Stores the list of all screens that should be shown when resuming OOBE.
const chromeos::OobeScreen kResumableScreens[] = {
    chromeos::OobeScreen::SCREEN_OOBE_NETWORK,
    chromeos::OobeScreen::SCREEN_OOBE_UPDATE,
    chromeos::OobeScreen::SCREEN_OOBE_EULA,
    chromeos::OobeScreen::SCREEN_OOBE_ENROLLMENT,
    chromeos::OobeScreen::SCREEN_TERMS_OF_SERVICE,
    chromeos::OobeScreen::SCREEN_SYNC_CONSENT,
    chromeos::OobeScreen::SCREEN_ARC_TERMS_OF_SERVICE,
    chromeos::OobeScreen::SCREEN_AUTO_ENROLLMENT_CHECK,
    chromeos::OobeScreen::SCREEN_RECOMMEND_APPS};

// Checks if device is in tablet mode, and that HID-detection screen is not
// disabled by flag.
bool CanShowHIDDetectionScreen() {
  return !TabletModeClient::Get()->tablet_mode_enabled() &&
         !base::CommandLine::ForCurrentProcess()->HasSwitch(
             chromeos::switches::kDisableHIDDetectionOnOOBE);
}

bool IsResumableScreen(chromeos::OobeScreen screen) {
  for (size_t i = 0; i < arraysize(kResumableScreens); ++i) {
    if (screen == kResumableScreens[i])
      return true;
  }
  return false;
}

struct Entry {
  chromeos::OobeScreen screen;
  const char* uma_name;
};

// Some screens had multiple different names in the past (they have since been
// unified). We need to always use the same name for UMA stats, though.
constexpr const Entry kLegacyUmaOobeScreenNames[] = {
    {chromeos::OobeScreen::SCREEN_ARC_TERMS_OF_SERVICE, "arc_tos"},
    {chromeos::OobeScreen::SCREEN_OOBE_ENROLLMENT, "enroll"},
    {chromeos::OobeScreen::SCREEN_OOBE_NETWORK, "network"},
    {chromeos::OobeScreen::SCREEN_CREATE_SUPERVISED_USER_FLOW,
     "supervised-user-creation-flow"},
    {chromeos::OobeScreen::SCREEN_TERMS_OF_SERVICE, "tos"},
    {chromeos::OobeScreen::SCREEN_USER_IMAGE_PICKER, "image"}};

void RecordUMAHistogramForOOBEStepCompletionTime(chromeos::OobeScreen screen,
                                                 base::TimeDelta step_time) {
  // Fetch screen name; make sure to use initial UMA name if the name has
  // changed.
  std::string screen_name = chromeos::GetOobeScreenName(screen);
  for (const auto& entry : kLegacyUmaOobeScreenNames) {
    if (entry.screen == screen) {
      screen_name = entry.uma_name;
      break;
    }
  }

  screen_name[0] = std::toupper(screen_name[0]);
  std::string histogram_name = "OOBE.StepCompletionTime." + screen_name;
  // Equivalent to using UMA_HISTOGRAM_MEDIUM_TIMES. UMA_HISTOGRAM_MEDIUM_TIMES
  // can not be used here, because |histogram_name| is calculated dynamically
  // and changes from call to call.
  base::HistogramBase* histogram = base::Histogram::FactoryTimeGet(
      histogram_name, base::TimeDelta::FromMilliseconds(10),
      base::TimeDelta::FromMinutes(3), 50,
      base::HistogramBase::kUmaTargetedHistogramFlag);
  histogram->AddTime(step_time);
}

bool IsRemoraRequisition() {
  policy::DeviceCloudPolicyManagerChromeOS* policy_manager =
      g_browser_process->platform_part()
          ->browser_policy_connector_chromeos()
          ->GetDeviceCloudPolicyManager();
  return policy_manager && policy_manager->IsRemoraRequisition();
}

bool IsSharkRequisition() {
  policy::DeviceCloudPolicyManagerChromeOS* policy_manager =
      g_browser_process->platform_part()
          ->browser_policy_connector_chromeos()
          ->GetDeviceCloudPolicyManager();
  return policy_manager && policy_manager->IsSharkRequisition();
}

// Checks if a controller device ("Master") is detected during the bootstrapping
// or shark/remora setup process.
bool IsControllerDetected() {
  return g_browser_process->local_state()->GetBoolean(
      prefs::kOobeControllerDetected);
}

void SetControllerDetectedPref(bool value) {
  PrefService* prefs = g_browser_process->local_state();
  prefs->SetBoolean(prefs::kOobeControllerDetected, value);
  prefs->CommitPendingWrite();
}

// Checks if the device is a "slave" device in the bootstrapping process.
bool IsBootstrappingSlave() {
  return g_browser_process->local_state()->GetBoolean(
      prefs::kIsBootstrappingSlave);
}

// Checks if the device is a "Master" device in the bootstrapping process.
bool IsBootstrappingMaster() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      chromeos::switches::kOobeBootstrappingMaster);
}

bool NetworkAllowUpdate(const chromeos::NetworkState* network) {
  if (!network || !network->IsConnectedState())
    return false;
  if (network->type() == shill::kTypeBluetooth ||
      (network->type() == shill::kTypeCellular &&
       !help_utils_chromeos::IsUpdateOverCellularAllowed(
           false /* interactive */))) {
    return false;
  }
  return true;
}

}  // namespace

namespace chromeos {

// static
const int WizardController::kMinAudibleOutputVolumePercent = 10;

// Initialize default controller.
// static
WizardController* WizardController::default_controller_ = nullptr;

// static
bool WizardController::skip_post_login_screens_ = false;

// static
bool WizardController::skip_enrollment_prompts_ = false;

// static
bool WizardController::zero_delay_enabled_ = false;

///////////////////////////////////////////////////////////////////////////////
// WizardController, public:

PrefService* WizardController::local_state_for_testing_ = nullptr;

WizardController::WizardController(LoginDisplayHost* host, OobeUI* oobe_ui)
    : host_(host), oobe_ui_(oobe_ui), weak_factory_(this) {
  DCHECK(default_controller_ == nullptr);
  default_controller_ = this;
  screen_manager_ = std::make_unique<ScreenManager>(this);
  // In session OOBE was initiated from voice interaction keyboard shortcuts.
  is_in_session_oobe_ =
      session_manager::SessionManager::Get()->IsSessionStarted();
  if (!ash_util::IsRunningInMash()) {
    AccessibilityManager* accessibility_manager = AccessibilityManager::Get();
    if (accessibility_manager) {
      // accessibility_manager could be null in Tests.
      accessibility_subscription_ = accessibility_manager->RegisterCallback(
          base::Bind(&WizardController::OnAccessibilityStatusChanged,
                     base::Unretained(this)));
    }
  } else {
    NOTIMPLEMENTED();
  }
}

WizardController::~WizardController() {
  screen_manager_.reset();
  // |remora_controller| has to be reset after |screen_manager_| is reset.
  remora_controller_.reset();
  if (shark_connection_listener_.get()) {
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(
        FROM_HERE, shark_connection_listener_.release());
  }
  if (default_controller_ == this) {
    default_controller_ = nullptr;
  } else {
    NOTREACHED() << "More than one controller are alive.";
  }
}

void WizardController::Init(OobeScreen first_screen) {
  VLOG(1) << "Starting OOBE wizard with screen: "
          << GetOobeScreenName(first_screen);
  first_screen_ = first_screen;

  bool oobe_complete = StartupUtils::IsOobeCompleted();
  if (!oobe_complete || first_screen == OobeScreen::SCREEN_SPECIAL_OOBE)
    is_out_of_box_ = true;

  // This is a hacky way to check for local state corruption, because
  // it depends on the fact that the local state is loaded
  // synchronously and at the first demand. IsEnterpriseManaged()
  // check is required because currently powerwash is disabled for
  // enterprise-enrolled devices.
  //
  // TODO (ygorshenin@): implement handling of the local state
  // corruption in the case of asynchronious loading.
  policy::BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  if (!connector->IsEnterpriseManaged()) {
    const PrefService::PrefInitializationStatus status =
        GetLocalState()->GetInitializationStatus();
    if (status == PrefService::INITIALIZATION_STATUS_ERROR) {
      OnLocalStateInitialized(false);
      return;
    } else if (status == PrefService::INITIALIZATION_STATUS_WAITING) {
      GetLocalState()->AddPrefInitObserver(
          base::BindOnce(&WizardController::OnLocalStateInitialized,
                         weak_factory_.GetWeakPtr()));
    }
  }

  // If the device is a Master device in bootstrapping process (mostly for demo
  // and test purpose), start the enrollment OOBE flow.
  if (IsBootstrappingMaster())
    connector->GetDeviceCloudPolicyManager()->SetDeviceEnrollmentAutoStart();

  // Use the saved screen preference from Local State.
  const std::string screen_pref =
      GetLocalState()->GetString(prefs::kOobeScreenPending);
  if (is_out_of_box_ && !screen_pref.empty() && !IsRemoraPairingOobe() &&
      !IsControllerDetected() &&
      (first_screen == OobeScreen::SCREEN_UNKNOWN ||
       first_screen == OobeScreen::SCREEN_TEST_NO_WINDOW)) {
    first_screen_ = GetOobeScreenFromName(screen_pref);
  }
  // We need to reset the kOobeControllerDetected pref to allow the user to have
  // the choice to setup the device manually. The pref will be set properly if
  // an eligible controller is detected later.
  SetControllerDetectedPref(false);

  if ((screen_pref.empty() ||
       GetLocalState()->HasPrefPath(prefs::kOobeMdMode)) ||
      GetLocalState()->GetBoolean(prefs::kOobeMdMode))
    SetShowMdOobe(true);

  // TODO(drcrash): Remove this after testing (http://crbug.com/647411).
  if (IsRemoraPairingOobe() || IsSharkRequisition()) {
    SetShowMdOobe(false);
  }

  AdvanceToScreen(first_screen_);
  if (!IsMachineHWIDCorrect() && !StartupUtils::IsDeviceRegistered() &&
      first_screen_ == OobeScreen::SCREEN_UNKNOWN)
    ShowWrongHWIDScreen();

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kOobeSkipToLogin)) {
    SkipToLoginForTesting(LoginScreenContext());
  }
}

ErrorScreen* WizardController::GetErrorScreen() {
  return oobe_ui_->GetErrorScreen();
}

BaseScreen* WizardController::GetScreen(OobeScreen screen) {
  if (screen == OobeScreen::SCREEN_ERROR_MESSAGE)
    return GetErrorScreen();
  return screen_manager_->GetScreen(screen);
}

BaseScreen* WizardController::CreateScreen(OobeScreen screen) {
  if (screen == OobeScreen::SCREEN_OOBE_NETWORK) {
    return new NetworkScreen(this, this, oobe_ui_->GetNetworkView());
  } else if (screen == OobeScreen::SCREEN_OOBE_UPDATE) {
    return new UpdateScreen(this, oobe_ui_->GetUpdateView(),
                            remora_controller_.get());
  } else if (screen == OobeScreen::SCREEN_USER_IMAGE_PICKER) {
    return new UserImageScreen(this, oobe_ui_->GetUserImageView());
  } else if (screen == OobeScreen::SCREEN_OOBE_EULA) {
    return new EulaScreen(this, this, oobe_ui_->GetEulaView());
  } else if (screen == OobeScreen::SCREEN_OOBE_ENROLLMENT) {
    return new EnrollmentScreen(this, oobe_ui_->GetEnrollmentScreenView());
  } else if (screen == OobeScreen::SCREEN_OOBE_RESET) {
    return new chromeos::ResetScreen(this, oobe_ui_->GetResetView());
  } else if (screen == OobeScreen::SCREEN_OOBE_DEMO_SETUP) {
    return new chromeos::DemoSetupScreen(this,
                                         oobe_ui_->GetDemoSetupScreenView());
  } else if (screen == OobeScreen::SCREEN_OOBE_ENABLE_DEBUGGING) {
    return new EnableDebuggingScreen(this,
                                     oobe_ui_->GetEnableDebuggingScreenView());
  } else if (screen == OobeScreen::SCREEN_KIOSK_ENABLE) {
    return new KioskEnableScreen(this, oobe_ui_->GetKioskEnableScreenView());
  } else if (screen == OobeScreen::SCREEN_KIOSK_AUTOLAUNCH) {
    return new KioskAutolaunchScreen(this,
                                     oobe_ui_->GetKioskAutolaunchScreenView());
  } else if (screen == OobeScreen::SCREEN_TERMS_OF_SERVICE) {
    return new TermsOfServiceScreen(this,
                                    oobe_ui_->GetTermsOfServiceScreenView());
  } else if (screen == OobeScreen::SCREEN_SYNC_CONSENT) {
    return new SyncConsentScreen(this, oobe_ui_->GetSyncConsentScreenView());
  } else if (screen == OobeScreen::SCREEN_ARC_TERMS_OF_SERVICE) {
    return new ArcTermsOfServiceScreen(
        this, oobe_ui_->GetArcTermsOfServiceScreenView());
  } else if (screen == OobeScreen::SCREEN_RECOMMEND_APPS) {
    return new RecommendAppsScreen(this,
                                   oobe_ui_->GetRecommendAppsScreenView());
  } else if (screen == OobeScreen::SCREEN_WRONG_HWID) {
    return new WrongHWIDScreen(this, oobe_ui_->GetWrongHWIDScreenView());
  } else if (screen == OobeScreen::SCREEN_CREATE_SUPERVISED_USER_FLOW) {
    return new SupervisedUserCreationScreen(
        this, oobe_ui_->GetSupervisedUserCreationScreenView());
  } else if (screen == OobeScreen::SCREEN_OOBE_HID_DETECTION) {
    return new chromeos::HIDDetectionScreen(this,
                                            oobe_ui_->GetHIDDetectionView());
  } else if (screen == OobeScreen::SCREEN_AUTO_ENROLLMENT_CHECK) {
    return new AutoEnrollmentCheckScreen(
        this, oobe_ui_->GetAutoEnrollmentCheckScreenView());
  } else if (screen == OobeScreen::SCREEN_OOBE_CONTROLLER_PAIRING) {
    if (!shark_controller_) {
      shark_controller_.reset(
          new pairing_chromeos::BluetoothControllerPairingController());
    }
    return new ControllerPairingScreen(
        this, this, oobe_ui_->GetControllerPairingScreenView(),
        shark_controller_.get());
  } else if (screen == OobeScreen::SCREEN_OOBE_HOST_PAIRING) {
    if (!remora_controller_) {
      DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
      DCHECK(content::ServiceManagerConnection::GetForProcess());
      service_manager::Connector* connector =
          content::ServiceManagerConnection::GetForProcess()->GetConnector();
      remora_controller_.reset(
          new pairing_chromeos::BluetoothHostPairingController(connector));
      remora_controller_->StartPairing();
    }
    return new HostPairingScreen(this, this,
                                 oobe_ui_->GetHostPairingScreenView(),
                                 remora_controller_.get());
  } else if (screen == OobeScreen::SCREEN_DEVICE_DISABLED) {
    return new DeviceDisabledScreen(this,
                                    oobe_ui_->GetDeviceDisabledScreenView());
  } else if (screen == OobeScreen::SCREEN_ENCRYPTION_MIGRATION) {
    return new EncryptionMigrationScreen(
        this, oobe_ui_->GetEncryptionMigrationScreenView());
  } else if (screen == OobeScreen::SCREEN_VOICE_INTERACTION_VALUE_PROP) {
    return new VoiceInteractionValuePropScreen(
        this, oobe_ui_->GetVoiceInteractionValuePropScreenView());
  } else if (screen == OobeScreen::SCREEN_WAIT_FOR_CONTAINER_READY) {
    return new WaitForContainerReadyScreen(
        this, oobe_ui_->GetWaitForContainerReadyScreenView());
  } else if (screen == OobeScreen::SCREEN_UPDATE_REQUIRED) {
    return new UpdateRequiredScreen(this,
                                    oobe_ui_->GetUpdateRequiredScreenView());
  }
  return nullptr;
}

void WizardController::SetCurrentScreenForTesting(BaseScreen* screen) {
  current_screen_ = screen;
}

void WizardController::ShowNetworkScreen() {
  VLOG(1) << "Showing network screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_OOBE_NETWORK);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_OOBE_NETWORK));

  // There are two possible screens where we listen to the incoming Bluetooth
  // connection request: the first one is the HID detection screen, which will
  // show up when there is no sufficient input devices. In this case, we just
  // keep the logic as it is today: always put the Bluetooth is discoverable
  // mode. The other place is the Network screen (here), which will show up when
  // there are input devices detected. In this case, we disable the Bluetooth by
  // default until the user explicitly enable it by pressing a key combo (Ctrl+
  // Alt+Shift+S).
  if (IsBootstrappingSlave())
    MaybeStartListeningForSharkConnection();
}

void WizardController::ShowLoginScreen(const LoginScreenContext& context) {
  // This may be triggered by multiply asynchronous events from the JS side.
  if (login_screen_started_)
    return;

  if (!time_eula_accepted_.is_null()) {
    base::TimeDelta delta = base::Time::Now() - time_eula_accepted_;
    UMA_HISTOGRAM_MEDIUM_TIMES("OOBE.EULAToSignInTime", delta);
  }
  VLOG(1) << "Showing login screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_SPECIAL_LOGIN);
  host_->StartSignInScreen(context);
  smooth_show_timer_.Stop();
  login_screen_started_ = true;
}

void WizardController::ShowUserImageScreen() {
  const user_manager::UserManager* user_manager =
      user_manager::UserManager::Get();
  // Skip user image selection for public sessions and ephemeral non-regual user
  // logins.
  if (user_manager->IsLoggedInAsPublicAccount() ||
      (user_manager->IsCurrentUserNonCryptohomeDataEphemeral() &&
       user_manager->GetActiveUser()->GetType() !=
           user_manager::USER_TYPE_REGULAR)) {
    OnUserImageSkipped();
    return;
  }
  VLOG(1) << "Showing user image screen.";

  // Status area has been already shown at sign in screen so it
  // doesn't make sense to hide it here and then show again at user session as
  // this produces undesired UX transitions.
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_USER_IMAGE_PICKER);

  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_USER_IMAGE_PICKER));
}

void WizardController::ShowEulaScreen() {
  VLOG(1) << "Showing EULA screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_OOBE_EULA);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_OOBE_EULA));
}

void WizardController::ShowEnrollmentScreen() {
  // Update the enrollment configuration and start the screen.
  prescribed_enrollment_config_ = g_browser_process->platform_part()
                                      ->browser_policy_connector_chromeos()
                                      ->GetPrescribedEnrollmentConfig();
  StartEnrollmentScreen(false);
}

void WizardController::ShowDemoModeSetupScreen() {
  VLOG(1) << "Showing demo mode setup screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_OOBE_DEMO_SETUP);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_OOBE_DEMO_SETUP));
}

void WizardController::ShowResetScreen() {
  VLOG(1) << "Showing reset screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_OOBE_RESET);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_OOBE_RESET));
}

void WizardController::ShowKioskEnableScreen() {
  VLOG(1) << "Showing kiosk enable screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_KIOSK_ENABLE);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_KIOSK_ENABLE));
}

void WizardController::ShowKioskAutolaunchScreen() {
  VLOG(1) << "Showing kiosk autolaunch screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_KIOSK_AUTOLAUNCH);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_KIOSK_AUTOLAUNCH));
}

void WizardController::ShowEnableDebuggingScreen() {
  VLOG(1) << "Showing enable developer features screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_OOBE_ENABLE_DEBUGGING);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_OOBE_ENABLE_DEBUGGING));
}

void WizardController::ShowTermsOfServiceScreen() {
  // Only show the Terms of Service when logging into a public account and Terms
  // of Service have been specified through policy. In all other cases, advance
  // to the post-ToS part immediately.
  if (!user_manager::UserManager::Get()->IsLoggedInAsPublicAccount() ||
      !ProfileManager::GetActiveUserProfile()->GetPrefs()->IsManagedPreference(
          prefs::kTermsOfServiceURL)) {
    OnTermsOfServiceAccepted();
    return;
  }

  VLOG(1) << "Showing Terms of Service screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_TERMS_OF_SERVICE);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_TERMS_OF_SERVICE));
}

void WizardController::ShowSyncConsentScreen() {
#if defined(GOOGLE_CHROME_BUILD)
  VLOG(1) << "Showing Sync Consent screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_SYNC_CONSENT);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_SYNC_CONSENT));
#else
  ShowArcTermsOfServiceScreen();
#endif
}

void WizardController::ShowArcTermsOfServiceScreen() {
  if (arc::IsArcTermsOfServiceOobeNegotiationNeeded()) {
    VLOG(1) << "Showing ARC Terms of Service screen.";
    UpdateStatusAreaVisibilityForScreen(
        OobeScreen::SCREEN_ARC_TERMS_OF_SERVICE);
    SetCurrentScreen(GetScreen(OobeScreen::SCREEN_ARC_TERMS_OF_SERVICE));
    // Assistant Wizard also uses wizard for ARC opt-in, unlike other scenarios
    // which use ArcSupport for now, because we're interested in only OOBE flow.
    // Note that this part also needs to be updated on b/65861628.
    // TODO(khmel): add unit test once we have support for OobeUI.
    if (!host_->IsVoiceInteractionOobe()) {
      ProfileManager::GetActiveUserProfile()->GetPrefs()->SetBoolean(
          arc::prefs::kArcTermsShownInOobe, true);
    }
  } else {
    ShowUserImageScreen();
  }
}

void WizardController::ShowRecommendAppsScreen() {
  // TODO(rsgingerrs): should maybe check if ToS has been accepted
  VLOG(1) << "Showing Recommend Apps screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_RECOMMEND_APPS);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_RECOMMEND_APPS));
}

void WizardController::ShowWrongHWIDScreen() {
  VLOG(1) << "Showing wrong HWID screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_WRONG_HWID);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_WRONG_HWID));
}

void WizardController::ShowAutoEnrollmentCheckScreen() {
  VLOG(1) << "Showing Auto-enrollment check screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_AUTO_ENROLLMENT_CHECK);
  AutoEnrollmentCheckScreen* screen =
      AutoEnrollmentCheckScreen::Get(screen_manager());
  if (retry_auto_enrollment_check_)
    screen->ClearState();
  screen->set_auto_enrollment_controller(GetAutoEnrollmentController());
  SetCurrentScreen(screen);
}

void WizardController::ShowSupervisedUserCreationScreen() {
  VLOG(1) << "Showing Locally managed user creation screen screen.";
  UpdateStatusAreaVisibilityForScreen(
      OobeScreen::SCREEN_CREATE_SUPERVISED_USER_FLOW);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_CREATE_SUPERVISED_USER_FLOW));
}

void WizardController::ShowArcKioskSplashScreen() {
  VLOG(1) << "Showing ARC kiosk splash screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_ARC_KIOSK_SPLASH);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_ARC_KIOSK_SPLASH));
}

void WizardController::ShowHIDDetectionScreen() {
  VLOG(1) << "Showing HID discovery screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_OOBE_HID_DETECTION);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_OOBE_HID_DETECTION));
  // In HID detection screen, puts the Bluetooth in discoverable mode and waits
  // for the incoming Bluetooth connection request. See the comments in
  // WizardController::ShowNetworkScreen() for more details.
  MaybeStartListeningForSharkConnection();
}

void WizardController::ShowControllerPairingScreen() {
  VLOG(1) << "Showing controller pairing screen.";
  UpdateStatusAreaVisibilityForScreen(
      OobeScreen::SCREEN_OOBE_CONTROLLER_PAIRING);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_OOBE_CONTROLLER_PAIRING));
}

void WizardController::ShowHostPairingScreen() {
  VLOG(1) << "Showing host pairing screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_OOBE_HOST_PAIRING);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_OOBE_HOST_PAIRING));
}

void WizardController::ShowDeviceDisabledScreen() {
  VLOG(1) << "Showing device disabled screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_DEVICE_DISABLED);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_DEVICE_DISABLED));
}

void WizardController::ShowEncryptionMigrationScreen() {
  VLOG(1) << "Showing encryption migration screen.";
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_ENCRYPTION_MIGRATION);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_ENCRYPTION_MIGRATION));
}

void WizardController::ShowVoiceInteractionValuePropScreen() {
  if (ShouldShowVoiceInteractionValueProp()) {
    VLOG(1) << "Showing voice interaction value prop screen.";
    UpdateStatusAreaVisibilityForScreen(
        OobeScreen::SCREEN_VOICE_INTERACTION_VALUE_PROP);
    SetCurrentScreen(
        GetScreen(OobeScreen::SCREEN_VOICE_INTERACTION_VALUE_PROP));
  } else {
    OnOobeFlowFinished();
  }
}

void WizardController::ShowWaitForContainerReadyScreen() {
  DCHECK(is_in_session_oobe_);
  // At this point we could make sure the value prop flow has been accepted.
  // Set the value prop pref as accepted in framework service.
  auto* service =
      arc::ArcVoiceInteractionFrameworkService::GetForBrowserContext(
          ProfileManager::GetActiveUserProfile());
  if (service)
    service->SetVoiceInteractionSetupCompleted();

  UpdateStatusAreaVisibilityForScreen(
      OobeScreen::SCREEN_WAIT_FOR_CONTAINER_READY);
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_WAIT_FOR_CONTAINER_READY));
}

void WizardController::ShowUpdateRequiredScreen() {
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_UPDATE_REQUIRED));
}

void WizardController::SkipToLoginForTesting(
    const LoginScreenContext& context) {
  VLOG(1) << "SkipToLoginForTesting.";
  StartupUtils::MarkEulaAccepted();
  PerformPostEulaActions();
  OnDeviceDisabledChecked(false /* device_disabled */);
}

void WizardController::SkipToUpdateForTesting() {
  VLOG(1) << "SkipToUpdateForTesting.";
  StartupUtils::MarkEulaAccepted();
  PerformPostEulaActions();
  InitiateOOBEUpdate();
}

pairing_chromeos::SharkConnectionListener*
WizardController::GetSharkConnectionListenerForTesting() {
  return shark_connection_listener_.get();
}

void WizardController::SkipUpdateEnrollAfterEula() {
  skip_update_enroll_after_eula_ = true;
}

///////////////////////////////////////////////////////////////////////////////
// WizardController, ExitHandlers:
void WizardController::OnHIDDetectionCompleted() {
  // Check for tests configuration.
  if (!StartupUtils::IsOobeCompleted())
    ShowNetworkScreen();
}

void WizardController::OnNetworkConnected() {
  if (is_official_build_) {
    if (!StartupUtils::IsEulaAccepted()) {
      ShowEulaScreen();
    } else {
      // Possible cases:
      // 1. EULA was accepted, forced shutdown/reboot during update.
      // 2. EULA was accepted, planned reboot after update.
      // Make sure that device is up to date.
      InitiateOOBEUpdate();
    }
  } else {
    InitiateOOBEUpdate();
  }
}

void WizardController::OnConnectionFailed() {
  // TODO(dpolukhin): show error message after login screen is displayed.
  ShowLoginScreen(LoginScreenContext());
}

void WizardController::OnUpdateCompleted() {
  if (IsSharkRequisition() || IsBootstrappingMaster()) {
    ShowControllerPairingScreen();
  } else if (IsControllerDetected()) {
    ShowHostPairingScreen();
  } else {
    ShowAutoEnrollmentCheckScreen();
  }
}

void WizardController::OnEulaAccepted() {
  time_eula_accepted_ = base::Time::Now();
  StartupUtils::MarkEulaAccepted();
  ChangeMetricsReportingStateWithReply(
      usage_statistics_reporting_,
      base::Bind(&WizardController::OnChangedMetricsReportingState,
                 weak_factory_.GetWeakPtr()));
  PerformPostEulaActions();

  if (skip_update_enroll_after_eula_) {
    ShowAutoEnrollmentCheckScreen();
  } else {
    InitiateOOBEUpdate();
  }
}

void WizardController::OnChangedMetricsReportingState(bool enabled) {
  CrosSettings::Get()->SetBoolean(kStatsReportingPref, enabled);
  if (!enabled)
    return;
#if defined(GOOGLE_CHROME_BUILD)
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock()},
      base::Bind(&breakpad::InitCrashReporter, std::string()));
#endif
}

void WizardController::OnUpdateErrorCheckingForUpdate() {
  // TODO(nkostylev): Update should be required during OOBE.
  // We do not want to block users from being able to proceed to the login
  // screen if there is any error checking for an update.
  // They could use "browse without sign-in" feature to set up the network to be
  // able to perform the update later.
  OnUpdateCompleted();
}

void WizardController::OnUpdateErrorUpdating(bool is_critical_update) {
  // If there was an error while getting or applying the update, return to
  // network selection screen if the OOBE isn't complete and the update is
  // deemed critical. Otherwise, similar to OnUpdateErrorCheckingForUpdate(),
  // we do not want to block users from being able to proceed to the login
  // screen.
  if (is_out_of_box_ && is_critical_update)
    ShowNetworkScreen();
  else
    OnUpdateCompleted();
}

void WizardController::EnableUserImageScreenReturnToPreviousHack() {
  user_image_screen_return_to_previous_hack_ = true;
}

void WizardController::OnUserImageSelected() {
  if (user_image_screen_return_to_previous_hack_) {
    user_image_screen_return_to_previous_hack_ = false;
    DCHECK(previous_screen_);
    if (previous_screen_) {
      SetCurrentScreen(previous_screen_);
      return;
    }
  }
  OnOobeFlowFinished();
}

void WizardController::OnUserImageSkipped() {
  OnUserImageSelected();
}

void WizardController::OnEnrollmentDone() {
  PerformOOBECompletedActions();

  // Restart to make the login page pick up the policy changes resulting from
  // enrollment recovery.  (Not pretty, but this codepath is rarely exercised.)
  if (prescribed_enrollment_config_.mode ==
      policy::EnrollmentConfig::MODE_RECOVERY) {
    chrome::AttemptRestart();
  }

  // TODO(mnissler): Unify the logic for auto-login for Public Sessions and
  // Kiosk Apps and make this code cover both cases: http://crbug.com/234694.
  if (KioskAppManager::Get()->IsAutoLaunchEnabled())
    AutoLaunchKioskApp();
  else
    ShowLoginScreen(LoginScreenContext());
}

void WizardController::OnDeviceModificationCanceled() {
  if (previous_screen_) {
    SetCurrentScreen(previous_screen_);
  } else {
    ShowLoginScreen(LoginScreenContext());
  }
}

void WizardController::OnKioskAutolaunchCanceled() {
  ShowLoginScreen(LoginScreenContext());
}

void WizardController::OnKioskAutolaunchConfirmed() {
  DCHECK(KioskAppManager::Get()->IsAutoLaunchEnabled());
  AutoLaunchKioskApp();
}

void WizardController::OnKioskEnableCompleted() {
  ShowLoginScreen(LoginScreenContext());
}

void WizardController::OnWrongHWIDWarningSkipped() {
  if (previous_screen_)
    SetCurrentScreen(previous_screen_);
  else
    ShowLoginScreen(LoginScreenContext());
}

void WizardController::OnTermsOfServiceDeclined() {
  // If the user declines the Terms of Service, end the session and return to
  // the login screen.
  DBusThreadManager::Get()->GetSessionManagerClient()->StopSession();
}

void WizardController::OnTermsOfServiceAccepted() {
  ShowSyncConsentScreen();
}

void WizardController::OnArcTermsOfServiceSkipped() {
  if (is_in_session_oobe_) {
    OnOobeFlowFinished();
    return;
  }
  // If the user finished with the PlayStore Terms of Service, advance to the
  // user image screen.
  ShowUserImageScreen();
}

void WizardController::OnArcTermsOfServiceAccepted() {
  if (is_in_session_oobe_) {
    ShowWaitForContainerReadyScreen();
    return;
  }
  // If the user finished with the PlayStore Terms of Service, advance to the
  // user image screen.
  ShowUserImageScreen();
}

void WizardController::OnRecommendAppsSkipped() {
  OnOobeFlowFinished(); // TODO(rsgingerrs): there may be a next step?
}

void WizardController::OnVoiceInteractionValuePropSkipped() {
  OnOobeFlowFinished();
}

void WizardController::OnVoiceInteractionValuePropAccepted() {
  if (is_in_session_oobe_ && arc::IsArcTermsOfServiceOobeNegotiationNeeded()) {
    ShowArcTermsOfServiceScreen();
    return;
  }
  ShowWaitForContainerReadyScreen();
}

void WizardController::OnWaitForContainerReadyFinished() {
  OnOobeFlowFinished();
  StartVoiceInteractionSetupWizard();
}

void WizardController::OnControllerPairingFinished() {
  ShowAutoEnrollmentCheckScreen();
}

void WizardController::OnAutoEnrollmentCheckCompleted() {
  // Check whether the device is disabled. OnDeviceDisabledChecked() will be
  // invoked when the result of this check is known. Until then, the current
  // screen will remain visible and will continue showing a spinner.
  g_browser_process->platform_part()
      ->device_disabling_manager()
      ->CheckWhetherDeviceDisabledDuringOOBE(
          base::Bind(&WizardController::OnDeviceDisabledChecked,
                     weak_factory_.GetWeakPtr()));
}

void WizardController::OnDemoSetupClosed() {
  DCHECK(previous_screen_);
  SetCurrentScreen(previous_screen_);
}

void WizardController::OnOobeFlowFinished() {
  if (is_in_session_oobe_) {
    host_->SetStatusAreaVisible(true);
    host_->Finalize(base::OnceClosure());
    host_ = nullptr;
    return;
  }

  if (!time_oobe_started_.is_null()) {
    base::TimeDelta delta = base::Time::Now() - time_oobe_started_;
    UMA_HISTOGRAM_CUSTOM_TIMES("OOBE.BootToSignInCompleted", delta,
                               base::TimeDelta::FromMilliseconds(10),
                               base::TimeDelta::FromMinutes(30), 100);
    time_oobe_started_ = base::Time();
  }

  // Launch browser and delete login host controller.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UserSessionManager::DoBrowserLaunch,
                     base::Unretained(UserSessionManager::GetInstance()),
                     ProfileManager::GetActiveUserProfile(), host_));
  host_ = nullptr;
}

void WizardController::OnDeviceDisabledChecked(bool device_disabled) {
  prescribed_enrollment_config_ = g_browser_process->platform_part()
                                      ->browser_policy_connector_chromeos()
                                      ->GetPrescribedEnrollmentConfig();
  if (device_disabled) {
    ShowDeviceDisabledScreen();
  } else if (skip_update_enroll_after_eula_ ||
             prescribed_enrollment_config_.should_enroll()) {
    StartEnrollmentScreen(skip_update_enroll_after_eula_);
  } else {
    PerformOOBECompletedActions();
    ShowLoginScreen(LoginScreenContext());
  }
}

void WizardController::InitiateOOBEUpdate() {
  if (IsRemoraRequisition()) {
    VLOG(1) << "Skip OOBE Update for remora.";
    OnUpdateCompleted();
    return;
  }

  // If this is a Cellular First device, instruct UpdateEngine to allow
  // updates over cellular data connections.
  if (chromeos::switches::IsCellularFirstDevice()) {
    DBusThreadManager::Get()
        ->GetUpdateEngineClient()
        ->SetUpdateOverCellularPermission(
            true, base::Bind(&WizardController::StartOOBEUpdate,
                             weak_factory_.GetWeakPtr()));
  } else {
    StartOOBEUpdate();
  }
}

void WizardController::StartOOBEUpdate() {
  VLOG(1) << "StartOOBEUpdate";
  SetCurrentScreenSmooth(GetScreen(OobeScreen::SCREEN_OOBE_UPDATE), true);
  UpdateScreen::Get(screen_manager())->StartNetworkCheck();
}

void WizardController::StartTimezoneResolve() {
  if (!g_browser_process->platform_part()
           ->GetTimezoneResolverManager()
           ->TimeZoneResolverShouldBeRunning())
    return;

  geolocation_provider_.reset(new SimpleGeolocationProvider(
      g_browser_process->system_request_context(),
      SimpleGeolocationProvider::DefaultGeolocationProviderURL()));
  geolocation_provider_->RequestGeolocation(
      base::TimeDelta::FromSeconds(kResolveTimeZoneTimeoutSeconds),
      false /* send_wifi_geolocation_data */,
      false /* send_cellular_geolocation_data */,
      base::Bind(&WizardController::OnLocationResolved,
                 weak_factory_.GetWeakPtr()));
}

void WizardController::PerformPostEulaActions() {
  DelayNetworkCall(
      base::TimeDelta::FromMilliseconds(kDefaultNetworkRetryDelayMS),
      base::Bind(&WizardController::StartTimezoneResolve,
                 weak_factory_.GetWeakPtr()));
  DelayNetworkCall(
      base::TimeDelta::FromMilliseconds(kDefaultNetworkRetryDelayMS),
      ServicesCustomizationDocument::GetInstance()
          ->EnsureCustomizationAppliedClosure());

  // Now that EULA has been accepted (for official builds), enable portal check.
  // ChromiumOS builds would go though this code path too.
  NetworkHandler::Get()->network_state_handler()->SetCheckPortalList(
      NetworkStateHandler::kDefaultCheckPortalList);
  GetAutoEnrollmentController()->Start();
  host_->PrewarmAuthentication();
  network_portal_detector::GetInstance()->Enable(true);
}

void WizardController::PerformOOBECompletedActions() {
  // Avoid marking OOBE as completed multiple times if going from login screen
  // to enrollment screen (and back).
  if (oobe_marked_completed_) {
    return;
  }

  UMA_HISTOGRAM_COUNTS_100(
      "HIDDetection.TimesDialogShownPerOOBECompleted",
      GetLocalState()->GetInteger(prefs::kTimesHIDDialogShown));
  GetLocalState()->ClearPref(prefs::kTimesHIDDialogShown);
  StartupUtils::MarkOobeCompleted();
  oobe_marked_completed_ = true;

  if (shark_connection_listener_.get())
    shark_connection_listener_->ResetController();
}

void WizardController::SetCurrentScreen(BaseScreen* new_current) {
  SetCurrentScreenSmooth(new_current, false);
}

void WizardController::ShowCurrentScreen() {
  // ShowCurrentScreen may get called by smooth_show_timer_ even after
  // flow has been switched to sign in screen (ExistingUserController).
  if (!oobe_ui_)
    return;

  // First remember how far have we reached so that we can resume if needed.
  if (is_out_of_box_ && IsResumableScreen(current_screen_->screen_id())) {
    StartupUtils::SaveOobePendingScreen(
        GetOobeScreenName(current_screen_->screen_id()));
  }

  smooth_show_timer_.Stop();

  UpdateStatusAreaVisibilityForScreen(current_screen_->screen_id());
  current_screen_->Show();
}

void WizardController::SetCurrentScreenSmooth(BaseScreen* new_current,
                                              bool use_smoothing) {
  VLOG(1) << "SetCurrentScreenSmooth: "
          << GetOobeScreenName(new_current->screen_id());
  if (current_screen_ == new_current || new_current == nullptr ||
      oobe_ui_ == nullptr) {
    return;
  }

  smooth_show_timer_.Stop();

  if (current_screen_)
    current_screen_->Hide();

  const OobeScreen screen = new_current->screen_id();
  if (IsOOBEStepToTrack(screen))
    screen_show_times_[GetOobeScreenName(screen)] = base::Time::Now();

  previous_screen_ = current_screen_;
  current_screen_ = new_current;

  oobe_ui_->UpdateLocalizedStringsIfNeeded();

  if (use_smoothing) {
    smooth_show_timer_.Start(FROM_HERE,
                             base::TimeDelta::FromMilliseconds(kShowDelayMs),
                             this, &WizardController::ShowCurrentScreen);
  } else {
    ShowCurrentScreen();
  }
}

void WizardController::UpdateStatusAreaVisibilityForScreen(OobeScreen screen) {
  if (screen == OobeScreen::SCREEN_OOBE_NETWORK) {
    // Hide the status area initially; it only appears after OOBE first animates
    // in. Keep it visible if the user goes back to the existing network screen.
    host_->SetStatusAreaVisible(
        screen_manager_->HasScreen(OobeScreen::SCREEN_OOBE_NETWORK));
  } else if (screen == OobeScreen::SCREEN_OOBE_RESET ||
             screen == OobeScreen::SCREEN_KIOSK_ENABLE ||
             screen == OobeScreen::SCREEN_KIOSK_AUTOLAUNCH ||
             screen == OobeScreen::SCREEN_OOBE_ENABLE_DEBUGGING ||
             screen == OobeScreen::SCREEN_WRONG_HWID ||
             screen == OobeScreen::SCREEN_ARC_KIOSK_SPLASH ||
             screen == OobeScreen::SCREEN_OOBE_CONTROLLER_PAIRING ||
             screen == OobeScreen::SCREEN_OOBE_HOST_PAIRING) {
    host_->SetStatusAreaVisible(false);
  } else {
    host_->SetStatusAreaVisible(true);
  }
}

void WizardController::SetShowMdOobe(bool show) {
  GetLocalState()->SetBoolean(prefs::kOobeMdMode, show);
}

void WizardController::OnHIDScreenNecessityCheck(bool screen_needed) {
  if (!oobe_ui_)
    return;

  if (screen_needed) {
    ShowHIDDetectionScreen();
  } else {
    ShowNetworkScreen();
  }
}

void WizardController::AdvanceToScreen(OobeScreen screen) {
  if (screen == OobeScreen::SCREEN_OOBE_NETWORK) {
    ShowNetworkScreen();
  } else if (screen == OobeScreen::SCREEN_SPECIAL_LOGIN) {
    ShowLoginScreen(LoginScreenContext());
  } else if (screen == OobeScreen::SCREEN_OOBE_UPDATE) {
    InitiateOOBEUpdate();
  } else if (screen == OobeScreen::SCREEN_USER_IMAGE_PICKER) {
    ShowUserImageScreen();
  } else if (screen == OobeScreen::SCREEN_OOBE_EULA) {
    ShowEulaScreen();
  } else if (screen == OobeScreen::SCREEN_OOBE_RESET) {
    ShowResetScreen();
  } else if (screen == OobeScreen::SCREEN_KIOSK_ENABLE) {
    ShowKioskEnableScreen();
  } else if (screen == OobeScreen::SCREEN_KIOSK_AUTOLAUNCH) {
    ShowKioskAutolaunchScreen();
  } else if (screen == OobeScreen::SCREEN_OOBE_ENABLE_DEBUGGING) {
    ShowEnableDebuggingScreen();
  } else if (screen == OobeScreen::SCREEN_OOBE_ENROLLMENT) {
    ShowEnrollmentScreen();
  } else if (screen == OobeScreen::SCREEN_OOBE_DEMO_SETUP) {
    ShowDemoModeSetupScreen();
  } else if (screen == OobeScreen::SCREEN_TERMS_OF_SERVICE) {
    ShowTermsOfServiceScreen();
  } else if (screen == OobeScreen::SCREEN_SYNC_CONSENT) {
    ShowSyncConsentScreen();
  } else if (screen == OobeScreen::SCREEN_ARC_TERMS_OF_SERVICE) {
    ShowArcTermsOfServiceScreen();
  } else if (screen == OobeScreen::SCREEN_RECOMMEND_APPS) {
    ShowRecommendAppsScreen();
  } else if (screen == OobeScreen::SCREEN_WRONG_HWID) {
    ShowWrongHWIDScreen();
  } else if (screen == OobeScreen::SCREEN_AUTO_ENROLLMENT_CHECK) {
    ShowAutoEnrollmentCheckScreen();
  } else if (screen == OobeScreen::SCREEN_CREATE_SUPERVISED_USER_FLOW) {
    ShowSupervisedUserCreationScreen();
  } else if (screen == OobeScreen::SCREEN_APP_LAUNCH_SPLASH) {
    AutoLaunchKioskApp();
  } else if (screen == OobeScreen::SCREEN_ARC_KIOSK_SPLASH) {
    ShowArcKioskSplashScreen();
  } else if (screen == OobeScreen::SCREEN_OOBE_HID_DETECTION) {
    ShowHIDDetectionScreen();
  } else if (screen == OobeScreen::SCREEN_OOBE_CONTROLLER_PAIRING) {
    ShowControllerPairingScreen();
  } else if (screen == OobeScreen::SCREEN_OOBE_HOST_PAIRING) {
    ShowHostPairingScreen();
  } else if (screen == OobeScreen::SCREEN_DEVICE_DISABLED) {
    ShowDeviceDisabledScreen();
  } else if (screen == OobeScreen::SCREEN_ENCRYPTION_MIGRATION) {
    ShowEncryptionMigrationScreen();
  } else if (screen == OobeScreen::SCREEN_VOICE_INTERACTION_VALUE_PROP) {
    ShowVoiceInteractionValuePropScreen();
  } else if (screen == OobeScreen::SCREEN_WAIT_FOR_CONTAINER_READY) {
    ShowWaitForContainerReadyScreen();
  } else if (screen == OobeScreen::SCREEN_UPDATE_REQUIRED) {
    ShowUpdateRequiredScreen();
  } else if (screen != OobeScreen::SCREEN_TEST_NO_WINDOW) {
    if (is_out_of_box_) {
      time_oobe_started_ = base::Time::Now();
      if (IsRemoraPairingOobe() || IsControllerDetected()) {
        ShowHostPairingScreen();
      } else if (CanShowHIDDetectionScreen()) {
        hid_screen_ = GetScreen(OobeScreen::SCREEN_OOBE_HID_DETECTION);
        base::Callback<void(bool)> on_check =
            base::Bind(&WizardController::OnHIDScreenNecessityCheck,
                       weak_factory_.GetWeakPtr());
        oobe_ui_->GetHIDDetectionView()->CheckIsScreenRequired(on_check);
      } else {
        ShowNetworkScreen();
      }
    } else {
      ShowLoginScreen(LoginScreenContext());
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// WizardController, BaseScreenDelegate overrides:
void WizardController::OnExit(BaseScreen& /* screen */,
                              ScreenExitCode exit_code,
                              const ::login::ScreenContext* /* context */) {
  VLOG(1) << "Wizard screen exit code: " << ExitCodeToString(exit_code);
  const OobeScreen previous_screen = current_screen_->screen_id();
  if (IsOOBEStepToTrack(previous_screen)) {
    RecordUMAHistogramForOOBEStepCompletionTime(
        previous_screen,
        base::Time::Now() -
            screen_show_times_[GetOobeScreenName(previous_screen)]);
  }
  switch (exit_code) {
    case ScreenExitCode::HID_DETECTION_COMPLETED:
      OnHIDDetectionCompleted();
      break;
    case ScreenExitCode::NETWORK_CONNECTED:
      OnNetworkConnected();
      break;
    case ScreenExitCode::CONNECTION_FAILED:
      OnConnectionFailed();
      break;
    case ScreenExitCode::UPDATE_INSTALLED:
    case ScreenExitCode::UPDATE_NOUPDATE:
      OnUpdateCompleted();
      break;
    case ScreenExitCode::UPDATE_ERROR_CHECKING_FOR_UPDATE:
      OnUpdateErrorCheckingForUpdate();
      break;
    case ScreenExitCode::UPDATE_ERROR_UPDATING:
      OnUpdateErrorUpdating(false /* is_critical_update */);
      break;
    case ScreenExitCode::UPDATE_ERROR_UPDATING_CRITICAL_UPDATE:
      OnUpdateErrorUpdating(true /* is_critical_update */);
      break;
    case ScreenExitCode::USER_IMAGE_SELECTED:
      OnUserImageSelected();
      break;
    case ScreenExitCode::EULA_ACCEPTED:
      OnEulaAccepted();
      break;
    case ScreenExitCode::EULA_BACK:
      ShowNetworkScreen();
      break;
    case ScreenExitCode::ENABLE_DEBUGGING_CANCELED:
      OnDeviceModificationCanceled();
      break;
    case ScreenExitCode::ENABLE_DEBUGGING_FINISHED:
      OnDeviceModificationCanceled();
      break;
    case ScreenExitCode::ENTERPRISE_AUTO_ENROLLMENT_CHECK_COMPLETED:
      OnAutoEnrollmentCheckCompleted();
      break;
    case ScreenExitCode::ENTERPRISE_ENROLLMENT_COMPLETED:
      OnEnrollmentDone();
      break;
    case ScreenExitCode::ENTERPRISE_ENROLLMENT_BACK:
      retry_auto_enrollment_check_ = true;
      ShowAutoEnrollmentCheckScreen();
      break;
    case ScreenExitCode::RESET_CANCELED:
      OnDeviceModificationCanceled();
      break;
    case ScreenExitCode::KIOSK_AUTOLAUNCH_CANCELED:
      OnKioskAutolaunchCanceled();
      break;
    case ScreenExitCode::KIOSK_AUTOLAUNCH_CONFIRMED:
      OnKioskAutolaunchConfirmed();
      break;
    case ScreenExitCode::KIOSK_ENABLE_COMPLETED:
      OnKioskEnableCompleted();
      break;
    case ScreenExitCode::TERMS_OF_SERVICE_DECLINED:
      OnTermsOfServiceDeclined();
      break;
    case ScreenExitCode::TERMS_OF_SERVICE_ACCEPTED:
      OnTermsOfServiceAccepted();
      break;
    case ScreenExitCode::ARC_TERMS_OF_SERVICE_SKIPPED:
      OnArcTermsOfServiceSkipped();
      break;
    case ScreenExitCode::ARC_TERMS_OF_SERVICE_ACCEPTED:
      OnArcTermsOfServiceAccepted();
      break;
    case ScreenExitCode::WRONG_HWID_WARNING_SKIPPED:
      OnWrongHWIDWarningSkipped();
      break;
    case ScreenExitCode::CONTROLLER_PAIRING_FINISHED:
      OnControllerPairingFinished();
      break;
    case ScreenExitCode::VOICE_INTERACTION_VALUE_PROP_SKIPPED:
      OnVoiceInteractionValuePropSkipped();
      break;
    case ScreenExitCode::VOICE_INTERACTION_VALUE_PROP_ACCEPTED:
      OnVoiceInteractionValuePropAccepted();
      break;
    case ScreenExitCode::WAIT_FOR_CONTAINER_READY_FINISHED:
      OnWaitForContainerReadyFinished();
      break;
    case ScreenExitCode::WAIT_FOR_CONTAINER_READY_ERROR:
      OnOobeFlowFinished();
      break;
    case ScreenExitCode::SYNC_CONSENT_FINISHED:
      ShowArcTermsOfServiceScreen();
      break;
    case ScreenExitCode::RECOMMEND_APPS_SKIPPED:
      OnRecommendAppsSkipped();
      break;
    case ScreenExitCode::RECOMMEND_APPS_SELECTED:
      // TODO(rsgingerrs): Actions if user selects some apps to install
      break;
    case ScreenExitCode::DEMO_MODE_SETUP_CLOSED:
      OnDemoSetupClosed();
      break;
    default:
      NOTREACHED();
  }
}

void WizardController::ShowErrorScreen() {
  VLOG(1) << "Showing error screen.";
  SetCurrentScreen(GetScreen(OobeScreen::SCREEN_ERROR_MESSAGE));
}

void WizardController::HideErrorScreen(BaseScreen* parent_screen) {
  DCHECK(parent_screen);
  VLOG(1) << "Hiding error screen.";
  SetCurrentScreen(parent_screen);
}

void WizardController::SetUsageStatisticsReporting(bool val) {
  usage_statistics_reporting_ = val;
}

bool WizardController::GetUsageStatisticsReporting() const {
  return usage_statistics_reporting_;
}

void WizardController::SetHostNetwork() {
  if (!shark_controller_)
    return;
  NetworkScreen* network_screen = NetworkScreen::Get(screen_manager());
  std::string onc_spec;
  network_screen->GetConnectedWifiNetwork(&onc_spec);
  if (!onc_spec.empty())
    shark_controller_->SetHostNetwork(onc_spec);
}

void WizardController::SetHostConfiguration() {
  if (!shark_controller_)
    return;
  NetworkScreen* network_screen = NetworkScreen::Get(screen_manager());
  shark_controller_->SetHostConfiguration(
      true,  // Eula must be accepted before we get this far.
      network_screen->GetApplicationLocale(), network_screen->GetTimezone(),
      GetUsageStatisticsReporting(), network_screen->GetInputMethod());
}

void WizardController::ConfigureHostRequested(
    bool accepted_eula,
    const std::string& lang,
    const std::string& timezone,
    bool send_reports,
    const std::string& keyboard_layout) {
  VLOG(1) << "ConfigureHost locale=" << lang << ", timezone=" << timezone
          << ", keyboard_layout=" << keyboard_layout;
  if (accepted_eula)  // Always true.
    StartupUtils::MarkEulaAccepted();
  SetUsageStatisticsReporting(send_reports);

  NetworkScreen* network_screen = NetworkScreen::Get(screen_manager());
  network_screen->SetApplicationLocaleAndInputMethod(lang, keyboard_layout);
  network_screen->SetTimezone(timezone);

  // Don't block the OOBE update and the following enrollment process if there
  // is available and valid network already.
  const chromeos::NetworkState* network_state = chromeos::NetworkHandler::Get()
                                                    ->network_state_handler()
                                                    ->DefaultNetwork();
  if (NetworkAllowUpdate(network_state))
    InitiateOOBEUpdate();
}

void WizardController::AddNetworkRequested(const std::string& onc_spec) {
  remora_controller_->OnNetworkConnectivityChanged(
      pairing_chromeos::HostPairingController::CONNECTIVITY_CONNECTING);

  NetworkScreen* network_screen = NetworkScreen::Get(screen_manager());
  const chromeos::NetworkState* network_state = chromeos::NetworkHandler::Get()
                                                    ->network_state_handler()
                                                    ->DefaultNetwork();

  if (NetworkAllowUpdate(network_state)) {
    network_screen->CreateAndConnectNetworkFromOnc(
        onc_spec, base::DoNothing(), network_handler::ErrorCallback());
  } else {
    network_screen->CreateAndConnectNetworkFromOnc(
        onc_spec,
        base::Bind(&WizardController::OnSetHostNetworkSuccessful,
                   weak_factory_.GetWeakPtr()),
        base::Bind(&WizardController::OnSetHostNetworkFailed,
                   weak_factory_.GetWeakPtr()));
  }
}

void WizardController::RebootHostRequested() {
  DBusThreadManager::Get()->GetPowerManagerClient()->RequestRestart(
      power_manager::REQUEST_RESTART_OTHER, "login wizard reboot host");
}

void WizardController::OnEnableDebuggingScreenRequested() {
  if (!login_screen_started())
    AdvanceToScreen(OobeScreen::SCREEN_OOBE_ENABLE_DEBUGGING);
}

void WizardController::OnAccessibilityStatusChanged(
    const AccessibilityStatusEventDetails& details) {
  enum AccessibilityNotificationType type = details.notification_type;
  if (type == ACCESSIBILITY_MANAGER_SHUTDOWN) {
    accessibility_subscription_.reset();
    return;
  } else if (type != ACCESSIBILITY_TOGGLE_SPOKEN_FEEDBACK || !details.enabled) {
    return;
  }

  CrasAudioHandler* cras = CrasAudioHandler::Get();
  if (cras->IsOutputMuted()) {
    cras->SetOutputMute(false);
    cras->SetOutputVolumePercent(kMinAudibleOutputVolumePercent);
  } else if (cras->GetOutputVolumePercent() < kMinAudibleOutputVolumePercent) {
    cras->SetOutputVolumePercent(kMinAudibleOutputVolumePercent);
  }
}

void WizardController::AutoLaunchKioskApp() {
  KioskAppManager::App app_data;
  std::string app_id = KioskAppManager::Get()->GetAutoLaunchApp();
  CHECK(KioskAppManager::Get()->GetApp(app_id, &app_data));

  // Wait for the |CrosSettings| to become either trusted or permanently
  // untrusted.
  const CrosSettingsProvider::TrustedStatus status =
      CrosSettings::Get()->PrepareTrustedValues(base::Bind(
          &WizardController::AutoLaunchKioskApp, weak_factory_.GetWeakPtr()));
  if (status == CrosSettingsProvider::TEMPORARILY_UNTRUSTED)
    return;

  if (status == CrosSettingsProvider::PERMANENTLY_UNTRUSTED) {
    // If the |cros_settings_| are permanently untrusted, show an error message
    // and refuse to auto-launch the kiosk app.
    GetErrorScreen()->SetUIState(NetworkError::UI_STATE_LOCAL_STATE_ERROR);
    host_->SetStatusAreaVisible(false);
    ShowErrorScreen();
    return;
  }

  if (system::DeviceDisablingManager::IsDeviceDisabledDuringNormalOperation()) {
    // If the device is disabled, bail out. A device disabled screen will be
    // shown by the DeviceDisablingManager.
    return;
  }

  const bool diagnostic_mode = false;
  const bool auto_launch = true;
  host_->StartAppLaunch(app_id, diagnostic_mode, auto_launch);
}

// static
void WizardController::SetZeroDelays() {
  kShowDelayMs = 0;
  zero_delay_enabled_ = true;
}

// static
bool WizardController::IsZeroDelayEnabled() {
  return zero_delay_enabled_;
}

// static
bool WizardController::IsOOBEStepToTrack(OobeScreen screen_id) {
  return (screen_id == OobeScreen::SCREEN_OOBE_HID_DETECTION ||
          screen_id == OobeScreen::SCREEN_OOBE_NETWORK ||
          screen_id == OobeScreen::SCREEN_OOBE_UPDATE ||
          screen_id == OobeScreen::SCREEN_USER_IMAGE_PICKER ||
          screen_id == OobeScreen::SCREEN_OOBE_EULA ||
          screen_id == OobeScreen::SCREEN_SPECIAL_LOGIN ||
          screen_id == OobeScreen::SCREEN_WRONG_HWID);
}

// static
void WizardController::SkipPostLoginScreensForTesting() {
  skip_post_login_screens_ = true;
  if (!default_controller_ || !default_controller_->current_screen())
    return;

  const OobeScreen current_screen_id =
      default_controller_->current_screen()->screen_id();
  if (current_screen_id == OobeScreen::SCREEN_TERMS_OF_SERVICE ||
      current_screen_id == OobeScreen::SCREEN_SYNC_CONSENT ||
      current_screen_id == OobeScreen::SCREEN_ARC_TERMS_OF_SERVICE ||
      current_screen_id == OobeScreen::SCREEN_USER_IMAGE_PICKER) {
    default_controller_->OnOobeFlowFinished();
  } else {
    LOG(WARNING) << "SkipPostLoginScreensForTesting(): Ignore screen "
                 << static_cast<int>(current_screen_id);
  }
}

// static
void WizardController::SkipEnrollmentPromptsForTesting() {
  skip_enrollment_prompts_ = true;
}

// static
bool WizardController::UsingHandsOffEnrollment() {
  return policy::DeviceCloudPolicyManagerChromeOS::
             GetZeroTouchEnrollmentMode() ==
         policy::ZeroTouchEnrollmentMode::HANDS_OFF;
}

void WizardController::OnLocalStateInitialized(bool /* succeeded */) {
  if (GetLocalState()->GetInitializationStatus() !=
      PrefService::INITIALIZATION_STATUS_ERROR) {
    return;
  }
  GetErrorScreen()->SetUIState(NetworkError::UI_STATE_LOCAL_STATE_ERROR);
  host_->SetStatusAreaVisible(false);
  ShowErrorScreen();
}

PrefService* WizardController::GetLocalState() {
  if (local_state_for_testing_)
    return local_state_for_testing_;
  return g_browser_process->local_state();
}

void WizardController::OnTimezoneResolved(
    std::unique_ptr<TimeZoneResponseData> timezone,
    bool server_error) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(timezone.get());
  // To check that "this" is not destroyed try to access some member
  // (timezone_provider_) in this case. Expect crash here.
  DCHECK(timezone_provider_.get());

  timezone_resolved_ = true;
  base::ScopedClosureRunner inform_test(on_timezone_resolved_for_testing_);
  on_timezone_resolved_for_testing_.Reset();

  VLOG(1) << "Resolved local timezone={" << timezone->ToStringForDebug()
          << "}.";

  if (timezone->status != TimeZoneResponseData::OK) {
    LOG(WARNING) << "Resolve TimeZone: failed to resolve timezone.";
    return;
  }

  policy::BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  if (connector->IsEnterpriseManaged()) {
    std::string policy_timezone;
    if (CrosSettings::Get()->GetString(kSystemTimezonePolicy,
                                       &policy_timezone) &&
        !policy_timezone.empty()) {
      VLOG(1) << "Resolve TimeZone: TimeZone settings are overridden"
              << " by DevicePolicy.";
      return;
    }
  }

  if (!timezone->timeZoneId.empty()) {
    VLOG(1) << "Resolve TimeZone: setting timezone to '" << timezone->timeZoneId
            << "'";
    chromeos::system::SetSystemAndSigninScreenTimezone(timezone->timeZoneId);
  }
}

TimeZoneProvider* WizardController::GetTimezoneProvider() {
  if (!timezone_provider_) {
    timezone_provider_.reset(
        new TimeZoneProvider(g_browser_process->system_request_context(),
                             DefaultTimezoneProviderURL()));
  }
  return timezone_provider_.get();
}

void WizardController::OnLocationResolved(const Geoposition& position,
                                          bool server_error,
                                          const base::TimeDelta elapsed) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const base::TimeDelta timeout =
      base::TimeDelta::FromSeconds(kResolveTimeZoneTimeoutSeconds);
  // Ignore invalid position.
  if (!position.Valid())
    return;

  if (elapsed >= timeout) {
    LOG(WARNING) << "Resolve TimeZone: got location after timeout ("
                 << elapsed.InSecondsF() << " seconds elapsed). Ignored.";
    return;
  }

  if (!g_browser_process->platform_part()
           ->GetTimezoneResolverManager()
           ->TimeZoneResolverShouldBeRunning()) {
    return;
  }

  // WizardController owns TimezoneProvider, so timezone request is silently
  // cancelled on destruction.
  GetTimezoneProvider()->RequestTimezone(
      position, timeout - elapsed,
      base::Bind(&WizardController::OnTimezoneResolved,
                 base::Unretained(this)));
}

bool WizardController::SetOnTimeZoneResolvedForTesting(
    const base::Closure& callback) {
  if (timezone_resolved_)
    return false;

  on_timezone_resolved_for_testing_ = callback;
  return true;
}

bool WizardController::IsRemoraPairingOobe() const {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kHostPairingOobe);
}

bool WizardController::ShouldShowVoiceInteractionValueProp() const {
  // If the OOBE flow was initiated from voice interaction shortcut, we will
  // show Arc terms later.
  if (!is_in_session_oobe_ && !arc::IsArcPlayStoreEnabledForProfile(
                                  ProfileManager::GetActiveUserProfile())) {
    VLOG(1) << "Skip Voice Interaction Value Prop screen because Arc Terms is "
            << "skipped.";
    return false;
  }
  if (!chromeos::switches::IsVoiceInteractionEnabled()) {
    VLOG(1) << "Skip Voice Interaction Value Prop screen because voice "
            << "interaction service is disabled.";
    return false;
  }
  return true;
}

void WizardController::StartVoiceInteractionSetupWizard() {
  auto* service =
      arc::ArcVoiceInteractionFrameworkService::GetForBrowserContext(
          ProfileManager::GetActiveUserProfile());
  if (service)
    service->StartVoiceInteractionSetupWizard();
}

void WizardController::MaybeStartListeningForSharkConnection() {
  // We shouldn't be here if we are running pairing OOBE already.
  if (IsControllerDetected())
    return;

  if (!shark_connection_listener_) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    DCHECK(content::ServiceManagerConnection::GetForProcess());
    service_manager::Connector* connector =
        content::ServiceManagerConnection::GetForProcess()->GetConnector();
    shark_connection_listener_.reset(
        new pairing_chromeos::SharkConnectionListener(
            connector, base::Bind(&WizardController::OnSharkConnected,
                                  weak_factory_.GetWeakPtr())));
  }
}

void WizardController::OnSharkConnected(
    std::unique_ptr<pairing_chromeos::HostPairingController>
        remora_controller) {
  VLOG(1) << "OnSharkConnected";
  remora_controller_ = std::move(remora_controller);
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(
      FROM_HERE, shark_connection_listener_.release());
  SetControllerDetectedPref(true);
  ShowHostPairingScreen();
}

void WizardController::OnSetHostNetworkSuccessful() {
  remora_controller_->OnNetworkConnectivityChanged(
      pairing_chromeos::HostPairingController::CONNECTIVITY_CONNECTED);
  InitiateOOBEUpdate();
}

void WizardController::OnSetHostNetworkFailed(
    const std::string& error_name,
    std::unique_ptr<base::DictionaryValue> error_data) {
  std::string error_message;
  JSONStringValueSerializer serializer(&error_message);
  serializer.Serialize(*error_data);
  error_message = error_name + ": " + error_message;

  remora_controller_->SetErrorCodeAndMessage(
      static_cast<int>(
          pairing_chromeos::HostPairingController::ErrorCode::NETWORK_ERROR),
      error_message);

  remora_controller_->OnNetworkConnectivityChanged(
      pairing_chromeos::HostPairingController::CONNECTIVITY_NONE);
}

void WizardController::StartEnrollmentScreen(bool force_interactive) {
  VLOG(1) << "Showing enrollment screen."
          << " Forcing interactive enrollment: " << force_interactive << ".";

  // Determine the effective enrollment configuration. If there is a valid
  // prescribed configuration, use that. If not, figure out which variant of
  // manual enrollment is taking place.
  policy::EnrollmentConfig effective_config = prescribed_enrollment_config_;
  if (!effective_config.should_enroll() ||
      (force_interactive && !effective_config.should_enroll_interactively())) {
    effective_config.mode =
        prescribed_enrollment_config_.management_domain.empty()
            ? policy::EnrollmentConfig::MODE_MANUAL
            : policy::EnrollmentConfig::MODE_MANUAL_REENROLLMENT;
  }

  EnrollmentScreen* screen = EnrollmentScreen::Get(screen_manager());
  screen->SetParameters(effective_config, shark_controller_.get());
  UpdateStatusAreaVisibilityForScreen(OobeScreen::SCREEN_OOBE_ENROLLMENT);
  SetCurrentScreen(screen);
}

AutoEnrollmentController* WizardController::GetAutoEnrollmentController() {
  if (!auto_enrollment_controller_)
    auto_enrollment_controller_ = std::make_unique<AutoEnrollmentController>();
  return auto_enrollment_controller_.get();
}

}  // namespace chromeos
