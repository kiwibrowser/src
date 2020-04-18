// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines specific implementation of BrowserDistribution class for
// Google Chrome.

#include "chrome/installer/util/google_chrome_distribution.h"

#include <windows.h>
#include <msi.h>
#include <shellapi.h>

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#include "chrome/common/chrome_paths_internal.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/app_registration_data.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/installer_util_strings.h"
#include "chrome/installer/util/l10n_string_util.h"
#include "chrome/installer/util/uninstall_metrics.h"
#include "chrome/installer/util/updating_app_registration_data.h"
#include "chrome/installer/util/wmi.h"
#include "third_party/crashpad/crashpad/client/crash_report_database.h"
#include "third_party/crashpad/crashpad/client/settings.h"

namespace {

// Substitute the locale parameter in uninstall URL with whatever
// Google Update tells us is the locale. In case we fail to find
// the locale, we use US English.
base::string16 LocalizeUrl(const wchar_t* url) {
  base::string16 language;
  if (!GoogleUpdateSettings::GetLanguage(&language))
    language = L"en-US";  // Default to US English.
  return base::ReplaceStringPlaceholders(url, language, NULL);
}

base::string16 GetUninstallSurveyUrl() {
  const wchar_t kSurveyUrl[] = L"https://support.google.com/chrome/"
                               L"contact/chromeuninstall3?hl=$1";
  return LocalizeUrl(kSurveyUrl);
}

bool NavigateToUrlWithEdge(const base::string16& url) {
  base::string16 protocol_url = L"microsoft-edge:" + url;
  SHELLEXECUTEINFO info = { sizeof(info) };
  info.fMask = SEE_MASK_NOASYNC;
  info.lpVerb = L"open";
  info.lpFile = protocol_url.c_str();
  info.nShow = SW_SHOWNORMAL;
  if (::ShellExecuteEx(&info))
    return true;
  PLOG(ERROR) << "Failed to launch Edge for uninstall survey";
  return false;
}

void NavigateToUrlWithIExplore(const base::string16& url) {
  base::FilePath iexplore;
  if (!base::PathService::Get(base::DIR_PROGRAM_FILES, &iexplore))
    return;

  iexplore = iexplore.AppendASCII("Internet Explorer");
  iexplore = iexplore.AppendASCII("iexplore.exe");

  base::string16 command = L"\"" + iexplore.value() + L"\" " + url;

  int pid = 0;
  // The reason we use WMI to launch the process is because the uninstall
  // process runs inside a Job object controlled by the shell. As long as there
  // are processes running, the shell will not close the uninstall applet. WMI
  // allows us to escape from the Job object so the applet will close.
  installer::WMIProcess::Launch(command, &pid);
}

}  // namespace

GoogleChromeDistribution::GoogleChromeDistribution()
    : BrowserDistribution(std::make_unique<UpdatingAppRegistrationData>(
          install_static::GetAppGuid())) {}

void GoogleChromeDistribution::DoPostUninstallOperations(
    const base::Version& version,
    const base::FilePath& local_data_path,
    const base::string16& distribution_data) {
  // Send the Chrome version and OS version as params to the form.
  // It would be nice to send the locale, too, but I don't see an
  // easy way to get that in the existing code. It's something we
  // can add later, if needed.
  // We depend on installed_version.GetString() not having spaces or other
  // characters that need escaping: 0.2.13.4. Should that change, we will
  // need to escape the string before using it in a URL.
  const base::string16 kVersionParam = L"crversion";
  const base::string16 kOSParam = L"os";

  const base::win::OSInfo* os_info = base::win::OSInfo::GetInstance();
  base::win::OSInfo::VersionNumber version_number = os_info->version_number();
  base::string16 os_version =
      base::StringPrintf(L"%d.%d.%d", version_number.major,
                         version_number.minor, version_number.build);

  base::string16 url = GetUninstallSurveyUrl() + L"&" + kVersionParam + L"=" +
                       base::ASCIIToUTF16(version.GetString()) + L"&" +
                       kOSParam + L"=" + os_version;

  base::string16 uninstall_metrics;
  if (installer::ExtractUninstallMetricsFromFile(local_data_path,
                                                 &uninstall_metrics)) {
    // The user has opted into anonymous usage data collection, so append
    // metrics and distribution data.
    url += uninstall_metrics;
    if (!distribution_data.empty()) {
      url += L"&";
      url += distribution_data;
    }
  }

  if (os_info->version() >= base::win::VERSION_WIN10 &&
      NavigateToUrlWithEdge(url)) {
    return;
  }
  NavigateToUrlWithIExplore(url);
}

