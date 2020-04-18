// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/third_party_conflicts_manager_win.h"

#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/conflicts/incompatible_applications_updater_win.h"
#include "chrome/browser/conflicts/installed_applications_win.h"
#include "chrome/browser/conflicts/module_database_win.h"
#include "chrome/browser/conflicts/module_info_util_win.h"
#include "chrome/browser/conflicts/module_list_filter_win.h"

namespace {

std::unique_ptr<CertificateInfo> CreateExeCertificateInfo() {
  auto certificate_info = std::make_unique<CertificateInfo>();

  base::FilePath exe_path;
  if (base::PathService::Get(base::FILE_EXE, &exe_path))
    GetCertificateInfo(exe_path, certificate_info.get());

  return certificate_info;
}

std::unique_ptr<ModuleListFilter> CreateModuleListFilter(
    const base::FilePath& module_list_path) {
  auto module_list_filter = std::make_unique<ModuleListFilter>();

  if (!module_list_filter->Initialize(module_list_path))
    return nullptr;

  return module_list_filter;
}

}  // namespace

ThirdPartyConflictsManager::ThirdPartyConflictsManager(
    ModuleDatabase* module_database)
    : module_database_(module_database),
      module_list_received_(false),
      on_module_database_idle_called_(false),
      weak_ptr_factory_(this) {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&CreateExeCertificateInfo),
      base::BindOnce(&ThirdPartyConflictsManager::OnExeCertificateCreated,
                     weak_ptr_factory_.GetWeakPtr()));
}

ThirdPartyConflictsManager::~ThirdPartyConflictsManager() = default;

void ThirdPartyConflictsManager::OnModuleDatabaseIdle() {
  if (on_module_database_idle_called_)
    return;

  on_module_database_idle_called_ = true;

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(
          []() { return std::make_unique<InstalledApplications>(); }),
      base::BindOnce(
          &ThirdPartyConflictsManager::OnInstalledApplicationsCreated,
          weak_ptr_factory_.GetWeakPtr()));
}

void ThirdPartyConflictsManager::LoadModuleList(const base::FilePath& path) {
  if (module_list_received_)
    return;

  module_list_received_ = true;

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&CreateModuleListFilter, path),
      base::BindOnce(&ThirdPartyConflictsManager::OnModuleListFilterCreated,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ThirdPartyConflictsManager::OnExeCertificateCreated(
    std::unique_ptr<CertificateInfo> exe_certificate_info) {
  exe_certificate_info_ = std::move(exe_certificate_info);

  if (module_list_filter_ && installed_applications_)
    InitializeIncompatibleApplicationsUpdater();
}

void ThirdPartyConflictsManager::OnModuleListFilterCreated(
    std::unique_ptr<ModuleListFilter> module_list_filter) {
  module_list_filter_ = std::move(module_list_filter);

  // A valid |module_list_filter_| is critical to the blocking of third-party
  // modules. By returning early here, the |incompatible_applications_updater_|
  // instance never gets created, thus disabling the identification of
  // incompatible applications.
  if (!module_list_filter_) {
    // Mark the module list as not received so that a new one may trigger the
    // creation of a valid filter.
    module_list_received_ = false;
    return;
  }

  if (exe_certificate_info_ && installed_applications_)
    InitializeIncompatibleApplicationsUpdater();
}

void ThirdPartyConflictsManager::OnInstalledApplicationsCreated(
    std::unique_ptr<InstalledApplications> installed_applications) {
  installed_applications_ = std::move(installed_applications);

  if (exe_certificate_info_ && module_list_filter_)
    InitializeIncompatibleApplicationsUpdater();
}

void ThirdPartyConflictsManager::InitializeIncompatibleApplicationsUpdater() {
  DCHECK(exe_certificate_info_);
  DCHECK(module_list_filter_);
  DCHECK(installed_applications_);

  incompatible_applications_updater_ =
      std::make_unique<IncompatibleApplicationsUpdater>(
          *exe_certificate_info_, *module_list_filter_,
          *installed_applications_);
  module_database_->AddObserver(incompatible_applications_updater_.get());
}
