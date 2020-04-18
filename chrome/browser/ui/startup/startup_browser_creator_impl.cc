// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"

#include <stddef.h>

#include <algorithm>
#include <iterator>

#include "base/auto_reset.h"
#include "base/base_paths.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/i18n/case_conversion.h"
#include "base/location.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/optional.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/browser/apps/install_chrome_app.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/launch_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/obsolete_system/obsolete_system.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/session_crashed_bubble.h"
#include "chrome/browser/ui/startup/automation_infobar_delegate.h"
#include "chrome/browser/ui/startup/bad_flags_prompt.h"
#include "chrome/browser/ui/startup/default_browser_prompt.h"
#include "chrome/browser/ui/startup/google_api_keys_infobar_delegate.h"
#include "chrome/browser/ui/startup/obsolete_system_infobar_delegate.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/startup/startup_tab_provider.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_metrics.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/rappor/public/rappor_utils.h"
#include "components/rappor/rappor_service_impl.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "google_apis/google_api_keys.h"
#include "rlz/buildflags/buildflags.h"
#include "ui/base/ui_features.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#import "chrome/browser/mac/dock.h"
#include "chrome/browser/mac/install_from_dmg.h"
#include "chrome/browser/ui/cocoa/keystone_infobar_delegate.h"
#endif

#if defined(OS_MACOSX) && !BUILDFLAG(MAC_VIEWS_BROWSER)
#include "chrome/browser/ui/startup/session_crashed_infobar_delegate.h"
#endif

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#include "chrome/browser/apps/app_launch_for_metro_restart_win.h"
#if defined(GOOGLE_CHROME_BUILD)
#include "chrome/browser/conflicts/incompatible_applications_updater_win.h"
#endif  // defined(GOOGLE_CHROME_BUILD)
#include "chrome/browser/notifications/notification_platform_bridge_win.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/shell_integration_win.h"
#endif  // defined(OS_WIN)

#if BUILDFLAG(ENABLE_RLZ)
#include "components/google/core/browser/google_util.h"
#include "components/rlz/rlz_tracker.h"  // nogncheck
#endif

using content::ChildProcessSecurityPolicy;
using content::WebContents;
using extensions::Extension;

