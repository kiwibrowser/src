// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file declares utility functions for the installer. The original reason
// for putting these functions in installer\util library is so that we can
// separate out the critical logic and write unit tests for it.

#ifndef CHROME_INSTALLER_UTIL_INSTALL_UTIL_H_
#define CHROME_INSTALLER_UTIL_INSTALL_UTIL_H_

#include <windows.h>
#include <tchar.h>

#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/win/scoped_handle.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/util_constants.h"

class WorkItemList;

namespace base {
class Version;
}

// This is a utility class that provides common installation related
// utility methods that can be used by installer and also unit tested
// independently.
class InstallUtil {
 public:
  // Attempts to trigger the command that would be run by Active Setup for a
  // system-level Chrome. For use only when system-level Chrome is installed.
  static void TriggerActiveSetupCommand();

  // Launches given exe as admin on Vista.
  static bool ExecuteExeAsAdmin(const base::CommandLine& cmd, DWORD* exit_code);

  // Reads the uninstall command for Chromium from the Windows registry and
  // returns it. If |system_install| is true the command is read from HKLM,
  // otherwise from HKCU. Returns an empty CommandLine if Chrome is not
  // installed.
  static base::CommandLine GetChromeUninstallCmd(bool system_install);

  // Find the version of Chrome installed on the system by checking the
  // Google Update registry key. Fills |version| with the version or a
  // default-constructed Version if no version is found.
  // system_install: if true, looks for version number under the HKLM root,
  //                 otherwise looks under the HKCU.
  static void GetChromeVersion(BrowserDistribution* dist,
                               bool system_install,
                               base::Version* version);

  // Find the last critical update (version) of Chrome. Fills |version| with the
  // version or a default-constructed Version if no version is found. A critical
  // update is a specially flagged version (by Google Update) that contains an
  // important security fix.
  // system_install: if true, looks for version number under the HKLM root,
  //                 otherwise looks under the HKCU.
  static void GetCriticalUpdateVersion(BrowserDistribution* dist,
                                       bool system_install,
                                       base::Version* version);

  // This function checks if the current OS is supported for Chromium.
  static bool IsOSSupported();

  // Adds work items to |install_list| to set installer error information in the
  // registry for consumption by Google Update. |install_list| must be best-
  // effort with rollback disabled. |state_key| must be the full path to an
  // app's ClientState key.  See InstallerState::WriteInstallerResult for more
  // details.
  static void AddInstallerResultItems(bool system_install,
                                      const base::string16& state_key,
                                      installer::InstallStatus status,
                                      int string_resource_id,
                                      const base::string16* const launch_cmd,
                                      WorkItemList* install_list);

  // Returns true if this installation path is per user, otherwise returns false
  // (per machine install, meaning: the exe_path contains the path to Program
  // Files).
  // TODO(grt): consider replacing all callers with direct use of
  // InstallDetails.
  static bool IsPerUserInstall();

  // Returns true if the sentinel file exists (or the path cannot be obtained).
  static bool IsFirstRunSentinelPresent();

  // Test to see if a Start menu shortcut exists with the right toast activator
  // CLSID registered.
  static bool IsStartMenuShortcutWithActivatorGuidInstalled();

  // Returns the toast activator registry path if found, or an empty string in
  // case of error.
  static base::string16 GetToastActivatorRegistryPath();

  // Populates |path| with EULA sentinel file path. Returns false on error.
  static bool GetEULASentinelFilePath(base::FilePath* path);

  // Deletes the registry key at path key_path under the key given by root_key.
  static bool DeleteRegistryKey(HKEY root_key,
                                const base::string16& key_path,
                                REGSAM wow64_access);

  // Deletes the registry value named value_name at path key_path under the key
  // given by reg_root.
  static bool DeleteRegistryValue(HKEY reg_root, const base::string16& key_path,
                                  REGSAM wow64_access,
                                  const base::string16& value_name);

  // An interface to a predicate function for use by DeleteRegistryKeyIf and
  // DeleteRegistryValueIf.
  class RegistryValuePredicate {
   public:
    virtual ~RegistryValuePredicate() { }
    virtual bool Evaluate(const base::string16& value) const = 0;
  };

  // The result of a conditional delete operation (i.e., DeleteFOOIf).
  enum ConditionalDeleteResult {
    NOT_FOUND,      // The condition was not satisfied.
    DELETED,        // The condition was satisfied and the delete succeeded.
    DELETE_FAILED   // The condition was satisfied but the delete failed.
  };

