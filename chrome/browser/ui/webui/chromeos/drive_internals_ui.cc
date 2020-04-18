// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/drive_internals_ui.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/chromeos/drive/debug_info_collector.h"
#include "chrome/browser/chromeos/drive/drive_integration_service.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/file_manager/path_util.h"
#include "chrome/browser/drive/drive_notification_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "components/drive/drive.pb.h"
#include "components/drive/drive_api_util.h"
#include "components/drive/drive_notification_manager.h"
#include "components/drive/drive_pref_names.h"
#include "components/drive/event_logger.h"
#include "components/drive/job_list.h"
#include "components/drive/service/drive_service_interface.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "google_apis/drive/auth_service.h"
#include "google_apis/drive/drive_api_error_codes.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/time_util.h"

using content::BrowserThread;

namespace chromeos {

namespace {

// Gets metadata of all files and directories in |root_path|
// recursively. Stores the result as a list of dictionaries like:
//
// [{ path: 'GCache/v1/tmp/<local_id>',
//    size: 12345,
//    is_directory: false,
//    last_modified: '2005-08-09T09:57:00-08:00',
//  },...]
//
// The list is sorted by the path.
void GetGCacheContents(const base::FilePath& root_path,
                       base::ListValue* gcache_contents,
                       base::DictionaryValue* gcache_summary) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(gcache_contents);
  DCHECK(gcache_summary);

  // Use this map to sort the result list by the path.
  std::map<base::FilePath, std::unique_ptr<base::DictionaryValue>> files;

  const int options = (base::FileEnumerator::FILES |
                       base::FileEnumerator::DIRECTORIES |
                       base::FileEnumerator::SHOW_SYM_LINKS);
  base::FileEnumerator enumerator(root_path, true /* recursive */, options);

  int64_t total_size = 0;
  for (base::FilePath current = enumerator.Next(); !current.empty();
       current = enumerator.Next()) {
    base::FileEnumerator::FileInfo info = enumerator.GetInfo();
    int64_t size = info.GetSize();
    const bool is_directory = info.IsDirectory();
    const bool is_symbolic_link = base::IsLink(info.GetName());
    const base::Time last_modified = info.GetLastModifiedTime();

    auto entry = std::make_unique<base::DictionaryValue>();
    entry->SetString("path", current.value());
    // Use double instead of integer for large files.
    entry->SetDouble("size", size);
    entry->SetBoolean("is_directory", is_directory);
    entry->SetBoolean("is_symbolic_link", is_symbolic_link);
    entry->SetString(
        "last_modified",
        google_apis::util::FormatTimeAsStringLocaltime(last_modified));
    // Print lower 9 bits in octal format.
    entry->SetString(
        "permission",
        base::StringPrintf("%03o", info.stat().st_mode & 0x1ff));
    files[current] = std::move(entry);

    total_size += size;
  }

  // Convert |files| into |gcache_contents|.
  for (auto& it : files)
    gcache_contents->Append(std::move(it.second));

  gcache_summary->SetDouble("total_size", total_size);
}

// Gets the available disk space for the path |home_path|.
void GetFreeDiskSpace(const base::FilePath& home_path,
                      base::DictionaryValue* local_storage_summary) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(local_storage_summary);

  const int64_t free_space = base::SysInfo::AmountOfFreeDiskSpace(home_path);
  local_storage_summary->SetDouble("free_space", free_space);
}