namespace {

// Utility functions ----------------------------------------------------------

// This enum is used to define the buckets for an enumerated UMA histogram.
// Hence,
//   (a) existing enumerated constants should never be deleted or reordered, and
//   (b) new constants should only be appended at the end of the enumeration.
enum LaunchMode {
  LM_TO_BE_DECIDED = 0,         // Possibly direct launch or via a shortcut.
  LM_AS_WEBAPP = 1,             // Launched as a installed web application.
  LM_WITH_URLS = 2,             // Launched with urls in the cmd line.
  LM_OTHER = 3,                 // Not launched from a shortcut.
  LM_SHORTCUT_NONAME = 4,       // Launched from shortcut but no name available.
  LM_SHORTCUT_UNKNOWN = 5,      // Launched from user-defined shortcut.
  LM_SHORTCUT_QUICKLAUNCH = 6,  // Launched from the quick launch bar.
  LM_SHORTCUT_DESKTOP = 7,      // Launched from a desktop shortcut.
  LM_SHORTCUT_TASKBAR = 8,      // Launched from the taskbar.
  LM_USER_EXPERIMENT = 9,  // Launched after acceptance of a user experiment.
  LM_OTHER_OS = 10,        // Result bucket for OSes with no coverage here.
  LM_MAC_UNDOCKED_DISK_LAUNCH = 11,   // Undocked launch from disk.
  LM_MAC_DOCKED_DISK_LAUNCH = 12,     // Docked launch from disk.
  LM_MAC_UNDOCKED_DMG_LAUNCH = 13,    // Undocked launch from a dmg.
  LM_MAC_DOCKED_DMG_LAUNCH = 14,      // Docked launch from a dmg.
  LM_MAC_DOCK_STATUS_ERROR = 15,      // Error determining dock status.
  LM_MAC_DMG_STATUS_ERROR = 16,       // Error determining dmg status.
  LM_MAC_DOCK_DMG_STATUS_ERROR = 17,  // Error determining dock and dmg status.
  LM_WIN_PLATFORM_NOTIFICATION = 18,  // Launched from toast notification
                                      // activation on Windows.
  LM_SHORTCUT_START_MENU = 19,        // A Windows Start Menu shortcut.
};

// Returns a LaunchMode value if one can be determined with low overhead, or
// LM_TO_BE_DECIDED if a call to GetLaunchModeSlow is required.
LaunchMode GetLaunchModeFast();

// Returns a LaunchMode value; may require a bit of extra work. This will be
// called on a background thread outside of the critical startup path.
LaunchMode GetLaunchModeSlow();

#if defined(OS_WIN)
// Returns the path to the shortcut from which Chrome was launched, or null if
// not launched via a shortcut.
base::Optional<const wchar_t*> GetShortcutPath() {
  STARTUPINFOW si = { sizeof(si) };
  GetStartupInfoW(&si);
  if (!(si.dwFlags & STARTF_TITLEISLINKNAME))
    return base::nullopt;
  return base::Optional<const wchar_t*>(si.lpTitle);
}

LaunchMode GetLaunchModeFast() {
  auto shortcut_path = GetShortcutPath();
  if (!shortcut_path)
    return LM_OTHER;
  if (!shortcut_path.value())
    return LM_SHORTCUT_NONAME;
  return LM_TO_BE_DECIDED;
}

LaunchMode GetLaunchModeSlow() {
  auto shortcut_path = GetShortcutPath();
  DCHECK(shortcut_path);
  DCHECK(shortcut_path.value());

  const base::string16 shortcut(base::i18n::ToLower(shortcut_path.value()));

  // The windows quick launch path is not localized.
  if (shortcut.find(L"\\quick launch\\") != base::StringPiece16::npos)
    return LM_SHORTCUT_TASKBAR;

  // Check the common shortcut locations.
  static constexpr struct {
    int path_key;
    LaunchMode launch_mode;
  } kPathKeysAndModes[] = {
      {base::DIR_COMMON_START_MENU, LM_SHORTCUT_START_MENU},
      {base::DIR_START_MENU, LM_SHORTCUT_START_MENU},
      {base::DIR_COMMON_DESKTOP, LM_SHORTCUT_DESKTOP},
      {base::DIR_USER_DESKTOP, LM_SHORTCUT_DESKTOP},
  };
  base::FilePath candidate;
  for (const auto& item : kPathKeysAndModes) {
    if (base::PathService::Get(item.path_key, &candidate) &&
        base::StartsWith(shortcut, base::i18n::ToLower(candidate.value()),
                         base::CompareCase::SENSITIVE)) {
      return item.launch_mode;
    }
  }

  return LM_SHORTCUT_UNKNOWN;
}
#elif defined(OS_MACOSX)  // defined(OS_WIN)
LaunchMode GetLaunchModeFast() {
  DiskImageStatus dmg_launch_status =
      IsAppRunningFromReadOnlyDiskImage(nullptr);
  dock::ChromeInDockStatus dock_launch_status = dock::ChromeIsInTheDock();

  if (dock_launch_status == dock::ChromeInDockFailure &&
      dmg_launch_status == DiskImageStatusFailure)
    return LM_MAC_DOCK_DMG_STATUS_ERROR;

  if (dock_launch_status == dock::ChromeInDockFailure)
    return LM_MAC_DOCK_STATUS_ERROR;

  if (dmg_launch_status == DiskImageStatusFailure)
    return LM_MAC_DMG_STATUS_ERROR;

  bool dmg_launch = dmg_launch_status == DiskImageStatusTrue;
  bool dock_launch = dock_launch_status == dock::ChromeInDockTrue;

  if (dmg_launch && dock_launch)
    return LM_MAC_DOCKED_DMG_LAUNCH;

  if (dmg_launch)
    return LM_MAC_UNDOCKED_DMG_LAUNCH;

  if (dock_launch)
    return LM_MAC_DOCKED_DISK_LAUNCH;

  return LM_MAC_UNDOCKED_DISK_LAUNCH;
}

LaunchMode GetLaunchModeSlow() {
  NOTREACHED();
  return LM_TO_BE_DECIDED;
}
#else                     // defined(OS_WIN)
// TODO(cpu): Port to other platforms.
LaunchMode GetLaunchModeFast() {
  return LM_OTHER_OS;
}

LaunchMode GetLaunchModeSlow() {
  NOTREACHED();
  return LM_OTHER_OS;
}
#endif                    // defined(OS_WIN)

// Log in a histogram the frequency of launching by the different methods. See
// LaunchMode enum for the actual values of the buckets.
void RecordLaunchModeHistogram(LaunchMode mode) {
  static constexpr char kHistogramName[] = "Launch.Modes";
  if (mode == LM_TO_BE_DECIDED &&
      (mode = GetLaunchModeFast()) == LM_TO_BE_DECIDED) {
    // The mode couldn't be determined with a fast path. Perform a more
    // expensive evaluation out of the critical startup path.
    base::PostTaskWithTraits(FROM_HERE,
                             {base::TaskPriority::BACKGROUND,
                              base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
                             base::BindOnce([]() {
                               base::UmaHistogramSparse(kHistogramName,
                                                        GetLaunchModeSlow());
                             }));
  } else {
    base::UmaHistogramSparse(kHistogramName, mode);
  }
}

void UrlsToTabs(const std::vector<GURL>& urls, StartupTabs* tabs) {
  for (size_t i = 0; i < urls.size(); ++i) {
    StartupTab tab;
    tab.is_pinned = false;
    tab.url = urls[i];
    tabs->push_back(tab);
  }
}

std::vector<GURL> TabsToUrls(const StartupTabs& tabs) {
  std::vector<GURL> urls;
  urls.reserve(tabs.size());
  std::transform(tabs.begin(), tabs.end(), std::back_inserter(urls),
                 [](const StartupTab& tab) { return tab.url; });
  return urls;
}

// Return true if the command line option --app-id is used.  Set
// |out_extension| to the app to open, and |out_launch_container|
// to the type of window into which the app should be open.
bool GetAppLaunchContainer(
    Profile* profile,
    const std::string& app_id,
    const Extension** out_extension,
    extensions::LaunchContainer* out_launch_container) {

  const Extension* extension = extensions::ExtensionRegistry::Get(
      profile)->enabled_extensions().GetByID(app_id);
  // The extension with id |app_id| may have been uninstalled.
  if (!extension)
    return false;

  // Don't launch platform apps in incognito mode.
  if (profile->IsOffTheRecord() && extension->is_platform_app())
    return false;

  // Look at preferences to find the right launch container. If no
  // preference is set, launch as a window.
  extensions::LaunchContainer launch_container = extensions::GetLaunchContainer(
      extensions::ExtensionPrefs::Get(profile), extension);

  if (!extensions::util::IsNewBookmarkAppsEnabled() &&
      !extensions::HasPreferredLaunchContainer(
          extensions::ExtensionPrefs::Get(profile), extension)) {
    launch_container = extensions::LAUNCH_CONTAINER_WINDOW;
  }

  *out_extension = extension;
  *out_launch_container = launch_container;
  return true;
}

void RecordCmdLineAppHistogram(extensions::Manifest::Type app_type) {
  extensions::RecordAppLaunchType(extension_misc::APP_LAUNCH_CMD_LINE_APP,
                                  app_type);
}

// TODO(koz): Consolidate this function and remove the special casing.
const Extension* GetPlatformApp(Profile* profile,
                                const std::string& extension_id) {
  const Extension* extension =
      extensions::ExtensionRegistry::Get(profile)->GetExtensionById(
          extension_id, extensions::ExtensionRegistry::EVERYTHING);
  return extension && extension->is_platform_app() ? extension : NULL;
}

// Appends the contents of |from| to the end of |to|.
void AppendTabs(const StartupTabs& from, StartupTabs* to) {
  if (!from.empty())
    to->insert(to->end(), from.begin(), from.end());
}

}  // namespace

StartupBrowserCreatorImpl::StartupBrowserCreatorImpl(
    const base::FilePath& cur_dir,
    const base::CommandLine& command_line,
    chrome::startup::IsFirstRun is_first_run)
    : cur_dir_(cur_dir),
      command_line_(command_line),
      profile_(NULL),
      browser_creator_(NULL),
      is_first_run_(is_first_run == chrome::startup::IS_FIRST_RUN) {}

StartupBrowserCreatorImpl::StartupBrowserCreatorImpl(
    const base::FilePath& cur_dir,
    const base::CommandLine& command_line,
    StartupBrowserCreator* browser_creator,
    chrome::startup::IsFirstRun is_first_run)
    : cur_dir_(cur_dir),
      command_line_(command_line),
      profile_(NULL),
      browser_creator_(browser_creator),
      is_first_run_(is_first_run == chrome::startup::IS_FIRST_RUN) {}

StartupBrowserCreatorImpl::~StartupBrowserCreatorImpl() {
}

bool StartupBrowserCreatorImpl::Launch(Profile* profile,
                                       const std::vector<GURL>& urls_to_open,
                                       bool process_startup) {
  UMA_HISTOGRAM_COUNTS_100(
      "Startup.BrowserLaunchURLCount",
      static_cast<base::HistogramBase::Sample>(urls_to_open.size()));
  RecordRapporOnStartupURLs(urls_to_open);

  DCHECK(profile);
  profile_ = profile;

#if defined(OS_WIN)
  // If the command line has the kNotificationLaunchId switch, then this
  // Launch() call is from notification_helper.exe to process toast activation.
  // Delegate to the notification system; do not open a browser window here.
  if (command_line_.HasSwitch(switches::kNotificationLaunchId)) {
    if (NotificationPlatformBridgeWin::HandleActivation(command_line_)) {
      RecordLaunchModeHistogram(LM_WIN_PLATFORM_NOTIFICATION);
      return true;
    }
    return false;
  }
#endif  // defined(OS_WIN)

  if (command_line_.HasSwitch(switches::kAppId)) {
    std::string app_id = command_line_.GetSwitchValueASCII(switches::kAppId);
    const Extension* extension = GetPlatformApp(profile, app_id);
    // If |app_id| is a disabled or terminated platform app we handle it
    // specially here, otherwise it will be handled below.
    if (extension) {
      RecordCmdLineAppHistogram(extensions::Manifest::TYPE_PLATFORM_APP);
      AppLaunchParams params(
          profile, extension, extensions::LAUNCH_CONTAINER_NONE,
          WindowOpenDisposition::NEW_WINDOW, extensions::SOURCE_COMMAND_LINE);
      params.command_line = command_line_;
      params.current_directory = cur_dir_;
      ::OpenApplicationWithReenablePrompt(params);
      return true;
    }
  }

  // Open the required browser windows and tabs. First, see if
  // we're being run as an application window. If so, the user
  // opened an app shortcut.  Don't restore tabs or open initial
  // URLs in that case. The user should see the window as an app,
  // not as chrome.
  // Special case is when app switches are passed but we do want to restore
  // session. In that case open app window + focus it after session is restored.
  if (OpenApplicationWindow(profile)) {
    RecordLaunchModeHistogram(LM_AS_WEBAPP);
  } else {
    // Check the true process command line for --try-chrome-again=N rather than
    // the one parsed for startup URLs and such.
    if (!base::CommandLine::ForCurrentProcess()
             ->GetSwitchValueNative(switches::kTryChromeAgain)
             .empty()) {
      RecordLaunchModeHistogram(LM_USER_EXPERIMENT);
    } else {
      RecordLaunchModeHistogram(urls_to_open.empty() ? LM_TO_BE_DECIDED
                                                     : LM_WITH_URLS);
    }

    DetermineURLsAndLaunch(process_startup, urls_to_open);

    if (command_line_.HasSwitch(switches::kInstallChromeApp)) {
      install_chrome_app::InstallChromeApp(
          command_line_.GetSwitchValueASCII(switches::kInstallChromeApp));
    }

    // If this is an app launch, but we didn't open an app window, it may
    // be an app tab.
    OpenApplicationTab(profile);

#if defined(OS_MACOSX)
    if (process_startup) {
      // Check whether the auto-update system needs to be promoted from user
      // to system.
      KeystoneInfoBar::PromotionInfoBar(profile);
    }
#endif
  }

  // In kiosk mode, we want to always be fullscreen, so switch to that now.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kKioskMode) ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kStartFullscreen)) {
    // It's possible for there to be no browser window, e.g. if someone
    // specified a non-sensical combination of options
    // ("--kiosk --no_startup_window"); do nothing in that case.
    Browser* browser = BrowserList::GetInstance()->GetLastActive();
    if (browser)
      chrome::ToggleFullscreenMode(browser);
  }

#if defined(OS_WIN)
  // TODO(gab): This could now run only during Active Setup (i.e. on explicit
  // Active Setup versioning and on OS upgrades) instead of every startup.
  // http://crbug.com/577697
  if (process_startup)
    shell_integration::win::MigrateTaskbarPins();
#endif  // defined(OS_WIN)

