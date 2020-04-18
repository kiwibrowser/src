// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/recovery_improved_component_installer.h"

#include <iterator>
#include <utility>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "chrome/browser/component_updater/component_updater_utils.h"

// This component is behind a Finch experiment. To enable the registration of
// the component, run Chrome with --enable-features=ImprovedRecoveryComponent.
namespace component_updater {

// The SHA256 of the SubjectPublicKeyInfo used to sign the component CRX.
// The component id is: ihnlcenocehgdaegdmhbidjhnhdchfmm
constexpr uint8_t kRecoveryImprovedPublicKeySHA256[32] = {
    0x87, 0xdb, 0x24, 0xde, 0x24, 0x76, 0x30, 0x46, 0x3c, 0x71, 0x83,
    0x97, 0xd7, 0x32, 0x75, 0xcc, 0xd5, 0x7f, 0xec, 0x09, 0x60, 0x6d,
    0x20, 0xc3, 0x81, 0xd7, 0xce, 0x7b, 0x10, 0x15, 0x44, 0xd1};

RecoveryImprovedInstallerPolicy::RecoveryImprovedInstallerPolicy(
    PrefService* prefs)
    : prefs_(prefs) {}

RecoveryImprovedInstallerPolicy::~RecoveryImprovedInstallerPolicy() {}

bool RecoveryImprovedInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool RecoveryImprovedInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
RecoveryImprovedInstallerPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void RecoveryImprovedInstallerPolicy::OnCustomUninstall() {}

void RecoveryImprovedInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  DVLOG(1) << "RecoveryImproved component is ready.";
}

// Called during startup and installation before ComponentReady().
bool RecoveryImprovedInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath RecoveryImprovedInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath(FILE_PATH_LITERAL("RecoveryImproved"));
}

void RecoveryImprovedInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(std::begin(kRecoveryImprovedPublicKeySHA256),
               std::end(kRecoveryImprovedPublicKeySHA256));
}

std::string RecoveryImprovedInstallerPolicy::GetName() const {
  return "Chrome Improved Recovery";
}

update_client::InstallerAttributes
RecoveryImprovedInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string> RecoveryImprovedInstallerPolicy::GetMimeTypes() const {
  return std::vector<std::string>();
}

void RegisterRecoveryImprovedComponent(ComponentUpdateService* cus,
                                       PrefService* prefs) {
#if defined(GOOGLE_CHROME_BUILD)
#if defined(OS_WIN) || defined(OS_MACOSX)
  // The improved recovery components requires elevation in the case where
  // Chrome is installed per-machine. The elevation mechanism is not implemented
  // yet; therefore, the component is not registered in this case.
  if (!IsPerUserInstall())
    return;

  DVLOG(1) << "Registering RecoveryImproved component.";

  // |cus| takes ownership of |installer| through the CrxComponent instance.
  auto installer = base::MakeRefCounted<ComponentInstaller>(
      std::make_unique<RecoveryImprovedInstallerPolicy>(prefs));
  installer->Register(cus, base::OnceClosure());
#endif
#endif
}

void RegisterPrefsForRecoveryImprovedComponent(PrefRegistrySimple* registry) {}

}  // namespace component_updater
