// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/chromeos_metrics_provider.h"

#include <stddef.h>
#include <string>
#include <vector>

#include "base/barrier_closure.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chromeos/system/statistics_provider.h"
#include "components/metrics/metrics_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"
#include "components/variations/service/variations_field_trial_creator.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_device.h"
#include "third_party/metrics_proto/chrome_user_metrics_extension.pb.h"
#include "ui/display/display.h"
#include "ui/events/event_utils.h"

using metrics::ChromeUserMetricsExtension;
using metrics::SampledProfile;
using metrics::SystemProfileProto;
typedef SystemProfileProto::Hardware::Bluetooth::PairedDevice PairedDevice;

namespace {

PairedDevice::Type AsBluetoothDeviceType(
    device::BluetoothDeviceType device_type) {
  switch (device_type) {
    case device::BluetoothDeviceType::UNKNOWN:
      return PairedDevice::DEVICE_UNKNOWN;
    case device::BluetoothDeviceType::COMPUTER:
      return PairedDevice::DEVICE_COMPUTER;
    case device::BluetoothDeviceType::PHONE:
      return PairedDevice::DEVICE_PHONE;
    case device::BluetoothDeviceType::MODEM:
      return PairedDevice::DEVICE_MODEM;
    case device::BluetoothDeviceType::AUDIO:
      return PairedDevice::DEVICE_AUDIO;
    case device::BluetoothDeviceType::CAR_AUDIO:
      return PairedDevice::DEVICE_CAR_AUDIO;
    case device::BluetoothDeviceType::VIDEO:
      return PairedDevice::DEVICE_VIDEO;
    case device::BluetoothDeviceType::PERIPHERAL:
      return PairedDevice::DEVICE_PERIPHERAL;
    case device::BluetoothDeviceType::JOYSTICK:
      return PairedDevice::DEVICE_JOYSTICK;
    case device::BluetoothDeviceType::GAMEPAD:
      return PairedDevice::DEVICE_GAMEPAD;
    case device::BluetoothDeviceType::KEYBOARD:
      return PairedDevice::DEVICE_KEYBOARD;
    case device::BluetoothDeviceType::MOUSE:
      return PairedDevice::DEVICE_MOUSE;
    case device::BluetoothDeviceType::TABLET:
      return PairedDevice::DEVICE_TABLET;
    case device::BluetoothDeviceType::KEYBOARD_MOUSE_COMBO:
      return PairedDevice::DEVICE_KEYBOARD_MOUSE_COMBO;
  }

  NOTREACHED();
  return PairedDevice::DEVICE_UNKNOWN;
}

void IncrementPrefValue(const char* path) {
  PrefService* pref = g_browser_process->local_state();
  DCHECK(pref);
  int value = pref->GetInteger(path);
  pref->SetInteger(path, value + 1);
}

const base::Feature kUmaShortHWClass{"UmaShortHWClass",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

// Called on a background thread to load hardware class information.
std::string GetFullHardwareClassOnBackgroundThread() {
  std::string full_hardware_class;
  chromeos::system::StatisticsProvider::GetInstance()->GetMachineStatistic(
      "hardware_class", &full_hardware_class);
  return full_hardware_class;
}

}  // namespace

ChromeOSMetricsProvider::ChromeOSMetricsProvider()
    : registered_user_count_at_log_initialization_(false),
      user_count_at_log_initialization_(0),
      weak_ptr_factory_(this) {}

ChromeOSMetricsProvider::~ChromeOSMetricsProvider() {
}

// static
void ChromeOSMetricsProvider::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kStabilityOtherUserCrashCount, 0);
  registry->RegisterIntegerPref(prefs::kStabilityKernelCrashCount, 0);
  registry->RegisterIntegerPref(prefs::kStabilitySystemUncleanShutdownCount, 0);
}

// static
void ChromeOSMetricsProvider::LogCrash(const std::string& crash_type) {
  if (crash_type == "user")
    IncrementPrefValue(prefs::kStabilityOtherUserCrashCount);
  else if (crash_type == "kernel")
    IncrementPrefValue(prefs::kStabilityKernelCrashCount);
  else if (crash_type == "uncleanshutdown")
    IncrementPrefValue(prefs::kStabilitySystemUncleanShutdownCount);
  else
    NOTREACHED() << "Unexpected Chrome OS crash type " << crash_type;

  // Wake up metrics logs sending if necessary now that new
  // log data is available.
  g_browser_process->metrics_service()->OnApplicationNotIdle();
}