  return true;
}

Browser* StartupBrowserCreatorImpl::OpenURLsInBrowser(
    Browser* browser,
    bool process_startup,
    const std::vector<GURL>& urls) {
  StartupTabs tabs;
  UrlsToTabs(urls, &tabs);
  return OpenTabsInBrowser(browser, process_startup, tabs);
}

Browser* StartupBrowserCreatorImpl::OpenTabsInBrowser(Browser* browser,
                                                      bool process_startup,
                                                      const StartupTabs& tabs) {
  DCHECK(!tabs.empty());

  // If we don't yet have a profile, try to use the one we're given from
  // |browser|. While we may not end up actually using |browser| (since it
  // could be a popup window), we can at least use the profile.
  if (!profile_ && browser)
    profile_ = browser->profile();

  if (!browser || !browser->is_type_tabbed()) {
    // Startup browsers are not counted as being created by a user_gesture
    // because of historical accident, even though the startup browser was
    // created in response to the user clicking on chrome. There was an
    // incomplete check on whether a user gesture created a window which looked
    // at the state of the MessageLoop.
    Browser::CreateParams params = Browser::CreateParams(profile_, false);
    browser = new Browser(params);
  }

  bool first_tab = true;
  ProtocolHandlerRegistry* registry = profile_ ?
      ProtocolHandlerRegistryFactory::GetForBrowserContext(profile_) : NULL;
  for (size_t i = 0; i < tabs.size(); ++i) {
    // We skip URLs that we'd have to launch an external protocol handler for.
    // This avoids us getting into an infinite loop asking ourselves to open
    // a URL, should the handler be (incorrectly) configured to be us. Anyone
    // asking us to open such a URL should really ask the handler directly.
    bool handled_by_chrome = ProfileIOData::IsHandledURL(tabs[i].url) ||
        (registry && registry->IsHandledProtocol(tabs[i].url.scheme()));
    if (!process_startup && !handled_by_chrome)
      continue;

    int add_types = first_tab ? TabStripModel::ADD_ACTIVE :
                                TabStripModel::ADD_NONE;
    add_types |= TabStripModel::ADD_FORCE_INDEX;
    if (tabs[i].is_pinned)
      add_types |= TabStripModel::ADD_PINNED;

    NavigateParams params(browser, tabs[i].url,
                          ui::PAGE_TRANSITION_AUTO_TOPLEVEL);
    params.disposition = first_tab ? WindowOpenDisposition::NEW_FOREGROUND_TAB
                                   : WindowOpenDisposition::NEW_BACKGROUND_TAB;
    params.tabstrip_add_types = add_types;

#if BUILDFLAG(ENABLE_RLZ)
    if (process_startup && google_util::IsGoogleHomePageUrl(tabs[i].url)) {
      params.extra_headers = rlz::RLZTracker::GetAccessPointHttpHeader(
          rlz::RLZTracker::ChromeHomePage());
    }
#endif  // BUILDFLAG(ENABLE_RLZ)

    Navigate(&params);

    first_tab = false;
  }
  if (!browser->tab_strip_model()->GetActiveWebContents()) {
    // TODO(sky): this is a work around for 110909. Figure out why it's needed.
    if (!browser->tab_strip_model()->count())
      chrome::AddTabAt(browser, GURL(), -1, true);
    else
      browser->tab_strip_model()->ActivateTabAt(0, false);
  }

  // The default behavior is to show the window, as expressed by the default
  // value of StartupBrowserCreated::show_main_browser_window_. If this was set
  // to true ahead of this place, it means another task must have been spawned
  // to take care of that.
  if (!browser_creator_ || browser_creator_->show_main_browser_window())
    browser->window()->Show();

  return browser;
}

