// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/file_manager/private_api_misc.h"

#include <stddef.h>

#include <algorithm>
#include <set>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/crostini/crostini_manager.h"
#include "chrome/browser/chromeos/crostini/crostini_util.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/extensions/file_manager/private_api_util.h"
#include "chrome/browser/chromeos/file_manager/fileapi_util.h"
#include "chrome/browser/chromeos/file_manager/volume_manager.h"
#include "chrome/browser/chromeos/file_system_provider/mount_path_util.h"
#include "chrome/browser/chromeos/file_system_provider/service.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"
#include "chrome/browser/chromeos/fileapi/recent_model.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/extensions/devtools_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/common/extensions/api/file_manager_private_internal.h"
#include "chrome/common/extensions/api/manifest_types.h"
#include "chrome/common/pref_names.h"
#include "chrome/services/file_util/public/cpp/zip_file_creator.h"
#include "chromeos/settings/timezone_settings.h"
#include "components/account_id/account_id.h"
#include "components/drive/drive_pref_names.h"
#include "components/drive/event_logger.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/user_manager/user_manager.h"
#include "components/zoom/page_zoom.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_zoom.h"
#include "content/public/common/service_manager_connection.h"
#include "extensions/browser/api/file_handlers/mime_util.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "google_apis/drive/auth_service.h"
#include "storage/browser/fileapi/external_mount_points.h"
#include "storage/common/fileapi/file_system_types.h"
#include "ui/base/webui/web_ui_util.h"
#include "url/gurl.h"