// Formats |entry| into text.
std::string FormatEntry(const base::FilePath& path,
                        const drive::ResourceEntry& entry) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  using base::StringAppendF;

  std::string out;
  StringAppendF(&out, "%s\n", path.AsUTF8Unsafe().c_str());
  StringAppendF(&out, "  title: %s\n", entry.title().c_str());
  StringAppendF(&out, "  local_id: %s\n", entry.local_id().c_str());
  StringAppendF(&out, "  resource_id: %s\n", entry.resource_id().c_str());
  StringAppendF(&out, "  parent_local_id: %s\n",
                entry.parent_local_id().c_str());
  StringAppendF(&out, "  shared: %s\n", entry.shared() ? "true" : "false");
  StringAppendF(&out, "  shared_with_me: %s\n",
                entry.shared_with_me() ? "true" : "false");
  StringAppendF(&out, "  alternate_url: %s\n", entry.alternate_url().c_str());

  const drive::PlatformFileInfoProto& file_info = entry.file_info();
  StringAppendF(&out, "  file_info\n");
  StringAppendF(&out, "    size: %" PRId64 "\n", file_info.size());
  StringAppendF(&out, "    is_directory: %d\n", file_info.is_directory());
  StringAppendF(&out, "    is_symbolic_link: %d\n",
                file_info.is_symbolic_link());

  const base::Time last_modified = base::Time::FromInternalValue(
      file_info.last_modified());
  const base::Time last_modified_by_me =
      base::Time::FromInternalValue(entry.last_modified_by_me());
  const base::Time last_accessed = base::Time::FromInternalValue(
      file_info.last_accessed());
  const base::Time creation_time = base::Time::FromInternalValue(
      file_info.creation_time());
  StringAppendF(&out, "    last_modified: %s\n",
                google_apis::util::FormatTimeAsString(last_modified).c_str());
  StringAppendF(
      &out, "    last_modified_by_me: %s\n",
      google_apis::util::FormatTimeAsString(last_modified_by_me).c_str());
  StringAppendF(&out, "    last_accessed: %s\n",
                google_apis::util::FormatTimeAsString(last_accessed).c_str());
  StringAppendF(&out, "    creation_time: %s\n",
                google_apis::util::FormatTimeAsString(creation_time).c_str());

  if (entry.has_file_specific_info()) {
    const drive::FileSpecificInfo& file_specific_info =
        entry.file_specific_info();
    StringAppendF(&out, "    content_mime_type: %s\n",
                  file_specific_info.content_mime_type().c_str());
    StringAppendF(&out, "    file_md5: %s\n",
                  file_specific_info.md5().c_str());
    StringAppendF(&out, "    document_extension: %s\n",
                  file_specific_info.document_extension().c_str());
    StringAppendF(&out, "    is_hosted_document: %d\n",
                  file_specific_info.is_hosted_document());
  }

  if (entry.has_directory_specific_info()) {
    StringAppendF(&out, "  directory_info\n");
    const drive::DirectorySpecificInfo& directory_specific_info =
        entry.directory_specific_info();
    StringAppendF(&out, "    changestamp: %" PRId64 "\n",
                  directory_specific_info.changestamp());
  }

  return out;
}

std::string SeverityToString(logging::LogSeverity severity) {
  switch (severity) {
    case logging::LOG_INFO:
      return "info";
    case logging::LOG_WARNING:
      return "warning";
    case logging::LOG_ERROR:
      return "error";
    default:  // Treat all other higher severities as ERROR.
      return "error";
  }
}

// Appends {'key': key, 'value': value} dictionary to the |list|.
void AppendKeyValue(base::ListValue* list,
                    const std::string& key,
                    const std::string& value) {
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetString("key", key);
  dict->SetString("value", value);
  list->Append(std::move(dict));
}

// Class to handle messages from chrome://drive-internals.
class DriveInternalsWebUIHandler : public content::WebUIMessageHandler {
 public:
  DriveInternalsWebUIHandler()
      : last_sent_event_id_(-1),
        weak_ptr_factory_(this) {
  }

  ~DriveInternalsWebUIHandler() override {}

 private:
  // WebUIMessageHandler override.
  void RegisterMessages() override;

  // Returns a DriveIntegrationService.
  drive::DriveIntegrationService* GetIntegrationService();

  // Returns a DriveService instance.
  drive::DriveServiceInterface* GetDriveService();

  // Returns a DebugInfoCollector instance.
  drive::DebugInfoCollector* GetDebugInfoCollector();

  // Called when the page is first loaded.
  void OnPageLoaded(const base::ListValue* args);

