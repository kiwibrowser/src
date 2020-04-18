// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/chrome_test_launcher.h"

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/debug/leak_annotations.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/process/process_metrics.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/test_file_util.h"
#include "build/build_config.h"
#include "chrome/app/chrome_main_delegate.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/install_static/test/scoped_install_details.h"
#include "chrome/test/base/chrome_test_suite.h"
#include "chrome/utility/chrome_content_utility_client.h"
#include "components/crash/content/app/crashpad.h"
#include "content/public/app/content_main.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/network_service_test_helper.h"
#include "content/public/test/test_launcher.h"
#include "content/public/test/test_utils.h"
#include "services/service_manager/runner/common/switches.h"
#include "ui/base/test/ui_controls.h"

#if defined(OS_MACOSX)
#include "chrome/browser/chrome_browser_application_mac.h"
#endif  // defined(OS_MACOSX)

#if defined(USE_AURA)
#include "ui/aura/test/ui_controls_factory_aura.h"
#include "ui/base/test/ui_controls_aura.h"
#endif

#if defined(OS_CHROMEOS)
#include "ash/mojo_interface_factory.h"
#include "ash/mojo_test_interface_factory.h"
#include "ash/test/ui_controls_factory_ash.h"
#endif

#if defined(OS_LINUX) || defined(OS_ANDROID)
#include "chrome/app/chrome_crash_reporter_client.h"
#endif

#if defined(OS_WIN)
#include "base/win/registry.h"
#include "chrome/app/chrome_crash_reporter_client_win.h"
#include "chrome/install_static/install_util.h"
#endif

ChromeTestSuiteRunner::ChromeTestSuiteRunner() {}
ChromeTestSuiteRunner::~ChromeTestSuiteRunner() {}

int ChromeTestSuiteRunner::RunTestSuite(int argc, char** argv) {
  return ChromeTestSuite(argc, argv).Run();
}

ChromeTestLauncherDelegate::ChromeTestLauncherDelegate(
    ChromeTestSuiteRunner* runner)
    : runner_(runner) {}
ChromeTestLauncherDelegate::~ChromeTestLauncherDelegate() {}

int ChromeTestLauncherDelegate::RunTestSuite(int argc, char** argv) {
  return runner_->RunTestSuite(argc, argv);
}

bool ChromeTestLauncherDelegate::AdjustChildProcessCommandLine(
    base::CommandLine* command_line,
    const base::FilePath& temp_data_dir) {
  base::CommandLine new_command_line(command_line->GetProgram());
  base::CommandLine::SwitchMap switches = command_line->GetSwitches();

  // Strip out user-data-dir if present.  We will add it back in again later.
  switches.erase(switches::kUserDataDir);

  for (base::CommandLine::SwitchMap::const_iterator iter = switches.begin();
       iter != switches.end(); ++iter) {
    new_command_line.AppendSwitchNative((*iter).first, (*iter).second);
  }

  new_command_line.AppendSwitchPath(switches::kUserDataDir, temp_data_dir);

  *command_line = new_command_line;
  return true;
}

content::ContentMainDelegate*
ChromeTestLauncherDelegate::CreateContentMainDelegate() {
  return new ChromeMainDelegate();
}

void ChromeTestLauncherDelegate::PreSharding() {
#if defined(OS_WIN)
  // Pre-test cleanup for registry state keyed off the profile dir (which can
  // proliferate with the use of uniquely named scoped_dirs):
  // https://crbug.com/721245. This needs to be here in order not to be racy
  // with any tests that will access that state.
  base::win::RegKey distrubution_key;
  LONG result = distrubution_key.Open(HKEY_CURRENT_USER,
                                      install_static::GetRegistryPath().c_str(),
                                      KEY_SET_VALUE);

  if (result != ERROR_SUCCESS) {
    LOG_IF(ERROR, result != ERROR_FILE_NOT_FOUND)
        << "Failed to open distribution key for cleanup: " << result;
    return;
  }

  result = distrubution_key.DeleteKey(L"PreferenceMACs");
  LOG_IF(ERROR, result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND)
      << "Failed to cleanup PreferenceMACs: " << result;
#endif
}

int LaunchChromeTests(size_t parallel_jobs,
                      content::TestLauncherDelegate* delegate,
                      int argc,
                      char** argv) {
#if defined(OS_MACOSX)
  chrome_browser_application_mac::RegisterBrowserCrApp();
#endif

#if defined(OS_WIN)
  // Create a primordial InstallDetails instance for the test.
  install_static::ScopedInstallDetails install_details;
#endif

#if defined(OS_LINUX) || defined(OS_ANDROID) || defined(OS_WIN)
  // We leak this pointer intentionally. The crash client needs to outlive
  // all other code.
  ChromeCrashReporterClient* crash_client = new ChromeCrashReporterClient();
  ANNOTATE_LEAKING_OBJECT_PTR(crash_client);
  crash_reporter::SetCrashReporterClient(crash_client);
#endif

  // Setup a working test environment for the network service in case it's used.
  // Only create this object in the utility process, so that its members don't
  // interfere with other test objects in the browser process.
  std::unique_ptr<content::NetworkServiceTestHelper>
      network_service_test_helper;
  if (base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kProcessType) == switches::kUtilityProcess) {
    network_service_test_helper =
        std::make_unique<content::NetworkServiceTestHelper>();
    ChromeContentUtilityClient::SetNetworkBinderCreationCallback(base::Bind(
        [](content::NetworkServiceTestHelper* helper,
           service_manager::BinderRegistry* registry) {
          helper->RegisterNetworkBinders(registry);
        },
        network_service_test_helper.get()));
  }

#if defined(OS_CHROMEOS)
  // Inject the test interfaces for ash. Use a callback to avoid linking test
  // interface support into production code.
  ash::mojo_interface_factory::SetRegisterInterfacesCallback(
      base::Bind(&ash::mojo_test_interface_factory::RegisterInterfaces));
#endif

  return content::LaunchTests(delegate, parallel_jobs, argc, argv);
}
