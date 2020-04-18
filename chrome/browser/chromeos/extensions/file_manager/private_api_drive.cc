// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/file_manager/private_api_drive.h"

#include <map>
#include <memory>
#include <set>
#include <utility>

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/drive/drive_integration_service.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/extensions/file_manager/private_api_util.h"
#include "chrome/browser/chromeos/file_manager/file_tasks.h"
#include "chrome/browser/chromeos/file_manager/fileapi_util.h"
#include "chrome/browser/chromeos/file_manager/url_util.h"
#include "chrome/browser/chromeos/file_system_provider/mount_path_util.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_interface.h"
#include "chrome/browser/chromeos/fileapi/external_file_url_util.h"
#include "chrome/browser/chromeos/fileapi/file_system_backend.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/common/extensions/api/file_manager_private_internal.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state_handler.h"
#include "components/drive/drive_app_registry.h"
#include "components/drive/event_logger.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/drive/auth_service.h"
#include "google_apis/drive/drive_api_url_generator.h"
#include "google_apis/drive/drive_switches.h"
#include "storage/common/fileapi/file_system_info.h"
#include "storage/common/fileapi/file_system_util.h"
#include "url/gurl.h"

using content::BrowserThread;

using chromeos::file_system_provider::EntryMetadata;
using chromeos::file_system_provider::ProvidedFileSystemInterface;
using chromeos::file_system_provider::util::FileSystemURLParser;
using extensions::api::file_manager_private::EntryProperties;
using extensions::api::file_manager_private::EntryPropertyName;
using file_manager::util::EntryDefinition;
using file_manager::util::EntryDefinitionCallback;
using file_manager::util::EntryDefinitionList;
using file_manager::util::EntryDefinitionListCallback;
using file_manager::util::FileDefinition;
using file_manager::util::FileDefinitionList;
using google_apis::DriveApiUrlGenerator;

namespace extensions {
namespace {

// List of connection types of drive.
// Keep this in sync with the DriveConnectionType in common/js/util.js.
const char kDriveConnectionTypeOffline[] = "offline";
const char kDriveConnectionTypeMetered[] = "metered";
const char kDriveConnectionTypeOnline[] = "online";

// List of reasons of kDriveConnectionType*.
// Keep this in sync with the DriveConnectionReason in common/js/util.js.
const char kDriveConnectionReasonNotReady[] = "not_ready";
const char kDriveConnectionReasonNoNetwork[] = "no_network";
const char kDriveConnectionReasonNoService[] = "no_service";

// Maximum dimension of thumbnail in file manager. File manager shows 180x180
// thumbnail. Given that we support hdpi devices, maximum dimension is 360.
const int kFileManagerMaximumThumbnailDimension = 360;

// Copies properties from |entry_proto| to |properties|. |shared_with_me| is
// given from the running profile.
void FillEntryPropertiesValueForDrive(const drive::ResourceEntry& entry_proto,
                                      bool shared_with_me,
                                      EntryProperties* properties) {
  properties->shared_with_me.reset(new bool(shared_with_me));
  properties->shared.reset(new bool(entry_proto.shared()));
  properties->starred.reset(new bool(entry_proto.starred()));

  const drive::PlatformFileInfoProto& file_info = entry_proto.file_info();
  properties->size.reset(new double(file_info.size()));
  properties->modification_time.reset(new double(
      base::Time::FromInternalValue(file_info.last_modified()).ToJsTime()));
  properties->modification_by_me_time.reset(new double(
      base::Time::FromInternalValue(entry_proto.last_modified_by_me())
          .ToJsTime()));

  if (entry_proto.has_alternate_url()) {
    properties->alternate_url.reset(
        new std::string(entry_proto.alternate_url()));

    // Set |share_url| to a modified version of |alternate_url| that opens the
    // sharing dialog for files and folders (add ?userstoinvite="" to the URL).
    // TODO(sashab): Add an endpoint to the Drive API that generates this URL,
    // instead of manually modifying it here.
    GURL share_url = GURL(entry_proto.alternate_url());
    GURL::Replacements replacements;
    std::string new_query =
        (share_url.has_query() ? share_url.query() + "&" : "") +
        "userstoinvite=%22%22";
    replacements.SetQueryStr(new_query);
    properties->share_url.reset(
        new std::string(share_url.ReplaceComponents(replacements).spec()));
  }

  if (!entry_proto.has_file_specific_info())
    return;

  const drive::FileSpecificInfo& file_specific_info =
      entry_proto.file_specific_info();

  if (!entry_proto.resource_id().empty()) {
    DriveApiUrlGenerator url_generator(
        (GURL(google_apis::DriveApiUrlGenerator::kBaseUrlForProduction)),
        (GURL(google_apis::DriveApiUrlGenerator::
                  kBaseThumbnailUrlForProduction)),
        google_apis::GetTeamDrivesIntegrationSwitch());
    properties->thumbnail_url.reset(new std::string(
        url_generator.GetThumbnailUrl(entry_proto.resource_id(),
                                      500 /* width */, 500 /* height */,
                                      false /* not cropped */).spec()));
    properties->cropped_thumbnail_url.reset(new std::string(
        url_generator.GetThumbnailUrl(
                          entry_proto.resource_id(),
                          kFileManagerMaximumThumbnailDimension /* width */,
                          kFileManagerMaximumThumbnailDimension /* height */,
                          true /* cropped */).spec()));
  }
  if (file_specific_info.has_image_width()) {
    properties->image_width.reset(
        new int(file_specific_info.image_width()));
  }
  if (file_specific_info.has_image_height()) {
    properties->image_height.reset(
        new int(file_specific_info.image_height()));
  }
  if (file_specific_info.has_image_rotation()) {
    properties->image_rotation.reset(
        new int(file_specific_info.image_rotation()));
  }
  properties->hosted.reset(new bool(file_specific_info.is_hosted_document()));
  properties->content_mime_type.reset(
      new std::string(file_specific_info.content_mime_type()));
  properties->pinned.reset(
      new bool(file_specific_info.cache_state().is_pinned()));
  properties->dirty.reset(
      new bool(file_specific_info.cache_state().is_dirty()));
  properties->present.reset(
      new bool(file_specific_info.cache_state().is_present()));

  if (file_specific_info.cache_state().is_present()) {
    properties->available_offline.reset(new bool(true));
  } else if (file_specific_info.is_hosted_document() &&
             file_specific_info.has_document_extension()) {
    const std::string file_extension = file_specific_info.document_extension();
    // What's available offline? See the 'Web' column at:
    // https://support.google.com/drive/answer/1628467
    properties->available_offline.reset(
        new bool(file_extension == ".gdoc" || file_extension == ".gdraw" ||
                 file_extension == ".gsheet" || file_extension == ".gslides"));
  } else {
    properties->available_offline.reset(new bool(false));
  }

  properties->available_when_metered.reset(
      new bool(file_specific_info.cache_state().is_present() ||
               file_specific_info.is_hosted_document()));
}

// Creates entry definition list for (metadata) search result info list.
template <class T>
void ConvertSearchResultInfoListToEntryDefinitionList(
    Profile* profile,
    const std::string& extension_id,
    const std::vector<T>& search_result_info_list,
    const EntryDefinitionListCallback& callback) {
  FileDefinitionList file_definition_list;

  for (size_t i = 0; i < search_result_info_list.size(); ++i) {
    FileDefinition file_definition;
    file_definition.virtual_path =
        file_manager::util::ConvertDrivePathToRelativeFileSystemPath(
            profile, extension_id, search_result_info_list.at(i).path);
    file_definition.is_directory = search_result_info_list.at(i).is_directory;
    file_definition_list.push_back(file_definition);
  }

  file_manager::util::ConvertFileDefinitionListToEntryDefinitionList(
      profile,
      extension_id,
      file_definition_list,  // Safe, since copied internally.
      callback);
}

class SingleEntryPropertiesGetterForDrive {
 public:
  typedef base::Callback<void(std::unique_ptr<EntryProperties> properties,
                              base::File::Error error)>
      ResultCallback;