  // Updates respective sections.
  void UpdateDriveRelatedPreferencesSection();
  void UpdateConnectionStatusSection(
      drive::DriveServiceInterface* drive_service);
  void UpdateAboutResourceSection(
      drive::DriveServiceInterface* drive_service);
  void UpdateAppListSection(
      drive::DriveServiceInterface* drive_service);
  void UpdateLocalMetadataSection(
      drive::DebugInfoCollector* debug_info_collector);
  void UpdateDeltaUpdateStatusSection(
      drive::DebugInfoCollector* debug_info_collector);
  void UpdateInFlightOperationsSection(drive::JobListInterface* job_list);
  void UpdateGCacheContentsSection();
  void UpdateFileSystemContentsSection();
  void UpdateLocalStorageUsageSection();
  void UpdateCacheContentsSection(
      drive::DebugInfoCollector* debug_info_collector);
  void UpdateEventLogSection();
  void UpdatePathConfigurationsSection();

  // Called when GetGCacheContents() is complete.
  void OnGetGCacheContents(base::ListValue* gcache_contents,
                           base::DictionaryValue* cache_summary);

  // Called when GetResourceEntryByPath() is complete.
  void OnGetResourceEntryByPath(const base::FilePath& path,
                                drive::FileError error,
                                std::unique_ptr<drive::ResourceEntry> entry);

  // Called when ReadDirectoryByPath() is complete.
  void OnReadDirectoryByPath(
      const base::FilePath& parent_path,
      drive::FileError error,
      std::unique_ptr<drive::ResourceEntryVector> entries);

  // Called as the iterator for DebugInfoCollector::IterateFileCache().
  void UpdateCacheEntry(const std::string& local_id,
                        const drive::FileCacheEntry& cache_entry);

  // Called when GetFreeDiskSpace() is complete.
  void OnGetFreeDiskSpace(base::DictionaryValue* local_storage_summary);

  // Called when GetAboutResource() call to DriveService is complete.
  void OnGetAboutResource(
      google_apis::DriveApiErrorCode status,
      std::unique_ptr<google_apis::AboutResource> about_resource);

  // Called when GetAppList() call to DriveService is complete.
  void OnGetAppList(google_apis::DriveApiErrorCode status,
                    std::unique_ptr<google_apis::AppList> app_list);

  // Callback for DebugInfoCollector::GetMetadata for local update.
  void OnGetFilesystemMetadataForLocal(
      const drive::FileSystemMetadata& metadata);

  // Callback for DebugInfoCollector::GetMetadata for delta update.
  void OnGetFilesystemMetadataForDeltaUpdate(
      const drive::FileSystemMetadata& metadata);

  // Called when the page requests periodic update.
  void OnPeriodicUpdate(const base::ListValue* args);

  // Called when the corresponding button on the page is pressed.
  void ClearAccessToken(const base::ListValue* args);
  void ClearRefreshToken(const base::ListValue* args);
  void ResetDriveFileSystem(const base::ListValue* args);
  void ListFileEntries(const base::ListValue* args);

  // Called after file system reset for ResetDriveFileSystem is done.
  void ResetFinished(bool success);

  // The last event sent to the JavaScript side.
  int last_sent_event_id_;

  base::WeakPtrFactory<DriveInternalsWebUIHandler> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DriveInternalsWebUIHandler);
};

void DriveInternalsWebUIHandler::OnGetAboutResource(
    google_apis::DriveApiErrorCode status,
    std::unique_ptr<google_apis::AboutResource> parsed_about_resource) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (status != google_apis::HTTP_SUCCESS) {
    LOG(ERROR) << "Failed to get about resource";
    return;
  }
  DCHECK(parsed_about_resource);

  base::DictionaryValue about_resource;
  about_resource.SetDouble("account-quota-total",
                           parsed_about_resource->quota_bytes_total());
  about_resource.SetDouble("account-quota-used",
                           parsed_about_resource->quota_bytes_used_aggregate());
  about_resource.SetDouble("account-largest-changestamp-remote",
                           parsed_about_resource->largest_change_id());
  about_resource.SetString("root-resource-id",
                           parsed_about_resource->root_folder_id());

  web_ui()->CallJavascriptFunctionUnsafe("updateAboutResource", about_resource);
}

