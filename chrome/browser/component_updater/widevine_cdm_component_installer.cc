// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/native_library.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cdm_registry.h"
#include "content/public/common/cdm_info.h"
#include "crypto/sha2.h"
// TODO(crbug.com/825041): Move EncryptionMode out of decrypt_config and
// rename it to EncryptionScheme.
#include "media/base/decrypt_config.h"
#include "media/base/video_codecs.h"
#include "media/cdm/supported_cdm_versions.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"

#include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR. NOLINT

using content::BrowserThread;
using content::CdmRegistry;

namespace component_updater {

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_IS_COMPONENT)

namespace {

// CRX hash. The extension id is: oimompecagnajdejgnnjijobebaeigek.
const uint8_t kWidevineSha2Hash[] = {
    0xe8, 0xce, 0xcf, 0x42, 0x06, 0xd0, 0x93, 0x49, 0x6d, 0xd9, 0x89,
    0xe1, 0x41, 0x04, 0x86, 0x4a, 0x8f, 0xbd, 0x86, 0x12, 0xb9, 0x58,
    0x9b, 0xfb, 0x4f, 0xbb, 0x1b, 0xa9, 0xd3, 0x85, 0x37, 0xef};
static_assert(arraysize(kWidevineSha2Hash) == crypto::kSHA256Length,
              "Wrong hash length");

// Name of the Widevine CDM OS in the component manifest.
const char kWidevineCdmPlatform[] =
#if defined(OS_MACOSX)
    "mac";
#elif defined(OS_WIN)
    "win";
#else  // OS_LINUX, etc. TODO(viettrungluu): Separate out Chrome OS and Android?
    "linux";
#endif

// Name of the Widevine CDM architecture in the component manifest.
const char kWidevineCdmArch[] =
#if defined(ARCH_CPU_X86)
    "x86";
#elif defined(ARCH_CPU_X86_64)
    "x64";
#else  // TODO(viettrungluu): Support an ARM check?
    "???";
#endif

// The CDM manifest includes several custom values, all beginning with "x-cdm-".
// All values are strings.
// All values that are lists are delimited by commas. No trailing commas.
// For example, "1,2,4".
const char kCdmValueDelimiter[] = ",";

// The following entries are required.
//  Interface versions are lists of integers (e.g. "1" or "1,2,4").
//  These are checked in this file before registering the CDM.
//  All match the interface versions from content_decryption_module.h that the
//  CDM supports.
//    Matches CDM_MODULE_VERSION.
const char kCdmModuleVersionsName[] = "x-cdm-module-versions";
//    Matches supported ContentDecryptionModule_* version(s).
const char kCdmInterfaceVersionsName[] = "x-cdm-interface-versions";
//    Matches supported Host_* version(s).
const char kCdmHostVersionsName[] = "x-cdm-host-versions";
//  The codecs list is a list of simple codec names (e.g. "vp8,vorbis").
const char kCdmCodecsListName[] = "x-cdm-codecs";
//  Whether persistent license is supported by the CDM: "true" or "false".
const char kCdmPersistentLicenseSupportName[] =
    "x-cdm-persistent-license-support";
const char kCdmSupportedEncryptionSchemesName[] =
    "x-cdm-supported-encryption-schemes";

// The following strings are used to specify supported codecs in the
// parameter |kCdmCodecsListName|.
const char kCdmSupportedCodecVp8[] = "vp8";
const char kCdmSupportedCodecVp9[] = "vp9.0";
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
const char kCdmSupportedCodecAvc1[] = "avc1";
#endif

// The following strings are used to specify supported encryption schemes in
// the parameter |kCdmSupportedEncryptionSchemesName|.
const char kCdmSupportedEncryptionSchemeCenc[] = "cenc";
const char kCdmSupportedEncryptionSchemeCbcs[] = "cbcs";

// Widevine CDM is packaged as a multi-CRX. Widevine CDM binaries are located in
// _platform_specific/<platform_arch> folder in the package. This function
// returns the platform-specific subdirectory that is part of that multi-CRX.
base::FilePath GetPlatformDirectory(const base::FilePath& base_path) {
  std::string platform_arch = kWidevineCdmPlatform;
  platform_arch += '_';
  platform_arch += kWidevineCdmArch;
  return base_path.AppendASCII("_platform_specific").AppendASCII(platform_arch);
}

typedef bool (*VersionCheckFunc)(int version);

bool CheckForCompatibleVersion(const base::DictionaryValue& manifest,
                               const std::string version_name,
                               VersionCheckFunc version_check_func) {
  std::string versions_string;
  if (!manifest.GetString(version_name, &versions_string)) {
    DVLOG(1) << "Widevine CDM component manifest missing " << version_name;
    return false;
  }
  DVLOG_IF(1, versions_string.empty())
      << "Widevine CDM component manifest has empty " << version_name;

  for (const base::StringPiece& ver_str :
       base::SplitStringPiece(versions_string, kCdmValueDelimiter,
                              base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    int version = 0;
    if (base::StringToInt(ver_str, &version))
      if (version_check_func(version))
        return true;
  }

  DVLOG(1) << "Widevine CDM component manifest has no supported "
           << version_name << " in '" << versions_string << "'";
  return false;
}

// Returns whether the CDM's API versions, as specified in the manifest, are
// supported in this Chrome binary and not disabled at run time.
// Checks the module API, CDM interface API, and Host API.
// This should never fail except in rare cases where the component has not been
// updated recently or the user downgrades Chrome.
bool IsCompatibleWithChrome(const base::DictionaryValue& manifest) {
  return CheckForCompatibleVersion(manifest, kCdmModuleVersionsName,
                                   media::IsSupportedCdmModuleVersion) &&
         CheckForCompatibleVersion(
             manifest, kCdmInterfaceVersionsName,
             media::IsSupportedAndEnabledCdmInterfaceVersion) &&
         CheckForCompatibleVersion(manifest, kCdmHostVersionsName,
                                   media::IsSupportedCdmHostVersion);
}

// Returns true and updates |supported_video_codecs| (if provided) if the
// appropriate manifest entry is valid. Returns false and does not modify
// |supported_video_codecs| if the manifest entry is incorrectly formatted.
bool GetCodecs(const base::DictionaryValue& manifest,
               std::vector<media::VideoCodec>* supported_video_codecs) {
  const base::Value* value = manifest.FindKey(kCdmCodecsListName);
  if (!value) {
    DLOG(WARNING) << "Widevine CDM component manifest is missing codecs.";
    return true;
  }

  if (!value->is_string()) {
    DLOG(ERROR) << "Manifest entry " << kCdmCodecsListName
                << " is not a string.";
    return false;
  }

  const std::string& codecs = value->GetString();
  if (codecs.empty()) {
    DLOG(WARNING) << "Widevine CDM component manifest has empty codecs list.";
    return true;
  }

  std::vector<media::VideoCodec> result;
  const std::vector<base::StringPiece> supported_codecs =
      base::SplitStringPiece(codecs, kCdmValueDelimiter, base::TRIM_WHITESPACE,
                             base::SPLIT_WANT_NONEMPTY);

  for (const auto& codec : supported_codecs) {
    if (codec == kCdmSupportedCodecVp8)
      result.push_back(media::VideoCodec::kCodecVP8);
    else if (codec == kCdmSupportedCodecVp9)
      result.push_back(media::VideoCodec::kCodecVP9);
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
    else if (codec == kCdmSupportedCodecAvc1)
      result.push_back(media::VideoCodec::kCodecH264);
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)
  }

  if (supported_video_codecs)
    supported_video_codecs->swap(result);
  return true;
}

// Returns true and updates |supports_persistent_license| (if provided) if
// the appropriate manifest entry is valid. Returns false if the manifest
// entry is incorrectly formatted.
bool GetPersistentLicenseSupport(const base::DictionaryValue& manifest,
                                 bool* supports_persistent_license) {
  bool result = false;
  const base::Value* value = manifest.FindKey(kCdmPersistentLicenseSupportName);
  if (value) {
    if (value->is_bool())
      result = value->GetBool();
    else
      return false;
  }

  if (supports_persistent_license)
    *supports_persistent_license = result;
  return true;
}

// Returns true and updates |supported_encryption_schemes| (if provided) if
// the appropriate manifest entry is valid. Returns false and does not modify
// |supported_encryption_schemes| if the manifest entry is incorrectly
// formatted. It is assumed that all CDMs support 'cenc', so if the manifest
// entry is missing, the result will indicate support for 'cenc' only.
// Incorrect types in the manifest entry will log the error and fail.
// Unrecognized values will be reported but otherwise ignored.
bool GetEncryptionSchemes(
    const base::DictionaryValue& manifest,
    base::flat_set<media::EncryptionMode>* supported_encryption_schemes) {
  const base::Value* value =
      manifest.FindKey(kCdmSupportedEncryptionSchemesName);
  if (!value) {
    // No manifest entry found, so assume only 'cenc' supported for backwards
    // compatibility.
    if (supported_encryption_schemes)
      supported_encryption_schemes->insert(media::EncryptionMode::kCenc);
    return true;
  }

  if (!value->is_list()) {
    DLOG(ERROR) << "Manifest entry " << kCdmSupportedEncryptionSchemesName
                << " is not a list.";
    return false;
  }

  const base::Value::ListStorage& list = value->GetList();
  base::flat_set<media::EncryptionMode> result;
  for (const auto& item : list) {
    if (!item.is_string()) {
      DLOG(ERROR) << "Unrecognized item type in manifest entry "
                  << kCdmSupportedEncryptionSchemesName;
      return false;
    }

    const std::string& scheme = item.GetString();
    if (scheme == kCdmSupportedEncryptionSchemeCenc) {
      result.insert(media::EncryptionMode::kCenc);
    } else if (scheme == kCdmSupportedEncryptionSchemeCbcs) {
      result.insert(media::EncryptionMode::kCbcs);
    } else {
      DLOG(WARNING) << "Unrecognized encryption scheme " << scheme
                    << " in manifest entry "
                    << kCdmSupportedEncryptionSchemesName;
    }
  }

  // As the manifest entry exists, it must specify at least one valid value.
  if (result.empty())
    return false;

  if (supported_encryption_schemes)
    supported_encryption_schemes->swap(result);
  return true;
}

// Returns true if the entries in the manifest can be parsed correctly,
// false otherwise. Updates |supported_video_codecs|,
// |supports_persistent_license|, and |supported_encryption_schemes|,
// with the values obtained from the manifest, if they are provided.
// If this method returns false, the values may or may not be updated.
bool ParseManifest(
    const base::DictionaryValue& manifest,
    std::vector<media::VideoCodec>* supported_video_codecs,
    bool* supports_persistent_license,
    base::flat_set<media::EncryptionMode>* supported_encryption_schemes) {
  return GetEncryptionSchemes(manifest, supported_encryption_schemes) &&
         GetPersistentLicenseSupport(manifest, supports_persistent_license) &&
         GetCodecs(manifest, supported_video_codecs);
}

void RegisterWidevineCdmWithChrome(
    const base::Version& cdm_version,
    const base::FilePath& cdm_install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::vector<media::VideoCodec> supported_video_codecs;
  bool supports_persistent_license;
  base::flat_set<media::EncryptionMode> supported_encryption_schemes;

  // This check must be a subset of the check in VerifyInstallation() to
  // avoid the case where the CDM is accepted by the component updater
  // but not registered.
  if (!ParseManifest(*manifest, &supported_video_codecs,
                     &supports_persistent_license,
                     &supported_encryption_schemes)) {
    VLOG(1) << "Not registering Widevine CDM due to malformed manifest.";
    return;
  }

  VLOG(1) << "Register Widevine CDM with Chrome";

  const base::FilePath cdm_path =
      GetPlatformDirectory(cdm_install_dir)
          .AppendASCII(base::GetNativeLibraryName(kWidevineCdmLibraryName));
  CdmRegistry::GetInstance()->RegisterCdm(content::CdmInfo(
      kWidevineCdmDisplayName, kWidevineCdmGuid, cdm_version, cdm_path,
      kWidevineCdmFileSystemId, supported_video_codecs,
      supports_persistent_license, supported_encryption_schemes,
      kWidevineKeySystem, false));
}

}  // namespace

