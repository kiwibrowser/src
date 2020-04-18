// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_browser_main_mac.h"

#import <Cocoa/Cocoa.h>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/bundle_locations.h"
#import "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#import "chrome/browser/app_controller_mac.h"
#include "chrome/browser/apps/app_shim/app_shim_host_manager_mac.h"
#include "chrome/browser/browser_process.h"
#import "chrome/browser/chrome_browser_application_mac.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/mac/install_from_dmg.h"
#include "chrome/browser/mac/keychain_reauthorize.h"
#import "chrome/browser/mac/keystone_glue.h"
#include "chrome/browser/mac/mac_startup_profiler.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "components/crash/content/app/crashpad.h"
#include "components/metrics/metrics_service.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/result_codes.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_handle.h"

namespace {

// Writes an undocumented sentinel file that prevents Spotlight from indexing
// below a particular path in order to reap some power savings.
void EnsureMetadataNeverIndexFileOnFileThread(
    const base::FilePath& user_data_dir) {
  const char kMetadataNeverIndexFilename[] = ".metadata_never_index";
  base::FilePath metadata_file_path =
      user_data_dir.Append(kMetadataNeverIndexFilename);
  if (base::PathExists(metadata_file_path))
    return;

  if (base::WriteFile(metadata_file_path, nullptr, 0) == -1)
    DLOG(FATAL) << "Could not write .metadata_never_index file.";
}

void EnsureMetadataNeverIndexFile(const base::FilePath& user_data_dir) {
  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::Bind(&EnsureMetadataNeverIndexFileOnFileThread, user_data_dir));
}

}  // namespace

// ChromeBrowserMainPartsMac ---------------------------------------------------

ChromeBrowserMainPartsMac::ChromeBrowserMainPartsMac(
    const content::MainFunctionParams& parameters,
    std::unique_ptr<ui::DataPack> data_pack)
    : ChromeBrowserMainPartsPosix(parameters, std::move(data_pack)) {}

ChromeBrowserMainPartsMac::~ChromeBrowserMainPartsMac() {
}

int ChromeBrowserMainPartsMac::PreEarlyInitialization() {
  if (base::mac::WasLaunchedAsLoginItemRestoreState()) {
    base::CommandLine* singleton_command_line =
        base::CommandLine::ForCurrentProcess();
    singleton_command_line->AppendSwitch(switches::kRestoreLastSession);
  } else if (base::mac::WasLaunchedAsHiddenLoginItem()) {
    base::CommandLine* singleton_command_line =
        base::CommandLine::ForCurrentProcess();
    singleton_command_line->AppendSwitch(switches::kNoStartupWindow);
  }

  // Tell Cocoa to finish its initialization, which we want to do manually
  // instead of calling NSApplicationMain(). The primary reason is that NSAM()
  // never returns, which would leave all the objects currently on the stack
  // in scoped_ptrs hanging and never cleaned up. We then load the main nib
  // directly. The main event loop is run from common code using the
  // MessageLoop API, which works out ok for us because it's a wrapper around
  // CFRunLoop.

  // Initialize NSApplication using the custom subclass.
  chrome_browser_application_mac::RegisterBrowserCrApp();

  // If ui_task is not NULL, the app is actually a browser_test.
  if (!parameters().ui_task) {
    // The browser process only wants to support the language Cocoa will use,
    // so force the app locale to be overriden with that value. This must
    // happen before the ResourceBundle is loaded, which happens in
    // ChromeBrowserMainParts::PreEarlyInitialization().
    l10n_util::OverrideLocaleWithCocoaLocale();
  }

  return ChromeBrowserMainPartsPosix::PreEarlyInitialization();
}

