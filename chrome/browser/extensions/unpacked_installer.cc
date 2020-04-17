// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/unpacked_installer.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/extensions/extension_management.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/load_error_reporter.h"
#include "chrome/browser/extensions/permissions_updater.h"
#include "chrome/browser/profiles/profile.h"
#include "components/crx_file/id_util.h"
#include "components/sync/model/string_ordinal.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/declarative_net_request/utils.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/install_flag.h"
#include "extensions/browser/path_util.h"
#include "extensions/browser/policy_check.h"
#include "extensions/browser/preload_check_group.h"
#include "extensions/browser/requirements_checker.h"
#include "extensions/common/api/declarative_net_request/dnr_manifest_data.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_l10n_util.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/shared_module_info.h"
#include "extensions/common/permissions/permissions_data.h"

using content::BrowserThread;
using extensions::Extension;
using extensions::SharedModuleInfo;

namespace extensions {

namespace {

const char kUnpackedExtensionsBlacklistedError[] =
    "Loading of unpacked extensions is disabled by the administrator.";

const char kImportMinVersionNewer[] =
    "'import' version requested is newer than what is installed.";
const char kImportMissing[] = "'import' extension is not installed.";
const char kImportNotSharedModule[] = "'import' is not a shared module.";

// Deletes files reserved for use by the Extension system in the kMetadataFolder
// and the kMetadataFolder itself if it is empty.
void MaybeCleanupMetadataFolder(const base::FilePath& extension_path) {
  const std::vector<base::FilePath> reserved_filepaths =
      file_util::GetReservedMetadataFilePaths(extension_path);
  for (const auto& file : reserved_filepaths)
    base::DeleteFile(file, false /*recursive*/);

  const base::FilePath& metadata_dir = extension_path.Append(kMetadataFolder);
  if (base::IsDirectoryEmpty(metadata_dir))
    base::DeleteFile(metadata_dir, true /*recursive*/);
}

}  // namespace

// static
scoped_refptr<UnpackedInstaller> UnpackedInstaller::Create(
    ExtensionService* extension_service) {
  DCHECK(extension_service);
  return scoped_refptr<UnpackedInstaller>(
      new UnpackedInstaller(extension_service));
}

UnpackedInstaller::UnpackedInstaller(ExtensionService* extension_service)
    : service_weak_(extension_service->AsWeakPtr()),
      profile_(extension_service->profile()),
      require_modern_manifest_version_(true),
      be_noisy_on_failure_(true) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

UnpackedInstaller::~UnpackedInstaller() {
}

void UnpackedInstaller::Load(const base::FilePath& path_in) {
  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::Load: " << path_in << " - Step 6";
  DCHECK(extension_path_.empty());
  extension_path_ = path_in;
  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::Load: " << path_in << " - Step 7";
  GetExtensionFileTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&UnpackedInstaller::GetAbsolutePath, this));
  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::Load: " << path_in << " - Step 8";
}

bool UnpackedInstaller::LoadFromCommandLine(const base::FilePath& path_in,
                                            std::string* extension_id,
                                            bool only_allow_apps) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(extension_path_.empty());

  if (!service_weak_.get())
    return false;
  // Load extensions from the command line synchronously to avoid a race
  // between extension loading and loading an URL from the command line.
  base::ThreadRestrictions::ScopedAllowIO allow_io;

  extension_path_ =
      base::MakeAbsoluteFilePath(path_util::ResolveHomeDirectory(path_in));

  if (!IsLoadingUnpackedAllowed()) {
    ReportExtensionLoadError(kUnpackedExtensionsBlacklistedError);
    return false;
  }

  std::string error;
  if (!LoadExtension(Manifest::COMMAND_LINE, GetFlags(), &error)) {
    ReportExtensionLoadError(error);
    return false;
  }

  if (only_allow_apps && !extension()->is_platform_app()) {
#if defined(GOOGLE_CHROME_BUILD)
    // Avoid crashing for users with hijacked shortcuts.
    return true;
#else
    // Defined here to avoid unused variable errors in official builds.
    const char extension_instead_of_app_error[] =
        "App loading flags cannot be used to load extensions. Please use "
        "--load-extension instead.";
    ReportExtensionLoadError(extension_instead_of_app_error);
    return false;
#endif
  }

  extension()->permissions_data()->BindToCurrentThread();
  PermissionsUpdater(
      service_weak_->profile(), PermissionsUpdater::INIT_FLAG_TRANSIENT)
      .InitializePermissions(extension());
  StartInstallChecks();

  *extension_id = extension()->id();
  return true;
}