void DriveInternalsWebUIHandler::OnGetAppList(
    google_apis::DriveApiErrorCode status,
    std::unique_ptr<google_apis::AppList> parsed_app_list) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (status != google_apis::HTTP_SUCCESS) {
    LOG(ERROR) << "Failed to get app list";
    return;
  }
  DCHECK(parsed_app_list);

  base::DictionaryValue app_list;
  app_list.SetString("etag", parsed_app_list->etag());

  auto items = std::make_unique<base::ListValue>();
  for (size_t i = 0; i < parsed_app_list->items().size(); ++i) {
    const google_apis::AppResource* app = parsed_app_list->items()[i].get();
    auto app_data = std::make_unique<base::DictionaryValue>();
    app_data->SetString("name", app->name());
    app_data->SetString("application_id", app->application_id());
    app_data->SetString("object_type", app->object_type());
    app_data->SetBoolean("supports_create", app->supports_create());

    items->Append(std::move(app_data));
  }
  app_list.Set("items", std::move(items));

  web_ui()->CallJavascriptFunctionUnsafe("updateAppList", app_list);
}

void DriveInternalsWebUIHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "pageLoaded",
      base::BindRepeating(&DriveInternalsWebUIHandler::OnPageLoaded,
                          weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "periodicUpdate",
      base::BindRepeating(&DriveInternalsWebUIHandler::OnPeriodicUpdate,
                          weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "clearAccessToken",
      base::BindRepeating(&DriveInternalsWebUIHandler::ClearAccessToken,
                          weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "clearRefreshToken",
      base::BindRepeating(&DriveInternalsWebUIHandler::ClearRefreshToken,
                          weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "resetDriveFileSystem",
      base::BindRepeating(&DriveInternalsWebUIHandler::ResetDriveFileSystem,
                          weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "listFileEntries",
      base::BindRepeating(&DriveInternalsWebUIHandler::ListFileEntries,
                          weak_ptr_factory_.GetWeakPtr()));
}

drive::DriveIntegrationService*
DriveInternalsWebUIHandler::GetIntegrationService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Profile* profile = Profile::FromWebUI(web_ui());
  drive::DriveIntegrationService* service =
      drive::DriveIntegrationServiceFactory::FindForProfile(profile);
  if (!service || !service->is_enabled())
    return NULL;
  return service;
}

drive::DriveServiceInterface* DriveInternalsWebUIHandler::GetDriveService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Profile* profile = Profile::FromWebUI(web_ui());
  return drive::util::GetDriveServiceByProfile(profile);
}

drive::DebugInfoCollector* DriveInternalsWebUIHandler::GetDebugInfoCollector() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  drive::DriveIntegrationService* integration_service = GetIntegrationService();
  return integration_service ?
      integration_service->debug_info_collector() : NULL;
}

void DriveInternalsWebUIHandler::OnPageLoaded(const base::ListValue* args) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  drive::DriveIntegrationService* integration_service =
      GetIntegrationService();
  // |integration_service| may be NULL in the guest/incognito mode.
  if (!integration_service)
    return;

  drive::DriveServiceInterface* drive_service =
      integration_service->drive_service();
  DCHECK(drive_service);
  drive::DebugInfoCollector* debug_info_collector =
      integration_service->debug_info_collector();
  DCHECK(debug_info_collector);

  UpdateDriveRelatedPreferencesSection();
  UpdateConnectionStatusSection(drive_service);
  UpdateAboutResourceSection(drive_service);
  UpdateAppListSection(drive_service);
  UpdateLocalMetadataSection(debug_info_collector);
  UpdateDeltaUpdateStatusSection(debug_info_collector);
  UpdateInFlightOperationsSection(integration_service->job_list());
  UpdateGCacheContentsSection();
  UpdateCacheContentsSection(debug_info_collector);
  UpdateLocalStorageUsageSection();
  UpdatePathConfigurationsSection();

  // When the drive-internals page is reloaded by the reload key, the page
  // content is recreated, but this WebUI object is not (instead, OnPageLoaded
  // is called again). In that case, we have to forget the last sent ID here,
  // and resent whole the logs to the page.
  last_sent_event_id_ = -1;
  UpdateEventLogSection();
}