bool StartupBrowserCreatorImpl::IsAppLaunch(std::string* app_url,
                                            std::string* app_id) {
  if (command_line_.HasSwitch(switches::kApp)) {
    if (app_url)
      *app_url = command_line_.GetSwitchValueASCII(switches::kApp);
    return true;
  }
  if (command_line_.HasSwitch(switches::kAppId)) {
    if (app_id)
      *app_id = command_line_.GetSwitchValueASCII(switches::kAppId);
    return true;
  }
  return false;
}

bool StartupBrowserCreatorImpl::OpenApplicationWindow(Profile* profile) {
  std::string url_string, app_id;
  if (!IsAppLaunch(&url_string, &app_id))
    return false;

  // This can fail if the app_id is invalid.  It can also fail if the
  // extension is external, and has not yet been installed.
  // TODO(skerner): Do something reasonable here. Pop up a warning panel?
  // Open an URL to the gallery page of the extension id?
  if (!app_id.empty()) {
    extensions::LaunchContainer launch_container;
    const Extension* extension;
    if (!GetAppLaunchContainer(profile, app_id, &extension, &launch_container))
      return false;

    // TODO(skerner): Could pass in |extension| and |launch_container|,
    // and avoid calling GetAppLaunchContainer() both here and in
    // OpenApplicationTab().

    if (launch_container == extensions::LAUNCH_CONTAINER_TAB)
      return false;

    RecordCmdLineAppHistogram(extension->GetType());

    AppLaunchParams params(profile, extension, launch_container,
                           WindowOpenDisposition::NEW_WINDOW,
                           extensions::SOURCE_COMMAND_LINE);
    params.command_line = command_line_;
    params.current_directory = cur_dir_;
    WebContents* tab_in_app_window = ::OpenApplication(params);

    // Platform apps fire off a launch event which may or may not open a window.
    return (tab_in_app_window != NULL || CanLaunchViaEvent(extension));
  }

  if (url_string.empty())
    return false;

#if defined(OS_WIN)  // Fix up Windows shortcuts.
  base::ReplaceSubstringsAfterOffset(&url_string, 0, "\\x", "%");
#endif
  GURL url(url_string);

  // Restrict allowed URLs for --app switch.
  if (!url.is_empty() && url.is_valid()) {
    ChildProcessSecurityPolicy* policy =
        ChildProcessSecurityPolicy::GetInstance();
    if (policy->IsWebSafeScheme(url.scheme()) ||
        url.SchemeIs(url::kFileScheme)) {
      const extensions::Extension* extension =
          extensions::ExtensionRegistry::Get(profile)
              ->enabled_extensions().GetAppByURL(url);
      if (extension) {
        RecordCmdLineAppHistogram(extension->GetType());
      } else {
        extensions::RecordAppLaunchType(
            extension_misc::APP_LAUNCH_CMD_LINE_APP_LEGACY,
            extensions::Manifest::TYPE_HOSTED_APP);
      }

      WebContents* app_tab = ::OpenAppShortcutWindow(profile, url);
      return (app_tab != NULL);
    }
  }
  return false;
}