  // Creates an instance and starts the process.
  static void Start(const base::FilePath local_path,
                    const std::set<EntryPropertyName>& names,
                    Profile* const profile,
                    const ResultCallback& callback) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    SingleEntryPropertiesGetterForDrive* instance =
        new SingleEntryPropertiesGetterForDrive(local_path, names, profile,
                                                callback);
    instance->StartProcess();

    // The instance will be destroyed by itself.
  }

  virtual ~SingleEntryPropertiesGetterForDrive() {}

 private:
  SingleEntryPropertiesGetterForDrive(
      const base::FilePath local_path,
      const std::set<EntryPropertyName>& /* names */,
      Profile* const profile,
      const ResultCallback& callback)
      : callback_(callback),
        local_path_(local_path),
        running_profile_(profile),
        properties_(new EntryProperties),
        file_owner_profile_(NULL),
        weak_ptr_factory_(this) {
    DCHECK(!callback_.is_null());
    DCHECK(profile);
  }

  void StartProcess() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    file_path_ = drive::util::ExtractDrivePath(local_path_);
    file_owner_profile_ = drive::util::ExtractProfileFromPath(local_path_);

    if (!file_owner_profile_ ||
        !g_browser_process->profile_manager()->IsValidProfile(
            file_owner_profile_)) {
      CompleteGetEntryProperties(drive::FILE_ERROR_FAILED);
      return;
    }

    // Start getting the file info.
    drive::FileSystemInterface* const file_system =
        drive::util::GetFileSystemByProfile(file_owner_profile_);
    if (!file_system) {
      // |file_system| is NULL if Drive is disabled or not mounted.
      CompleteGetEntryProperties(drive::FILE_ERROR_FAILED);
      return;
    }

    file_system->GetResourceEntry(
        file_path_,
        base::Bind(&SingleEntryPropertiesGetterForDrive::OnGetFileInfo,
                   weak_ptr_factory_.GetWeakPtr()));
  }

  void OnGetFileInfo(drive::FileError error,
                     std::unique_ptr<drive::ResourceEntry> entry) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    if (error != drive::FILE_ERROR_OK) {
      CompleteGetEntryProperties(error);
      return;
    }

    DCHECK(entry);
    owner_resource_entry_.swap(entry);

    if (running_profile_->IsSameProfile(file_owner_profile_)) {
      StartParseFileInfo(owner_resource_entry_->shared_with_me());
      return;
    }