void DriveInternalsWebUIHandler::UpdateDriveRelatedPreferencesSection() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const char* kDriveRelatedPreferences[] = {
      drive::prefs::kDisableDrive,
      drive::prefs::kDisableDriveOverCellular,
      drive::prefs::kDisableDriveHostedFiles,
  };

  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* pref_service = profile->GetPrefs();

  base::ListValue preferences;
  for (size_t i = 0; i < arraysize(kDriveRelatedPreferences); ++i) {
    const std::string key = kDriveRelatedPreferences[i];
    // As of now, all preferences are boolean.
    const std::string value =
        (pref_service->GetBoolean(key.c_str()) ? "true" : "false");
    AppendKeyValue(&preferences, key, value);
  }

  web_ui()->CallJavascriptFunctionUnsafe("updateDriveRelatedPreferences",
                                         preferences);
}

void DriveInternalsWebUIHandler::UpdateConnectionStatusSection(
    drive::DriveServiceInterface* drive_service) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(drive_service);

  std::string status;
  switch (drive::util::GetDriveConnectionStatus(Profile::FromWebUI(web_ui()))) {
    case drive::util::DRIVE_DISCONNECTED_NOSERVICE:
      status = "no service";
      break;
    case drive::util::DRIVE_DISCONNECTED_NONETWORK:
      status = "no network";
      break;
    case drive::util::DRIVE_DISCONNECTED_NOTREADY:
      status = "not ready";
      break;
    case drive::util::DRIVE_CONNECTED_METERED:
      status = "metered";
      break;
    case drive::util::DRIVE_CONNECTED:
      status = "connected";
      break;
  }

  base::DictionaryValue connection_status;
  connection_status.SetString("status", status);
  connection_status.SetBoolean("has-refresh-token",
                               drive_service->HasRefreshToken());
  connection_status.SetBoolean("has-access-token",
                               drive_service->HasAccessToken());
  web_ui()->CallJavascriptFunctionUnsafe("updateConnectionStatus",
                                         connection_status);
}