void UnpackedInstaller::StartInstallChecks() {
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 1";
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ExtensionService* service = service_weak_.get();
  if (!service)
    return;
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 2";

  // TODO(crbug.com/421128): Enable these checks all the time.  The reason
  // they are disabled for extensions loaded from the command-line is that
  // installing unpacked extensions is asynchronous, but there can be
  // dependencies between the extensions loaded by the command line.
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 3";
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Extension: " << extension();
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Manifest: " << extension()->manifest();
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Manifest Location: " << extension()->manifest()->location();
  if (extension()->manifest()->location() != Manifest::COMMAND_LINE) {
    LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 3a";
    if (service->browser_terminating())
      return;

    // TODO(crbug.com/420147): Move this code to a utility class to avoid
    // duplication of SharedModuleService::CheckImports code.
    LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 3b";
    if (SharedModuleInfo::ImportsModules(extension())) {
      LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 3c";
      const std::vector<SharedModuleInfo::ImportInfo>& imports =
          SharedModuleInfo::GetImports(extension());
      std::vector<SharedModuleInfo::ImportInfo>::const_iterator i;
      LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 3d";
      for (i = imports.begin(); i != imports.end(); ++i) {
        LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 4a";
        base::Version version_required(i->minimum_version);
        const Extension* imported_module =
            service->GetExtensionById(i->extension_id, true);
        LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 4b";
        if (!imported_module) {
          ReportExtensionLoadError(kImportMissing);
          return;
        LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 4c";
        } else if (imported_module &&
                   !SharedModuleInfo::IsSharedModule(imported_module)) {
          LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 5a";
          ReportExtensionLoadError(kImportNotSharedModule);
          LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 5b";
          return;
        } else if (imported_module && (version_required.IsValid() &&
                                       imported_module->version().CompareTo(
                                           version_required) < 0)) {
          LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 6";
          ReportExtensionLoadError(kImportMinVersionNewer);
          LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 7";
          return;
        }
      }
    }
  }

  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 8";

  policy_check_ = std::make_unique<PolicyCheck>(profile_, extension_);
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 9";
  requirements_check_ = std::make_unique<RequirementsChecker>(extension_);
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 10";

  check_group_ = std::make_unique<PreloadCheckGroup>();
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 11";
  check_group_->set_stop_on_first_error(true);

  check_group_->AddCheck(policy_check_.get());
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 12";
  check_group_->AddCheck(requirements_check_.get());
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 13";
  check_group_->Start(
      base::BindOnce(&UnpackedInstaller::OnInstallChecksComplete, this));
  LOG(INFO) << "[EXTENSIONS] StartInstallChecks - Step 14";
}


void UnpackedInstaller::OnInstallChecksComplete(
    const PreloadCheck::Errors& errors) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  LOG(INFO) << "[EXTENSIONS] OnInstallChecksComplete - Step 1";

  if (errors.empty()) {
    LOG(INFO) << "[EXTENSIONS] OnInstallChecksComplete - Step 2";
    InstallExtension();
    LOG(INFO) << "[EXTENSIONS] OnInstallChecksComplete - Step 3";
    return;
  }

  LOG(INFO) << "[EXTENSIONS] OnInstallChecksComplete - Step 4";

  base::string16 error_message;
  if (errors.count(PreloadCheck::DISALLOWED_BY_POLICY))
    error_message = policy_check_->GetErrorMessage();
  else
    error_message = requirements_check_->GetErrorMessage();

  DCHECK(!error_message.empty());
  LOG(INFO) << "[EXTENSIONS] OnInstallChecksComplete - Step 5";
  ReportExtensionLoadError(base::UTF16ToUTF8(error_message));
  LOG(INFO) << "[EXTENSIONS] OnInstallChecksComplete - Step 6";
}

