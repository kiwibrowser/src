// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_CHROMEOS_METRICS_PROVIDER_H_
#define CHROME_BROWSER_METRICS_CHROMEOS_METRICS_PROVIDER_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/metrics/perf/perf_provider_chromeos.h"
#include "components/metrics/metrics_provider.h"

namespace device {
class BluetoothAdapter;
}

namespace metrics {
class ChromeUserMetricsExtension;
}

class PrefRegistrySimple;

// Performs ChromeOS specific metrics logging.
class ChromeOSMetricsProvider : public metrics::MetricsProvider {
 public:
  // Possible device enrollment status for a Chrome OS device.
  // Used by UMA histogram, so entries shouldn't be reordered or removed.
  enum EnrollmentStatus {
    NON_MANAGED,
    UNUSED,  // Formerly MANAGED_EDU, see crbug.com/462770.
    MANAGED,
    ERROR_GETTING_ENROLLMENT_STATUS,
    ENROLLMENT_STATUS_MAX,
  };

  ChromeOSMetricsProvider();
  ~ChromeOSMetricsProvider() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Records a crash.
  static void LogCrash(const std::string& crash_type);

  // Returns Enterprise Enrollment status.
  static EnrollmentStatus GetEnrollmentStatus();

  // Loads hardware class information. When this task is complete, |callback|
  // is run.
  void InitTaskGetHardwareClass(const base::Closure& callback);

  // Creates the Bluetooth adapter. When this task is complete, |callback| is
  // run.
  void InitTaskGetBluetoothAdapter(const base::Closure& callback);

  // metrics::MetricsProvider:
  void Init() override;
  void AsyncInit(const base::Closure& done_callback) override;
  void OnDidCreateMetricsLog() override;
  void ProvideSystemProfileMetrics(
      metrics::SystemProfileProto* system_profile_proto) override;
  void ProvideStabilityMetrics(
      metrics::SystemProfileProto* system_profile_proto) override;
  void ProvidePreviousSessionData(
      metrics::ChromeUserMetricsExtension* uma_proto) override;
  void ProvideCurrentSessionData(
      metrics::ChromeUserMetricsExtension* uma_proto) override;

 private:
  // Update the number of users logged into a multi-profile session.
  // If the number of users change while the log is open, the call invalidates
  // the user count value.
  void UpdateMultiProfileUserCount(
      metrics::SystemProfileProto* system_profile_proto);

  // Sets the Bluetooth Adapter instance used for the WriteBluetoothProto()
  // call and calls callback.
  void SetBluetoothAdapter(base::Closure callback,
                           scoped_refptr<device::BluetoothAdapter> adapter);

  // Sets the full hardware class, then calls the callback.
  void SetFullHardwareClass(base::Closure callback,
                            std::string full_hardware_class);

  // Writes info about paired Bluetooth devices on this system.
  void WriteBluetoothProto(metrics::SystemProfileProto* system_profile_proto);

  // Record the device enrollment status.
  void RecordEnrollmentStatus();

  // Record whether ARC is enabled or not for ARC capable devices.
  void RecordArcState();

  // For collecting systemwide perf data.
  metrics::PerfProvider perf_provider_;

  // Bluetooth Adapter instance for collecting information about paired devices.
  scoped_refptr<device::BluetoothAdapter> adapter_;

  // Whether the user count was registered at the last log initialization.
  bool registered_user_count_at_log_initialization_;

  // The user count at the time that a log was last initialized. Contains a
  // valid value only if |registered_user_count_at_log_initialization_| is
  // true.
  uint64_t user_count_at_log_initialization_;

  // Short Hardware class. This value identifies the board of the hardware.
  std::string hardware_class_;

  // Hardware class (e.g., hardware qualification ID). This value identifies
  // the configured system components such as CPU, WiFi adapter, etc.
  std::string full_hardware_class_;

  base::WeakPtrFactory<ChromeOSMetricsProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromeOSMetricsProvider);
};

#endif  // CHROME_BROWSER_METRICS_CHROMEOS_METRICS_PROVIDER_H_