    // If the running profile does not own the file, obtain the shared_with_me
    // flag from the running profile's value.
    drive::FileSystemInterface* const file_system =
        drive::util::GetFileSystemByProfile(running_profile_);
    if (!file_system) {
      CompleteGetEntryProperties(drive::FILE_ERROR_FAILED);
      return;
    }
    file_system->GetPathFromResourceId(
        owner_resource_entry_->resource_id(),
        base::Bind(&SingleEntryPropertiesGetterForDrive::OnGetRunningPath,
                   weak_ptr_factory_.GetWeakPtr()));
  }

  void OnGetRunningPath(drive::FileError error,
                        const base::FilePath& file_path) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    if (error != drive::FILE_ERROR_OK) {
      // The running profile does not know the file.
      StartParseFileInfo(false);
      return;
    }

    drive::FileSystemInterface* const file_system =
        drive::util::GetFileSystemByProfile(running_profile_);
    if (!file_system) {
      // The drive is disable for the running profile.
      StartParseFileInfo(false);
      return;
    }

    file_system->GetResourceEntry(
        file_path,
        base::Bind(&SingleEntryPropertiesGetterForDrive::OnGetShareInfo,
                   weak_ptr_factory_.GetWeakPtr()));
  }

  void OnGetShareInfo(drive::FileError error,
                      std::unique_ptr<drive::ResourceEntry> entry) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    if (error != drive::FILE_ERROR_OK) {
      CompleteGetEntryProperties(error);
      return;
    }

    DCHECK(entry.get());
    StartParseFileInfo(entry->shared_with_me());
  }

  void StartParseFileInfo(bool shared_with_me) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    FillEntryPropertiesValueForDrive(
        *owner_resource_entry_, shared_with_me, properties_.get());

    drive::FileSystemInterface* const file_system =
        drive::util::GetFileSystemByProfile(file_owner_profile_);
    drive::DriveAppRegistry* const app_registry =
        drive::util::GetDriveAppRegistryByProfile(file_owner_profile_);
    if (!file_system || !app_registry) {
      // |file_system| or |app_registry| is NULL if Drive is disabled.
      CompleteGetEntryProperties(drive::FILE_ERROR_FAILED);
      return;
    }

    // The properties meaningful for directories are already filled in
    // FillEntryPropertiesValueForDrive().
    if (!owner_resource_entry_->has_file_specific_info()) {
      CompleteGetEntryProperties(drive::FILE_ERROR_OK);
      return;
    }

    const drive::FileSpecificInfo& file_specific_info =
        owner_resource_entry_->file_specific_info();

    // Get drive WebApps that can accept this file. We just need to extract the
    // doc icon for the drive app, which is set as default.
    std::vector<drive::DriveAppInfo> drive_apps;
    app_registry->GetAppsForFile(file_path_.Extension(),
                                 file_specific_info.content_mime_type(),
                                 &drive_apps);
    if (!drive_apps.empty()) {
      std::string default_task_id =
          file_manager::file_tasks::GetDefaultTaskIdFromPrefs(
              *file_owner_profile_->GetPrefs(),
              file_specific_info.content_mime_type(),
              file_path_.Extension());
      file_manager::file_tasks::TaskDescriptor default_task;
      file_manager::file_tasks::ParseTaskID(default_task_id, &default_task);
      DCHECK(default_task_id.empty() || !default_task.app_id.empty());
      for (size_t i = 0; i < drive_apps.size(); ++i) {
        const drive::DriveAppInfo& app_info = drive_apps[i];
        if (default_task.app_id == app_info.app_id) {
          // The drive app is set as default. The Files app should use the doc
          // icon.
          const GURL doc_icon = drive::util::FindPreferredIcon(
              app_info.document_icons, drive::util::kPreferredIconSize);
          properties_->custom_icon_url.reset(new std::string(doc_icon.spec()));
        }
      }
    }

    CompleteGetEntryProperties(drive::FILE_ERROR_OK);
  }

  void CompleteGetEntryProperties(drive::FileError error) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK(!callback_.is_null());

    callback_.Run(std::move(properties_),
                  drive::FileErrorToBaseFileError(error));
    BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
  }

  // Given parameters.
  const ResultCallback callback_;
  const base::FilePath local_path_;
  Profile* const running_profile_;

  // Values used in the process.
  std::unique_ptr<EntryProperties> properties_;
  Profile* file_owner_profile_;
  base::FilePath file_path_;
  std::unique_ptr<drive::ResourceEntry> owner_resource_entry_;

  base::WeakPtrFactory<SingleEntryPropertiesGetterForDrive> weak_ptr_factory_;
};  // class SingleEntryPropertiesGetterForDrive

class SingleEntryPropertiesGetterForFileSystemProvider {
 public:
  typedef base::Callback<void(std::unique_ptr<EntryProperties> properties,
                              base::File::Error error)>
      ResultCallback;

  // Creates an instance and starts the process.
  static void Start(const storage::FileSystemURL file_system_url,
                    const std::set<EntryPropertyName>& names,
                    const ResultCallback& callback) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    SingleEntryPropertiesGetterForFileSystemProvider* instance =
        new SingleEntryPropertiesGetterForFileSystemProvider(file_system_url,
                                                             names, callback);
    instance->StartProcess();