void DriveInternalsWebUIHandler::UpdateAboutResourceSection(
    drive::DriveServiceInterface* drive_service) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(drive_service);

  drive_service->GetAboutResource(
      base::Bind(&DriveInternalsWebUIHandler::OnGetAboutResource,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DriveInternalsWebUIHandler::UpdateAppListSection(
    drive::DriveServiceInterface* drive_service) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(drive_service);

  drive_service->GetAppList(
      base::Bind(&DriveInternalsWebUIHandler::OnGetAppList,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DriveInternalsWebUIHandler::UpdateLocalMetadataSection(
    drive::DebugInfoCollector* debug_info_collector) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(debug_info_collector);

  debug_info_collector->GetMetadata(
      base::Bind(&DriveInternalsWebUIHandler::OnGetFilesystemMetadataForLocal,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DriveInternalsWebUIHandler::OnGetFilesystemMetadataForLocal(
    const drive::FileSystemMetadata& metadata) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::DictionaryValue local_metadata;
  local_metadata.SetString("account-start-page-token-local",
                           metadata.start_page_token);
  local_metadata.SetBoolean("account-metadata-refreshing", metadata.refreshing);
  web_ui()->CallJavascriptFunctionUnsafe("updateLocalMetadata", local_metadata);
}

void DriveInternalsWebUIHandler::ClearAccessToken(const base::ListValue* args) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  drive::DriveServiceInterface* drive_service = GetDriveService();
  if (drive_service)
    drive_service->ClearAccessToken();
}

void DriveInternalsWebUIHandler::ClearRefreshToken(
    const base::ListValue* args) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  drive::DriveServiceInterface* drive_service = GetDriveService();
  if (drive_service)
    drive_service->ClearRefreshToken();
}

void DriveInternalsWebUIHandler::ResetDriveFileSystem(
    const base::ListValue* args) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  drive::DriveIntegrationService* integration_service =
      GetIntegrationService();
  if (integration_service) {
    integration_service->ClearCacheAndRemountFileSystem(
        base::Bind(&DriveInternalsWebUIHandler::ResetFinished,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void DriveInternalsWebUIHandler::ResetFinished(bool success) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  web_ui()->CallJavascriptFunctionUnsafe("updateResetStatus",
                                         base::Value(success));
}

void DriveInternalsWebUIHandler::ListFileEntries(const base::ListValue* args) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  UpdateFileSystemContentsSection();
}

void DriveInternalsWebUIHandler::UpdateDeltaUpdateStatusSection(
    drive::DebugInfoCollector* debug_info_collector) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(debug_info_collector);

  debug_info_collector->GetMetadata(
      base::Bind(
          &DriveInternalsWebUIHandler::OnGetFilesystemMetadataForDeltaUpdate,
          weak_ptr_factory_.GetWeakPtr()));
}

void DriveInternalsWebUIHandler::OnGetFilesystemMetadataForDeltaUpdate(
    const drive::FileSystemMetadata& metadata) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Profile* profile = Profile::FromWebUI(web_ui());
  drive::DriveNotificationManager* drive_notification_manager =
      drive::DriveNotificationManagerFactory::FindForBrowserContext(profile);
  if (!drive_notification_manager)
    return;

  base::DictionaryValue delta_update_status;
  delta_update_status.SetBoolean(
      "push-notification-enabled",
      drive_notification_manager->push_notification_enabled());
  delta_update_status.SetString(
      "last-update-check-time",
      google_apis::util::FormatTimeAsStringLocaltime(
          metadata.last_update_check_time));
  delta_update_status.SetString(
      "last-update-check-error",
      drive::FileErrorToString(metadata.last_update_check_error));

  web_ui()->CallJavascriptFunctionUnsafe("updateDeltaUpdateStatus",
                                         delta_update_status);
}

void DriveInternalsWebUIHandler::UpdateInFlightOperationsSection(
    drive::JobListInterface* job_list) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(job_list);

  std::vector<drive::JobInfo> info_list = job_list->GetJobInfoList();

  base::ListValue in_flight_operations;
  for (size_t i = 0; i < info_list.size(); ++i) {
    const drive::JobInfo& info = info_list[i];

    auto dict = std::make_unique<base::DictionaryValue>();
    dict->SetInteger("id", info.job_id);
    dict->SetString("type", drive::JobTypeToString(info.job_type));
    dict->SetString("file_path", info.file_path.AsUTF8Unsafe());
    dict->SetString("state", drive::JobStateToString(info.state));
    dict->SetDouble("progress_current", info.num_completed_bytes);
    dict->SetDouble("progress_total", info.num_total_bytes);
    in_flight_operations.Append(std::move(dict));
  }
  web_ui()->CallJavascriptFunctionUnsafe("updateInFlightOperations",
                                         in_flight_operations);
}

void DriveInternalsWebUIHandler::UpdateGCacheContentsSection() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Start updating the GCache contents section.
  Profile* profile = Profile::FromWebUI(web_ui());
  const base::FilePath root_path = drive::util::GetCacheRootPath(profile);
  base::ListValue* gcache_contents = new base::ListValue;
  base::DictionaryValue* gcache_summary = new base::DictionaryValue;
  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&GetGCacheContents, root_path, gcache_contents,
                     gcache_summary),
      base::BindOnce(&DriveInternalsWebUIHandler::OnGetGCacheContents,
                     weak_ptr_factory_.GetWeakPtr(),
                     base::Owned(gcache_contents),
                     base::Owned(gcache_summary)));
}