int UnpackedInstaller::GetFlags() {
  std::string id = crx_file::id_util::GenerateIdForPath(extension_path_);
  bool allow_file_access =
      Manifest::ShouldAlwaysAllowFileAccess(Manifest::UNPACKED);
  ExtensionPrefs* prefs = ExtensionPrefs::Get(service_weak_->profile());
  if (prefs->HasAllowFileAccessSetting(id))
    allow_file_access = prefs->AllowFileAccess(id);

  int result = Extension::FOLLOW_SYMLINKS_ANYWHERE;
  if (allow_file_access)
    result |= Extension::ALLOW_FILE_ACCESS;
  if (require_modern_manifest_version_)
    result |= Extension::REQUIRE_MODERN_MANIFEST_VERSION;

  return result;
}

bool UnpackedInstaller::LoadExtension(Manifest::Location location,
                                      int flags,
                                      std::string* error) {
  base::AssertBlockingAllowed();

  // Clean up the kMetadataFolder if necessary. This prevents spurious
  // warnings/errors and ensures we don't treat a user provided file as one by
  // the Extension system.
  MaybeCleanupMetadataFolder(extension_path_);

  // Treat presence of illegal filenames as a hard error for unpacked
  // extensions. Don't do so for command line extensions since this breaks
  // Chrome OS autotests (crbug.com/764787).
  if (location == Manifest::UNPACKED &&
      !file_util::CheckForIllegalFilenames(extension_path_, error)) {
    return false;
  }

  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller Loading: " << extension_path_ << " - LoadExtension (pre)";

  extension_ =
      file_util::LoadExtension(extension_path_, location, flags, error);

  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller Loading: " << extension_path_ << " - LoadExtension (post)";

  return extension() &&
         extension_l10n_util::ValidateExtensionLocales(
             extension_path_, extension()->manifest()->value(), error) &&
         IndexAndPersistRulesIfNeeded(error);
}

bool UnpackedInstaller::IndexAndPersistRulesIfNeeded(std::string* error) {
  LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 1";
  DCHECK(extension());
  base::AssertBlockingAllowed();

  LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 2";
  const ExtensionResource* resource =
      declarative_net_request::DNRManifestData::GetRulesetResource(extension());
  LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 3";
  // The extension did not provide a ruleset.
  if (!resource)
    return true;
  LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 4";

  // TODO(crbug.com/761107): Change this so that we don't need to parse JSON
  // in the browser process.
  JSONFileValueDeserializer deserializer(resource->GetFilePath());
  LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 5";
  std::unique_ptr<base::Value> root = deserializer.Deserialize(nullptr, error);
  if (!root)
    return false;
  LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 6";

  if (!root->is_list()) {
    *error = manifest_errors::kDeclarativeNetRequestListNotPassed;
    return false;
  }
  LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 7";

  std::vector<InstallWarning> warnings;
  int ruleset_checksum;
  if (!declarative_net_request::IndexAndPersistRules(
          *base::ListValue::From(std::move(root)), *extension(), error,
          &warnings, &ruleset_checksum)) {
    LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 7a";
    return false;
  }

  LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 8";
  dnr_ruleset_checksum_ = ruleset_checksum;
  LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 9";
  extension_->AddInstallWarnings(warnings);
  LOG(INFO) << "[EXTENSIONS] IndexAndPresistRulesIfNeeded - Step 10";
  return true;
}

bool UnpackedInstaller::IsLoadingUnpackedAllowed() const {
  if (!service_weak_.get())
    return true;
  // If there is a "*" in the extension blacklist, then no extensions should be
  // allowed at all (except explicitly whitelisted extensions).
  return !ExtensionManagementFactory::GetForBrowserContext(
              service_weak_->profile())->BlacklistedByDefault();
}