    // The instance will be destroyed by itself.
  }

  virtual ~SingleEntryPropertiesGetterForFileSystemProvider() {}

 private:
  SingleEntryPropertiesGetterForFileSystemProvider(
      const storage::FileSystemURL& file_system_url,
      const std::set<EntryPropertyName>& names,
      const ResultCallback& callback)
      : callback_(callback),
        file_system_url_(file_system_url),
        names_(names),
        properties_(new EntryProperties),
        weak_ptr_factory_(this) {
    DCHECK(!callback_.is_null());
  }

  void StartProcess() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    FileSystemURLParser parser(file_system_url_);
    if (!parser.Parse()) {
      CompleteGetEntryProperties(base::File::FILE_ERROR_NOT_FOUND);
      return;
    }

    ProvidedFileSystemInterface::MetadataFieldMask field_mask =
        ProvidedFileSystemInterface::METADATA_FIELD_NONE;
    if (names_.find(api::file_manager_private::ENTRY_PROPERTY_NAME_SIZE) !=
        names_.end()) {
      field_mask |= ProvidedFileSystemInterface::METADATA_FIELD_SIZE;
    }
    if (names_.find(
            api::file_manager_private::ENTRY_PROPERTY_NAME_MODIFICATIONTIME) !=
        names_.end()) {
      field_mask |=
          ProvidedFileSystemInterface::METADATA_FIELD_MODIFICATION_TIME;
    }
    if (names_.find(
            api::file_manager_private::ENTRY_PROPERTY_NAME_CONTENTMIMETYPE) !=
        names_.end()) {
      field_mask |= ProvidedFileSystemInterface::METADATA_FIELD_MIME_TYPE;
    }
    if (names_.find(
            api::file_manager_private::ENTRY_PROPERTY_NAME_THUMBNAILURL) !=
        names_.end()) {
      field_mask |= ProvidedFileSystemInterface::METADATA_FIELD_THUMBNAIL;
    }

    parser.file_system()->GetMetadata(
        parser.file_path(), field_mask,
        base::Bind(&SingleEntryPropertiesGetterForFileSystemProvider::
                       OnGetMetadataCompleted,
                   weak_ptr_factory_.GetWeakPtr()));
  }

  void OnGetMetadataCompleted(std::unique_ptr<EntryMetadata> metadata,
                              base::File::Error result) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    if (result != base::File::FILE_OK) {
      CompleteGetEntryProperties(result);
      return;
    }

    if (names_.find(api::file_manager_private::ENTRY_PROPERTY_NAME_SIZE) !=
        names_.end()) {
      properties_->size.reset(new double(*metadata->size.get()));
    }

    if (names_.find(
            api::file_manager_private::ENTRY_PROPERTY_NAME_MODIFICATIONTIME) !=
        names_.end()) {
      properties_->modification_time.reset(
          new double(metadata->modification_time->ToJsTime()));
    }

    if (names_.find(
            api::file_manager_private::ENTRY_PROPERTY_NAME_CONTENTMIMETYPE) !=
            names_.end() &&
        metadata->mime_type.get()) {
      properties_->content_mime_type.reset(
          new std::string(*metadata->mime_type));
    }

    if (names_.find(
            api::file_manager_private::ENTRY_PROPERTY_NAME_THUMBNAILURL) !=
            names_.end() &&
        metadata->thumbnail.get()) {
      properties_->thumbnail_url.reset(new std::string(*metadata->thumbnail));
    }

    CompleteGetEntryProperties(base::File::FILE_OK);
  }

  void CompleteGetEntryProperties(base::File::Error result) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK(!callback_.is_null());

    callback_.Run(std::move(properties_), result);
    BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
  }

  // Given parameters.
  const ResultCallback callback_;
  const storage::FileSystemURL file_system_url_;
  const std::set<EntryPropertyName> names_;

  // Values used in the process.
  std::unique_ptr<EntryProperties> properties_;

  base::WeakPtrFactory<SingleEntryPropertiesGetterForFileSystemProvider>
      weak_ptr_factory_;
};  // class SingleEntryPropertiesGetterForDrive

}  // namespace

FileManagerPrivateInternalGetEntryPropertiesFunction::
    FileManagerPrivateInternalGetEntryPropertiesFunction()
    : processed_count_(0) {
}

FileManagerPrivateInternalGetEntryPropertiesFunction::
    ~FileManagerPrivateInternalGetEntryPropertiesFunction() {
}

bool FileManagerPrivateInternalGetEntryPropertiesFunction::RunAsync() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  using api::file_manager_private_internal::GetEntryProperties::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  scoped_refptr<storage::FileSystemContext> file_system_context =
      file_manager::util::GetFileSystemContextForRenderFrameHost(
          GetProfile(), render_frame_host());

  properties_list_.resize(params->urls.size());
  const std::set<EntryPropertyName> names_as_set(params->names.begin(),
                                                 params->names.end());
  for (size_t i = 0; i < params->urls.size(); i++) {
    const GURL url = GURL(params->urls[i]);
    const storage::FileSystemURL file_system_url =
        file_system_context->CrackURL(url);
    switch (file_system_url.type()) {
      case storage::kFileSystemTypeDrive:
        SingleEntryPropertiesGetterForDrive::Start(
            file_system_url.path(), names_as_set, GetProfile(),
            base::Bind(&FileManagerPrivateInternalGetEntryPropertiesFunction::
                           CompleteGetEntryProperties,
                       this, i, file_system_url));
        break;
      case storage::kFileSystemTypeProvided:
        SingleEntryPropertiesGetterForFileSystemProvider::Start(
            file_system_url, names_as_set,
            base::Bind(&FileManagerPrivateInternalGetEntryPropertiesFunction::
                           CompleteGetEntryProperties,
                       this, i, file_system_url));
        break;
      default:
        // TODO(yawano) Change this to support other voluems (e.g. local) ,and
        // integrate fileManagerPrivate.getMimeType to this method.
        LOG(ERROR) << "Not supported file system type.";
        CompleteGetEntryProperties(i, file_system_url,
                                   base::WrapUnique(new EntryProperties),
                                   base::File::FILE_ERROR_INVALID_OPERATION);
    }
  }

  return true;
}