ChromeOSMetricsProvider::EnrollmentStatus
ChromeOSMetricsProvider::GetEnrollmentStatus() {
  policy::BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  if (!connector)
    return ERROR_GETTING_ENROLLMENT_STATUS;

  return connector->IsEnterpriseManaged() ? MANAGED : NON_MANAGED;
}

void ChromeOSMetricsProvider::Init() {
  if (base::FeatureList::IsEnabled(kUmaShortHWClass)) {
    hardware_class_ =
        variations::VariationsFieldTrialCreator::GetShortHardwareClass();
  }
  perf_provider_.Init();
}

void ChromeOSMetricsProvider::AsyncInit(const base::Closure& done_callback) {
  bool need_hardware_class = !base::FeatureList::IsEnabled(kUmaShortHWClass);

  base::RepeatingClosure barrier =
      base::BarrierClosure(need_hardware_class ? 2 : 1, done_callback);
  if (need_hardware_class)
    InitTaskGetHardwareClass(barrier);
  InitTaskGetBluetoothAdapter(barrier);
}

void ChromeOSMetricsProvider::OnDidCreateMetricsLog() {
  registered_user_count_at_log_initialization_ = false;
  if (user_manager::UserManager::IsInitialized()) {
    registered_user_count_at_log_initialization_ = true;
    user_count_at_log_initialization_ =
        user_manager::UserManager::Get()->GetLoggedInUsers().size();
  }
}

void ChromeOSMetricsProvider::InitTaskGetHardwareClass(
    const base::Closure& callback) {
  // Run the (potentially expensive) task in the background to avoid blocking
  // the UI thread.
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&GetFullHardwareClassOnBackgroundThread),
      base::BindOnce(&ChromeOSMetricsProvider::SetFullHardwareClass,
                     weak_ptr_factory_.GetWeakPtr(), callback));
}

void ChromeOSMetricsProvider::InitTaskGetBluetoothAdapter(
    const base::Closure& callback) {
  device::BluetoothAdapterFactory::GetAdapter(
      base::Bind(&ChromeOSMetricsProvider::SetBluetoothAdapter,
                 weak_ptr_factory_.GetWeakPtr(), callback));
}

void ChromeOSMetricsProvider::ProvideSystemProfileMetrics(
    metrics::SystemProfileProto* system_profile_proto) {
  WriteBluetoothProto(system_profile_proto);
  UpdateMultiProfileUserCount(system_profile_proto);

  metrics::SystemProfileProto::Hardware* hardware =
      system_profile_proto->mutable_hardware();
  hardware->set_hardware_class(hardware_class_);
  hardware->set_full_hardware_class(full_hardware_class_);
  display::Display::TouchSupport has_touch =
      ui::GetInternalDisplayTouchSupport();
  if (has_touch == display::Display::TouchSupport::AVAILABLE)
    hardware->set_internal_display_supports_touch(true);
  else if (has_touch == display::Display::TouchSupport::UNAVAILABLE)
    hardware->set_internal_display_supports_touch(false);
}

void ChromeOSMetricsProvider::ProvideStabilityMetrics(
    metrics::SystemProfileProto* system_profile_proto) {
  metrics::SystemProfileProto::Stability* stability_proto =
      system_profile_proto->mutable_stability();
  PrefService* pref = g_browser_process->local_state();
  int count = pref->GetInteger(prefs::kStabilityOtherUserCrashCount);
  if (count) {
    stability_proto->set_other_user_crash_count(count);
    pref->SetInteger(prefs::kStabilityOtherUserCrashCount, 0);
  }

  count = pref->GetInteger(prefs::kStabilityKernelCrashCount);
  if (count) {
    stability_proto->set_kernel_crash_count(count);
    pref->SetInteger(prefs::kStabilityKernelCrashCount, 0);
  }

  count = pref->GetInteger(prefs::kStabilitySystemUncleanShutdownCount);
  if (count) {
    stability_proto->set_unclean_system_shutdown_count(count);
    pref->SetInteger(prefs::kStabilitySystemUncleanShutdownCount, 0);
  }
}

void ChromeOSMetricsProvider::ProvidePreviousSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  ProvideStabilityMetrics(uma_proto->mutable_system_profile());
  // The enrollment status and ARC state of a client are not likely to change
  // between browser restarts.  Hence, it's safe and useful to attach these
  // values to a previous session log.
  RecordEnrollmentStatus();
  RecordArcState();
}