void DriveInternalsWebUIHandler::UpdateFileSystemContentsSection() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  drive::DebugInfoCollector* debug_info_collector = GetDebugInfoCollector();
  if (!debug_info_collector)
    return;

  // Start rendering the file system tree as text.
  const base::FilePath root_path = drive::util::GetDriveGrandRootPath();

  debug_info_collector->GetResourceEntry(
      root_path,
      base::Bind(&DriveInternalsWebUIHandler::OnGetResourceEntryByPath,
                 weak_ptr_factory_.GetWeakPtr(),
                 root_path));

  debug_info_collector->ReadDirectory(
      root_path,
      base::Bind(&DriveInternalsWebUIHandler::OnReadDirectoryByPath,
                 weak_ptr_factory_.GetWeakPtr(),
                 root_path));
}

void DriveInternalsWebUIHandler::UpdateLocalStorageUsageSection() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Propagate the amount of local free space in bytes.
  base::FilePath home_path;
  if (base::PathService::Get(base::DIR_HOME, &home_path)) {
    base::DictionaryValue* local_storage_summary = new base::DictionaryValue;
    base::PostTaskWithTraitsAndReply(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
        base::BindOnce(&GetFreeDiskSpace, home_path, local_storage_summary),
        base::BindOnce(&DriveInternalsWebUIHandler::OnGetFreeDiskSpace,
                       weak_ptr_factory_.GetWeakPtr(),
                       base::Owned(local_storage_summary)));
  } else {
    LOG(ERROR) << "Home directory not found";
  }
}

void DriveInternalsWebUIHandler::UpdateCacheContentsSection(
    drive::DebugInfoCollector* debug_info_collector) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(debug_info_collector);

  debug_info_collector->IterateFileCache(
      base::Bind(&DriveInternalsWebUIHandler::UpdateCacheEntry,
                 weak_ptr_factory_.GetWeakPtr()),
      base::DoNothing());
}

void DriveInternalsWebUIHandler::UpdateEventLogSection() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  drive::DriveIntegrationService* integration_service =
      GetIntegrationService();
  if (!integration_service)
    return;

  const std::vector<drive::EventLogger::Event> log =
      integration_service->event_logger()->GetHistory();

  base::ListValue list;
  for (size_t i = 0; i < log.size(); ++i) {
    // Skip events which were already sent.
    if (log[i].id <= last_sent_event_id_)
      continue;

    std::string severity = SeverityToString(log[i].severity);

    auto dict = std::make_unique<base::DictionaryValue>();
    dict->SetString("key",
        google_apis::util::FormatTimeAsStringLocaltime(log[i].when));
    dict->SetString("value", "[" + severity + "] " + log[i].what);
    dict->SetString("class", "log-" + severity);
    list.Append(std::move(dict));
    last_sent_event_id_ = log[i].id;
  }
  if (!list.empty())
    web_ui()->CallJavascriptFunctionUnsafe("updateEventLog", list);
}

void DriveInternalsWebUIHandler::UpdatePathConfigurationsSection() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Profile* const profile = Profile::FromWebUI(web_ui());

  base::ListValue paths;

  AppendKeyValue(
      &paths, "Downloads",
      file_manager::util::GetDownloadsFolderForProfile(profile).AsUTF8Unsafe());
  AppendKeyValue(
      &paths, "Drive",
      drive::util::GetDriveMountPointPath(profile).AsUTF8Unsafe());

  const char* kPathPreferences[] = {
    prefs::kSelectFileLastDirectory,
    prefs::kSaveFileDefaultDirectory,
    prefs::kDownloadDefaultDirectory,
  };
  for (size_t i = 0; i < arraysize(kPathPreferences); ++i) {
    const char* const key = kPathPreferences[i];
    AppendKeyValue(&paths, key,
                   profile->GetPrefs()->GetFilePath(key).AsUTF8Unsafe());
  }

  web_ui()->CallJavascriptFunctionUnsafe("updatePathConfigurations", paths);
}