void UnpackedInstaller::GetAbsolutePath() {
  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::GetAbsolutePath - Step 1";
  base::AssertBlockingAllowed();
  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::GetAbsolutePath - Step 2";

  extension_path_ = base::MakeAbsoluteFilePath(extension_path_);
  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::GetAbsolutePath - Step 3";

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UnpackedInstaller::CheckExtensionFileAccess, this));
  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::GetAbsolutePath - Step 4";
}

void UnpackedInstaller::CheckExtensionFileAccess() {
  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::CheckExtensionFileAccess - Step 1";
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::CheckExtensionFileAccess - Step 2";
  if (!service_weak_.get())
    return;

  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::CheckExtensionFileAccess - Step 3";
  if (!IsLoadingUnpackedAllowed()) {
    ReportExtensionLoadError(kUnpackedExtensionsBlacklistedError);
    return;
  }

  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::CheckExtensionFileAccess - Step 4";
  GetExtensionFileTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&UnpackedInstaller::LoadWithFileAccess, this, GetFlags()));
  LOG(INFO) << "[EXTENSIONS] UnpackedInstaller::CheckExtensionFileAccess - Step 5";
}

void UnpackedInstaller::LoadWithFileAccess(int flags) {
  LOG(INFO) << "[EXTENSIONS] LoadWithFileAccess - Step 1";
  base::AssertBlockingAllowed();

  LOG(INFO) << "[EXTENSIONS] LoadWithFileAccess - Step 2";
  std::string error;
  LOG(INFO) << "[EXTENSIONS] LoadWithFileAccess - Step 3";
  if (!LoadExtension(Manifest::UNPACKED, flags, &error)) {
    LOG(INFO) << "[EXTENSIONS] LoadWithFileAccess - Step 4";
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&UnpackedInstaller::ReportExtensionLoadError, this,
                       error));
    LOG(INFO) << "[EXTENSIONS] LoadWithFileAccess - Step 5";
    return;
  }

  LOG(INFO) << "[EXTENSIONS] LoadWithFileAccess - Step 6";

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UnpackedInstaller::StartInstallChecks, this));
  LOG(INFO) << "[EXTENSIONS] LoadWithFileAccess - Step 7";
}

void UnpackedInstaller::ReportExtensionLoadError(const std::string &error) {
  LOG(INFO) << "[EXTENSIONS] ReportExtensionLoadError - Step 1";
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  LOG(INFO) << "[EXTENSIONS] ReportExtensionLoadError - Step 2";
  if (service_weak_.get()) {
    LoadErrorReporter::GetInstance()->ReportLoadError(
        extension_path_, error, service_weak_->profile(), be_noisy_on_failure_);
  }
  LOG(INFO) << "[EXTENSIONS] ReportExtensionLoadError - Step 3";

  if (!callback_.is_null()) {
    callback_.Run(nullptr, extension_path_, error);
    callback_.Reset();
  }
  LOG(INFO) << "[EXTENSIONS] ReportExtensionLoadError - Step 4";
}

void UnpackedInstaller::InstallExtension() {
  LOG(INFO) << "[EXTENSIONS] InstallExtension - Step 1";
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  LOG(INFO) << "[EXTENSIONS] InstallExtension - Step 2";
  if (!service_weak_.get()) {
    callback_.Reset();
    return;
  }

  LOG(INFO) << "[EXTENSIONS] InstallExtension - Step 3";
  PermissionsUpdater perms_updater(service_weak_->profile());
  perms_updater.InitializePermissions(extension());
  perms_updater.GrantActivePermissions(extension());

  LOG(INFO) << "[EXTENSIONS] InstallExtension - Step 4";
  service_weak_->OnExtensionInstalled(extension(), syncer::StringOrdinal(),
                                      kInstallFlagInstallImmediately,
                                      dnr_ruleset_checksum_);

  LOG(INFO) << "[EXTENSIONS] InstallExtension - Step 5";
  if (!callback_.is_null()) {
    callback_.Run(extension(), extension_path_, std::string());
    callback_.Reset();
  }
  LOG(INFO) << "[EXTENSIONS] InstallExtension - Step 6";
}

}  // namespace extensions