  // Deletes the key |key_to_delete_path| under |root_key| iff the value
  // |value_name| in the key |key_to_test_path| under |root_key| satisfies
  // |predicate|.  |value_name| may be either NULL or an empty string to test
  // the key's default value.
  static ConditionalDeleteResult DeleteRegistryKeyIf(
      HKEY root_key,
      const base::string16& key_to_delete_path,
      const base::string16& key_to_test_path,
      REGSAM wow64_access,
      const wchar_t* value_name,
      const RegistryValuePredicate& predicate);

  // Deletes the value |value_name| in the key |key_path| under |root_key| iff
  // its current value satisfies |predicate|.  |value_name| may be either NULL
  // or an empty string to test/delete the key's default value.
  static ConditionalDeleteResult DeleteRegistryValueIf(
      HKEY root_key,
      const wchar_t* key_path,
      REGSAM wow64_access,
      const wchar_t* value_name,
      const RegistryValuePredicate& predicate);

  // A predicate that performs a case-sensitive string comparison.
  class ValueEquals : public RegistryValuePredicate {
   public:
    explicit ValueEquals(const base::string16& value_to_match)
        : value_to_match_(value_to_match) { }
    bool Evaluate(const base::string16& value) const override;
   protected:
    base::string16 value_to_match_;
   private:
    DISALLOW_COPY_AND_ASSIGN(ValueEquals);
  };

  // Returns zero on install success, or an InstallStatus value otherwise.
  static int GetInstallReturnCode(installer::InstallStatus install_status);

  // Composes |program| and |arguments| into |command_line|.
  static void ComposeCommandLine(const base::string16& program,
                                 const base::string16& arguments,
                                 base::CommandLine* command_line);

  // Appends the installer switch that selects the current install mode (see
  // install_static::InstallDetails).
  static void AppendModeSwitch(base::CommandLine* command_line);

  // Returns a string in the form YYYYMMDD of the current date.
  static base::string16 GetCurrentDate();

  // Returns the highest Chrome version that was installed prior to a downgrade,
  // or an invalid Version if Chrome was not previously downgraded from a newer
  // version.
  static base::Version GetDowngradeVersion(bool system_install,
                                           const BrowserDistribution* dist);

  // Adds or removes downgrade version registry value. This function should only
  // be used for Chrome install.
  static void AddUpdateDowngradeVersionItem(
      bool system_install,
      const base::Version* current_version,
      const base::Version& new_version,
      const BrowserDistribution* dist,
      WorkItemList* list);

  // Returns the registry key path and value name where the enrollment token is
  // stored for machine level user cloud policies.
  static void GetMachineLevelUserCloudPolicyEnrollmentTokenRegistryPath(
      std::wstring* key_path,
      std::wstring* value_name);

  // Returns the registry key path and value name where the enrollment token is
  // stored for machine level user cloud policies.
  static void GetMachineLevelUserCloudPolicyDMTokenRegistryPath(
      std::wstring* key_path,
      std::wstring* value_name);

  // Returns the token used to enroll this chrome instance for machine level
  // user cloud policies.  Returns an empty string if this machine should not
  // be enrolled.
  static std::wstring GetMachineLevelUserCloudPolicyEnrollmentToken();

  // A predicate that compares the program portion of a command line with a
  // given file path.  First, the file paths are compared directly.  If they do
  // not match, the filesystem is consulted to determine if the paths reference
  // the same file.
  class ProgramCompare : public RegistryValuePredicate {
   public:
    explicit ProgramCompare(const base::FilePath& path_to_match);
    ~ProgramCompare() override;
    bool Evaluate(const base::string16& value) const override;
    bool EvaluatePath(const base::FilePath& path) const;

   protected:
    static bool OpenForInfo(const base::FilePath& path, base::File* file);
    static bool GetInfo(const base::File& file,
                        BY_HANDLE_FILE_INFORMATION* info);

    base::FilePath path_to_match_;
    base::File file_;
    BY_HANDLE_FILE_INFORMATION file_info_;

   private:
    DISALLOW_COPY_AND_ASSIGN(ProgramCompare);
  };  // class ProgramCompare

 private:
  DISALLOW_COPY_AND_ASSIGN(InstallUtil);
};


#endif  // CHROME_INSTALLER_UTIL_INSTALL_UTIL_H_