void DriveInternalsWebUIHandler::OnGetGCacheContents(
    base::ListValue* gcache_contents,
    base::DictionaryValue* gcache_summary) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(gcache_contents);
  DCHECK(gcache_summary);

  web_ui()->CallJavascriptFunctionUnsafe("updateGCacheContents",
                                         *gcache_contents, *gcache_summary);
}

void DriveInternalsWebUIHandler::OnGetResourceEntryByPath(
    const base::FilePath& path,
    drive::FileError error,
    std::unique_ptr<drive::ResourceEntry> entry) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (error == drive::FILE_ERROR_OK) {
    DCHECK(entry.get());
    const base::Value value(FormatEntry(path, *entry) + "\n");
    web_ui()->CallJavascriptFunctionUnsafe("updateFileSystemContents", value);
  }
}

void DriveInternalsWebUIHandler::OnReadDirectoryByPath(
    const base::FilePath& parent_path,
    drive::FileError error,
    std::unique_ptr<drive::ResourceEntryVector> entries) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (error == drive::FILE_ERROR_OK) {
    DCHECK(entries.get());

    drive::DebugInfoCollector* debug_info_collector = GetDebugInfoCollector();
    std::string file_system_as_text;
    for (size_t i = 0; i < entries->size(); ++i) {
      const drive::ResourceEntry& entry = (*entries)[i];
      const base::FilePath current_path = parent_path.Append(
          base::FilePath::FromUTF8Unsafe(entry.base_name()));

      file_system_as_text.append(FormatEntry(current_path, entry) + "\n");

      if (entry.file_info().is_directory()) {
        debug_info_collector->ReadDirectory(
            current_path,
            base::Bind(&DriveInternalsWebUIHandler::OnReadDirectoryByPath,
                       weak_ptr_factory_.GetWeakPtr(),
                       current_path));
      }
    }

    // There may be pending ReadDirectoryByPath() calls, but we can update
    // the page with what we have now. This results in progressive
    // updates, which is good for a large file system.
    const base::Value value(file_system_as_text);
    web_ui()->CallJavascriptFunctionUnsafe("updateFileSystemContents", value);
  }
}

void DriveInternalsWebUIHandler::UpdateCacheEntry(
    const std::string& local_id,
    const drive::FileCacheEntry& cache_entry) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Convert |cache_entry| into a dictionary.
  base::DictionaryValue value;
  value.SetString("local_id", local_id);
  value.SetString("md5", cache_entry.md5());
  value.SetBoolean("is_present", cache_entry.is_present());
  value.SetBoolean("is_pinned", cache_entry.is_pinned());
  value.SetBoolean("is_dirty", cache_entry.is_dirty());

  web_ui()->CallJavascriptFunctionUnsafe("updateCacheContents", value);
}

void DriveInternalsWebUIHandler::OnGetFreeDiskSpace(
    base::DictionaryValue* local_storage_summary) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(local_storage_summary);

  web_ui()->CallJavascriptFunctionUnsafe("updateLocalStorageUsage",
                                         *local_storage_summary);
}

void DriveInternalsWebUIHandler::OnPeriodicUpdate(const base::ListValue* args) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  drive::DriveIntegrationService* integration_service =
      GetIntegrationService();
  // |integration_service| may be NULL in the guest/incognito mode.
  if (!integration_service)
    return;

  UpdateInFlightOperationsSection(integration_service->job_list());
  UpdateEventLogSection();
}

}  // namespace

DriveInternalsUI::DriveInternalsUI(content::WebUI* web_ui)
    : WebUIController(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<DriveInternalsWebUIHandler>());

  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIDriveInternalsHost);
  source->AddResourcePath("drive_internals.css", IDR_DRIVE_INTERNALS_CSS);
  source->AddResourcePath("drive_internals.js", IDR_DRIVE_INTERNALS_JS);
  source->SetDefaultResource(IDR_DRIVE_INTERNALS_HTML);

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, source);
}

}  // namespace chromeos