bool StartupBrowserCreatorImpl::OpenApplicationTab(Profile* profile) {
  std::string app_id;
  // App shortcuts to URLs always open in an app window.  Because this
  // function will open an app that should be in a tab, there is no need
  // to look at the app URL.  OpenApplicationWindow() will open app url
  // shortcuts.
  if (!IsAppLaunch(NULL, &app_id) || app_id.empty())
    return false;

  extensions::LaunchContainer launch_container;
  const Extension* extension;
  if (!GetAppLaunchContainer(profile, app_id, &extension, &launch_container))
    return false;

  // If the user doesn't want to open a tab, fail.
  if (launch_container != extensions::LAUNCH_CONTAINER_TAB)
    return false;

  RecordCmdLineAppHistogram(extension->GetType());

  WebContents* app_tab = ::OpenApplication(
      AppLaunchParams(profile, extension, extensions::LAUNCH_CONTAINER_TAB,
                      WindowOpenDisposition::NEW_FOREGROUND_TAB,
                      extensions::SOURCE_COMMAND_LINE));
  return (app_tab != NULL);
}

void StartupBrowserCreatorImpl::DetermineURLsAndLaunch(
    bool process_startup,
    const std::vector<GURL>& cmd_line_urls) {
  // Don't open any browser windows if starting up in "background mode".
  if (process_startup && command_line_.HasSwitch(switches::kNoStartupWindow))
    return;

  StartupTabs cmd_line_tabs;
  UrlsToTabs(cmd_line_urls, &cmd_line_tabs);

  bool is_incognito_or_guest =
      profile_->GetProfileType() != Profile::ProfileType::REGULAR_PROFILE;
  bool is_post_crash_launch = HasPendingUncleanExit(profile_);
  bool has_incompatible_applications = false;
#if defined(OS_WIN) && defined(GOOGLE_CHROME_BUILD)
  if (is_post_crash_launch) {
    // Check if there are any incompatible applications cached from the last
    // Chrome run.
    has_incompatible_applications =
        IncompatibleApplicationsUpdater::IsWarningEnabled() &&
        IncompatibleApplicationsUpdater::HasCachedApplications();
  }
#endif
  const auto session_startup_pref =
      StartupBrowserCreator::GetSessionStartupPref(command_line_, profile_);
  // Both mandatory and recommended startup policies should skip promo pages.
  bool are_startup_urls_managed =
      session_startup_pref.TypeIsManaged(profile_->GetPrefs()) ||
      session_startup_pref.TypeIsRecommended(profile_->GetPrefs());
  StartupTabs tabs = DetermineStartupTabs(
      StartupTabProviderImpl(), cmd_line_tabs, process_startup,
      is_incognito_or_guest, is_post_crash_launch,
      has_incompatible_applications, are_startup_urls_managed);

  // Return immediately if we start an async restore, since the remainder of
  // that process is self-contained.
  if (MaybeAsyncRestore(tabs, process_startup, is_post_crash_launch))
    return;

  BrowserOpenBehaviorOptions behavior_options = 0;
  if (process_startup)
    behavior_options |= PROCESS_STARTUP;
  if (is_post_crash_launch)
    behavior_options |= IS_POST_CRASH_LAUNCH;
  if (command_line_.HasSwitch(switches::kOpenInNewWindow))
    behavior_options |= HAS_NEW_WINDOW_SWITCH;
  if (!cmd_line_tabs.empty())
    behavior_options |= HAS_CMD_LINE_TABS;

  BrowserOpenBehavior behavior =
      DetermineBrowserOpenBehavior(session_startup_pref, behavior_options);

  SessionRestore::BehaviorBitmask restore_options = 0;
  if (behavior == BrowserOpenBehavior::SYNCHRONOUS_RESTORE) {
#if defined(OS_MACOSX)
    bool was_mac_login_or_resume = base::mac::WasLaunchedAsLoginOrResumeItem();
#else
    bool was_mac_login_or_resume = false;
#endif
    restore_options = DetermineSynchronousRestoreOptions(
        browser_defaults::kAlwaysCreateTabbedBrowserOnSessionRestore,
        base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kCreateBrowserOnStartupForTests),
        was_mac_login_or_resume);
  }

  Browser* browser = RestoreOrCreateBrowser(
      tabs, behavior, restore_options, process_startup, is_post_crash_launch);

  // Finally, add info bars.
  AddInfoBarsIfNecessary(
      browser, process_startup ? chrome::startup::IS_PROCESS_STARTUP
                               : chrome::startup::IS_NOT_PROCESS_STARTUP);
}