namespace extensions {
namespace {

using api::file_manager_private::ProfileInfo;

const char kCWSScope[] = "https://www.googleapis.com/auth/chromewebstore";

// Obtains the current app window.
AppWindow* GetCurrentAppWindow(UIThreadExtensionFunction* function) {
  content::WebContents* const contents = function->GetSenderWebContents();
  return contents
             ? AppWindowRegistry::Get(function->browser_context())
                   ->GetAppWindowForWebContents(contents)
             : nullptr;
}

std::vector<ProfileInfo> GetLoggedInProfileInfoList() {
  DCHECK(user_manager::UserManager::IsInitialized());
  const std::vector<Profile*>& profiles =
      g_browser_process->profile_manager()->GetLoadedProfiles();
  std::set<Profile*> original_profiles;
  std::vector<ProfileInfo> result_profiles;

  for (Profile* profile : profiles) {
    // Filter the profile.
    profile = profile->GetOriginalProfile();
    if (original_profiles.count(profile))
      continue;
    original_profiles.insert(profile);
    const user_manager::User* const user =
        chromeos::ProfileHelper::Get()->GetUserByProfile(profile);
    if (!user || !user->is_logged_in())
      continue;

    // Make a ProfileInfo.
    ProfileInfo profile_info;
    profile_info.profile_id =
        multi_user_util::GetAccountIdFromProfile(profile).GetUserEmail();
    profile_info.display_name = base::UTF16ToUTF8(user->GetDisplayName());
    // TODO(hirono): Remove the property from the profile_info.
    profile_info.is_current_profile = true;

    result_profiles.push_back(std::move(profile_info));
  }

  return result_profiles;
}

// Converts a list of file system urls (as strings) to a pair of a provided file
// system object and a list of unique paths on the file system. In case of an
// error, false is returned and the error message set.
bool ConvertURLsToProvidedInfo(
    const scoped_refptr<storage::FileSystemContext>& file_system_context,
    const std::vector<std::string>& urls,
    chromeos::file_system_provider::ProvidedFileSystemInterface** file_system,
    std::vector<base::FilePath>* paths,
    std::string* error) {
  DCHECK(file_system);
  DCHECK(error);

  if (urls.empty()) {
    *error = "At least one file must be specified.";
    return false;
  }

  *file_system = nullptr;
  for (const auto url : urls) {
    const storage::FileSystemURL file_system_url(
        file_system_context->CrackURL(GURL(url)));

    chromeos::file_system_provider::util::FileSystemURLParser parser(
        file_system_url);
    if (!parser.Parse()) {
      *error = "Related provided file system not found.";
      return false;
    }

    if (*file_system != nullptr) {
      if (*file_system != parser.file_system()) {
        *error = "All entries must be on the same file system.";
        return false;
      }
    } else {
      *file_system = parser.file_system();
    }
    paths->push_back(parser.file_path());
  }

  // Erase duplicates.
  std::sort(paths->begin(), paths->end());
  paths->erase(std::unique(paths->begin(), paths->end()), paths->end());

  return true;
}

bool IsAllowedSource(storage::FileSystemType type,
                     api::file_manager_private::SourceRestriction restriction) {
  switch (restriction) {
    case api::file_manager_private::SOURCE_RESTRICTION_NONE:
      NOTREACHED();
      return false;

    case api::file_manager_private::SOURCE_RESTRICTION_ANY_SOURCE:
      return true;

    case api::file_manager_private::SOURCE_RESTRICTION_NATIVE_SOURCE:
      return type == storage::kFileSystemTypeNativeLocal;

    case api::file_manager_private::SOURCE_RESTRICTION_NATIVE_OR_DRIVE_SOURCE:
      return type == storage::kFileSystemTypeNativeLocal ||
             type == storage::kFileSystemTypeDrive;
  }
}

}  // namespace

ExtensionFunction::ResponseAction
FileManagerPrivateLogoutUserForReauthenticationFunction::Run() {
  const user_manager::User* user =
      chromeos::ProfileHelper::Get()->GetUserByProfile(
          Profile::FromBrowserContext(browser_context()));
  if (user) {
    user_manager::UserManager::Get()->SaveUserOAuthStatus(
        user->GetAccountId(), user_manager::User::OAUTH2_TOKEN_STATUS_INVALID);
  }

  chrome::AttemptUserExit();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
FileManagerPrivateGetPreferencesFunction::Run() {
  api::file_manager_private::Preferences result;
  Profile* profile = Profile::FromBrowserContext(browser_context());
  const PrefService* const service = profile->GetPrefs();

  result.drive_enabled = drive::util::IsDriveEnabledForProfile(profile);
  result.cellular_disabled =
      service->GetBoolean(drive::prefs::kDisableDriveOverCellular);
  result.hosted_files_disabled =
      service->GetBoolean(drive::prefs::kDisableDriveHostedFiles);
  result.search_suggest_enabled =
      service->GetBoolean(prefs::kSearchSuggestEnabled);
  result.use24hour_clock = service->GetBoolean(prefs::kUse24HourClock);
  result.allow_redeem_offers = true;
  if (!chromeos::CrosSettings::Get()->GetBoolean(
          chromeos::kAllowRedeemChromeOsRegistrationOffers,
          &result.allow_redeem_offers)) {
    result.allow_redeem_offers = true;
  }
  result.timezone =
      base::UTF16ToUTF8(chromeos::system::TimezoneSettings::GetInstance()
                            ->GetCurrentTimezoneID());

  drive::EventLogger* logger = file_manager::util::GetLogger(profile);
  if (logger)
    logger->Log(logging::LOG_INFO, "%s succeeded.", name());

  return RespondNow(OneArgument(result.ToValue()));
}

ExtensionFunction::ResponseAction
FileManagerPrivateSetPreferencesFunction::Run() {
  using extensions::api::file_manager_private::SetPreferences::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  PrefService* const service = profile->GetPrefs();

  if (params->change_info.cellular_disabled)
    service->SetBoolean(drive::prefs::kDisableDriveOverCellular,
                        *params->change_info.cellular_disabled);

  if (params->change_info.hosted_files_disabled)
    service->SetBoolean(drive::prefs::kDisableDriveHostedFiles,
                        *params->change_info.hosted_files_disabled);

  drive::EventLogger* logger = file_manager::util::GetLogger(profile);
  if (logger)
    logger->Log(logging::LOG_INFO, "%s succeeded.", name());
  return RespondNow(NoArguments());
}

FileManagerPrivateInternalZipSelectionFunction::
    FileManagerPrivateInternalZipSelectionFunction() {}

FileManagerPrivateInternalZipSelectionFunction::
    ~FileManagerPrivateInternalZipSelectionFunction() {}

bool FileManagerPrivateInternalZipSelectionFunction::RunAsync() {
  using extensions::api::file_manager_private_internal::ZipSelection::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  // First param is the parent directory URL.
  if (params->parent_url.empty())
    return false;

  base::FilePath src_dir = file_manager::util::GetLocalPathFromURL(
      render_frame_host(), GetProfile(), GURL(params->parent_url));
  if (src_dir.empty())
    return false;

  // Second param is the list of selected file URLs to be zipped.
  if (params->urls.empty())
    return false;

  std::vector<base::FilePath> files;
  for (size_t i = 0; i < params->urls.size(); ++i) {
    base::FilePath path = file_manager::util::GetLocalPathFromURL(
        render_frame_host(), GetProfile(), GURL(params->urls[i]));
    if (path.empty())
      return false;
    files.push_back(path);
  }

  // Third param is the name of the output zip file.
  if (params->dest_name.empty())
    return false;

  // Check if the dir path is under Drive mount point.
  // TODO(hshi): support create zip file on Drive (crbug.com/158690).
  if (drive::util::IsUnderDriveMountPoint(src_dir))
    return false;

  base::FilePath dest_file = src_dir.Append(params->dest_name);
  std::vector<base::FilePath> src_relative_paths;
  for (size_t i = 0; i != files.size(); ++i) {
    const base::FilePath& file_path = files[i];

    // Obtain the relative path of |file_path| under |src_dir|.
    base::FilePath relative_path;
    if (!src_dir.AppendRelativePath(file_path, &relative_path))
      return false;
    src_relative_paths.push_back(relative_path);
  }

  (new ZipFileCreator(
       base::Bind(&FileManagerPrivateInternalZipSelectionFunction::OnZipDone,
                  this),
       src_dir, src_relative_paths, dest_file))
      ->Start(
          content::ServiceManagerConnection::GetForProcess()->GetConnector());
  return true;
}

void FileManagerPrivateInternalZipSelectionFunction::OnZipDone(bool success) {
  SetResult(std::make_unique<base::Value>(success));
  SendResponse(true);
}

ExtensionFunction::ResponseAction FileManagerPrivateZoomFunction::Run() {
  using extensions::api::file_manager_private::Zoom::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  content::PageZoom zoom_type;
  switch (params->operation) {
    case api::file_manager_private::ZOOM_OPERATION_TYPE_IN:
      zoom_type = content::PAGE_ZOOM_IN;
      break;
    case api::file_manager_private::ZOOM_OPERATION_TYPE_OUT:
      zoom_type = content::PAGE_ZOOM_OUT;
      break;
    case api::file_manager_private::ZOOM_OPERATION_TYPE_RESET:
      zoom_type = content::PAGE_ZOOM_RESET;
      break;
    default:
      NOTREACHED();
      return RespondNow(Error(kUnknownErrorDoNotUse));
  }
  zoom::PageZoom::Zoom(GetSenderWebContents(), zoom_type);
  return RespondNow(NoArguments());
}

FileManagerPrivateRequestWebStoreAccessTokenFunction::
    FileManagerPrivateRequestWebStoreAccessTokenFunction() {
}

FileManagerPrivateRequestWebStoreAccessTokenFunction::
    ~FileManagerPrivateRequestWebStoreAccessTokenFunction() {
}

bool FileManagerPrivateRequestWebStoreAccessTokenFunction::RunAsync() {
  std::vector<std::string> scopes;
  scopes.push_back(kCWSScope);

  ProfileOAuth2TokenService* oauth_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(GetProfile());
  net::URLRequestContextGetter* url_request_context_getter =
      g_browser_process->system_request_context();

  if (!oauth_service) {
    drive::EventLogger* logger = file_manager::util::GetLogger(GetProfile());
    if (logger) {
      logger->Log(logging::LOG_ERROR,
                  "CWS OAuth token fetch failed. OAuth2TokenService can't "
                  "be retrieved.");
    }
    SetResult(std::make_unique<base::Value>());
    return false;
  }

  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(GetProfile());
  auth_service_.reset(new google_apis::AuthService(
      oauth_service,
      signin_manager->GetAuthenticatedAccountId(),
      url_request_context_getter,
      scopes));
  auth_service_->StartAuthentication(base::Bind(
      &FileManagerPrivateRequestWebStoreAccessTokenFunction::
          OnAccessTokenFetched,
      this));

  return true;
}

void FileManagerPrivateRequestWebStoreAccessTokenFunction::OnAccessTokenFetched(
    google_apis::DriveApiErrorCode code,
    const std::string& access_token) {
  drive::EventLogger* logger = file_manager::util::GetLogger(GetProfile());

  if (code == google_apis::HTTP_SUCCESS) {
    DCHECK(auth_service_->HasAccessToken());
    DCHECK(access_token == auth_service_->access_token());
    if (logger)
      logger->Log(logging::LOG_INFO, "CWS OAuth token fetch succeeded.");
    SetResult(std::make_unique<base::Value>(access_token));
    SendResponse(true);
  } else {
    if (logger) {
      logger->Log(logging::LOG_ERROR,
                  "CWS OAuth token fetch failed. (DriveApiErrorCode: %s)",
                  google_apis::DriveApiErrorCodeToString(code).c_str());
    }
    SetResult(std::make_unique<base::Value>());
    SendResponse(false);
  }
}

ExtensionFunction::ResponseAction FileManagerPrivateGetProfilesFunction::Run() {
  const std::vector<ProfileInfo>& profiles = GetLoggedInProfileInfoList();

  // Obtains the display profile ID.
  AppWindow* const app_window = GetCurrentAppWindow(this);
  MultiUserWindowManager* const window_manager =
      MultiUserWindowManager::GetInstance();
  const AccountId current_profile_id = multi_user_util::GetAccountIdFromProfile(
      Profile::FromBrowserContext(browser_context()));
  const AccountId display_profile_id =
      window_manager && app_window
          ? window_manager->GetUserPresentingWindow(
                app_window->GetNativeWindow())
          : EmptyAccountId();

  return RespondNow(
      ArgumentList(api::file_manager_private::GetProfiles::Results::Create(
          profiles, current_profile_id.GetUserEmail(),
          display_profile_id.is_valid() ? display_profile_id.GetUserEmail()
                                        : current_profile_id.GetUserEmail())));
}

ExtensionFunction::ResponseAction
FileManagerPrivateOpenInspectorFunction::Run() {
  using extensions::api::file_manager_private::OpenInspector::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  switch (params->type) {
    case extensions::api::file_manager_private::INSPECTION_TYPE_NORMAL:
      // Open inspector for foreground page.
      DevToolsWindow::OpenDevToolsWindow(GetSenderWebContents());
      break;
    case extensions::api::file_manager_private::INSPECTION_TYPE_CONSOLE:
      // Open inspector for foreground page and bring focus to the console.
      DevToolsWindow::OpenDevToolsWindow(
          GetSenderWebContents(), DevToolsToggleAction::ShowConsolePanel());
      break;
    case extensions::api::file_manager_private::INSPECTION_TYPE_ELEMENT:
      // Open inspector for foreground page in inspect element mode.
      DevToolsWindow::OpenDevToolsWindow(GetSenderWebContents(),
                                         DevToolsToggleAction::Inspect());
      break;
    case extensions::api::file_manager_private::INSPECTION_TYPE_BACKGROUND:
      // Open inspector for background page.
      extensions::devtools_util::InspectBackgroundPage(
          extension(), Profile::FromBrowserContext(browser_context()));
      break;
    default:
      NOTREACHED();
      return RespondNow(Error(
          base::StringPrintf("Unexpected inspection type(%d) is specified.",
                             static_cast<int>(params->type))));
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
FileManagerPrivateOpenSettingsSubpageFunction::Run() {
  using extensions::api::file_manager_private::OpenSettingsSubpage::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  chrome::ShowSettingsSubPageForProfile(ProfileManager::GetActiveUserProfile(),
                                        params->sub_page);
  return RespondNow(NoArguments());
}

FileManagerPrivateInternalGetMimeTypeFunction::
    FileManagerPrivateInternalGetMimeTypeFunction() {
}

FileManagerPrivateInternalGetMimeTypeFunction::
    ~FileManagerPrivateInternalGetMimeTypeFunction() {
}

bool FileManagerPrivateInternalGetMimeTypeFunction::RunAsync() {
  using extensions::api::file_manager_private_internal::GetMimeType::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  // Convert file url to local path.
  const scoped_refptr<storage::FileSystemContext> file_system_context =
      file_manager::util::GetFileSystemContextForRenderFrameHost(
          GetProfile(), render_frame_host());

  storage::FileSystemURL file_system_url(
      file_system_context->CrackURL(GURL(params->url)));

  app_file_handler_util::GetMimeTypeForLocalPath(
      GetProfile(), file_system_url.path(),
      base::Bind(&FileManagerPrivateInternalGetMimeTypeFunction::OnGetMimeType,
                 this));

  return true;
}

void FileManagerPrivateInternalGetMimeTypeFunction::OnGetMimeType(
    const std::string& mimeType) {
  SetResult(std::make_unique<base::Value>(mimeType));
  SendResponse(true);
}

ExtensionFunction::ResponseAction
FileManagerPrivateIsPiexLoaderEnabledFunction::Run() {
#if defined(OFFICIAL_BUILD)
  return RespondNow(OneArgument(std::make_unique<base::Value>(true)));
#else
  return RespondNow(OneArgument(std::make_unique<base::Value>(false)));
#endif
}

FileManagerPrivateGetProvidersFunction::FileManagerPrivateGetProvidersFunction()
    : chrome_details_(this) {}

ExtensionFunction::ResponseAction
FileManagerPrivateGetProvidersFunction::Run() {
  using chromeos::file_system_provider::Capabilities;
  using chromeos::file_system_provider::IconSet;
  using chromeos::file_system_provider::ProviderId;
  using chromeos::file_system_provider::ProviderInterface;
  using chromeos::file_system_provider::Service;
  const Service* const service = Service::Get(chrome_details_.GetProfile());

  using api::file_manager_private::Provider;
  std::vector<Provider> result;
  for (const auto& pair : service->GetProviders()) {
    const ProviderInterface* const provider = pair.second.get();
    const ProviderId provider_id = provider->GetId();

    Provider result_item;
    result_item.provider_id = provider->GetId().ToString();
    const IconSet& icon_set = provider->GetIconSet();
    file_manager::util::FillIconSet(&result_item.icon_set, icon_set);
    result_item.name = provider->GetName();

    const Capabilities capabilities = provider->GetCapabilities();
    result_item.configurable = capabilities.configurable;
    result_item.watchable = capabilities.watchable;
    result_item.multiple_mounts = capabilities.multiple_mounts;
    switch (capabilities.source) {
      case SOURCE_FILE:
        result_item.source =
            api::manifest_types::FILE_SYSTEM_PROVIDER_SOURCE_FILE;
        break;
      case SOURCE_DEVICE:
        result_item.source =
            api::manifest_types::FILE_SYSTEM_PROVIDER_SOURCE_DEVICE;
        break;
      case SOURCE_NETWORK:
        result_item.source =
            api::manifest_types::FILE_SYSTEM_PROVIDER_SOURCE_NETWORK;
        break;
    }
    result.push_back(std::move(result_item));
  }

  return RespondNow(ArgumentList(
      api::file_manager_private::GetProviders::Results::Create(result)));
}

FileManagerPrivateAddProvidedFileSystemFunction::
    FileManagerPrivateAddProvidedFileSystemFunction()
    : chrome_details_(this) {
}

ExtensionFunction::ResponseAction
FileManagerPrivateAddProvidedFileSystemFunction::Run() {
  using extensions::api::file_manager_private::AddProvidedFileSystem::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  using chromeos::file_system_provider::Service;
  using chromeos::file_system_provider::ProvidingExtensionInfo;
  using chromeos::file_system_provider::ProviderId;
  Service* const service = Service::Get(chrome_details_.GetProfile());

  if (!service->RequestMount(ProviderId::FromString(params->provider_id)))
    return RespondNow(Error("Failed to request a new mount."));

  return RespondNow(NoArguments());
}

FileManagerPrivateConfigureVolumeFunction::
    FileManagerPrivateConfigureVolumeFunction()
    : chrome_details_(this) {
}

ExtensionFunction::ResponseAction
FileManagerPrivateConfigureVolumeFunction::Run() {
  using extensions::api::file_manager_private::ConfigureVolume::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  using file_manager::VolumeManager;
  using file_manager::Volume;
  VolumeManager* const volume_manager =
      VolumeManager::Get(chrome_details_.GetProfile());
  base::WeakPtr<Volume> volume =
      volume_manager->FindVolumeById(params->volume_id);
  if (!volume.get())
    return RespondNow(Error("Volume not found."));
  if (!volume->configurable())
    return RespondNow(Error("Volume not configurable."));

  switch (volume->type()) {
    case file_manager::VOLUME_TYPE_PROVIDED: {
      using chromeos::file_system_provider::Service;
      Service* const service = Service::Get(chrome_details_.GetProfile());
      DCHECK(service);

      using chromeos::file_system_provider::ProvidedFileSystemInterface;
      ProvidedFileSystemInterface* const file_system =
          service->GetProvidedFileSystem(volume->provider_id(),
                                         volume->file_system_id());
      if (file_system)
        file_system->Configure(base::Bind(
            &FileManagerPrivateConfigureVolumeFunction::OnCompleted, this));
      break;
    }
    default:
      NOTIMPLEMENTED();
  }

  return RespondLater();
}

void FileManagerPrivateConfigureVolumeFunction::OnCompleted(
    base::File::Error result) {
  if (result != base::File::FILE_OK) {
    Respond(Error("Failed to complete configuration."));
    return;
  }

  Respond(NoArguments());
}

namespace {
bool IsCrostiniEnabledForProfile(Profile* profile) {
  return IsCrostiniUIAllowedForProfile(profile) && IsCrostiniEnabled(profile);
}
}  // namespace

ExtensionFunction::ResponseAction
FileManagerPrivateIsCrostiniEnabledFunction::Run() {
  return RespondNow(
      OneArgument(std::make_unique<base::Value>(IsCrostiniEnabledForProfile(
          Profile::FromBrowserContext(browser_context())))));
}

FileManagerPrivateMountCrostiniContainerFunction::
    FileManagerPrivateMountCrostiniContainerFunction() = default;

FileManagerPrivateMountCrostiniContainerFunction::
    ~FileManagerPrivateMountCrostiniContainerFunction() = default;

bool FileManagerPrivateMountCrostiniContainerFunction::RunAsync() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  DCHECK(IsCrostiniEnabledForProfile(profile));
  crostini::CrostiniManager::GetInstance()->RestartCrostini(
      profile, kCrostiniDefaultVmName, kCrostiniDefaultContainerName,
      base::BindOnce(
          &FileManagerPrivateMountCrostiniContainerFunction::RestartCallback,
          this));
  return true;
}

void FileManagerPrivateMountCrostiniContainerFunction::RestartCallback(
    crostini::ConciergeClientResult result) {
  if (result != crostini::ConciergeClientResult::SUCCESS) {
    Respond(Error(
        base::StringPrintf("Error restarting crostini container: %d", result)));
    return;
  }

  crostini::CrostiniManager::GetInstance()->GetContainerSshKeys(
      kCrostiniDefaultVmName, kCrostiniDefaultContainerName,
      CryptohomeIdForProfile(Profile::FromBrowserContext(browser_context())),
      base::BindOnce(
          &FileManagerPrivateMountCrostiniContainerFunction::SshKeysCallback,
          this));
}

void FileManagerPrivateMountCrostiniContainerFunction::SshKeysCallback(
    crostini::ConciergeClientResult result,
    const std::string& container_public_key,
    const std::string& host_private_key) {
  if (result != crostini::ConciergeClientResult::SUCCESS) {
    Respond(Error(
        base::StringPrintf("Error fetching crostini ssh keys: %d", result)));
    return;
  }

  // Add an observer for OnMountEvent and keep this object alive to receive it.
  chromeos::disks::DiskMountManager* manager =
      chromeos::disks::DiskMountManager::GetInstance();
  manager->AddObserver(this);
  self_ = this;

  // Call to sshfs to mount.
  // Path = sshfs://<username>@linuxhost:
  // Label = crostini_<cryptohome_id>_<vm_name>_<container_name>
  Profile* profile = Profile::FromBrowserContext(browser_context());
  std::string host = "linuxhost";
  std::string port = "2222";
  source_path_ = base::StringPrintf(
      "sshfs://%s@%s:", ContainerUserNameForProfile(profile).c_str(),
      host.c_str());
  mount_label_ = base::StringPrintf(
      "crostini_%s_%s_%s", CryptohomeIdForProfile(profile).c_str(),
      kCrostiniDefaultVmName, kCrostiniDefaultContainerName);
  std::vector<std::string> mount_options;
  std::string base64_known_hosts;
  std::string base64_identity;
  base::Base64Encode(host_private_key, &base64_identity);
  base::Base64Encode(
      base::StringPrintf("[%s]:%s %s", host.c_str(), port.c_str(),
                         container_public_key.c_str()),
      &base64_known_hosts);
  mount_options.push_back("UserKnownHostsBase64=" + base64_known_hosts);
  mount_options.push_back("IdentityBase64=" + base64_identity);
  mount_options.push_back("Port=" + port);
  manager->MountPath(source_path_, "", mount_label_, mount_options,
                     chromeos::MOUNT_TYPE_NETWORK_STORAGE,
                     chromeos::MOUNT_ACCESS_MODE_READ_WRITE);
}

void FileManagerPrivateMountCrostiniContainerFunction::OnMountEvent(
    chromeos::disks::DiskMountManager::MountEvent event,
    chromeos::MountError error_code,
    const chromeos::disks::DiskMountManager::MountPointInfo& mount_info) {
  // Ignore any other mount/unmount events.
  if (event != chromeos::disks::DiskMountManager::MountEvent::MOUNTING ||
      mount_info.source_path != source_path_) {
    return;
  }
  // Remove observer and self ref.
  chromeos::disks::DiskMountManager::GetInstance()->RemoveObserver(this);
  auto self = std::move(self_);

  if (error_code != chromeos::MountError::MOUNT_ERROR_NONE) {
    Respond(Error(base::StringPrintf(
        "Error mounting crostini container: error_code=%d, "
        "source_path=%s, mount_path=%s, mount_type=%d, mount_condition=%d",
        error_code, mount_info.source_path.c_str(),
        mount_info.mount_path.c_str(), mount_info.mount_type,
        mount_info.mount_condition)));
    return;
  }

  // Register filesystem and add volume to VolumeManager.
  base::FilePath mount_path =
      base::FilePath(FILE_PATH_LITERAL(mount_info.mount_path));
  storage::ExternalMountPoints::GetSystemInstance()->RegisterFileSystem(
      mount_label_, storage::kFileSystemTypeNativeLocal,
      storage::FileSystemMountOption(), mount_path);

  file_manager::VolumeManager::Get(browser_context())
      ->AddSshfsCrostiniVolume(mount_path);
  Respond(NoArguments());
}

FileManagerPrivateInternalGetCustomActionsFunction::
    FileManagerPrivateInternalGetCustomActionsFunction()
    : chrome_details_(this) {}

ExtensionFunction::ResponseAction
FileManagerPrivateInternalGetCustomActionsFunction::Run() {
  using extensions::api::file_manager_private_internal::GetCustomActions::
      Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  const scoped_refptr<storage::FileSystemContext> file_system_context =
      file_manager::util::GetFileSystemContextForRenderFrameHost(
          chrome_details_.GetProfile(), render_frame_host());

  std::vector<base::FilePath> paths;
  chromeos::file_system_provider::ProvidedFileSystemInterface* file_system =
      nullptr;
  std::string error;

  if (!ConvertURLsToProvidedInfo(file_system_context, params->urls,
                                 &file_system, &paths, &error)) {
    return RespondNow(Error(error));
  }

  DCHECK(file_system);
  file_system->GetActions(
      paths,
      base::Bind(
          &FileManagerPrivateInternalGetCustomActionsFunction::OnCompleted,
          this));
  return RespondLater();
}

void FileManagerPrivateInternalGetCustomActionsFunction::OnCompleted(
    const chromeos::file_system_provider::Actions& actions,
    base::File::Error result) {
  if (result != base::File::FILE_OK) {
    Respond(Error("Failed to fetch actions."));
    return;
  }

  using api::file_system_provider::Action;
  std::vector<Action> items;
  for (const auto& action : actions) {
    Action item;
    item.id = action.id;
    item.title.reset(new std::string(action.title));
    items.push_back(std::move(item));
  }

  Respond(ArgumentList(
      api::file_manager_private_internal::GetCustomActions::Results::Create(
          items)));
}

FileManagerPrivateInternalExecuteCustomActionFunction::
    FileManagerPrivateInternalExecuteCustomActionFunction()
    : chrome_details_(this) {}

ExtensionFunction::ResponseAction
FileManagerPrivateInternalExecuteCustomActionFunction::Run() {
  using extensions::api::file_manager_private_internal::ExecuteCustomAction::
      Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  const scoped_refptr<storage::FileSystemContext> file_system_context =
      file_manager::util::GetFileSystemContextForRenderFrameHost(
          chrome_details_.GetProfile(), render_frame_host());

  std::vector<base::FilePath> paths;
  chromeos::file_system_provider::ProvidedFileSystemInterface* file_system =
      nullptr;
  std::string error;

  if (!ConvertURLsToProvidedInfo(file_system_context, params->urls,
                                 &file_system, &paths, &error)) {
    return RespondNow(Error(error));
  }

  DCHECK(file_system);
  file_system->ExecuteAction(
      paths, params->action_id,
      base::Bind(
          &FileManagerPrivateInternalExecuteCustomActionFunction::OnCompleted,
          this));
  return RespondLater();
}

void FileManagerPrivateInternalExecuteCustomActionFunction::OnCompleted(
    base::File::Error result) {
  if (result != base::File::FILE_OK) {
    Respond(Error("Failed to execute the action."));
    return;
  }

  Respond(NoArguments());
}

FileManagerPrivateInternalGetRecentFilesFunction::
    FileManagerPrivateInternalGetRecentFilesFunction()
    : chrome_details_(this) {}

ExtensionFunction::ResponseAction
FileManagerPrivateInternalGetRecentFilesFunction::Run() {
  using extensions::api::file_manager_private_internal::GetRecentFiles::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  const scoped_refptr<storage::FileSystemContext> file_system_context =
      file_manager::util::GetFileSystemContextForRenderFrameHost(
          chrome_details_.GetProfile(), render_frame_host());

  chromeos::RecentModel* model =
      chromeos::RecentModel::GetForProfile(chrome_details_.GetProfile());

  model->GetRecentFiles(
      file_system_context.get(),
      Extension::GetBaseURLFromExtensionId(extension_id()),
      base::BindOnce(
          &FileManagerPrivateInternalGetRecentFilesFunction::OnGetRecentFiles,
          this, params->restriction));
  return RespondLater();
}

void FileManagerPrivateInternalGetRecentFilesFunction::OnGetRecentFiles(
    api::file_manager_private::SourceRestriction restriction,
    const std::vector<chromeos::RecentFile>& files) {
  file_manager::util::FileDefinitionList file_definition_list;
  for (const auto& file : files) {
    // Filter out files from non-allowed sources.
    // We do this filtering here rather than in RecentModel so that the set of
    // files returned with some restriction is a subset of what would be
    // returned without restriction. Anyway, the maximum number of files
    // returned from RecentModel is large enough.
    if (!IsAllowedSource(file.url().type(), restriction))
      continue;

    file_manager::util::FileDefinition file_definition;
    const bool result =
        file_manager::util::ConvertAbsoluteFilePathToRelativeFileSystemPath(
            chrome_details_.GetProfile(), extension_id(), file.url().path(),
            &file_definition.virtual_path);
    if (!result)
      continue;

    // Recent file system only lists regular files, not directories.
    file_definition.is_directory = false;
    file_definition_list.emplace_back(std::move(file_definition));
  }

  file_manager::util::ConvertFileDefinitionListToEntryDefinitionList(
      chrome_details_.GetProfile(), extension_id(),
      file_definition_list,  // Safe, since copied internally.
      base::Bind(&FileManagerPrivateInternalGetRecentFilesFunction::
                     OnConvertFileDefinitionListToEntryDefinitionList,
                 this));
}

void FileManagerPrivateInternalGetRecentFilesFunction::
    OnConvertFileDefinitionListToEntryDefinitionList(
        std::unique_ptr<file_manager::util::EntryDefinitionList>
            entry_definition_list) {
  DCHECK(entry_definition_list);

  auto entries = std::make_unique<base::ListValue>();

  for (const auto& definition : *entry_definition_list) {
    if (definition.error != base::File::FILE_OK)
      continue;
    auto entry = std::make_unique<base::DictionaryValue>();
    entry->SetString("fileSystemName", definition.file_system_name);
    entry->SetString("fileSystemRoot", definition.file_system_root_url);
    entry->SetString("fileFullPath", "/" + definition.full_path.AsUTF8Unsafe());
    entry->SetBoolean("fileIsDirectory", definition.is_directory);
    entries->Append(std::move(entry));
  }

  Respond(OneArgument(std::move(entries)));
}

}  // namespace extensions