void FileManagerPrivateInternalGetEntryPropertiesFunction::
    CompleteGetEntryProperties(size_t index,
                               const storage::FileSystemURL& url,
                               std::unique_ptr<EntryProperties> properties,
                               base::File::Error error) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(0 <= processed_count_ && processed_count_ < properties_list_.size());

  if (error == base::File::FILE_OK) {
    properties->external_file_url.reset(
        new std::string(chromeos::FileSystemURLToExternalFileURL(url).spec()));
  }
  properties_list_[index] = std::move(*properties);

  processed_count_++;
  if (processed_count_ < properties_list_.size())
    return;

  results_ = extensions::api::file_manager_private_internal::
      GetEntryProperties::Results::Create(properties_list_);
  SendResponse(true);
}

bool FileManagerPrivateInternalPinDriveFileFunction::RunAsync() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  using extensions::api::file_manager_private_internal::PinDriveFile::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  drive::FileSystemInterface* const file_system =
      drive::util::GetFileSystemByProfile(GetProfile());
  if (!file_system)  // |file_system| is NULL if Drive is disabled.
    return false;

  const base::FilePath drive_path =
      drive::util::ExtractDrivePath(file_manager::util::GetLocalPathFromURL(
          render_frame_host(), GetProfile(), GURL(params->url)));
  if (params->pin) {
    file_system->Pin(
        drive_path,
        base::Bind(
            &FileManagerPrivateInternalPinDriveFileFunction::OnPinStateSet,
            this));
  } else {
    file_system->Unpin(
        drive_path,
        base::Bind(
            &FileManagerPrivateInternalPinDriveFileFunction::OnPinStateSet,
            this));
  }
  return true;
}

void FileManagerPrivateInternalPinDriveFileFunction::OnPinStateSet(
    drive::FileError error) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (error == drive::FILE_ERROR_OK) {
    SendResponse(true);
  } else {
    SetError(drive::FileErrorToString(error));
    SendResponse(false);
  }
}

bool FileManagerPrivateInternalEnsureFileDownloadedFunction::RunAsync() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  using extensions::api::file_manager_private_internal::EnsureFileDownloaded::
      Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  const base::FilePath drive_path =
      drive::util::ExtractDrivePath(file_manager::util::GetLocalPathFromURL(
          render_frame_host(), GetProfile(), GURL(params->url)));
  if (drive_path.empty()) {
    // Not under Drive. No need to fill the cache.
    SendResponse(true);
    return true;
  }

  drive::FileSystemInterface* const file_system =
      drive::util::GetFileSystemByProfile(GetProfile());
  if (!file_system)  // |file_system| is NULL if Drive is disabled.
    return false;

  file_system->GetFile(
      drive_path,
      base::Bind(&FileManagerPrivateInternalEnsureFileDownloadedFunction::
                     OnDownloadFinished,
                 this));
  return true;
}

void FileManagerPrivateInternalEnsureFileDownloadedFunction::OnDownloadFinished(
    drive::FileError error,
    const base::FilePath& file_path,
    std::unique_ptr<drive::ResourceEntry> entry) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (error == drive::FILE_ERROR_OK) {
    SendResponse(true);
  } else {
    SetError(drive::FileErrorToString(error));
    SendResponse(false);
  }
}

bool FileManagerPrivateInternalCancelFileTransfersFunction::RunAsync() {
  using extensions::api::file_manager_private_internal::CancelFileTransfers::
      Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  drive::DriveIntegrationService* integration_service =
      drive::DriveIntegrationServiceFactory::FindForProfile(GetProfile());
  if (!integration_service || !integration_service->IsMounted())
    return false;

  drive::JobListInterface* const job_list = integration_service->job_list();
  DCHECK(job_list);
  const std::vector<drive::JobInfo> jobs = job_list->GetJobInfoList();

  // Create the mapping from file path to job ID.
  typedef std::map<base::FilePath, std::vector<drive::JobID>> PathToIdMap;
  PathToIdMap path_to_id_map;
  for (size_t i = 0; i < jobs.size(); ++i) {
    if (drive::IsActiveFileTransferJobInfo(jobs[i]))
      path_to_id_map[jobs[i].file_path].push_back(jobs[i].job_id);
  }

  for (size_t i = 0; i < params->urls.size(); ++i) {
    base::FilePath file_path = file_manager::util::GetLocalPathFromURL(
        render_frame_host(), GetProfile(), GURL(params->urls[i]));
    if (file_path.empty())
      continue;

    file_path = drive::util::ExtractDrivePath(file_path);
    DCHECK(file_path.empty());

    // Cancel all the jobs for the file.
    PathToIdMap::iterator it = path_to_id_map.find(file_path);
    if (it != path_to_id_map.end()) {
      for (size_t i = 0; i < it->second.size(); ++i)
        job_list->CancelJob(it->second[i]);
    }
  }

  SendResponse(true);
  return true;
}

bool FileManagerPrivateCancelAllFileTransfersFunction::RunAsync() {
  drive::DriveIntegrationService* const integration_service =
      drive::DriveIntegrationServiceFactory::FindForProfile(GetProfile());
  if (!integration_service || !integration_service->IsMounted())
    return false;

  drive::JobListInterface* const job_list = integration_service->job_list();
  DCHECK(job_list);
  const std::vector<drive::JobInfo> jobs = job_list->GetJobInfoList();

  for (size_t i = 0; i < jobs.size(); ++i) {
    if (drive::IsActiveFileTransferJobInfo(jobs[i]))
      job_list->CancelJob(jobs[i].job_id);
  }

  SendResponse(true);
  return true;
}