StartupTabs StartupBrowserCreatorImpl::DetermineStartupTabs(
    const StartupTabProvider& provider,
    const StartupTabs& cmd_line_tabs,
    bool process_startup,
    bool is_incognito_or_guest,
    bool is_post_crash_launch,
    bool has_incompatible_applications,
    bool are_startup_urls_managed) {
  // Only the New Tab Page or command line URLs may be shown in incognito mode.
  // A similar policy exists for crash recovery launches, to prevent getting the
  // user stuck in a crash loop.
  if (is_incognito_or_guest || is_post_crash_launch) {
    if (!cmd_line_tabs.empty())
      return cmd_line_tabs;

    if (is_post_crash_launch) {
      const StartupTabs tabs =
          provider.GetPostCrashTabs(has_incompatible_applications);
      if (!tabs.empty())
        return tabs;
    }

    return StartupTabs({StartupTab(GURL(chrome::kChromeUINewTabURL), false)});
  }

  // A trigger on a profile may indicate that we should show a tab which
  // offers to reset the user's settings.  When this appears, it is first, and
  // may be shown alongside command-line tabs.
  StartupTabs tabs = provider.GetResetTriggerTabs(profile_);

  // URLs passed on the command line supersede all others.
  AppendTabs(cmd_line_tabs, &tabs);
  if (!cmd_line_tabs.empty())
    return tabs;

  // A Master Preferences file provided with this distribution may specify
  // tabs to be displayed on first run, overriding all non-command-line tabs,
  // including the profile reset tab.
  StartupTabs distribution_tabs =
      provider.GetDistributionFirstRunTabs(browser_creator_);
  if (!distribution_tabs.empty())
    return distribution_tabs;

  StartupTabs onboarding_tabs;
  // Only do promos if the startup pref is not managed.
  if (!are_startup_urls_managed) {
    // This is a launch from a prompt presented to an inactive user who chose to
    // open Chrome and is being brought to a specific URL for this one launch.
    // Launch the browser with the desired welcome back URL in the foreground
    // and the other ordinary URLs (e.g., a restored session) in the background.
    StartupTabs welcome_back_tabs = provider.GetWelcomeBackTabs(
        profile_, browser_creator_, process_startup);
    AppendTabs(welcome_back_tabs, &tabs);

    // Policies for onboarding (e.g., first run) may show promotional and
    // introductory content depending on a number of system status factors,
    // including OS and whether or not this is First Run.
    onboarding_tabs = provider.GetOnboardingTabs(profile_);
    AppendTabs(onboarding_tabs, &tabs);
  }

  // If the user has set the preference indicating URLs to show on opening,
  // read and add those.
  StartupTabs prefs_tabs = provider.GetPreferencesTabs(command_line_, profile_);
  AppendTabs(prefs_tabs, &tabs);

  // Potentially add the New Tab Page. Onboarding content is designed to
  // replace (and eventually funnel the user to) the NTP. Likewise, URLs
  // from preferences are explicitly meant to override showing the NTP.
  if (onboarding_tabs.empty() && prefs_tabs.empty())
    AppendTabs(provider.GetNewTabPageTabs(command_line_, profile_), &tabs);

  // Maybe add any tabs which the user has previously pinned.
  AppendTabs(provider.GetPinnedTabs(command_line_, profile_), &tabs);

  return tabs;
}