class WidevineCdmComponentInstallerPolicy : public ComponentInstallerPolicy {
 public:
  WidevineCdmComponentInstallerPolicy();
  ~WidevineCdmComponentInstallerPolicy() override {}

 private:
  // The following methods override ComponentInstallerPolicy.
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictionaryValue& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::DictionaryValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& path,
                      std::unique_ptr<base::DictionaryValue> manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  std::vector<std::string> GetMimeTypes() const override;

  // Updates CDM path if necessary.
  void UpdateCdmPath(const base::Version& cdm_version,
                     const base::FilePath& cdm_install_dir,
                     std::unique_ptr<base::DictionaryValue> manifest);

  DISALLOW_COPY_AND_ASSIGN(WidevineCdmComponentInstallerPolicy);
};

WidevineCdmComponentInstallerPolicy::WidevineCdmComponentInstallerPolicy() {}

bool WidevineCdmComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool WidevineCdmComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
WidevineCdmComponentInstallerPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void WidevineCdmComponentInstallerPolicy::OnCustomUninstall() {}

// Once the CDM is ready, update the CDM path.
void WidevineCdmComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    std::unique_ptr<base::DictionaryValue> manifest) {
  if (!IsCompatibleWithChrome(*manifest)) {
    VLOG(1) << "Installed Widevine CDM component is incompatible.";
    return;
  }

  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&WidevineCdmComponentInstallerPolicy::UpdateCdmPath,
                     base::Unretained(this), version, path,
                     base::Passed(&manifest)));
}