void ChromeBrowserMainPartsMac::PreMainMessageLoopStart() {
  MacStartupProfiler::GetInstance()->Profile(
      MacStartupProfiler::PRE_MAIN_MESSAGE_LOOP_START);
  ChromeBrowserMainPartsPosix::PreMainMessageLoopStart();

  // ChromeBrowserMainParts should have loaded the resource bundle by this
  // point (needed to load the nib).
  CHECK(ui::ResourceBundle::HasSharedInstance());

  // This is a no-op if the KeystoneRegistration framework is not present.
  // The framework is only distributed with branded Google Chrome builds.
  [[KeystoneGlue defaultKeystoneGlue] registerWithKeystone];

  // Disk image installation is sort of a first-run task, so it shares the
  // no first run switches.
  //
  // This needs to be done after the resource bundle is initialized (for
  // access to localizations in the UI) and after Keystone is initialized
  // (because the installation may need to promote Keystone) but before the
  // app controller is set up (and thus before MainMenu.nib is loaded, because
  // the app controller assumes that a browser has been set up and will crash
  // upon receipt of certain notifications if no browser exists), before
  // anyone tries doing anything silly like firing off an import job, and
  // before anything creating preferences like Local State in order for the
  // relaunched installed application to still consider itself as first-run.
  if (!first_run::IsFirstRunSuppressed(parsed_command_line())) {
    if (MaybeInstallFromDiskImage()) {
      // The application was installed and the installed copy has been
      // launched.  This process is now obsolete.  Exit.
      exit(0);
    }
  }

  // Now load the nib (from the right bundle).
  base::scoped_nsobject<NSNib> nib(
      [[NSNib alloc] initWithNibNamed:@"MainMenu"
                               bundle:base::mac::FrameworkBundle()]);
  // TODO(viettrungluu): crbug.com/20504 - This currently leaks, so if you
  // change this, you'll probably need to change the Valgrind suppression.
  NSArray* top_level_objects = nil;
  [nib instantiateWithOwner:NSApp topLevelObjects:&top_level_objects];
  for (NSObject* object : top_level_objects)
    [object retain];
  // Make sure the app controller has been created.
  DCHECK([NSApp delegate]);

  // Do Keychain reauthorization. This gets two chances to run. If the first
  // try doesn't complete successfully (crashes or is interrupted for any
  // reason), there will be a second chance. Once this step completes
  // successfully, it should never have to run again.
  NSString* const keychain_reauthorize_pref =
      @"KeychainReauthorizeInAppSpring2017";
  const int kKeychainReauthorizeMaxTries = 2;

  chrome::KeychainReauthorizeIfNeeded(keychain_reauthorize_pref,
                                      kKeychainReauthorizeMaxTries);
}

void ChromeBrowserMainPartsMac::PostMainMessageLoopStart() {
  MacStartupProfiler::GetInstance()->Profile(
      MacStartupProfiler::POST_MAIN_MESSAGE_LOOP_START);
  ChromeBrowserMainPartsPosix::PostMainMessageLoopStart();
}

void ChromeBrowserMainPartsMac::PreProfileInit() {
  MacStartupProfiler::GetInstance()->Profile(
      MacStartupProfiler::PRE_PROFILE_INIT);
  ChromeBrowserMainPartsPosix::PreProfileInit();

  // This is called here so that the app shim socket is only created after
  // taking the singleton lock.
  g_browser_process->platform_part()->app_shim_host_manager()->Init();
}

void ChromeBrowserMainPartsMac::PostProfileInit() {
  MacStartupProfiler::GetInstance()->Profile(
      MacStartupProfiler::POST_PROFILE_INIT);
  ChromeBrowserMainPartsPosix::PostProfileInit();

  g_browser_process->metrics_service()->RecordBreakpadRegistration(
      crash_reporter::GetUploadsEnabled());

  if (first_run::IsChromeFirstRun())
    EnsureMetadataNeverIndexFile(user_data_dir());

  // Activation of Keystone is not automatic but done in response to the
  // counting and reporting of profiles.
  KeystoneGlue* glue = [KeystoneGlue defaultKeystoneGlue];
  if (glue && ![glue isRegisteredAndActive]) {
    // If profile loading has failed, we still need to handle other tasks
    // like marking of the product as active.
    [glue updateProfileCountsWithNumProfiles:0
                         numSignedInProfiles:0];
  }
}

void ChromeBrowserMainPartsMac::DidEndMainMessageLoop() {
  AppController* appController =
      base::mac::ObjCCastStrict<AppController>([NSApp delegate]);
  [appController didEndMainMessageLoop];
}
