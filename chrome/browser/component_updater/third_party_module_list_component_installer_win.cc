// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/third_party_module_list_component_installer_win.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "base/version.h"
#include "chrome/browser/conflicts/module_database_win.h"
#include "chrome/browser/conflicts/third_party_conflicts_manager_win.h"
#include "components/component_updater/component_updater_paths.h"
#include "content/public/browser/browser_thread.h"

using component_updater::ComponentUpdateService;

namespace {

// The relative path of the expected module list file inside of an installation
// of this component.
const wchar_t kRelativeModuleListPath[] = L"module_list_proto";

}  // namespace

namespace component_updater {

// The SHA256 of the SubjectPublicKeyInfo used to sign the component.
// The component id is: EHGIDPNDBLLACPJALKIIMKBADGJFNNMC
const uint8_t kThirdPartyModuleListPublicKeySHA256[32] = {
    0x47, 0x68, 0x3f, 0xd3, 0x1b, 0xb0, 0x2f, 0x90, 0xba, 0x88, 0xca,
    0x10, 0x36, 0x95, 0xdd, 0xc2, 0x29, 0xd1, 0x4f, 0x38, 0xf2, 0x9d,
    0x6c, 0x9c, 0x68, 0x6c, 0xa2, 0xa4, 0xa2, 0x8e, 0xa5, 0x5c};

// The name of the component. This is used in the chrome://components page.
const char kThirdPartyModuleListName[] = "Third Party Module List";

ThirdPartyModuleListComponentInstallerPolicy::
    ThirdPartyModuleListComponentInstallerPolicy(
        ThirdPartyConflictsManager* manager)
    : manager_(manager) {}

ThirdPartyModuleListComponentInstallerPolicy::
    ~ThirdPartyModuleListComponentInstallerPolicy() {}

bool ThirdPartyModuleListComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool ThirdPartyModuleListComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  // Public data is delivered via this component, no need for encryption.
  return false;
}

update_client::CrxInstaller::Result
ThirdPartyModuleListComponentInstallerPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);  // Nothing custom here.
}

void ThirdPartyModuleListComponentInstallerPolicy::OnCustomUninstall() {}

// NOTE: This is always called on the main UI thread. It is called once every
// startup to notify of an already installed component, and may be called
// repeatedly after that every time a new component is ready.
void ThirdPartyModuleListComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  // Forward the notification to the ThirdPartyConflictsManager on the current
  // (UI) thread. The manager is responsible for the work of actually loading
  // the module list, etc, on background threads.
  manager_->LoadModuleList(GetModuleListPath(install_dir));
}

bool ThirdPartyModuleListComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  // This is called during startup and installation before ComponentReady().
  // The component is considered valid if the expected file exists in the
  // installation.
  return base::PathExists(GetModuleListPath(install_dir));
}

base::FilePath
ThirdPartyModuleListComponentInstallerPolicy::GetRelativeInstallDir() const {
  static constexpr wchar_t kRelativeModuleListInstallDir[] =
      L"ThirdPartyModuleList"
#ifdef _WIN64
      "64";
#else
      "32";
#endif

  return base::FilePath(kRelativeModuleListInstallDir);
}

void ThirdPartyModuleListComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(std::begin(kThirdPartyModuleListPublicKeySHA256),
               std::end(kThirdPartyModuleListPublicKeySHA256));
}

std::string ThirdPartyModuleListComponentInstallerPolicy::GetName() const {
  return kThirdPartyModuleListName;
}

std::vector<std::string>
ThirdPartyModuleListComponentInstallerPolicy::GetMimeTypes() const {
  return std::vector<std::string>();
}

update_client::InstallerAttributes
ThirdPartyModuleListComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

// static
base::FilePath ThirdPartyModuleListComponentInstallerPolicy::GetModuleListPath(
    const base::FilePath& install_dir) {
  return install_dir.Append(kRelativeModuleListPath);
}

void RegisterThirdPartyModuleListComponent(ComponentUpdateService* cus) {
  DVLOG(1) << "Registering Third Party Module List component.";

  // Get a handle to the manager. This will only exist if the corresponding
  // feature is enabled.
  ModuleDatabase* database = ModuleDatabase::GetInstance();
  if (!database)
    return;
  ThirdPartyConflictsManager* manager =
      database->third_party_conflicts_manager();
  if (!manager)
    return;
  auto installer = base::MakeRefCounted<ComponentInstaller>(
      std::make_unique<ThirdPartyModuleListComponentInstallerPolicy>(manager));
  installer->Register(cus, base::OnceClosure());
}

}  // namespace component_updater