bool WidevineCdmComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  return IsCompatibleWithChrome(manifest) &&
         base::PathExists(GetPlatformDirectory(install_dir)
                              .AppendASCII(base::GetNativeLibraryName(
                                  kWidevineCdmLibraryName))) &&
         ParseManifest(manifest, nullptr, nullptr, nullptr);
}

// The base directory on Windows looks like:
// <profile>\AppData\Local\Google\Chrome\User Data\WidevineCdm\.
base::FilePath WidevineCdmComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(kWidevineCdmBaseDirectory);
}

void WidevineCdmComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kWidevineSha2Hash,
               kWidevineSha2Hash + arraysize(kWidevineSha2Hash));
}

std::string WidevineCdmComponentInstallerPolicy::GetName() const {
  return kWidevineCdmDisplayName;
}

update_client::InstallerAttributes
WidevineCdmComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string> WidevineCdmComponentInstallerPolicy::GetMimeTypes()
    const {
  return std::vector<std::string>();
}

void WidevineCdmComponentInstallerPolicy::UpdateCdmPath(
    const base::Version& cdm_version,
    const base::FilePath& cdm_install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  // On some platforms (e.g. Mac) we use symlinks for paths. Convert paths to
  // absolute paths to avoid unexpected failure. base::MakeAbsoluteFilePath()
  // requires IO so it can only be done in this function.
  const base::FilePath absolute_cdm_install_dir =
      base::MakeAbsoluteFilePath(cdm_install_dir);
  if (absolute_cdm_install_dir.empty()) {
    PLOG(WARNING) << "Failed to get absolute CDM install path.";
    return;
  }

  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)
      ->PostTask(
          FROM_HERE,
          base::BindOnce(&RegisterWidevineCdmWithChrome, cdm_version,
                         absolute_cdm_install_dir, base::Passed(&manifest)));
}

#endif  // defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_IS_COMPONENT)

void RegisterWidevineCdmComponent(ComponentUpdateService* cus) {
#if defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_IS_COMPONENT)
  auto installer = base::MakeRefCounted<ComponentInstaller>(
      std::make_unique<WidevineCdmComponentInstallerPolicy>());
  installer->Register(cus, base::OnceClosure());
#endif  // defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_IS_COMPONENT)
}

}  // namespace component_updater
