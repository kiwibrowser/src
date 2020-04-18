// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_APP_INSTALL_EVENT_LOG_COLLECTOR_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_APP_INSTALL_EVENT_LOG_COLLECTOR_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/arc/policy/arc_policy_bridge.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chromeos/dbus/power_manager_client.h"
#include "components/arc/common/policy.mojom.h"
#include "net/base/network_change_notifier.h"

class Profile;

namespace enterprise_management {
class AppInstallReportLogEvent;
}
namespace policy {

// Listens for and logs events related to app push-installs.
class AppInstallEventLogCollector
    : public chromeos::PowerManagerClient::Observer,
      public arc::ArcPolicyBridge::Observer,
      public net::NetworkChangeNotifier::NetworkChangeObserver,
      public ArcAppListPrefs::Observer {
 public:
  // The delegate that events are forwarded to for inclusion in the log.
  class Delegate {
   public:
    // Adds an identical log entry for every app whose push-install is pending.
    // The |event|'s timestamp is set to the current time if not set yet.
    virtual void AddForAllPackages(
        std::unique_ptr<enterprise_management::AppInstallReportLogEvent>
            event) = 0;

    // Adds a log entry for |package|. The |event|'s timestamp is set to the
    // current time if not set yet. If |gather_disk_space_info| is |true|,
    // information about total and free disk space is gathered in the background
    // and added to |event| before adding it to the log.
    virtual void Add(
        const std::string& package,
        bool gather_disk_space_info,
        std::unique_ptr<enterprise_management::AppInstallReportLogEvent>
            event) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  // Delegate must outlive |this|.
  AppInstallEventLogCollector(Delegate* delegate,
                              Profile* profile,
                              const std::set<std::string>& pending_packages);
  ~AppInstallEventLogCollector() override;

  // Called whenever the list of pending app-install requests changes.
  void OnPendingPackagesChanged(const std::set<std::string>& pending_packages);

  // Called in case of login and pending apps.
  void AddLoginEvent();

  // Called in case of logout and pending apps.
  void AddLogoutEvent();

  // chromeos::PowerManagerClient::Observer:
  void SuspendImminent(power_manager::SuspendImminent::Reason reason) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

  // net::NetworkChangeNotifier::NetworkChangeObserver:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // arc::ArcPolicyBridge::Observer:
  void OnCloudDpsRequested(base::Time time,
                           const std::set<std::string>& package_names) override;
  void OnCloudDpsSucceeded(base::Time time,
                           const std::set<std::string>& package_names) override;
  void OnCloudDpsFailed(base::Time time,
                        const std::string& package_name,
                        arc::mojom::InstallErrorReason reason) override;

  // ArcAppListPrefs::Observer:
  void OnInstallationStarted(const std::string& package_name) override;
  void OnInstallationFinished(const std::string& package_name,
                              bool success) override;

 private:
  Delegate* const delegate_;
  Profile* const profile_;

  // Whether the device is currently online.
  bool online_ = false;

  // Set of apps whose push-install is currently pending.
  std::set<std::string> pending_packages_;

  DISALLOW_COPY_AND_ASSIGN(AppInstallEventLogCollector);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_APP_INSTALL_EVENT_LOG_COLLECTOR_H_
