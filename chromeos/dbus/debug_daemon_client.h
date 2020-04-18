// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_DEBUG_DAEMON_CLIENT_H_
#define CHROMEOS_DBUS_DEBUG_DAEMON_CLIENT_H_

#include <stdint.h>
#include <sys/types.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task_runner.h"
#include "base/trace_event/tracing_agent.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

// DebugDaemonClient is used to communicate with the debug daemon.
class CHROMEOS_EXPORT DebugDaemonClient
    : public DBusClient,
      public base::trace_event::TracingAgent {
 public:
  ~DebugDaemonClient() override;

  // Requests to store debug logs into |file_descriptor| and calls |callback|
  // when completed. Debug logs will be stored in the .tgz if
  // |is_compressed| is true, otherwise in logs will be stored in .tar format.
  // This method duplicates |file_descriptor| so it's OK to close the FD without
  // waiting for the result.
  virtual void DumpDebugLogs(bool is_compressed,
                             int file_descriptor,
                             VoidDBusMethodCallback callback) = 0;

  // Requests to change debug mode to given |subsystem| and calls
  // |callback| when completed. |subsystem| should be one of the
  // following: "wifi", "ethernet", "cellular" or "none".
  virtual void SetDebugMode(const std::string& subsystem,
                            VoidDBusMethodCallback callback) = 0;

  // Gets information about routes.
  virtual void GetRoutes(
      bool numeric,
      bool ipv6,
      DBusMethodCallback<std::vector<std::string> /* routes */> callback) = 0;

  // Gets information about network status as json.
  virtual void GetNetworkStatus(DBusMethodCallback<std::string> callback) = 0;

  // Gets information about modem status as json.
  virtual void GetModemStatus(DBusMethodCallback<std::string> callback) = 0;

  // Gets information about WiMAX status as json.
  virtual void GetWiMaxStatus(DBusMethodCallback<std::string> callback) = 0;

  // Gets information about network interfaces as json.
  // For details, please refer to
  // http://gerrit.chromium.org/gerrit/#/c/28045/5/src/helpers/netif.cc
  virtual void GetNetworkInterfaces(
      DBusMethodCallback<std::string> callback) = 0;

  // Runs perf (via quipper) with arguments for |duration| (converted to
  // seconds) and returns data collected over the passed |file_descriptor|.
  // |callback| is called on the completion of the D-Bus call.
  // Note that quipper failures may occur after successfully running the D-Bus
  // method. Such errors can be detected by |file_descriptor| and all its
  // duplicates being closed with no data written.
  // This method duplicates |file_descriptor| so it's OK to close the FD without
  // waiting for the result.
  virtual void GetPerfOutput(base::TimeDelta duration,
                             const std::vector<std::string>& perf_args,
                             int file_descriptor,
                             VoidDBusMethodCallback callback) = 0;

  // Callback type for GetScrubbedLogs(), GetAllLogs() or GetUserLogFiles().
  using GetLogsCallback =
      base::Callback<void(bool succeeded,
                          const std::map<std::string, std::string>& logs)>;

  // Gets scrubbed logs from debugd.
  virtual void GetScrubbedLogs(const GetLogsCallback& callback) = 0;

  // Gets the scrubbed logs from debugd that are very large and cannot be
  // returned directly from D-Bus. These logs will include ARC and cheets
  // system information.
  virtual void GetScrubbedBigLogs(const GetLogsCallback& callback) = 0;

  // Gets all logs collected by debugd.
  virtual void GetAllLogs(const GetLogsCallback& callback) = 0;

  // Gets list of user log files that must be read by Chrome.
  virtual void GetUserLogFiles(const GetLogsCallback& callback) = 0;

  // Gets an individual log source provided by debugd.
  virtual void GetLog(const std::string& log_name,
                      DBusMethodCallback<std::string> callback) = 0;

  virtual void SetStopAgentTracingTaskRunner(
      scoped_refptr<base::TaskRunner> task_runner) = 0;

  // Called once TestICMP() is complete. Takes two parameters:
  // - succeeded: information was obtained successfully.
  // - status: information about ICMP connectivity to a specified host as json.
  //   For details please refer to
  //   https://gerrit.chromium.org/gerrit/#/c/30310/2/src/helpers/icmp.cc
  typedef base::Callback<void(bool succeeded, const std::string& status)>
      TestICMPCallback;

  // Tests ICMP connectivity to a specified host. The |ip_address| contains the
  // IPv4 or IPv6 address of the host, for example "8.8.8.8".
  virtual void TestICMP(const std::string& ip_address,
                        const TestICMPCallback& callback) = 0;

  // Tests ICMP connectivity to a specified host. The |ip_address| contains the
  // IPv4 or IPv6 address of the host, for example "8.8.8.8".
  virtual void TestICMPWithOptions(
      const std::string& ip_address,
      const std::map<std::string, std::string>& options,
      const TestICMPCallback& callback) = 0;

  // Called once EnableDebuggingFeatures() is complete. |succeeded| will be true
  // if debugging features have been successfully enabled.
  typedef base::Callback<void(bool succeeded)> EnableDebuggingCallback;

  // Enables debugging features (sshd, boot from USB). |password| is a new
  // password for root user. Can be only called in dev mode.
  virtual void EnableDebuggingFeatures(
      const std::string& password,
      const EnableDebuggingCallback& callback) = 0;

  static const int DEV_FEATURE_NONE = 0;
  static const int DEV_FEATURE_ALL_ENABLED =
      debugd::DevFeatureFlag::DEV_FEATURE_ROOTFS_VERIFICATION_REMOVED |
      debugd::DevFeatureFlag::DEV_FEATURE_BOOT_FROM_USB_ENABLED |
      debugd::DevFeatureFlag::DEV_FEATURE_SSH_SERVER_CONFIGURED |
      debugd::DevFeatureFlag::DEV_FEATURE_DEV_MODE_ROOT_PASSWORD_SET;

  // Called once QueryDebuggingFeatures() is complete. |succeeded| will be true
  // if debugging features have been successfully enabled. |feature_mask| is a
  // bitmask made out of DebuggingFeature enum values.
  typedef base::Callback<void(bool succeeded,
                              int feature_mask)> QueryDevFeaturesCallback;
  // Checks which debugging features have been already enabled.
  virtual void QueryDebuggingFeatures(
      const QueryDevFeaturesCallback& callback) = 0;

  // Removes rootfs verification from the file system. Can be only called in
  // dev mode.
  virtual void RemoveRootfsVerification(
      const EnableDebuggingCallback& callback) = 0;

  // Trigger uploading of crashes.
  virtual void UploadCrashes() = 0;

  // Runs the callback as soon as the service becomes available.
  virtual void WaitForServiceToBeAvailable(
      WaitForServiceToBeAvailableCallback callback) = 0;

  // A callback for SetOomScoreAdj().
  typedef base::Callback<void(bool success, const std::string& output)>
      SetOomScoreAdjCallback;

  // Set OOM score oom_score_adj for some process.
  // Note that the corresponding DBus configuration of the debugd method
  // "SetOomScoreAdj" only permits setting OOM score for processes running by
  // user chronos or Android apps.
  virtual void SetOomScoreAdj(
      const std::map<pid_t, int32_t>& pid_to_oom_score_adj,
      const SetOomScoreAdjCallback& callback) = 0;

  // A callback to handle the result of CupsAdd[Auto|Manually]ConfiguredPrinter.
  // A zero status means success, non-zero statuses are used to convey different
  // errors.
  using CupsAddPrinterCallback = DBusMethodCallback<int32_t>;

  // Calls CupsAddManuallyConfiguredPrinter.  |name| is the printer
  // name. |uri| is the device.  |ppd_contents| is the contents of the
  // PPD file used to drive the device.  |callback| is called with
  // true if adding the printer to CUPS was successful and false if
  // there was an error.  |error_callback| will be called if there was
  // an error in communicating with debugd.
  virtual void CupsAddManuallyConfiguredPrinter(
      const std::string& name,
      const std::string& uri,
      const std::string& ppd_contents,
      CupsAddPrinterCallback callback) = 0;

  // Calls CupsAddAutoConfiguredPrinter.  |name| is the printer
  // name. |uri| is the device.  |callback| is called with true if
  // adding the printer to CUPS was successful and false if there was
  // an error.  |error_callback| will be called if there was an error
  // in communicating with debugd.
  virtual void CupsAddAutoConfiguredPrinter(
      const std::string& name,
      const std::string& uri,
      CupsAddPrinterCallback callback) = 0;

  // A callback to handle the result of CupsRemovePrinter.
  using CupsRemovePrinterCallback = base::Callback<void(bool success)>;

  // Calls CupsRemovePrinter.  |name| is the printer name as registered in
  // CUPS.  |callback| is called with true if removing the printer from CUPS was
  // successful and false if there was an error.  |error_callback| will be
  // called if there was an error in communicating with debugd.
  virtual void CupsRemovePrinter(const std::string& name,
                                 const CupsRemovePrinterCallback& callback,
                                 const base::Closure& error_callback) = 0;

  // A callback to handle the result of StartConcierge/StopConcierge.
  using ConciergeCallback = base::OnceCallback<void(bool success)>;
  // Calls debugd::kStartVmConcierge, which starts the Concierge service.
  // |callback| is called when the method finishes.
  virtual void StartConcierge(ConciergeCallback callback) = 0;
  // Calls debugd::kStopVmConcierge, which stops the Concierge service.
  // |callback| is called when the method finishes.
  virtual void StopConcierge(ConciergeCallback callback) = 0;

  // A callback to handle the result of SetRlzPingSent.
  using SetRlzPingSentCallback = base::OnceCallback<void(bool success)>;
  // Calls debugd::kSetRlzPingSent, which sets |should_send_rlz_ping| in RW_VPD
  // to 0.
  virtual void SetRlzPingSent(SetRlzPingSentCallback callback) = 0;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static DebugDaemonClient* Create();

 protected:
  // Create() should be used instead.
  DebugDaemonClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(DebugDaemonClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_DEBUG_DAEMON_CLIENT_H_