bool FileManagerPrivateSearchDriveFunction::RunAsync() {
  using extensions::api::file_manager_private::SearchDrive::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  drive::FileSystemInterface* const file_system =
      drive::util::GetFileSystemByProfile(GetProfile());
  if (!file_system) {
    // |file_system| is NULL if Drive is disabled.
    return false;
  }

  file_system->Search(
      params->search_params.query, GURL(params->search_params.next_feed),
      base::Bind(&FileManagerPrivateSearchDriveFunction::OnSearch, this));
  return true;
}

void FileManagerPrivateSearchDriveFunction::OnSearch(
    drive::FileError error,
    const GURL& next_link,
    std::unique_ptr<SearchResultInfoList> results) {
  if (error != drive::FILE_ERROR_OK) {
    SendResponse(false);
    return;
  }

  // Outlives the following conversion, since the pointer is bound to the
  // callback.
  DCHECK(results.get());
  const SearchResultInfoList& results_ref = *results.get();

  ConvertSearchResultInfoListToEntryDefinitionList(
      GetProfile(),
      extension_->id(),
      results_ref,
      base::Bind(&FileManagerPrivateSearchDriveFunction::OnEntryDefinitionList,
                 this,
                 next_link,
                 base::Passed(&results)));
}

void FileManagerPrivateSearchDriveFunction::OnEntryDefinitionList(
    const GURL& next_link,
    std::unique_ptr<SearchResultInfoList> search_result_info_list,
    std::unique_ptr<EntryDefinitionList> entry_definition_list) {
  DCHECK_EQ(search_result_info_list->size(), entry_definition_list->size());
  auto entries = std::make_unique<base::ListValue>();

  // Convert Drive files to something File API stack can understand.
  for (EntryDefinitionList::const_iterator it = entry_definition_list->begin();
       it != entry_definition_list->end();
       ++it) {
    auto entry = std::make_unique<base::DictionaryValue>();
    entry->SetString("fileSystemName", it->file_system_name);
    entry->SetString("fileSystemRoot", it->file_system_root_url);
    entry->SetString("fileFullPath", "/" + it->full_path.AsUTF8Unsafe());
    entry->SetBoolean("fileIsDirectory", it->is_directory);
    entries->Append(std::move(entry));
  }

  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  result->Set("entries", std::move(entries));
  result->SetString("nextFeed", next_link.spec());

  SetResult(std::move(result));
  SendResponse(true);
}

bool FileManagerPrivateSearchDriveMetadataFunction::RunAsync() {
  using api::file_manager_private::SearchDriveMetadata::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  drive::EventLogger* logger = file_manager::util::GetLogger(GetProfile());
  if (logger) {
    logger->Log(
        logging::LOG_INFO, "%s[%d] called. (types: '%s', maxResults: '%d')",
        name(), request_id(),
        api::file_manager_private::ToString(params->search_params.types),
        params->search_params.max_results);
  }
  set_log_on_completion(true);

  drive::FileSystemInterface* const file_system =
      drive::util::GetFileSystemByProfile(GetProfile());
  if (!file_system) {
    // |file_system| is NULL if Drive is disabled.
    return false;
  }

  int options = -1;
  switch (params->search_params.types) {
    case api::file_manager_private::SEARCH_TYPE_EXCLUDE_DIRECTORIES:
      options = drive::SEARCH_METADATA_EXCLUDE_DIRECTORIES;
      break;
    case api::file_manager_private::SEARCH_TYPE_SHARED_WITH_ME:
      options = drive::SEARCH_METADATA_SHARED_WITH_ME;
      break;
    case api::file_manager_private::SEARCH_TYPE_OFFLINE:
      options = drive::SEARCH_METADATA_OFFLINE;
      break;
    case api::file_manager_private::SEARCH_TYPE_ALL:
      options = drive::SEARCH_METADATA_ALL;
      break;
    case api::file_manager_private::SEARCH_TYPE_NONE:
      break;
  }
  DCHECK_NE(options, -1);

  file_system->SearchMetadata(
      params->search_params.query, options, params->search_params.max_results,
      drive::MetadataSearchOrder::LAST_ACCESSED,
      base::Bind(
          &FileManagerPrivateSearchDriveMetadataFunction::OnSearchMetadata,
          this));
  return true;
}

void FileManagerPrivateSearchDriveMetadataFunction::OnSearchMetadata(
    drive::FileError error,
    std::unique_ptr<drive::MetadataSearchResultVector> results) {
  if (error != drive::FILE_ERROR_OK) {
    SendResponse(false);
    return;
  }

  // Outlives the following conversion, since the pointer is bound to the
  // callback.
  DCHECK(results.get());
  const drive::MetadataSearchResultVector& results_ref = *results.get();

  ConvertSearchResultInfoListToEntryDefinitionList(
      GetProfile(),
      extension_->id(),
      results_ref,
      base::Bind(
          &FileManagerPrivateSearchDriveMetadataFunction::OnEntryDefinitionList,
          this,
          base::Passed(&results)));
}

