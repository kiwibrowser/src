// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/downgrade/user_data_downgrade.h"

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/time.h"
#include "base/version.h"
#include "base/win/registry.h"
#include "chrome/browser/policy/policy_path_parser.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_version.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/install_util.h"
#include "content/public/browser/browser_thread.h"

namespace downgrade {

namespace {

// Return the disk cache dir override value if exists or empty path for default
// disk cache dir.
base::FilePath GetDiskCacheDir() {
  base::FilePath disk_cache_dir;
  policy::path_parser::CheckDiskCacheDirPolicy(&disk_cache_dir);
  if (disk_cache_dir.empty()) {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    disk_cache_dir = command_line->GetSwitchValuePath(switches::kDiskCacheDir);
  }
  if (disk_cache_dir.ReferencesParent())
    return base::MakeAbsoluteFilePath(disk_cache_dir);
  return disk_cache_dir;
}

base::FilePath GetLastVersionFile(const base::FilePath& user_data_dir) {
  return user_data_dir.Append(kDowngradeLastVersionFile);
}

// Return the temporary path that |source_path| will be renamed to later.
base::FilePath GetTempDirNameForDelete(const base::FilePath& source_path) {
  if (source_path.empty())
    return base::FilePath();

  base::FilePath target_path(source_path.AddExtension(kDowngradeDeleteSuffix));
  int uniquifier =
      base::GetUniquePathNumber(source_path, kDowngradeDeleteSuffix);
  if (uniquifier < 0)
    return base::FilePath();
  if (uniquifier > 0) {
    return target_path.InsertBeforeExtensionASCII(
        base::StringPrintf(" (%d)", uniquifier));
  }

  return target_path;
}

// Rename the |source| directory to |target|. Create |source| directory after
// rename if |recreate| is true.
void RenameDirectory(const base::FilePath& source,
                     const base::FilePath& target,
                     bool recreate) {
  if (!source.empty() && !target.empty() && base::Move(source, target) &&
      recreate) {
    base::CreateDirectory(source);
  }
}

void DeleteAllRenamedUserDirectories(const base::FilePath& path) {
  if (path.empty())
    return;
  base::FilePath dir_name = path.DirName();
  // Does not support root directory
  if (dir_name == path)
    return;

  base::FilePath::StringType pattern =
      path.BaseName().value() + FILE_PATH_LITERAL("*") + kDowngradeDeleteSuffix;
  base::FileEnumerator enumerator(dir_name, false,
                                  base::FileEnumerator::DIRECTORIES, pattern);
  for (base::FilePath dir = enumerator.Next(); !dir.empty();
       dir = enumerator.Next()) {
    base::DeleteFile(dir, true);
  }
}

void DeleteMovedUserData(const base::FilePath& user_data_dir,
                         const base::FilePath& disk_cache_dir) {
  DeleteAllRenamedUserDirectories(user_data_dir);
  DeleteAllRenamedUserDirectories(disk_cache_dir);
}

enum InstallLevel { SYSTEM_INSTALL, USER_INSTALL };

InstallLevel GetInstallLevel() {
  return InstallUtil::IsPerUserInstall() ? USER_INSTALL : SYSTEM_INSTALL;
}

}  // namespace

const base::FilePath::CharType kDowngradeLastVersionFile[] =
    FILE_PATH_LITERAL("Last Version");
const base::FilePath::CharType kDowngradeDeleteSuffix[] =
    FILE_PATH_LITERAL(".CHROME_DELETE");

void MoveUserDataForFirstRunAfterDowngrade() {
  base::FilePath user_data_dir;
  if (!base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir))
    return;
  InstallLevel install_level = GetInstallLevel();
  base::Version current_version(chrome::kChromeVersion);
  base::Version downgrade_version = InstallUtil::GetDowngradeVersion(
      install_level == SYSTEM_INSTALL, BrowserDistribution::GetDistribution());
  if (downgrade_version.IsValid() && downgrade_version > current_version) {
    base::FilePath disk_cache_dir(GetDiskCacheDir());
    // Without the browser process singleton protection, the directory may be
    // copied multiple times. In order to prevent that from happening, the temp
    // directory's name will be computed before the Chrome Version file is
    // read. Because the deletion will be scheduled after the singleton is
    // acquired, the directory can only be moved twice in the worst case.
    // Also, doing this after the downgrade version check to reduce performance
    // cost for the normal launch.
    base::FilePath temp_disk_cache_dir(GetTempDirNameForDelete(disk_cache_dir));
    base::FilePath temp_user_data_dir(GetTempDirNameForDelete(user_data_dir));
    base::Version last_version = GetLastVersion(user_data_dir);
    if (last_version.IsValid() && last_version > current_version) {
      // Do not recreate |disk_cache_dir| as it will be initialized later.
      RenameDirectory(disk_cache_dir, temp_disk_cache_dir, false);
      RenameDirectory(user_data_dir, temp_user_data_dir, true);
    }
  }
}

void UpdateLastVersion(const base::FilePath& user_data_dir) {
  base::WriteFile(GetLastVersionFile(user_data_dir), chrome::kChromeVersion,
                  strlen(chrome::kChromeVersion));
}

base::Version GetLastVersion(const base::FilePath& user_data_dir) {
  std::string last_version_str;
  if (base::ReadFileToString(GetLastVersionFile(user_data_dir),
                             &last_version_str)) {
    return base::Version(
        base::TrimWhitespaceASCII(last_version_str, base::TRIM_ALL)
            .as_string());
  }
  return base::Version();
}

void DeleteMovedUserDataSoon() {
  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  content::BrowserThread::PostAfterStartupTask(
      FROM_HERE,
      base::CreateTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}),
      base::Bind(&DeleteMovedUserData, user_data_dir, GetDiskCacheDir()));
}

bool IsMSIInstall() {
  InstallLevel install_level = GetInstallLevel();
  base::win::RegKey key;
  DWORD is_msi = 3;
  return key.Open((install_level == SYSTEM_INSTALL) ? HKEY_LOCAL_MACHINE
                                                    : HKEY_CURRENT_USER,
                  BrowserDistribution::GetDistribution()->GetStateKey().c_str(),
                  KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS &&
         key.ReadValueDW(google_update::kRegMSIField, &is_msi) ==
             ERROR_SUCCESS &&
         is_msi != 0;
}

}  // namespace downgrade
