// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_INCOMPATIBLE_APPLICATIONS_UPDATER_WIN_H_
#define CHROME_BROWSER_CONFLICTS_INCOMPATIBLE_APPLICATIONS_UPDATER_WIN_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/conflicts/installed_applications_win.h"
#include "chrome/browser/conflicts/module_database_observer_win.h"
#include "chrome/browser/conflicts/proto/module_list.pb.h"

struct CertificateInfo;
class ModuleListFilter;
class PrefRegistrySimple;

// Maintains a list of incompatible applications that are installed on the
// machine. These applications cause unwanted DLLs to be loaded into Chrome.
//
// Because the list is expensive to build, it is cached into the Local State
// file so that it is available at startup.
class IncompatibleApplicationsUpdater : public ModuleDatabaseObserver {
 public:
  struct IncompatibleApplication {
    IncompatibleApplication(
        InstalledApplications::ApplicationInfo info,
        std::unique_ptr<chrome::conflicts::BlacklistAction> blacklist_action);
    ~IncompatibleApplication();

    // Needed for std::remove_if().
    IncompatibleApplication(IncompatibleApplication&& incompatible_application);
    IncompatibleApplication& operator=(
        IncompatibleApplication&& incompatible_application);

    InstalledApplications::ApplicationInfo info;
    std::unique_ptr<chrome::conflicts::BlacklistAction> blacklist_action;
  };

  // Creates an instance of the updater.
  // The parameters must outlive the lifetime of this class.
  IncompatibleApplicationsUpdater(
      const CertificateInfo& exe_certificate_info,
      const ModuleListFilter& module_list_filter,
      const InstalledApplications& installed_applications);
  ~IncompatibleApplicationsUpdater() override;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  // Returns true if the tracking of incompatible applications is enabled. The
  // return value will not change throughout the lifetime of the process.
  static bool IsWarningEnabled();

  // Returns true if the cache contains at least one incompatible application.
  // Only call this if IsIncompatibleApplicationsWarningEnabled() returns true.
  static bool HasCachedApplications();

  // Returns all the cached incompatible applications.
  // Only call this if IsIncompatibleApplicationsWarningEnabled() returns true.
  static std::vector<IncompatibleApplication> GetCachedApplications();

  // ModuleDatabaseObserver:
  void OnNewModuleFound(const ModuleInfoKey& module_key,
                        const ModuleInfoData& module_data) override;
  void OnModuleDatabaseIdle() override;

 private:
  const CertificateInfo& exe_certificate_info_;

  const ModuleListFilter& module_list_filter_;

  const InstalledApplications& installed_applications_;

  // Temporarily holds incompatible applications that were recently found.
  std::vector<IncompatibleApplication> incompatible_applications_;

  // Becomes false on the first call to OnModuleDatabaseIdle.
  bool before_first_idle_ = true;

  DISALLOW_COPY_AND_ASSIGN(IncompatibleApplicationsUpdater);
};

#endif  // CHROME_BROWSER_CONFLICTS_INCOMPATIBLE_APPLICATIONS_UPDATER_WIN_H_
