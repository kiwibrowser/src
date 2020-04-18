// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/chromeos/device_storage_handler.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <string>

#include "base/files/file_util.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browsing_data/browsing_data_appcache_helper.h"
#include "chrome/browser/browsing_data/browsing_data_cache_storage_helper.h"
#include "chrome/browser/browsing_data/browsing_data_channel_id_helper.h"
#include "chrome/browser/browsing_data/browsing_data_cookie_helper.h"
#include "chrome/browser/browsing_data/browsing_data_database_helper.h"
#include "chrome/browser/browsing_data/browsing_data_file_system_helper.h"
#include "chrome/browser/browsing_data/browsing_data_flash_lso_helper.h"
#include "chrome/browser/browsing_data/browsing_data_indexed_db_helper.h"
#include "chrome/browser/browsing_data/browsing_data_local_storage_helper.h"
#include "chrome/browser/browsing_data/browsing_data_service_worker_helper.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/file_manager/path_util.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/cryptohome/cryptohome_util.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "components/arc/arc_util.h"
#include "components/browsing_data/content/conditional_cache_counting_helper.h"
#include "components/drive/chromeos/file_system_interface.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/text/bytes_formatting.h"

namespace chromeos {
namespace settings {
namespace {

void GetSizeStatBlocking(const base::FilePath& mount_path,
                         int64_t* total_size,
                         int64_t* available_size) {
  int64_t size = base::SysInfo::AmountOfTotalDiskSpace(mount_path);
  if (size >= 0)
    *total_size = size;
  size = base::SysInfo::AmountOfFreeDiskSpace(mount_path);
  if (size >= 0)
    *available_size = size;
}

// Threshold to show a message indicating space is critically low (512 MB).
const int64_t kSpaceCriticallyLowBytes = 512 * 1024 * 1024;

// Threshold to show a message indicating space is low (1 GB).
const int64_t kSpaceLowBytes = 1 * 1024 * 1024 * 1024;

}  // namespace

StorageHandler::StorageHandler()
    : browser_cache_size_(-1),
      has_browser_cache_size_(false),
      browser_site_data_size_(-1),
      has_browser_site_data_size_(false),
      updating_downloads_size_(false),
      updating_drive_cache_size_(false),
      updating_browsing_data_size_(false),
      updating_android_size_(false),
      updating_other_users_size_(false),
      weak_ptr_factory_(this) {}

StorageHandler::~StorageHandler() {
}

void StorageHandler::RegisterMessages() {
  DCHECK(web_ui());

  web_ui()->RegisterMessageCallback(
      "updateStorageInfo",
      base::BindRepeating(&StorageHandler::HandleUpdateStorageInfo,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "openDownloads", base::BindRepeating(&StorageHandler::HandleOpenDownloads,
                                           base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "openArcStorage",
      base::BindRepeating(&StorageHandler::HandleOpenArcStorage,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "clearDriveCache",
      base::BindRepeating(&StorageHandler::HandleClearDriveCache,
                          base::Unretained(this)));
}

void StorageHandler::HandleUpdateStorageInfo(const base::ListValue* args) {
  AllowJavascript();

  UpdateSizeStat();
  UpdateDownloadsSize();
  UpdateDriveCacheSize();
  UpdateBrowsingDataSize();
  UpdateOtherUsersSize();
  UpdateAndroidSize();
}

void StorageHandler::HandleOpenDownloads(
    const base::ListValue* unused_args) {
  Profile* const profile = Profile::FromWebUI(web_ui());
  const base::FilePath downloads_path =
      file_manager::util::GetDownloadsFolderForProfile(profile);
  platform_util::OpenItem(
       profile,
       downloads_path,
       platform_util::OPEN_FOLDER,
       platform_util::OpenOperationCallback());
}

void StorageHandler::HandleOpenArcStorage(
    const base::ListValue* unused_args) {
  auto* arc_storage_manager = arc::ArcStorageManager::GetForBrowserContext(
      Profile::FromWebUI(web_ui()));
  if (arc_storage_manager)
    arc_storage_manager->OpenPrivateVolumeSettings();
}

void StorageHandler::HandleClearDriveCache(
    const base::ListValue* unused_args) {
  drive::FileSystemInterface* const file_system =
      drive::util::GetFileSystemByProfile(Profile::FromWebUI(web_ui()));
  file_system->FreeDiskSpaceIfNeededFor(
      std::numeric_limits<int64_t>::max(),  // Removes as much as possible.
      base::Bind(&StorageHandler::OnClearDriveCacheDone,
                 weak_ptr_factory_.GetWeakPtr()));
}

void StorageHandler::UpdateSizeStat() {
  Profile* const profile = Profile::FromWebUI(web_ui());
  const base::FilePath downloads_path =
     file_manager::util::GetDownloadsFolderForProfile(profile);

  int64_t* total_size = new int64_t(0);
  int64_t* available_size = new int64_t(0);
  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::Bind(&GetSizeStatBlocking, downloads_path, total_size,
                 available_size),
      base::Bind(&StorageHandler::OnGetSizeStat, weak_ptr_factory_.GetWeakPtr(),
                 base::Owned(total_size), base::Owned(available_size)));
}

void StorageHandler::OnGetSizeStat(int64_t* total_size,
                                   int64_t* available_size) {
  int64_t used_size = *total_size - *available_size;
  base::DictionaryValue size_stat;
  size_stat.SetString("totalSize", ui::FormatBytes(*total_size));
  size_stat.SetString("availableSize", ui::FormatBytes(*available_size));
  size_stat.SetString("usedSize", ui::FormatBytes(used_size));
  size_stat.SetDouble("usedRatio",
                      static_cast<double>(used_size) / *total_size);
  int storage_space_state = STORAGE_SPACE_NORMAL;
  if (*available_size < kSpaceCriticallyLowBytes)
    storage_space_state = STORAGE_SPACE_CRITICALLY_LOW;
  else if (*available_size < kSpaceLowBytes)
    storage_space_state = STORAGE_SPACE_LOW;
  size_stat.SetInteger("spaceState", storage_space_state);

  FireWebUIListener("storage-size-stat-changed", size_stat);
}

void StorageHandler::UpdateDownloadsSize() {
  if (updating_downloads_size_)
    return;
  updating_downloads_size_ = true;

  Profile* const profile = Profile::FromWebUI(web_ui());
  const base::FilePath downloads_path =
      file_manager::util::GetDownloadsFolderForProfile(profile);

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&base::ComputeDirectorySize, downloads_path),
      base::Bind(&StorageHandler::OnGetDownloadsSize,
                 weak_ptr_factory_.GetWeakPtr()));
}

void StorageHandler::OnGetDownloadsSize(int64_t size) {
  updating_downloads_size_ = false;
  FireWebUIListener("storage-downloads-size-changed",
                    base::Value(ui::FormatBytes(size)));
}

void StorageHandler::UpdateDriveCacheSize() {
  if (updating_drive_cache_size_)
    return;

  drive::FileSystemInterface* const file_system =
      drive::util::GetFileSystemByProfile(Profile::FromWebUI(web_ui()));
  if (!file_system)
    return;

  // Shows the item "Offline cache" and start calculating size.
  FireWebUIListener("storage-drive-enabled-changed", base::Value(true));
  updating_drive_cache_size_ = true;
  file_system->CalculateCacheSize(base::Bind(
      &StorageHandler::OnGetDriveCacheSize, weak_ptr_factory_.GetWeakPtr()));
}

void StorageHandler::OnGetDriveCacheSize(int64_t size) {
  updating_drive_cache_size_ = false;
  FireWebUIListener("storage-drive-cache-size-changed",
                    base::Value(ui::FormatBytes(size)), base::Value(size > 0));
}

void StorageHandler::UpdateBrowsingDataSize() {
  if (updating_browsing_data_size_)
    return;
  updating_browsing_data_size_ = true;

  has_browser_cache_size_ = false;
  has_browser_site_data_size_ = false;
  Profile* const profile = Profile::FromWebUI(web_ui());
  // Fetch the size of http cache in browsing data.
  // ConditionalCacheCountingHelper deletes itself when it is done.
  browsing_data::ConditionalCacheCountingHelper::CreateForRange(
      content::BrowserContext::GetDefaultStoragePartition(profile),
      base::Time(), base::Time::Max())
      ->CountAndDestroySelfWhenFinished(base::Bind(
          &StorageHandler::OnGetCacheSize, weak_ptr_factory_.GetWeakPtr()));

  // Fetch the size of site data in browsing data.
  if (!site_data_size_collector_.get()) {
    content::StoragePartition* storage_partition =
        content::BrowserContext::GetDefaultStoragePartition(profile);
    site_data_size_collector_.reset(new SiteDataSizeCollector(
        storage_partition->GetPath(),
        new BrowsingDataCookieHelper(profile->GetRequestContext()),
        new BrowsingDataDatabaseHelper(profile),
        new BrowsingDataLocalStorageHelper(profile),
        new BrowsingDataAppCacheHelper(profile),
        new BrowsingDataIndexedDBHelper(
            storage_partition->GetIndexedDBContext()),
        BrowsingDataFileSystemHelper::Create(
            storage_partition->GetFileSystemContext()),
        BrowsingDataChannelIDHelper::Create(profile->GetRequestContext()),
        new BrowsingDataServiceWorkerHelper(
            storage_partition->GetServiceWorkerContext()),
        new BrowsingDataCacheStorageHelper(
            storage_partition->GetCacheStorageContext()),
        BrowsingDataFlashLSOHelper::Create(profile)));
  }
  site_data_size_collector_->Fetch(
      base::Bind(&StorageHandler::OnGetBrowsingDataSize,
                 weak_ptr_factory_.GetWeakPtr(), true));
}

void StorageHandler::OnGetCacheSize(bool is_upper_limit, int64_t size) {
  DCHECK(!is_upper_limit);
  OnGetBrowsingDataSize(false, size);
}

void StorageHandler::OnGetBrowsingDataSize(bool is_site_data, int64_t size) {
  if (is_site_data) {
    has_browser_site_data_size_ = true;
    browser_site_data_size_ = size;
  } else {
    has_browser_cache_size_ = true;
    browser_cache_size_ = size;
  }
  if (has_browser_cache_size_ && has_browser_site_data_size_) {
    base::string16 size_string;
    if (browser_cache_size_ >= 0 && browser_site_data_size_ >= 0) {
      size_string = ui::FormatBytes(
          browser_site_data_size_ + browser_cache_size_);
    } else {
      size_string =
          l10n_util::GetStringUTF16(IDS_SETTINGS_STORAGE_SIZE_UNKNOWN);
    }
    updating_browsing_data_size_ = false;
    FireWebUIListener("storage-browsing-data-size-changed",
                      base::Value(size_string));
  }
}

void StorageHandler::UpdateOtherUsersSize() {
  if (updating_other_users_size_)
    return;
  updating_other_users_size_ = true;

  other_users_.clear();
  user_sizes_.clear();
  const user_manager::UserList& users =
      user_manager::UserManager::Get()->GetUsers();
  for (auto* user : users) {
    if (user->is_active())
      continue;
    other_users_.push_back(user);
    DBusThreadManager::Get()->GetCryptohomeClient()->GetAccountDiskUsage(
        cryptohome::Identification(user->GetAccountId()),
        base::BindOnce(&StorageHandler::OnGetOtherUserSize,
                       weak_ptr_factory_.GetWeakPtr()));
  }
  // We should show "0 B" if there is no other user.
  if (other_users_.empty()) {
    updating_other_users_size_ = false;
    FireWebUIListener("storage-other-users-size-changed",
                      base::Value(ui::FormatBytes(0)));
  }
}

void StorageHandler::OnGetOtherUserSize(
    base::Optional<cryptohome::BaseReply> reply) {
  user_sizes_.push_back(cryptohome::AccountDiskUsageReplyToUsageSize(reply));
  if (user_sizes_.size() == other_users_.size()) {
    base::string16 size_string;
    // If all the requests succeed, shows the total bytes in the UI.
    if (std::count(user_sizes_.begin(), user_sizes_.end(), -1) == 0) {
      size_string = ui::FormatBytes(
          std::accumulate(user_sizes_.begin(), user_sizes_.end(), 0LL));
    } else {
      size_string =
          l10n_util::GetStringUTF16(IDS_SETTINGS_STORAGE_SIZE_UNKNOWN);
    }
    updating_other_users_size_ = false;
    FireWebUIListener("storage-other-users-size-changed",
                      base::Value(size_string));
  }
}

void StorageHandler::UpdateAndroidSize() {
  if (updating_android_size_)
    return;
  updating_android_size_ = true;

  Profile* const profile = Profile::FromWebUI(web_ui());
  if (!arc::IsArcPlayStoreEnabledForProfile(profile) ||
      arc::IsArcOptInVerificationDisabled()) {
    return;
  }

  // Shows the item "Android apps and cache" and start calculating size.
  FireWebUIListener("storage-android-enabled-changed", base::Value(true));
  bool success = false;
  auto* arc_storage_manager =
      arc::ArcStorageManager::GetForBrowserContext(profile);
  if (arc_storage_manager) {
    success = arc_storage_manager->GetApplicationsSize(base::BindOnce(
        &StorageHandler::OnGetAndroidSize, weak_ptr_factory_.GetWeakPtr()));
  }
  if (!success)
    updating_android_size_ = false;
}

void StorageHandler::OnGetAndroidSize(bool succeeded,
                                      arc::mojom::ApplicationsSizePtr size) {
  base::string16 size_string;
  if (succeeded) {
    uint64_t total_bytes = size->total_code_bytes +
                           size->total_data_bytes +
                           size->total_cache_bytes;
    size_string = ui::FormatBytes(total_bytes);
  } else {
    size_string = l10n_util::GetStringUTF16(IDS_SETTINGS_STORAGE_SIZE_UNKNOWN);
  }
  updating_android_size_ = false;
  FireWebUIListener("storage-android-size-changed", base::Value(size_string));
}

void StorageHandler::OnClearDriveCacheDone(bool success) {
  UpdateDriveCacheSize();
}

}  // namespace settings
}  // namespace chromeos