void FileManagerPrivateSearchDriveMetadataFunction::OnEntryDefinitionList(
    std::unique_ptr<drive::MetadataSearchResultVector> search_result_info_list,
    std::unique_ptr<EntryDefinitionList> entry_definition_list) {
  DCHECK_EQ(search_result_info_list->size(), entry_definition_list->size());
  std::unique_ptr<base::ListValue> results_list(new base::ListValue());

  // Convert Drive files to something File API stack can understand.  See
  // file_browser_handler_custom_bindings.cc and
  // file_manager_private_custom_bindings.js for how this is magically
  // converted to a FileEntry.
  for (size_t i = 0; i < entry_definition_list->size(); ++i) {
    auto result_dict = std::make_unique<base::DictionaryValue>();

    // FileEntry fields.
    auto entry = std::make_unique<base::DictionaryValue>();
    entry->SetString(
        "fileSystemName", entry_definition_list->at(i).file_system_name);
    entry->SetString(
        "fileSystemRoot", entry_definition_list->at(i).file_system_root_url);
    entry->SetString(
        "fileFullPath",
        "/" + entry_definition_list->at(i).full_path.AsUTF8Unsafe());
    entry->SetBoolean("fileIsDirectory",
                      entry_definition_list->at(i).is_directory);

    result_dict->Set("entry", std::move(entry));
    result_dict->SetString(
        "highlightedBaseName",
        search_result_info_list->at(i).highlighted_base_name);
    results_list->Append(std::move(result_dict));
  }

  SetResult(std::move(results_list));
  SendResponse(true);
}

ExtensionFunction::ResponseAction
FileManagerPrivateGetDriveConnectionStateFunction::Run() {
  api::file_manager_private::DriveConnectionState result;

  switch (drive::util::GetDriveConnectionStatus(
      Profile::FromBrowserContext(browser_context()))) {
    case drive::util::DRIVE_DISCONNECTED_NOSERVICE:
      result.type = kDriveConnectionTypeOffline;
      result.reason.reset(new std::string(kDriveConnectionReasonNoService));
      break;
    case drive::util::DRIVE_DISCONNECTED_NONETWORK:
      result.type = kDriveConnectionTypeOffline;
      result.reason.reset(new std::string(kDriveConnectionReasonNoNetwork));
      break;
    case drive::util::DRIVE_DISCONNECTED_NOTREADY:
      result.type = kDriveConnectionTypeOffline;
      result.reason.reset(new std::string(kDriveConnectionReasonNotReady));
      break;
    case drive::util::DRIVE_CONNECTED_METERED:
      result.type = kDriveConnectionTypeMetered;
      break;
    case drive::util::DRIVE_CONNECTED:
      result.type = kDriveConnectionTypeOnline;
      break;
  }

  result.has_cellular_network_access =
      chromeos::NetworkHandler::Get()
          ->network_state_handler()
          ->FirstNetworkByType(chromeos::NetworkTypePattern::Mobile());

  drive::EventLogger* logger = file_manager::util::GetLogger(
      Profile::FromBrowserContext(browser_context()));
  if (logger)
    logger->Log(logging::LOG_INFO, "%s succeeded.", name());
  return RespondNow(ArgumentList(
      api::file_manager_private::GetDriveConnectionState::Results::Create(
          result)));
}

bool FileManagerPrivateRequestAccessTokenFunction::RunAsync() {
  using extensions::api::file_manager_private::RequestAccessToken::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  drive::DriveServiceInterface* const drive_service =
      drive::util::GetDriveServiceByProfile(GetProfile());

  if (!drive_service) {
    // DriveService is not available.
    SetResult(std::make_unique<base::Value>(std::string()));
    SendResponse(true);
    return true;
  }

  // If refreshing is requested, then clear the token to refetch it.
  if (params->refresh)
    drive_service->ClearAccessToken();

  // Retrieve the cached auth token (if available), otherwise the AuthService
  // instance will try to refetch it.
  drive_service->RequestAccessToken(
      base::Bind(&FileManagerPrivateRequestAccessTokenFunction::
                      OnAccessTokenFetched, this));
  return true;
}

void FileManagerPrivateRequestAccessTokenFunction::OnAccessTokenFetched(
    google_apis::DriveApiErrorCode code,
    const std::string& access_token) {
  SetResult(std::make_unique<base::Value>(access_token));
  SendResponse(true);
}

bool FileManagerPrivateInternalGetShareUrlFunction::RunAsync() {
  using extensions::api::file_manager_private_internal::GetShareUrl::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  const base::FilePath path = file_manager::util::GetLocalPathFromURL(
      render_frame_host(), GetProfile(), GURL(params->url));
  DCHECK(drive::util::IsUnderDriveMountPoint(path));

  const base::FilePath drive_path = drive::util::ExtractDrivePath(path);

  drive::FileSystemInterface* const file_system =
      drive::util::GetFileSystemByProfile(GetProfile());
  if (!file_system) {
    // |file_system| is NULL if Drive is disabled.
    return false;
  }

  file_system->GetShareUrl(
      drive_path,
      GURL("chrome-extension://" + extension_id()),  // embed origin
      base::Bind(&FileManagerPrivateInternalGetShareUrlFunction::OnGetShareUrl,
                 this));
  return true;
}

void FileManagerPrivateInternalGetShareUrlFunction::OnGetShareUrl(
    drive::FileError error,
    const GURL& share_url) {
  if (error != drive::FILE_ERROR_OK) {
    SetError("Share Url for this item is not available.");
    SendResponse(false);
    return;
  }

  SetResult(std::make_unique<base::Value>(share_url.spec()));
  SendResponse(true);
}

