// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/crl_set_component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "net/cert/crl_set.h"
#include "net/ssl/ssl_config_service.h"

namespace component_updater {

namespace {

// kCrlSetPublicKeySHA256 is the SHA256 hash of the SubjectPublicKeyInfo of the
// key that's used to sign generated CRL sets.
static const uint8_t kCrlSetPublicKeySHA256[32] = {
    0x75, 0xda, 0xf8, 0xcb, 0x77, 0x68, 0x40, 0x33, 0x65, 0x4c, 0x97,
    0xe5, 0xc5, 0x1b, 0xcd, 0x81, 0x7b, 0x1e, 0xeb, 0x11, 0x2c, 0xe1,
    0xa4, 0x33, 0x8c, 0xf5, 0x72, 0x5e, 0xed, 0xb8, 0x43, 0x97,
};

void LoadCRLSet(const base::FilePath& crl_path) {
  base::AssertBlockingAllowed();
  scoped_refptr<net::CRLSet> crl_set;
  std::string crl_set_bytes;
  if (!base::ReadFileToString(crl_path, &crl_set_bytes) ||
      !net::CRLSet::Parse(crl_set_bytes, &crl_set)) {
    return;
  }
  net::SSLConfigService::SetCRLSetIfNewer(crl_set);
}

class CRLSetPolicy : public ComponentInstallerPolicy {
 public:
  CRLSetPolicy();
  ~CRLSetPolicy() override;

 private:
  // ComponentInstallerPolicy implementation.
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictionaryValue& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::DictionaryValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      std::unique_ptr<base::DictionaryValue> manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  std::vector<std::string> GetMimeTypes() const override;

  DISALLOW_COPY_AND_ASSIGN(CRLSetPolicy);
};

CRLSetPolicy::CRLSetPolicy() {}

CRLSetPolicy::~CRLSetPolicy() {}

bool CRLSetPolicy::SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool CRLSetPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result CRLSetPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);  // Nothing custom here.
}

void CRLSetPolicy::OnCustomUninstall() {}

bool CRLSetPolicy::VerifyInstallation(const base::DictionaryValue& manifest,
                                      const base::FilePath& install_dir) const {
  return base::PathExists(install_dir.Append(FILE_PATH_LITERAL("crl-set")));
}

void CRLSetPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  base::PostTaskWithTraits(
      FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
      base::BindOnce(&LoadCRLSet,
                     install_dir.Append(FILE_PATH_LITERAL("crl-set"))));
}

base::FilePath CRLSetPolicy::GetRelativeInstallDir() const {
  return base::FilePath(FILE_PATH_LITERAL("CertificateRevocation"));
}

void CRLSetPolicy::GetHash(std::vector<uint8_t>* hash) const {
  hash->assign(std::begin(kCrlSetPublicKeySHA256),
               std::end(kCrlSetPublicKeySHA256));
}

std::string CRLSetPolicy::GetName() const {
  return "CRLSet";
}

update_client::InstallerAttributes CRLSetPolicy::GetInstallerAttributes()
    const {
  return update_client::InstallerAttributes();
}

std::vector<std::string> CRLSetPolicy::GetMimeTypes() const {
  return std::vector<std::string>();
}

}  // namespace

void RegisterCRLSetComponent(ComponentUpdateService* cus,
                             const base::FilePath& user_data_dir) {
  auto installer = base::MakeRefCounted<ComponentInstaller>(
      std::make_unique<CRLSetPolicy>());
  installer->Register(cus, base::OnceClosure());
}

}  // namespace component_updater
