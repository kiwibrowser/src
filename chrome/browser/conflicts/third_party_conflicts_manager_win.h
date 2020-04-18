// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_THIRD_PARTY_CONFLICTS_MANAGER_WIN_H_
#define CHROME_BROWSER_CONFLICTS_THIRD_PARTY_CONFLICTS_MANAGER_WIN_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/conflicts/module_database_observer_win.h"

struct CertificateInfo;
class InstalledApplications;
class ModuleDatabase;
class ModuleListFilter;
class IncompatibleApplicationsUpdater;

namespace base {
class FilePath;
}

// This class owns all the third-party conflicts-related classes and is
// responsible for their initialization.
class ThirdPartyConflictsManager : public ModuleDatabaseObserver {
 public:
  explicit ThirdPartyConflictsManager(ModuleDatabase* module_database);
  ~ThirdPartyConflictsManager() override;

  // ModuleDatabaseObserver:
  void OnModuleDatabaseIdle() override;

  // Loads the |module_list_filter_| using the Module List at |path|.
  void LoadModuleList(const base::FilePath& path);

 private:
  // Called when |exe_certificate_info_| finishes its initialization.
  void OnExeCertificateCreated(
      std::unique_ptr<CertificateInfo> exe_certificate_info);

  // Called when |module_list_filter_| finishes its initialization.
  void OnModuleListFilterCreated(
      std::unique_ptr<ModuleListFilter> module_list_filter);

  // Called when |installed_applications_| finishes its initialization.
  void OnInstalledApplicationsCreated(
      std::unique_ptr<InstalledApplications> installed_applications);

  // Initializes |incompatible_applications_updater_| when the
  // exe_certificate_info_, the module_list_filter_ and the
  // installed_applications_ are available.
  void InitializeIncompatibleApplicationsUpdater();

  ModuleDatabase* module_database_;

  // Indicates if the initial Module List has been received. Used to prevent the
  // creation of multiple ModuleListFilter instances.
  bool module_list_received_;

  // Indicates if the OnModuleDatabaseIdle() function has been called once
  // already. Used to prevent the creation of multiple InstalledApplications
  // instances.
  bool on_module_database_idle_called_;

  // The certificate info of the current executable.
  std::unique_ptr<CertificateInfo> exe_certificate_info_;

  // Filters third-party modules against a whitelist and a blacklist.
  std::unique_ptr<ModuleListFilter> module_list_filter_;

  // Retrieves the list of installed applications.
  std::unique_ptr<InstalledApplications> installed_applications_;

  // Maintains the cache of incompatible applications.
  std::unique_ptr<IncompatibleApplicationsUpdater>
      incompatible_applications_updater_;

  base::WeakPtrFactory<ThirdPartyConflictsManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThirdPartyConflictsManager);
};

#endif  // CHROME_BROWSER_CONFLICTS_THIRD_PARTY_CONFLICTS_MANAGER_WIN_H_