base::string16 GoogleChromeDistribution::GetPublisherName() {
  const base::string16& publisher_name =
      installer::GetLocalizedString(IDS_ABOUT_VERSION_COMPANY_NAME_BASE);
  return publisher_name;
}

base::string16 GoogleChromeDistribution::GetAppDescription() {
  const base::string16& app_description =
      installer::GetLocalizedString(IDS_SHORTCUT_TOOLTIP_BASE);
  return app_description;
}

std::string GoogleChromeDistribution::GetSafeBrowsingName() {
  return "googlechrome";
}

base::string16 GoogleChromeDistribution::GetDistributionData(HKEY root_key) {
  base::string16 sub_key(google_update::kRegPathClientState);
  sub_key.append(L"\\");
  sub_key.append(install_static::GetAppGuid());

  base::win::RegKey client_state_key(
      root_key, sub_key.c_str(), KEY_READ | KEY_WOW64_32KEY);
  base::string16 result;
  base::string16 brand_value;
  if (client_state_key.ReadValue(google_update::kRegRLZBrandField,
                                 &brand_value) == ERROR_SUCCESS) {
    result = google_update::kRegRLZBrandField;
    result.append(L"=");
    result.append(brand_value);
    result.append(L"&");
  }

  base::string16 client_value;
  if (client_state_key.ReadValue(google_update::kRegClientField,
                                 &client_value) == ERROR_SUCCESS) {
    result.append(google_update::kRegClientField);
    result.append(L"=");
    result.append(client_value);
    result.append(L"&");
  }

  base::string16 ap_value;
  // If we fail to read the ap key, send up "&ap=" anyway to indicate
  // that this was probably a stable channel release.
  client_state_key.ReadValue(google_update::kRegApField, &ap_value);
  result.append(google_update::kRegApField);
  result.append(L"=");
  result.append(ap_value);

  // Crash client id.
  // While it would be convenient to use the path service to get
  // chrome::DIR_CRASH_DUMPS, that points to the dump location for the installer
  // rather than for the browser. For per-user installs they are the same, yet
  // for system-level installs the installer uses the system temp directory (see
  // setup/installer_crash_reporting.cc's ConfigureCrashReporting).
  // TODO(grt): use install_static::GetDefaultCrashDumpLocation (with an option
  // to suppress creating the directory) once setup.exe uses
  // install_static::InstallDetails.
  base::FilePath crash_dir;
  if (chrome::GetDefaultUserDataDirectory(&crash_dir)) {
    crash_dir = crash_dir.Append(FILE_PATH_LITERAL("Crashpad"));
    crashpad::UUID client_id;
    std::unique_ptr<crashpad::CrashReportDatabase> database(
        crashpad::CrashReportDatabase::InitializeWithoutCreating(crash_dir));
    if (database && database->GetSettings()->GetClientID(&client_id))
      result.append(L"&crash_client_id=").append(client_id.ToString16());
  }

  return result;
}

// This method checks if we need to change "ap" key in Google Update to try
// full installer as fall back method in case incremental installer fails.
// - If incremental installer fails we append a magic string ("-full"), if
// it is not present already, so that Google Update server next time will send
// full installer to update Chrome on the local machine
// - If we are currently running full installer, we remove this magic
// string (if it is present) regardless of whether installer failed or not.
// There is no fall-back for full installer :)
void GoogleChromeDistribution::UpdateInstallStatus(bool system_install,
    installer::ArchiveType archive_type,
    installer::InstallStatus install_status) {
  GoogleUpdateSettings::UpdateInstallStatus(
      system_install, archive_type,
      InstallUtil::GetInstallReturnCode(install_status),
      install_static::GetAppGuid());
}