bool StartupBrowserCreatorImpl::MaybeAsyncRestore(const StartupTabs& tabs,
                                                  bool process_startup,
                                                  bool is_post_crash_launch) {
  // Restore is performed synchronously on startup, and is never performed when
  // launching after crashing.
  if (process_startup || is_post_crash_launch)
    return false;

  // Note: there's no session service in incognito or guest mode.
  SessionService* service =
      SessionServiceFactory::GetForProfileForSessionRestore(profile_);

  return service && service->RestoreIfNecessary(TabsToUrls(tabs));
}

Browser* StartupBrowserCreatorImpl::RestoreOrCreateBrowser(
    const StartupTabs& tabs,
    BrowserOpenBehavior behavior,
    SessionRestore::BehaviorBitmask restore_options,
    bool process_startup,
    bool is_post_crash_launch) {
  Browser* browser = nullptr;
  if (behavior == BrowserOpenBehavior::SYNCHRONOUS_RESTORE) {
    browser = SessionRestore::RestoreSession(profile_, nullptr, restore_options,
                                             TabsToUrls(tabs));
    if (browser)
      return browser;
  } else if (behavior == BrowserOpenBehavior::USE_EXISTING) {
    browser = chrome::FindTabbedBrowser(profile_, process_startup);
  }

  base::AutoReset<bool> synchronous_launch_resetter(
      &StartupBrowserCreator::in_synchronous_profile_launch_, true);

  // OpenTabsInBrowser requires at least one tab be passed. As a fallback to
  // prevent a crash, use the NTP if |tabs| is empty. This could happen if
  // we expected a session restore to happen but it did not occur/succeed.
  browser = OpenTabsInBrowser(
      browser, process_startup,
      (tabs.empty()
           ? StartupTabs({StartupTab(GURL(chrome::kChromeUINewTabURL), false)})
           : tabs));

  // Now that a restore is no longer possible, it is safe to clear DOM storage,
  // unless this is a crash recovery.
  if (!is_post_crash_launch) {
    content::BrowserContext::GetDefaultStoragePartition(profile_)
        ->GetDOMStorageContext()
        ->StartScavengingUnusedSessionStorage();
  }

  return browser;
}