bool FileManagerPrivateInternalRequestDriveShareFunction::RunAsync() {
  using extensions::api::file_manager_private_internal::RequestDriveShare::
      Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  const base::FilePath path = file_manager::util::GetLocalPathFromURL(
      render_frame_host(), GetProfile(), GURL(params->url));
  const base::FilePath drive_path = drive::util::ExtractDrivePath(path);
  Profile* const owner_profile = drive::util::ExtractProfileFromPath(path);

  if (!owner_profile)
    return false;

  drive::FileSystemInterface* const owner_file_system =
      drive::util::GetFileSystemByProfile(owner_profile);
  if (!owner_file_system)
    return false;

  const user_manager::User* const user =
      chromeos::ProfileHelper::Get()->GetUserByProfile(GetProfile());
  if (!user || !user->is_logged_in())
    return false;

  google_apis::drive::PermissionRole role =
      google_apis::drive::PERMISSION_ROLE_READER;
  switch (params->share_type) {
    case api::file_manager_private::DRIVE_SHARE_TYPE_NONE:
      NOTREACHED();
      return false;
    case api::file_manager_private::DRIVE_SHARE_TYPE_CAN_EDIT:
      role = google_apis::drive::PERMISSION_ROLE_WRITER;
      break;
    case api::file_manager_private::DRIVE_SHARE_TYPE_CAN_COMMENT:
      role = google_apis::drive::PERMISSION_ROLE_COMMENTER;
      break;
    case api::file_manager_private::DRIVE_SHARE_TYPE_CAN_VIEW:
      role = google_apis::drive::PERMISSION_ROLE_READER;
      break;
  }

  // Share |drive_path| in |owner_file_system| to
  // |user->GetAccountId().GetUserEmail()|.
  owner_file_system->AddPermission(
      drive_path, user->GetAccountId().GetUserEmail(), role,
      base::Bind(
          &FileManagerPrivateInternalRequestDriveShareFunction::OnAddPermission,
          this));
  return true;
}

void FileManagerPrivateInternalRequestDriveShareFunction::OnAddPermission(
    drive::FileError error) {
  SendResponse(error == drive::FILE_ERROR_OK);
}

FileManagerPrivateInternalGetDownloadUrlFunction::
    FileManagerPrivateInternalGetDownloadUrlFunction() {}

FileManagerPrivateInternalGetDownloadUrlFunction::
    ~FileManagerPrivateInternalGetDownloadUrlFunction() {}

bool FileManagerPrivateInternalGetDownloadUrlFunction::RunAsync() {
  using extensions::api::file_manager_private_internal::GetShareUrl::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  // Start getting the file info.
  drive::FileSystemInterface* const file_system =
      drive::util::GetFileSystemByProfile(GetProfile());
  if (!file_system) {
    // |file_system| is NULL if Drive is disabled or not mounted.
    SetError("Drive is disabled or not mounted.");
    // Intentionally returns a blank.
    SetResult(std::make_unique<base::Value>(std::string()));
    return false;
  }

  const base::FilePath path = file_manager::util::GetLocalPathFromURL(
      render_frame_host(), GetProfile(), GURL(params->url));
  if (!drive::util::IsUnderDriveMountPoint(path)) {
    SetError("The given file is not in Drive.");
    // Intentionally returns a blank.
    SetResult(std::make_unique<base::Value>(std::string()));
    return false;
  }
  base::FilePath file_path = drive::util::ExtractDrivePath(path);

  file_system->GetResourceEntry(
      file_path,
      base::Bind(
          &FileManagerPrivateInternalGetDownloadUrlFunction::OnGetResourceEntry,
          this));
  return true;
}

void FileManagerPrivateInternalGetDownloadUrlFunction::OnGetResourceEntry(
    drive::FileError error,
    std::unique_ptr<drive::ResourceEntry> entry) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (error != drive::FILE_ERROR_OK) {
    SetError("Download Url for this item is not available.");
    // Intentionally returns a blank.
    SetResult(std::make_unique<base::Value>(std::string()));
    SendResponse(false);
    return;
  }

  DriveApiUrlGenerator url_generator(
      (GURL(google_apis::DriveApiUrlGenerator::kBaseUrlForProduction)),
      (GURL(
          google_apis::DriveApiUrlGenerator::kBaseThumbnailUrlForProduction)),
      google_apis::GetTeamDrivesIntegrationSwitch());
  download_url_ = url_generator.GenerateDownloadFileUrl(entry->resource_id());

  ProfileOAuth2TokenService* oauth2_token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(GetProfile());
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(GetProfile());
  const std::string& account_id = signin_manager->GetAuthenticatedAccountId();
  std::vector<std::string> scopes;
  scopes.push_back("https://www.googleapis.com/auth/drive.readonly");

  auth_service_.reset(
      new google_apis::AuthService(oauth2_token_service,
                                   account_id,
                                   GetProfile()->GetRequestContext(),
                                   scopes));
  auth_service_->StartAuthentication(base::Bind(
      &FileManagerPrivateInternalGetDownloadUrlFunction::OnTokenFetched, this));
}

void FileManagerPrivateInternalGetDownloadUrlFunction::OnTokenFetched(
    google_apis::DriveApiErrorCode code,
    const std::string& access_token) {
  if (code != google_apis::HTTP_SUCCESS) {
    SetError("Not able to fetch the token.");
    // Intentionally returns a blank.
    SetResult(std::make_unique<base::Value>(std::string()));
    SendResponse(false);
    return;
  }

  const std::string url =
      download_url_.Resolve("?alt=media&access_token=" + access_token).spec();
  SetResult(std::make_unique<base::Value>(url));

  SendResponse(true);
}

}  // namespace extensions