void ChromeOSMetricsProvider::ProvideCurrentSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  ProvideStabilityMetrics(uma_proto->mutable_system_profile());
  std::vector<SampledProfile> sampled_profiles;
  if (perf_provider_.GetSampledProfiles(&sampled_profiles)) {
    for (auto& profile : sampled_profiles) {
      uma_proto->add_sampled_profile()->Swap(&profile);
    }
  }

  RecordEnrollmentStatus();
  RecordArcState();
}

void ChromeOSMetricsProvider::WriteBluetoothProto(
    metrics::SystemProfileProto* system_profile_proto) {
  metrics::SystemProfileProto::Hardware* hardware =
      system_profile_proto->mutable_hardware();

  SystemProfileProto::Hardware::Bluetooth* bluetooth =
      hardware->mutable_bluetooth();

  bluetooth->set_is_present(adapter_->IsPresent());
  bluetooth->set_is_enabled(adapter_->IsPowered());

  device::BluetoothAdapter::DeviceList devices = adapter_->GetDevices();
  for (device::BluetoothAdapter::DeviceList::iterator iter = devices.begin();
       iter != devices.end();
       ++iter) {
    device::BluetoothDevice* device = *iter;
    // Don't collect information about LE devices yet.
    if (!device->IsPaired())
      continue;

    PairedDevice* paired_device = bluetooth->add_paired_device();
    paired_device->set_bluetooth_class(device->GetBluetoothClass());
    paired_device->set_type(AsBluetoothDeviceType(device->GetDeviceType()));

    // |address| is xx:xx:xx:xx:xx:xx, extract the first three components and
    // pack into a uint32_t.
    std::string address = device->GetAddress();
    if (address.size() > 9 && address[2] == ':' && address[5] == ':' &&
        address[8] == ':') {
      std::string vendor_prefix_str;
      uint64_t vendor_prefix;

      base::RemoveChars(address.substr(0, 9), ":", &vendor_prefix_str);
      DCHECK_EQ(6U, vendor_prefix_str.size());
      base::HexStringToUInt64(vendor_prefix_str, &vendor_prefix);

      paired_device->set_vendor_prefix(vendor_prefix);
    }

    switch (device->GetVendorIDSource()) {
      case device::BluetoothDevice::VENDOR_ID_BLUETOOTH:
        paired_device->set_vendor_id_source(PairedDevice::VENDOR_ID_BLUETOOTH);
        break;
      case device::BluetoothDevice::VENDOR_ID_USB:
        paired_device->set_vendor_id_source(PairedDevice::VENDOR_ID_USB);
        break;
      default:
        paired_device->set_vendor_id_source(PairedDevice::VENDOR_ID_UNKNOWN);
    }

    paired_device->set_vendor_id(device->GetVendorID());
    paired_device->set_product_id(device->GetProductID());
    paired_device->set_device_id(device->GetDeviceID());
  }
}

void ChromeOSMetricsProvider::UpdateMultiProfileUserCount(
    metrics::SystemProfileProto* system_profile_proto) {
  if (user_manager::UserManager::IsInitialized()) {
    size_t user_count =
        user_manager::UserManager::Get()->GetLoggedInUsers().size();

    // We invalidate the user count if it changed while the log was open.
    if (registered_user_count_at_log_initialization_ &&
        user_count != user_count_at_log_initialization_) {
      user_count = 0;
    }

    system_profile_proto->set_multi_profile_user_count(user_count);
  }
}

void ChromeOSMetricsProvider::SetBluetoothAdapter(
    base::Closure callback,
    scoped_refptr<device::BluetoothAdapter> adapter) {
  adapter_ = adapter;
  callback.Run();
}

void ChromeOSMetricsProvider::SetFullHardwareClass(
    base::Closure callback,
    std::string full_hardware_class) {
  if (!base::FeatureList::IsEnabled(kUmaShortHWClass)) {
    DCHECK(hardware_class_.empty());
    hardware_class_ = full_hardware_class;
  }
  full_hardware_class_ = full_hardware_class;
  callback.Run();
}

void ChromeOSMetricsProvider::RecordEnrollmentStatus() {
  UMA_STABILITY_HISTOGRAM_ENUMERATION(
      "UMA.EnrollmentStatus", GetEnrollmentStatus(), ENROLLMENT_STATUS_MAX);
}

void ChromeOSMetricsProvider::RecordArcState() {
  arc::ArcSessionManager* arc_session_manager = arc::ArcSessionManager::Get();
  if (arc_session_manager)
    arc_session_manager->RecordArcState();
}