void StartupBrowserCreatorImpl::AddInfoBarsIfNecessary(
    Browser* browser,
    chrome::startup::IsProcessStartup is_process_startup) {
  if (!browser || !profile_ || browser->tab_strip_model()->count() == 0)
    return;

  if (HasPendingUncleanExit(browser->profile()) &&
      !SessionCrashedBubble::Show(browser)) {
#if defined(OS_MACOSX) && !BUILDFLAG(MAC_VIEWS_BROWSER)
    SessionCrashedInfoBarDelegate::Create(browser);
#endif
  }

  if (command_line_.HasSwitch(switches::kEnableAutomation))
    AutomationInfoBarDelegate::Create();

  // The below info bars are only added to the first profile which is launched.
  // Other profiles might be restoring the browsing sessions asynchronously,
  // so we cannot add the info bars to the focused tabs here.
  //
  // These info bars are not shown when the browser is being controlled by
  // automated tests, so that they don't interfere with tests that assume no
  // info bars.
  if (is_process_startup == chrome::startup::IS_PROCESS_STARTUP &&
      !command_line_.HasSwitch(switches::kTestType) &&
      !command_line_.HasSwitch(switches::kEnableAutomation)) {
    content::WebContents* web_contents =
        browser->tab_strip_model()->GetActiveWebContents();
    DCHECK(web_contents);
    chrome::ShowBadFlagsPrompt(web_contents);
    InfoBarService* infobar_service =
        InfoBarService::FromWebContents(web_contents);
    if (!google_apis::HasKeysConfigured())
      GoogleApiKeysInfoBarDelegate::Create(infobar_service);
    if (ObsoleteSystem::IsObsoleteNowOrSoon()) {
      PrefService* local_state = g_browser_process->local_state();
      if (!local_state ||
          !local_state->GetBoolean(prefs::kSuppressUnsupportedOSWarning))
        ObsoleteSystemInfoBarDelegate::Create(infobar_service);
    }

#if !defined(OS_CHROMEOS)
    if (!command_line_.HasSwitch(switches::kNoDefaultBrowserCheck)) {
      // Generally, the default browser prompt should not be shown on first
      // run. However, when the set-as-default dialog has been suppressed, we
      // need to allow it.
      if (!is_first_run_ ||
          (browser_creator_ &&
           browser_creator_->is_default_browser_dialog_suppressed())) {
        ShowDefaultBrowserPrompt(profile_);
      }
    }
#endif
  }
}

void StartupBrowserCreatorImpl::RecordRapporOnStartupURLs(
    const std::vector<GURL>& urls_to_open) {
  for (const GURL& url : urls_to_open) {
    rappor::SampleDomainAndRegistryFromGURL(g_browser_process->rappor_service(),
                                            "Startup.BrowserLaunchURL", url);
  }
}

// static
StartupBrowserCreatorImpl::BrowserOpenBehavior
StartupBrowserCreatorImpl::DetermineBrowserOpenBehavior(
    const SessionStartupPref& pref,
    BrowserOpenBehaviorOptions options) {
  if (!(options & PROCESS_STARTUP)) {
    // For existing processes, restore would have happened before invoking this
    // function. If Chrome was launched with passed URLs, assume these should
    // be appended to an existing window if possible, unless overridden by a
    // switch.
    return ((options & HAS_CMD_LINE_TABS) && !(options & HAS_NEW_WINDOW_SWITCH))
               ? BrowserOpenBehavior::USE_EXISTING
               : BrowserOpenBehavior::NEW;
  }

  if (pref.type == SessionStartupPref::LAST) {
    // Don't perform a session restore on a post-crash launch, as this could
    // cause a crash loop.
    if (!(options & IS_POST_CRASH_LAUNCH))
      return BrowserOpenBehavior::SYNCHRONOUS_RESTORE;
  }

  return BrowserOpenBehavior::NEW;
}

// static
SessionRestore::BehaviorBitmask
StartupBrowserCreatorImpl::DetermineSynchronousRestoreOptions(
    bool has_create_browser_default,
    bool has_create_browser_switch,
    bool was_mac_login_or_resume) {
  SessionRestore::BehaviorBitmask options = SessionRestore::SYNCHRONOUS;

  // Suppress the creation of a new window on Mac when restoring with no windows
  // if launching Chrome via a login item or the resume feature in OS 10.7+.
  if (!was_mac_login_or_resume &&
      (has_create_browser_default || has_create_browser_switch))
    options |= SessionRestore::ALWAYS_CREATE_TABBED_BROWSER;

  return options;
}
