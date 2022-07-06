// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser.h"

#include <stddef.h>

#include <algorithm>
#include <string>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/optional.h"
#include "base/process/process_info.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/background/background_contents.h"
#include "chrome/browser/background/background_contents_service.h"
#include "chrome/browser/background/background_contents_service_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/content_settings/mixed_content_settings_tab_helper.h"
#include "chrome/browser/content_settings/sound_content_setting_observer.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/custom_handlers/register_protocol_handler_permission_request.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/devtools/devtools_toggle_action.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/download/download_core_service.h"
#include "chrome/browser/download/download_core_service_factory.h"
#include "chrome/browser/extensions/api/tabs/tabs_event_router.h"
#include "chrome/browser/extensions/api/tabs/tabs_windows_api.h"
#include "chrome/browser/extensions/browser_extension_window_controller.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_ui_util.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/file_select_helper.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "chrome/browser/page_load_metrics/metrics_web_contents_observer.h"
#include "chrome/browser/pepper_broker_infobar_delegate.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/plugins/plugin_finder.h"
#include "chrome/browser/plugins/plugin_metadata.h"
#include "chrome/browser/policy/developer_tools_policy_handler.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/printing/background_printing_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_destroyer.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/repost_form_warning_controller.h"
#include "chrome/browser/resource_coordinator/tab_manager_web_contents_data.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/sync_ui_util.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/blocked_content/framebust_block_tab_helper.h"
#include "chrome/browser/ui/blocked_content/popup_blocker_tab_helper.h"
#include "chrome/browser/ui/blocked_content/popup_tracker.h"
#include "chrome/browser/ui/bluetooth/bluetooth_chooser_controller.h"
#include "chrome/browser/ui/bluetooth/bluetooth_chooser_desktop.h"
#include "chrome/browser/ui/bookmarks/bookmark_tab_helper.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_instant_controller.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_live_tab_context.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_tab_strip_model_delegate.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_toolbar_model_delegate.h"
#include "chrome/browser/ui/browser_ui_prefs.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_bubble_manager.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/exclusive_access/mouse_lock_controller.h"
#include "chrome/browser/ui/extensions/hosted_app_browser_controller.h"
#include "chrome/browser/ui/fast_unload_controller.h"
#include "chrome/browser/ui/find_bar/find_bar.h"
#include "chrome/browser/ui/find_bar/find_bar_controller.h"
#include "chrome/browser/ui/find_bar/find_tab_helper.h"
#include "chrome/browser/ui/global_error/global_error.h"
#include "chrome/browser/ui/global_error/global_error_service.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "chrome/browser/ui/javascript_dialogs/javascript_dialog_tab_helper.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/permission_bubble/chooser_bubble_delegate.h"
#include "chrome/browser/ui/search/search_tab_helper.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/status_bubble.h"
#include "chrome/browser/ui/sync/browser_synced_window_delegate.h"
#include "chrome/browser/ui/tab_contents/core_tab_helper.h"
#include "chrome/browser/ui/tab_dialogs.h"
#include "chrome/browser/ui/tab_helpers.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog.h"
#include "chrome/browser/ui/tabs/tab_menu_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/browser/ui/unload_controller.h"
#include "chrome/browser/ui/webui/signin/login_ui_service.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"
#include "chrome/browser/ui/window_sizer/window_sizer.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/browser/vr/vr_tab_helper.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/custom_handlers/protocol_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/profiling.h"
#include "chrome/common/ssl_insecure_content.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/bubble/bubble_controller.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/keep_alive_registry/keep_alive_registry.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/prefs/pref_service.h"
#include "components/search/search.h"
#include "components/security_state/content/content_utils.h"
#include "components/security_state/core/security_state.h"
#include "components/sessions/core/session_types.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/startup_metric_utils/browser/startup_metric_utils.h"
#include "components/toolbar/toolbar_model_impl.h"
#include "components/translate/core/browser/language_state.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "components/zoom/zoom_controller.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/keyboard_event_processing_result.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/overscroll_configuration.h"
#include "content/public/browser/picture_in_picture_window_controller.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/page_zoom.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/webplugininfo.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "net/base/filename_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cookies/cookie_monster.h"
#include "net/url_request/url_request_context.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/geometry/point.h"
#include "ui/shell_dialogs/selected_file_info.h"

#if defined(OS_MACOSX)
#include "content/public/browser/browser_plugin_guest_manager.h"
#include "extensions/browser/process_manager.h"
#endif

#if defined(OS_WIN)
#include <windows.h>
#include <shellapi.h>
#include "chrome/browser/ui/view_ids.h"
#include "components/autofill/core/browser/autofill_ie_toolbar_import_win.h"
#include "ui/base/touch/touch_device.h"
#include "ui/base/win/shell.h"
#endif  // OS_WIN

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/fileapi/external_file_url_util.h"
#include "chrome/browser/ui/settings_window_manager_chromeos.h"
#endif

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/printing/browser/print_composite_client.h"
#endif

using base::TimeDelta;
using base::UserMetricsAction;
using content::NativeWebKeyboardEvent;
using content::NavigationController;
using content::NavigationEntry;
using content::OpenURLParams;
using content::PluginService;
using content::Referrer;
using content::RenderWidgetHostView;
using content::SiteInstance;
using content::WebContents;
using extensions::Extension;
using ui::WebDialogDelegate;
using web_modal::WebContentsModalDialogManager;

///////////////////////////////////////////////////////////////////////////////

namespace {

// How long we wait before updating the browser chrome while loading a page.
const int kUIUpdateCoalescingTimeMS = 200;

BrowserWindow* CreateBrowserWindow(Browser* browser, bool user_gesture) {
  LOG(INFO) << "[EXTENSIONS] Calling BrowserWindow::CreateBrowserWindow from CreateBrowserWindow";
  return BrowserWindow::CreateBrowserWindow(std::move(browser), user_gesture);
}

// Is the fast tab unload experiment enabled?
bool IsFastTabUnloadEnabled() {
  return true || base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableFastUnload);
}

const extensions::Extension* GetExtensionForOrigin(
    Profile* profile,
    const GURL& security_origin) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (!security_origin.SchemeIs(extensions::kExtensionScheme))
    return nullptr;

  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(profile)->enabled_extensions().GetByID(
          security_origin.host());
  DCHECK(extension);
  return extension;
#else
  return nullptr;
#endif
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// Browser, CreateParams:

Browser::CreateParams::CreateParams(Profile* profile, bool user_gesture)
    : CreateParams(TYPE_TABBED, profile, user_gesture) {}

Browser::CreateParams::CreateParams(Type type,
                                    Profile* profile,
                                    bool user_gesture)
    : type(type), profile(profile), user_gesture(user_gesture) {}

Browser::CreateParams::CreateParams(const CreateParams& other) = default;

// static
Browser::CreateParams Browser::CreateParams::CreateForApp(
    const std::string& app_name,
    bool trusted_source,
    const gfx::Rect& window_bounds,
    Profile* profile,
    bool user_gesture) {
  DCHECK(!app_name.empty());

  CreateParams params(TYPE_POPUP, profile, user_gesture);
  params.app_name = app_name;
  params.trusted_source = trusted_source;
  params.initial_bounds = window_bounds;

  return params;
}

// static
Browser::CreateParams Browser::CreateParams::CreateForDevTools(
    Profile* profile) {
  CreateParams params(TYPE_POPUP, profile, true);
  params.trusted_source = true;
  return params;
}

////////////////////////////////////////////////////////////////////////////////
// Browser, InterstitialObserver:

class Browser::InterstitialObserver : public content::WebContentsObserver {
 public:
  InterstitialObserver(Browser* browser, content::WebContents* web_contents)
      : WebContentsObserver(web_contents),
        browser_(browser) {
  }

  void DidAttachInterstitialPage() override {
    browser_->UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_TAB_STATE);
  }

  void DidDetachInterstitialPage() override {
    browser_->UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_TAB_STATE);
  }

 private:
  Browser* browser_;

  DISALLOW_COPY_AND_ASSIGN(InterstitialObserver);
};

///////////////////////////////////////////////////////////////////////////////
// Browser, Constructors, Creation, Showing:

Browser::Browser(const CreateParams& params)
    : extension_registry_observer_(this),
      type_(params.type),
      profile_(params.profile),
      window_(NULL),
      tab_strip_model_delegate_(new chrome::BrowserTabStripModelDelegate(this)),
      tab_strip_model_(
          std::make_unique<TabStripModel>(tab_strip_model_delegate_.get(),
                                          params.profile)),
      app_name_(params.app_name),
      is_trusted_source_(params.trusted_source),
      session_id_(SessionID::NewUnique()),
      cancel_download_confirmation_state_(NOT_PROMPTED),
      override_bounds_(params.initial_bounds),
      initial_show_state_(params.initial_show_state),
      initial_workspace_(params.initial_workspace),
      is_session_restore_(params.is_session_restore),
      content_setting_bubble_model_delegate_(
          new BrowserContentSettingBubbleModelDelegate(this)),
      toolbar_model_delegate_(new BrowserToolbarModelDelegate(this)),
      live_tab_context_(new BrowserLiveTabContext(this)),
      synced_window_delegate_(new BrowserSyncedWindowDelegate(this)),
      bookmark_bar_state_(BookmarkBar::HIDDEN),
      command_controller_(new chrome::BrowserCommandController(this)),
      window_has_shown_(false),
      chrome_updater_factory_(this),
      weak_factory_(this) {
  // If this causes a crash then a window is being opened using a profile type
  // that is disallowed by policy. The crash prevents the disabled window type
  // from opening at all, but the path that triggered it should be fixed.
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 1";
  CHECK(IncognitoModePrefs::CanOpenBrowser(profile_));
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 2";
  CHECK(!profile_->IsGuestSession() || profile_->IsOffTheRecord())
      << "Only off the record browser may be opened in guest mode";
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 3";
  DCHECK(!profile_->IsSystemProfile())
      << "The system profile should never have a real browser.";
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 4";
  // TODO(mlerman): After this hits stable channel, see if there are counts
  // for this metric. If not, change the DCHECK above to a CHECK.
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 5";
  if (profile_->IsSystemProfile())
    base::RecordAction(base::UserMetricsAction("BrowserForSystemProfile"));

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 6";
  // TODO(jeremy): Move to initializer list once flag is removed.
  if (IsFastTabUnloadEnabled())
    fast_unload_controller_.reset(new FastUnloadController(this));
  else
    unload_controller_.reset(new UnloadController(this));
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 7";

  tab_strip_model_->AddObserver(this);

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 8";
  toolbar_model_.reset(new ToolbarModelImpl(toolbar_model_delegate_.get(),
                                            content::kMaxURLDisplayChars));

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 9";
  extension_registry_observer_.Add(
      extensions::ExtensionRegistry::Get(profile_));
#if !defined(OS_ANDROID)
  registrar_.Add(
      this, chrome::NOTIFICATION_BROWSER_THEME_CHANGED,
      content::Source<ThemeService>(
          ThemeServiceFactory::GetForProfile(profile_)));
#endif
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 10";
  registrar_.Add(this, chrome::NOTIFICATION_WEB_CONTENT_SETTINGS_CHANGED,
                 content::NotificationService::AllSources());

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 11";
  profile_pref_registrar_.Init(profile_->GetPrefs());
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 12";
  profile_pref_registrar_.Add(
      prefs::kDevToolsAvailability,
      base::BindRepeating(&Browser::OnDevToolsAvailabilityChanged,
                          base::Unretained(this)));
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 13";
  profile_pref_registrar_.Add(
      bookmarks::prefs::kShowBookmarkBar,
      base::BindRepeating(&Browser::UpdateBookmarkBarState,
                          base::Unretained(this),
                          BOOKMARK_BAR_STATE_CHANGE_PREF_CHANGE));
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 14";

  if (search::IsInstantExtendedAPIEnabled() && is_type_tabbed())
    instant_controller_.reset(new BrowserInstantController(this));

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 15";
  if (extensions::HostedAppBrowserController::IsForHostedApp(this)) {
    hosted_app_controller_.reset(
        new extensions::HostedAppBrowserController(this));
  }
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 16";

  UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_INIT);

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 17";
  ProfileMetrics::LogProfileLaunch(profile_);

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 18";
  if (params.skip_window_init_for_testing)
    return;

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 19: " << params.window;
  window_ = params.window ? params.window
                          : CreateBrowserWindow(this, params.user_gesture);

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 20";
  if (hosted_app_controller_)
    hosted_app_controller_->UpdateLocationBarVisibility(false);

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 21";
  // Create the extension window controller before sending notifications.
  extension_window_controller_.reset(
      new extensions::BrowserExtensionWindowController(this));

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 22";
  SessionService* session_service =
      SessionServiceFactory::GetForProfileForSessionRestore(profile_);
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 23";
  if (session_service)
    session_service->WindowOpened(this);

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 24";
  // TODO(beng): move to ChromeBrowserMain:
  if (first_run::ShouldDoPersonalDataManagerFirstRun()) {
#if defined(OS_WIN)
    // Notify PDM that this is a first run.
    ImportAutofillDataWin(
        autofill::PersonalDataManagerFactory::GetForProfile(profile_));
#endif  // defined(OS_WIN)
  }

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 25";
//  exclusive_access_manager_.reset(
//      new ExclusiveAccessManager(window_->GetExclusiveAccessContext()));

  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 26";
  BrowserList::AddBrowser(this);
  LOG(INFO) << "[EXTENSIONS] Creating new Browser() window - Step 27";
}

Browser::~Browser() {
  // Stop observing notifications before continuing with destruction. Profile
  // destruction will unload extensions and reentrant calls to Browser:: should
  // be avoided while it is being torn down.
  registrar_.RemoveAll();
  extension_registry_observer_.RemoveAll();

  // The tab strip should not have any tabs at this point.
  DCHECK(tab_strip_model_->empty());
  tab_strip_model_->RemoveObserver(this);
  bubble_manager_.reset();

  // Destroy the BrowserCommandController before removing the browser, so that
  // it doesn't act on any notifications that are sent as a result of removing
  // the browser.
  command_controller_.reset();
  BrowserList::RemoveBrowser(this);

  // If closing the window is going to trigger a shutdown, then we need to
  // schedule all active downloads to be cancelled. This needs to be after
  // removing |this| from BrowserList so that OkToClose...() can determine
  // whether there are any other windows open for the browser.
  int num_downloads;
  if (!browser_defaults::kBrowserAliveWithNoWindows &&
      OkToCloseWithInProgressDownloads(&num_downloads) ==
          DOWNLOAD_CLOSE_BROWSER_SHUTDOWN) {
    DownloadCoreService::CancelAllDownloads();
  }

  SessionService* session_service =
      SessionServiceFactory::GetForProfile(profile_);
  if (session_service)
    session_service->WindowClosed(session_id_);

  sessions::TabRestoreService* tab_restore_service =
      TabRestoreServiceFactory::GetForProfile(profile());
  if (tab_restore_service)
    tab_restore_service->BrowserClosed(live_tab_context());

  profile_pref_registrar_.RemoveAll();

  // Destroy BrowserExtensionWindowController before the incognito profile
  // is destroyed to make sure the chrome.windows.onRemoved event is sent.
  extension_window_controller_.reset();

  // Destroy BrowserInstantController before the incongnito profile is destroyed
  // because the InstantController destructor depends on this profile.
  instant_controller_.reset();

  // The system incognito profile should not try be destroyed using
  // ProfileDestroyer::DestroyProfileWhenAppropriate(). This profile can be
  // used, at least, by the user manager window. This window is not a browser,
  // therefore, BrowserList::IsIncognitoSessionActiveForProfile(profile_)
  // returns false, while the user manager window is still opened.
  // This cannot be fixed in ProfileDestroyer::DestroyProfileWhenAppropriate(),
  // because the ProfileManager needs to be able to destroy all profiles when
  // it is destroyed. See crbug.com/527035
  //
  // A profile created with Profile::CreateOffTheRecordProfile() should not be
  // destroyed directly by Browser (e.g. for offscreen tabs,
  // https://crbug.com/664351).
  if (profile_->IsOffTheRecord() &&
      profile_->GetOriginalProfile()->HasOffTheRecordProfile() &&
      profile_->GetOriginalProfile()->GetOffTheRecordProfile() == profile_ &&
      !BrowserList::IsIncognitoSessionActiveForProfile(profile_) &&
      !profile_->GetOriginalProfile()->IsSystemProfile()) {
    if (profile_->IsGuestSession()) {
// ChromeOS handles guest data independently.
#if !defined(OS_CHROMEOS)
      // Clear all browsing data once a Guest Session completes. The Guest
      // profile has BrowserContextKeyedServices that the Incognito profile
      // doesn't, so the ProfileDestroyer can't delete it properly.
      // TODO(mlerman): Delete the guest using an improved ProfileDestroyer.
      profiles::RemoveBrowsingDataForProfile(profile_->GetPath());
#endif
    } else {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
      // The Printing Background Manager holds onto preview dialog WebContents
      // whose corresponding print jobs have not yet fully spooled. Make sure
      // these get destroyed before tearing down the incognito profile so that
      // their render frame hosts can exit in time - see crbug.com/579155
      g_browser_process->background_printing_manager()
          ->DeletePreviewContentsForBrowserContext(profile_);
#endif
      // An incognito profile is no longer needed, this indirectly frees
      // its cache and cookies once it gets destroyed at the appropriate time.
      ProfileDestroyer::DestroyProfileWhenAppropriate(profile_);
    }
  }

  // There may be pending file dialogs, we need to tell them that we've gone
  // away so they don't try and call back to us.
  if (select_file_dialog_.get())
    select_file_dialog_->ListenerDestroyed();
}

///////////////////////////////////////////////////////////////////////////////
// Getters & Setters

ChromeBubbleManager* Browser::GetBubbleManager() {
  if (!bubble_manager_)
    bubble_manager_.reset(new ChromeBubbleManager(tab_strip_model_.get()));
  return bubble_manager_.get();
}

FindBarController* Browser::GetFindBarController() {
  if (!find_bar_controller_.get()) {
    FindBar* find_bar = window_->CreateFindBar();
    find_bar_controller_.reset(new FindBarController(find_bar, this));
    find_bar->SetFindBarController(find_bar_controller_.get());
    find_bar_controller_->ChangeWebContents(
        tab_strip_model_->GetActiveWebContents());
    find_bar_controller_->find_bar()->MoveWindowIfNecessary(gfx::Rect());
  }
  return find_bar_controller_.get();
}

bool Browser::HasFindBarController() const {
  return find_bar_controller_.get() != NULL;
}

bool Browser::is_app() const {
  return !app_name_.empty();
}

bool Browser::is_devtools() const {
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// Browser, State Storage and Retrieval for UI:

gfx::Image Browser::GetCurrentPageIcon() const {
  WebContents* web_contents = tab_strip_model_->GetActiveWebContents();
  // |web_contents| can be NULL since GetCurrentPageIcon() is called by the
  // window during the window's creation (before tabs have been added).
  favicon::FaviconDriver* favicon_driver =
      web_contents
          ? favicon::ContentFaviconDriver::FromWebContents(web_contents)
          : nullptr;
  return favicon_driver ? favicon_driver->GetFavicon() : gfx::Image();
}

base::string16 Browser::GetWindowTitleForCurrentTab(
    bool include_app_name) const {
  return GetWindowTitleFromWebContents(
      include_app_name, tab_strip_model_->GetActiveWebContents());
}

base::string16 Browser::GetWindowTitleForTab(bool include_app_name,
                                             int index) const {
  return GetWindowTitleFromWebContents(
      include_app_name, tab_strip_model_->GetWebContentsAt(index));
}

base::string16 Browser::GetWindowTitleFromWebContents(
    bool include_app_name,
    content::WebContents* contents) const {
  base::string16 title;

  // |contents| can be NULL because GetWindowTitleForCurrentTab is called by the
  // window during the window's creation (before tabs have been added).
  if (contents) {
    title = hosted_app_controller_ ? hosted_app_controller_->GetTitle()
                                   : contents->GetTitle();
    FormatTitleForDisplay(&title);
  }

  // If there is no title, leave it empty for apps.
  if (title.empty() && !is_app())
    title = CoreTabHelper::GetDefaultTitle();

#if defined(OS_MACOSX)
  // On Mac, we don't want to suffix the page title with the application name.
  return title;
#endif

  // If there is no title and this is an app, fall back on the app name. This
  // ensures that the native window gets a title which is important for a11y,
  // for example the window selector uses the Aura window title.
  if (title.empty() && is_app() && include_app_name) {
    return base::UTF8ToUTF16(hosted_app_controller_
                                 ? hosted_app_controller_->GetAppShortName()
                                 : app_name());
  }

  // Include the app name in window titles for tabbed browser windows when
  // requested with |include_app_name|.
  return (!is_app() && include_app_name) ?
      l10n_util::GetStringFUTF16(IDS_BROWSER_WINDOW_TITLE_FORMAT, title):
      title;
}

// static
void Browser::FormatTitleForDisplay(base::string16* title) {
  size_t current_index = 0;
  size_t match_index;
  while ((match_index = title->find(L'\n', current_index)) !=
         base::string16::npos) {
    title->replace(match_index, 1, base::string16());
    current_index = match_index;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Browser, OnBeforeUnload handling:

bool Browser::ShouldCloseWindow() {
  if (!CanCloseWithInProgressDownloads())
    return false;
  if (IsFastTabUnloadEnabled())
    return fast_unload_controller_->ShouldCloseWindow();
  return unload_controller_->ShouldCloseWindow();
}

bool Browser::TryToCloseWindow(
    bool skip_beforeunload,
    const base::Callback<void(bool)>& on_close_confirmed) {
  cancel_download_confirmation_state_ = RESPONSE_RECEIVED;
  if (IsFastTabUnloadEnabled()) {
    return fast_unload_controller_->TryToCloseWindow(skip_beforeunload,
                                                     on_close_confirmed);
  }
  return unload_controller_->TryToCloseWindow(skip_beforeunload,
                                              on_close_confirmed);
}

void Browser::ResetTryToCloseWindow() {
  cancel_download_confirmation_state_ = NOT_PROMPTED;
  if (IsFastTabUnloadEnabled())
    fast_unload_controller_->ResetTryToCloseWindow();
  else
    unload_controller_->ResetTryToCloseWindow();
}

bool Browser::HasCompletedUnloadProcessing() const {
  DCHECK(IsFastTabUnloadEnabled());
  return fast_unload_controller_->HasCompletedUnloadProcessing();
}

bool Browser::IsAttemptingToCloseBrowser() const {
  if (IsFastTabUnloadEnabled())
    return fast_unload_controller_->is_attempting_to_close_browser();
  return unload_controller_->is_attempting_to_close_browser();
}

bool Browser::ShouldRunUnloadListenerBeforeClosing(
    content::WebContents* web_contents) {
  if (IsFastTabUnloadEnabled())
    return fast_unload_controller_->ShouldRunUnloadEventsHelper(web_contents);
  return unload_controller_->ShouldRunUnloadEventsHelper(web_contents);
}

bool Browser::RunUnloadListenerBeforeClosing(
    content::WebContents* web_contents) {
  if (IsFastTabUnloadEnabled())
    return fast_unload_controller_->RunUnloadEventsHelper(web_contents);
  return unload_controller_->RunUnloadEventsHelper(web_contents);
}

void Browser::OnWindowClosing() {
  if (!ShouldCloseWindow())
    return;

  // Application should shutdown on last window close if the user is explicitly
  // trying to quit, or if there is nothing keeping the browser alive (such as
  // AppController on the Mac, or BackgroundContentsService for background
  // pages).
  bool should_quit_if_last_browser =
      browser_shutdown::IsTryingToQuit() ||
      KeepAliveRegistry::GetInstance()->IsKeepingAliveOnlyByBrowserOrigin();

  if (should_quit_if_last_browser && ShouldStartShutdown())
    browser_shutdown::OnShutdownStarting(browser_shutdown::WINDOW_CLOSE);

  // Don't use GetForProfileIfExisting here, we want to force creation of the
  // session service so that user can restore what was open.
  SessionService* session_service =
      SessionServiceFactory::GetForProfile(profile());
  if (session_service)
    session_service->WindowClosing(session_id());

  sessions::TabRestoreService* tab_restore_service =
      TabRestoreServiceFactory::GetForProfile(profile());

#if defined(USE_AURA)
  if (tab_restore_service && is_app() && !is_devtools())
    tab_restore_service->BrowserClosing(live_tab_context());
#endif

  if (tab_restore_service && is_type_tabbed() && tab_strip_model_->count())
    tab_restore_service->BrowserClosing(live_tab_context());

  BrowserList::NotifyBrowserCloseStarted(this);

  if (!IsFastTabUnloadEnabled())
    tab_strip_model_->CloseAllTabs();
}

////////////////////////////////////////////////////////////////////////////////
// In-progress download termination handling:

void Browser::InProgressDownloadResponse(bool cancel_downloads) {
  if (cancel_downloads) {
    cancel_download_confirmation_state_ = RESPONSE_RECEIVED;
    chrome::CloseWindow(this);
    return;
  }

  // Sets the confirmation state to NOT_PROMPTED so that if the user tries to
  // close again we'll show the warning again.
  cancel_download_confirmation_state_ = NOT_PROMPTED;

  // Show the download page so the user can figure-out what downloads are still
  // in-progress.
  chrome::ShowDownloads(this);

  // Reset UnloadController::is_attempting_to_close_browser_ so that we don't
  // prompt every time any tab is closed. http://crbug.com/305516
  if (IsFastTabUnloadEnabled())
    fast_unload_controller_->CancelWindowClose();
  else
    unload_controller_->CancelWindowClose();
}

Browser::DownloadClosePreventionType Browser::OkToCloseWithInProgressDownloads(
    int* num_downloads_blocking) const {
  DCHECK(num_downloads_blocking);
  *num_downloads_blocking = 0;

  // If we're not running a full browser process with a profile manager
  // (testing), it's ok to close the browser.
  if (!g_browser_process->profile_manager())
    return DOWNLOAD_CLOSE_OK;

  int total_download_count =
      DownloadCoreService::NonMaliciousDownloadCountAllProfiles();
  if (total_download_count == 0)
    return DOWNLOAD_CLOSE_OK;   // No downloads; can definitely close.

  // Figure out how many windows are open total, and associated with this
  // profile, that are relevant for the ok-to-close decision.
  int profile_window_count = 0;
  int total_window_count = 0;
  for (auto* browser : *BrowserList::GetInstance()) {
    // Don't count this browser window or any other in the process of closing.
    // Window closing may be delayed, and windows that are in the process of
    // closing don't count against our totals.
    if (browser == this || browser->IsAttemptingToCloseBrowser())
      continue;

    if (browser->profile() == profile())
      profile_window_count++;
    total_window_count++;
  }

  // If there aren't any other windows, we're at browser shutdown,
  // which would cancel all current downloads.
  if (total_window_count == 0) {
    *num_downloads_blocking = total_download_count;
    return DOWNLOAD_CLOSE_BROWSER_SHUTDOWN;
  }

  // If there aren't any other windows on our profile, and we're an incognito
  // profile, and there are downloads associated with that profile,
  // those downloads would be cancelled by our window (-> profile) close.
  DownloadCoreService* download_core_service =
      DownloadCoreServiceFactory::GetForBrowserContext(profile());
  if ((profile_window_count == 0) &&
      (download_core_service->NonMaliciousDownloadCount() > 0) &&
      profile()->IsOffTheRecord()) {
    *num_downloads_blocking =
        download_core_service->NonMaliciousDownloadCount();
    return DOWNLOAD_CLOSE_LAST_WINDOW_IN_INCOGNITO_PROFILE;
  }

  // Those are the only conditions under which we will block shutdown.
  return DOWNLOAD_CLOSE_OK;
}

////////////////////////////////////////////////////////////////////////////////
// Browser, Tab adding/showing functions:

void Browser::WindowFullscreenStateWillChange() {
}

void Browser::WindowFullscreenStateChanged() {
  command_controller_->FullscreenStateChanged();
  UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_TOGGLE_FULLSCREEN);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, Assorted browser commands:

void Browser::ToggleFullscreenModeWithExtension(const GURL& extension_url) {
}

bool Browser::SupportsWindowFeature(WindowFeature feature) const {
  return SupportsWindowFeatureImpl(feature, true);
}

bool Browser::CanSupportWindowFeature(WindowFeature feature) const {
  return SupportsWindowFeatureImpl(feature, false);
}

void Browser::OpenFile() {
  base::RecordAction(UserMetricsAction("OpenFile"));
  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(
                tab_strip_model_->GetActiveWebContents()));

  const base::FilePath directory = profile_->last_selected_directory();

  // TODO(beng): figure out how to juggle this.
  gfx::NativeWindow parent_window = window_->GetNativeWindow();
  ui::SelectFileDialog::FileTypeInfo file_types;
  file_types.allowed_paths = ui::SelectFileDialog::FileTypeInfo::ANY_PATH;
  select_file_dialog_->SelectFile(ui::SelectFileDialog::SELECT_OPEN_FILE,
                                  base::string16(),
                                  directory,
                                  &file_types,
                                  0,
                                  base::FilePath::StringType(),
                                  parent_window,
                                  NULL);
}

void Browser::UpdateDownloadShelfVisibility(bool visible) {
  if (GetStatusBubble())
    GetStatusBubble()->UpdateDownloadShelfVisibility(visible);
}

///////////////////////////////////////////////////////////////////////////////

void Browser::UpdateUIForNavigationInTab(WebContents* contents,
                                         ui::PageTransition transition,
                                         NavigateParams::WindowAction action,
                                         bool user_initiated) {
  LOG(INFO) << "[EXTENSIONS] Calling Browser::UpdateUIForNavigationInTab";
  tab_strip_model_->TabNavigating(contents, transition);

  bool contents_is_selected =
      contents == tab_strip_model_->GetActiveWebContents();

  if (GetStatusBubble())
    GetStatusBubble()->Hide();

  // Update the location bar. This is synchronous. We specifically don't
  // update the load state since the load hasn't started yet and updating it
  // will put it out of sync with the actual state like whether we're
  // displaying a favicon, which controls the throbber. If we updated it here,
  // the throbber will show the default favicon for a split second when
  // navigating away from the new tab page.
  ScheduleUIUpdate(contents, content::INVALIDATE_TYPE_URL);

  if (contents_is_selected &&
      (window()->IsActive() || action == NavigateParams::SHOW_WINDOW)) {
    contents->SetInitialFocus();
  }
}

void Browser::RegisterKeepAlive() {
  keep_alive_.reset(new ScopedKeepAlive(KeepAliveOrigin::BROWSER,
                                        KeepAliveRestartOption::DISABLED));
}
void Browser::UnregisterKeepAlive() {
  keep_alive_.reset();
}

///////////////////////////////////////////////////////////////////////////////
// Browser, PageNavigator implementation:

WebContents* Browser::OpenURL(const OpenURLParams& params) {
  return OpenURLFromTab(NULL, params);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, TabStripModelObserver implementation:

void Browser::TabInsertedAt(TabStripModel* tab_strip_model,
                            WebContents* contents,
                            int index,
                            bool foreground) {
  LOG(INFO) << "[EXTENSIONS] Browser::TabInsertedAt - Step 1";
  SetAsDelegate(contents, true);

  LOG(INFO) << "[EXTENSIONS] Browser::TabInsertedAt - Step 2";
  SessionTabHelper::FromWebContents(contents)->SetWindowID(session_id());

  LOG(INFO) << "[EXTENSIONS] Browser::TabInsertedAt - Step 3";
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_TAB_PARENTED,
      content::Source<content::WebContents>(contents),
      content::NotificationService::NoDetails());

  LOG(INFO) << "[EXTENSIONS] Browser::TabInsertedAt - Step 4";
  SyncHistoryWithTabs(index);

  LOG(INFO) << "[EXTENSIONS] Browser::TabInsertedAt - Step 5";
  // Make sure the loading state is updated correctly, otherwise the throbber
  // won't start if the page is loading. Note that we don't want to
  // ScheduleUIUpdate() because the tab may not have been inserted in the UI
  // yet if this function is called before TabStripModel::TabInsertedAt().
  UpdateWindowForLoadingStateChanged(contents, true);

  LOG(INFO) << "[EXTENSIONS] Browser::TabInsertedAt - Step 6";
  interstitial_observers_.push_back(new InterstitialObserver(this, contents));

  LOG(INFO) << "[EXTENSIONS] Browser::TabInsertedAt - Step 7";
  SessionService* session_service =
      SessionServiceFactory::GetForProfile(profile_);
  LOG(INFO) << "[EXTENSIONS] Browser::TabInsertedAt - Step 8";
  if (session_service) {
    session_service->TabInserted(contents);
    int new_active_index = tab_strip_model_->active_index();
    if (index < new_active_index)
      session_service->SetSelectedTabInWindow(session_id(),
                                              new_active_index);
  }
  LOG(INFO) << "[EXTENSIONS] Browser::TabInsertedAt - Step 9";
}

void Browser::TabClosingAt(TabStripModel* tab_strip_model,
                           WebContents* contents,
                           int index) {
  // Typically, ModalDialogs are closed when the WebContents is destroyed.
  // However, when the tab is being closed, we must first close the dialogs [to
  // give them an opportunity to clean up after themselves] while the state
  // associated with their tab is still valid.
  WebContentsModalDialogManager::FromWebContents(contents)->CloseAllDialogs();

  // Page load metrics need to be informed that the WebContents will soon be
  // destroyed, so that upcoming visiblity changes can be ignored.
  page_load_metrics::MetricsWebContentsObserver* metrics_observer =
      page_load_metrics::MetricsWebContentsObserver::FromWebContents(contents);
  metrics_observer->WebContentsWillSoonBeDestroyed();

  SessionService* session_service =
      SessionServiceFactory::GetForProfile(profile_);
  if (session_service)
    session_service->TabClosing(contents);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_TAB_CLOSING,
      content::Source<NavigationController>(&contents->GetController()),
      content::NotificationService::NoDetails());
}

void Browser::TabDetachedAt(WebContents* contents, int index, bool was_active) {
  if (!tab_strip_model_->closing_all()) {
    SessionService* session_service =
        SessionServiceFactory::GetForProfileIfExisting(profile_);
    if (session_service) {
      session_service->SetSelectedTabInWindow(session_id(),
                                              tab_strip_model_->active_index());
    }
  }

  TabDetachedAtImpl(contents, was_active, DETACH_TYPE_DETACH);
}

void Browser::TabDeactivated(WebContents* contents) {
  SearchTabHelper::FromWebContents(contents)->OnTabDeactivated();
}

void Browser::ActiveTabChanged(WebContents* old_contents,
                               WebContents* new_contents,
                               int index,
                               int reason) {
  LOG(INFO) << "[EXTENSIONS] Browser::ActiveTabChanged - Active tab has changed and we skipped the element";
  if (true)
      return;
// Mac correctly sets the initial background color of new tabs to the theme
// background color, so it does not need this block of code. Aura should
// implement this as well.
// https://crbug.com/719230
#if !defined(OS_MACOSX)
  // Copies the background color from an old WebContents to a new one that
  // replaces it on the screen. This allows the new WebContents to use the
  // old one's background color as the starting background color, before having
  // loaded any contents. As a result, we avoid flashing white when moving to
  // a new tab. (There is also code in RenderFrameHostManager to do something
  // similar for intra-tab navigations.)
  if (old_contents && new_contents) {
    // While GetMainFrame() is guaranteed to return non-null, GetView() is not,
    // e.g. between WebContents creation and creation of the
    // RenderWidgetHostView.
    RenderWidgetHostView* old_view = old_contents->GetMainFrame()->GetView();
    RenderWidgetHostView* new_view = new_contents->GetMainFrame()->GetView();
    if (old_view && new_view && old_view->GetBackgroundColor())
      new_view->SetBackgroundColor(*old_view->GetBackgroundColor());
  }
#endif

  base::RecordAction(UserMetricsAction("ActiveTabChanged"));

  // Update the bookmark state, since the BrowserWindow may query it during
  // OnActiveTabChanged() below.
  UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_TAB_SWITCH);

  // Let the BrowserWindow do its handling.  On e.g. views this changes the
  // focused object, which should happen before we update the toolbar below,
  // since the omnibox expects the correct element to already be focused when it
  // is updated.
  window_->OnActiveTabChanged(old_contents, new_contents, index, reason);

  // If we have any update pending, do it now.
  if (chrome_updater_factory_.HasWeakPtrs() && old_contents)
    ProcessPendingUIUpdates();

  // Propagate the profile to the location bar.
  UpdateToolbar((reason & CHANGE_REASON_REPLACED) == 0);

  // Update reload/stop state.
  command_controller_->LoadingStateChanged(new_contents->IsLoading(), true);

  // Update commands to reflect current state.
  command_controller_->TabStateChanged();

  // Reset the status bubble.
  StatusBubble* status_bubble = GetStatusBubble();
  if (status_bubble) {
    status_bubble->Hide();

    // Show the loading state (if any).
    status_bubble->SetStatus(CoreTabHelper::FromWebContents(
        tab_strip_model_->GetActiveWebContents())->GetStatusText());
  }

  if (HasFindBarController()) {
    find_bar_controller_->ChangeWebContents(new_contents);
    find_bar_controller_->find_bar()->MoveWindowIfNecessary(gfx::Rect());
  }

  // Update sessions (selected tab index and last active time). Don't force
  // creation of sessions. If sessions doesn't exist, the change will be picked
  // up by sessions when created.
  SessionService* session_service =
      SessionServiceFactory::GetForProfileIfExisting(profile_);
  if (session_service && !tab_strip_model_->closing_all()) {
    session_service->SetSelectedTabInWindow(session_id(),
                                            tab_strip_model_->active_index());
    SessionTabHelper* session_tab_helper =
        SessionTabHelper::FromWebContents(new_contents);
    session_service->SetLastActiveTime(session_id(),
                                       session_tab_helper->session_id(),
                                       base::TimeTicks::Now());
  }

  SearchTabHelper::FromWebContents(new_contents)->OnTabActivated();
}

void Browser::TabMoved(WebContents* contents,
                       int from_index,
                       int to_index) {
  DCHECK(from_index >= 0 && to_index >= 0);
  // Notify the history service.
  SyncHistoryWithTabs(std::min(from_index, to_index));
}

void Browser::TabReplacedAt(TabStripModel* tab_strip_model,
                            WebContents* old_contents,
                            WebContents* new_contents,
                            int index) {
  bool was_active = index == tab_strip_model_->active_index();
  TabDetachedAtImpl(old_contents, was_active, DETACH_TYPE_REPLACE);
  SessionService* session_service =
      SessionServiceFactory::GetForProfile(profile_);
  if (session_service)
    session_service->TabClosing(old_contents);
  TabInsertedAt(tab_strip_model, new_contents, index, was_active);

  if (!new_contents->GetController().IsInitialBlankNavigation()) {
    // Send out notification so that observers are updated appropriately.
    int entry_count = new_contents->GetController().GetEntryCount();
    new_contents->GetController().NotifyEntryChanged(
        new_contents->GetController().GetEntryAtIndex(entry_count - 1));
  }

  if (session_service) {
    // The new_contents may end up with a different navigation stack. Force
    // the session service to update itself.
    session_service->TabRestored(new_contents,
                                 tab_strip_model_->IsTabPinned(index));
  }
}

void Browser::TabPinnedStateChanged(TabStripModel* tab_strip_model,
                                    WebContents* contents,
                                    int index) {
  SessionService* session_service =
      SessionServiceFactory::GetForProfileIfExisting(profile());
  if (session_service) {
    SessionTabHelper* session_tab_helper =
        SessionTabHelper::FromWebContents(contents);
    session_service->SetPinnedState(session_id(),
                                    session_tab_helper->session_id(),
                                    tab_strip_model_->IsTabPinned(index));
  }
}

void Browser::TabStripEmpty() {
  // Close the frame after we return to the message loop (not immediately,
  // otherwise it will destroy this object before the stack has a chance to
  // cleanly unwind.)
  // Note: This will be called several times if TabStripEmpty is called several
  //       times. This is because it does not close the window if tabs are
  //       still present.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&Browser::CloseFrame, weak_factory_.GetWeakPtr()));

  // Instant may have visible WebContents that need to be detached before the
  // window system closes.
  instant_controller_.reset();
}

bool Browser::CanOverscrollContent() const {
#if defined(OS_WIN)
  // Don't enable overscroll on Windows machines unless they have a touch
  // screen as these machines typically don't have a touchpad capable of
  // horizontal scrolling. We are purposefully biased towards "no" here,
  // so that we don't waste resources capturing screenshots for horizontal
  // overscroll navigation unnecessarily.
  bool allow_overscroll = ui::GetTouchScreensAvailability() ==
      ui::TouchScreensAvailability::ENABLED;
#elif defined(USE_AURA)
  bool allow_overscroll = true;
#else
  bool allow_overscroll = false;
#endif

  if (!allow_overscroll)
    return false;

  if (is_app() || is_devtools() || !is_type_tabbed())
    return false;

  return content::OverscrollConfig::GetHistoryNavigationMode() !=
         content::OverscrollConfig::HistoryNavigationMode::kDisabled;
}

bool Browser::ShouldPreserveAbortedURLs(WebContents* source) {
  // Allow failed URLs to stick around in the omnibox on the NTP, but not when
  // other pages have committed.
  Profile* profile = Profile::FromBrowserContext(source->GetBrowserContext());
  if (!profile || !source->GetController().GetLastCommittedEntry())
    return false;
  GURL committed_url(source->GetController().GetLastCommittedEntry()->GetURL());
  return search::IsNTPURL(committed_url, profile);
}

void Browser::SetFocusToLocationBar(bool select_all) {
  // Two differences between this and FocusLocationBar():
  // (1) This doesn't get recorded in user metrics, since it's called
  //     internally.
  // (2) This checks whether the location bar can be focused, and if not, clears
  //     the focus.  FocusLocationBar() is only reached when the location bar is
  //     focusable, but this may be reached at other times, e.g. while in
  //     fullscreen mode, where we need to leave focus in a consistent state.
  window_->SetFocusToLocationBar(select_all);
}

content::KeyboardEventProcessingResult Browser::PreHandleKeyboardEvent(
    content::WebContents* source,
    const NativeWebKeyboardEvent& event) {
  // Forward keyboard events to the manager for fullscreen / mouse lock. This
  // may consume the event (e.g., Esc exits fullscreen mode).
  // TODO(koz): Write a test for this http://crbug.com/100441.
  return window()->PreHandleKeyboardEvent(event);
}

void Browser::HandleKeyboardEvent(content::WebContents* source,
                                  const NativeWebKeyboardEvent& event) {
  bool handled = false;

  if (!handled)
    window()->HandleKeyboardEvent(event);
}

bool Browser::TabsNeedBeforeUnloadFired() {
  if (IsFastTabUnloadEnabled())
    return fast_unload_controller_->TabsNeedBeforeUnloadFired();
  return unload_controller_->TabsNeedBeforeUnloadFired();
}

bool Browser::PreHandleGestureEvent(content::WebContents* source,
                                    const blink::WebGestureEvent& event) {
#if defined(OS_MACOSX)
  // Disable "smart zoom" (double-tap with two fingers on Mac trackpad)
  // for the PDF viewer, otherwise the viewer's controls will be scaled off
  // screen.
  // TODO(mcnee): Investigate having the PDF viewer handle the gesture
  // once it is a service. crbug.com/757541
  if (event.GetType() == blink::WebInputEvent::kGestureDoubleTap) {
    content::BrowserPluginGuestManager* guest_manager =
        source->GetBrowserContext()->GetGuestManager();
    if (guest_manager) {
      const content::WebContents* guest_contents =
          guest_manager->GetFullPageGuest(source);
      if (guest_contents) {
        const extensions::Extension* extension =
            extensions::ProcessManager::Get(guest_contents->GetBrowserContext())
                ->GetExtensionForWebContents(guest_contents);
        if (extension && extension->id() == extension_misc::kPdfExtensionId)
          return true;
      }
    }
  }
#endif  // defined(OS_MACOSX)

  // Disable pinch zooming in undocked dev tools window due to poor UX.
  return false;
}

bool Browser::CanDragEnter(content::WebContents* source,
                           const content::DropData& data,
                           blink::WebDragOperationsMask operations_allowed) {
#if defined(OS_CHROMEOS)
  // Disallow drag-and-drop navigation for Settings windows which do not support
  // external navigation.
  if ((operations_allowed & blink::kWebDragOperationLink) &&
      chrome::SettingsWindowManager::GetInstance()->IsSettingsBrowser(this)) {
    return false;
  }
#endif
  return true;
}

blink::WebSecurityStyle Browser::GetSecurityStyle(
    WebContents* web_contents,
    content::SecurityStyleExplanations* security_style_explanations) {
  SecurityStateTabHelper* helper =
      SecurityStateTabHelper::FromWebContents(web_contents);
  DCHECK(helper);
  security_state::SecurityInfo security_info;
  helper->GetSecurityInfo(&security_info);
  return security_state::GetSecurityStyle(security_info,
                                          security_style_explanations);
}

std::unique_ptr<content::BluetoothChooser> Browser::RunBluetoothChooser(
    content::RenderFrameHost* frame,
    const content::BluetoothChooser::EventHandler& event_handler) {
  std::unique_ptr<BluetoothChooserController> bluetooth_chooser_controller(
      new BluetoothChooserController(frame, event_handler));

  std::unique_ptr<BluetoothChooserDesktop> bluetooth_chooser_desktop(
      new BluetoothChooserDesktop(bluetooth_chooser_controller.get()));

  std::unique_ptr<ChooserBubbleDelegate> chooser_bubble_delegate(
      new ChooserBubbleDelegate(frame,
                                std::move(bluetooth_chooser_controller)));

  Browser* browser = chrome::FindBrowserWithWebContents(
      WebContents::FromRenderFrameHost(frame));
  BubbleReference bubble_reference = browser->GetBubbleManager()->ShowBubble(
      std::move(chooser_bubble_delegate));

  return std::move(bluetooth_chooser_desktop);
}

void Browser::RequestAppBannerFromDevTools(content::WebContents* web_contents) {
}

void Browser::PassiveInsecureContentFound(const GURL& resource_url) {
  // Note: this implementation is a mirror of
  // ContentSettingsObserver::passiveInsecureContentFound
  ReportInsecureContent(SslInsecureContentType::DISPLAY);
  FilteredReportInsecureContentDisplayed(resource_url);
}

bool Browser::ShouldAllowRunningInsecureContent(
    content::WebContents* web_contents,
    bool allowed_per_prefs,
    const url::Origin& origin,
    const GURL& resource_url) {
  // Note: this implementation is a mirror of
  // ContentSettingsObserver::allowRunningInsecureContent.
  FilteredReportInsecureContentRan(resource_url);

  MixedContentSettingsTabHelper* mixed_content_settings =
      MixedContentSettingsTabHelper::FromWebContents(web_contents);
  DCHECK(mixed_content_settings);
  if (allowed_per_prefs ||
      mixed_content_settings->is_running_insecure_content_allowed()) {
    return true;
  }

  // Note: this is a browser-side-translation of the call to DidBlockContentType
  // from inside ContentSettingsObserver::allowRunningInsecureContent.
  if (!origin.host().empty()) {
    TabSpecificContentSettings* tab_settings =
        TabSpecificContentSettings::FromWebContents(web_contents);
    DCHECK(tab_settings);
    tab_settings->OnContentBlockedWithDetail(CONTENT_SETTINGS_TYPE_MIXEDSCRIPT,
                                             base::UTF8ToUTF16(origin.host()));
  }
  return false;
}

void Browser::OnDidBlockFramebust(content::WebContents* web_contents,
                                  const GURL& url) {
  TabSpecificContentSettings* content_settings =
      TabSpecificContentSettings::FromWebContents(web_contents);
  DCHECK(content_settings);
  // TODO(csharrison): Add a click callback here to collect framebusting
  // click-through metrics.
  content_settings->OnFramebustBlocked(
      url, FramebustBlockTabHelper::ClickCallback());
}

gfx::Size Browser::EnterPictureInPicture(const viz::SurfaceId& surface_id,
                                         const gfx::Size& natural_size) {
  // If there was already a controller, close the existing window before
  // creating the next one.
  if (pip_window_controller_)
    pip_window_controller_->Close();

  // Create or update |pip_window_controller_| for the current WebContents.
  if (!pip_window_controller_ ||
      pip_window_controller_->GetInitiatorWebContents() !=
          tab_strip_model_->GetActiveWebContents()) {
    pip_window_controller_ =
        content::PictureInPictureWindowController::GetOrCreateForWebContents(
            tab_strip_model_->GetActiveWebContents());
  }

  pip_window_controller_->EmbedSurface(surface_id, natural_size);
  return pip_window_controller_->Show();
}

void Browser::ExitPictureInPicture() {
  if (pip_window_controller_)
    pip_window_controller_->Close();
}

bool Browser::IsMouseLocked() const {
  return false;
}

void Browser::OnWindowDidShow() {
  if (window_has_shown_)
    return;
  window_has_shown_ = true;

  startup_metric_utils::RecordBrowserWindowDisplay(base::TimeTicks::Now());

  // Nothing to do for non-tabbed windows.
  if (!is_type_tabbed())
    return;

  // Show any pending global error bubble.
  GlobalErrorService* service =
      GlobalErrorServiceFactory::GetForProfile(profile());
  GlobalError* error = service->GetFirstGlobalErrorWithBubbleView();
  if (error)
    error->ShowBubbleView(this);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, content::WebContentsDelegate implementation:

WebContents* Browser::OpenURLFromTab(WebContents* source,
                                     const OpenURLParams& params) {
  NavigateParams nav_params(this, params.url, params.transition);
  nav_params.FillNavigateParamsFromOpenURLParams(params);
  nav_params.source_contents = source;
  nav_params.tabstrip_add_types = TabStripModel::ADD_NONE;
  if (params.user_gesture)
    nav_params.window_action = NavigateParams::SHOW_WINDOW;
  nav_params.user_gesture = params.user_gesture;
  nav_params.blob_url_loader_factory = params.blob_url_loader_factory;
  bool is_popup = source && PopupBlockerTabHelper::ConsiderForPopupBlocking(
                                params.disposition);
  if (is_popup && PopupBlockerTabHelper::MaybeBlockPopup(
                      source, base::Optional<GURL>(), &nav_params, &params,
                      blink::mojom::WindowFeatures())) {
    return nullptr;
  }

  Navigate(&nav_params);

  if (is_popup && nav_params.navigated_or_inserted_contents)
    PopupTracker::CreateForWebContents(
        nav_params.navigated_or_inserted_contents, source);

  return nav_params.navigated_or_inserted_contents;
}

void Browser::NavigationStateChanged(WebContents* source,
                                     content::InvalidateTypes changed_flags) {
  //if (true)
  //  return;
  LOG(INFO) << "[EXTENSIONS] Calling Browser::NavigationStateChanged " << changed_flags;
  if (!command_controller_)
	      return;
  // Only update the UI when something visible has changed.
  if (changed_flags)
    ScheduleUIUpdate(source, changed_flags);

  // We can synchronously update commands since they will only change once per
  // navigation, so we don't have to worry about flickering. We do, however,
  // need to update the command state early on load to always present usable
  // actions in the face of slow-to-commit pages.
  if (changed_flags & (content::INVALIDATE_TYPE_URL |
                       content::INVALIDATE_TYPE_LOAD |
                       content::INVALIDATE_TYPE_TAB))
    command_controller_->TabStateChanged();

  if (hosted_app_controller_)
    hosted_app_controller_->UpdateLocationBarVisibility(true);
}

void Browser::VisibleSecurityStateChanged(WebContents* source) {
  // When the current tab's security state changes, we need to update the URL
  // bar to reflect the new state.
  DCHECK(source);
  if (tab_strip_model_->GetActiveWebContents() == source) {
    UpdateToolbar(false);

    if (hosted_app_controller_)
      hosted_app_controller_->UpdateLocationBarVisibility(true);
  }
}

void Browser::AddNewContents(WebContents* source,
                             std::unique_ptr<WebContents> new_contents,
                             WindowOpenDisposition disposition,
                             const gfx::Rect& initial_rect,
                             bool user_gesture,
                             bool* was_blocked) {
  // At this point the |new_contents| is beyond the popup blocker, but we use
  // the same logic for determining if the popup tracker needs to be attached.
  if (source && PopupBlockerTabHelper::ConsiderForPopupBlocking(disposition))
    PopupTracker::CreateForWebContents(new_contents.get(), source);
  chrome::AddWebContents(this, source, std::move(new_contents), disposition,
                         initial_rect);
}

void Browser::ActivateContents(WebContents* contents) {
  tab_strip_model_->ActivateTabAt(
      tab_strip_model_->GetIndexOfWebContents(contents), false);
  window_->Activate();
}

void Browser::LoadingStateChanged(WebContents* source,
                                  bool to_different_document) {
  LOG(INFO) << "[EXTENSIONS] Calling Browser::LoadingStateChanged " << to_different_document;
  ScheduleUIUpdate(source, content::INVALIDATE_TYPE_LOAD);

  UpdateWindowForLoadingStateChanged(source, to_different_document);
}

void Browser::CloseContents(WebContents* source) {
  bool can_close_contents;
  if (IsFastTabUnloadEnabled())
    can_close_contents = fast_unload_controller_->CanCloseContents(source);
  else
    can_close_contents = unload_controller_->CanCloseContents(source);

  if (can_close_contents)
    chrome::CloseWebContents(this, source, true);
}

void Browser::MoveContents(WebContents* source, const gfx::Rect& pos) {
  if (!IsPopupOrPanel(source)) {
    NOTREACHED() << "moving invalid browser type";
    return;
  }
  window_->SetBounds(pos);
}

bool Browser::IsPopupOrPanel(const WebContents* source) const {
  return is_type_popup();
}

void Browser::UpdateTargetURL(WebContents* source, const GURL& url) {
  if (!GetStatusBubble())
    return;

  if (source == tab_strip_model_->GetActiveWebContents())
    GetStatusBubble()->SetURL(url);
}

void Browser::ContentsMouseEvent(WebContents* source,
                                 bool motion,
                                 bool exited) {
  // Mouse motion events update the status bubble, if it exists.
  if (!GetStatusBubble() || (!motion && !exited))
    return;

  if (source == tab_strip_model_->GetActiveWebContents()) {
    GetStatusBubble()->MouseMoved(exited);
    if (exited)
      GetStatusBubble()->SetURL(GURL());
  }
}

void Browser::ContentsZoomChange(bool zoom_in) {
  chrome::ExecuteCommand(this, zoom_in ? IDC_ZOOM_PLUS : IDC_ZOOM_MINUS);
}

bool Browser::TakeFocus(content::WebContents* source,
                        bool reverse) {
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_FOCUS_RETURNED_TO_BROWSER,
      content::Source<Browser>(this),
      content::NotificationService::NoDetails());
  return false;
}

void Browser::BeforeUnloadFired(WebContents* web_contents,
                                bool proceed,
                                bool* proceed_to_fire_unload) {
    *proceed_to_fire_unload =
        unload_controller_->BeforeUnloadFired(web_contents, proceed);
}

bool Browser::ShouldFocusLocationBarByDefault(WebContents* source) {
  // Navigations in background tabs shouldn't change the focus state of the
  // omnibox, since it's associated with the foreground tab.
  if (source != tab_strip_model_->GetActiveWebContents())
    return false;

  const content::NavigationEntry* entry =
      source->GetController().GetActiveEntry();
  if (entry) {
    const GURL& url = entry->GetURL();
    const GURL& virtual_url = entry->GetVirtualURL();
    if ((url.SchemeIs(content::kChromeUIScheme) &&
         url.host_piece() == chrome::kChromeUINewTabHost) ||
        (virtual_url.SchemeIs(content::kChromeUIScheme) &&
         virtual_url.host_piece() == chrome::kChromeUINewTabHost)) {
      return true;
    }
  }

  return search::NavEntryIsInstantNTP(source, entry);
}

void Browser::ShowRepostFormWarningDialog(WebContents* source) {
  TabModalConfirmDialog::Create(new RepostFormWarningController(source),
                                source);
}

bool Browser::ShouldCreateWebContents(
    content::WebContents* web_contents,
    content::RenderFrameHost* opener,
    content::SiteInstance* source_site_instance,
    int32_t route_id,
    int32_t main_frame_route_id,
    int32_t main_frame_widget_route_id,
    content::mojom::WindowContainerType window_container_type,
    const GURL& opener_url,
    const std::string& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  if (window_container_type ==
      content::mojom::WindowContainerType::BACKGROUND) {
    // If a BackgroundContents is created, suppress the normal WebContents.
    return !MaybeCreateBackgroundContents(
        source_site_instance, opener, opener_url, route_id, main_frame_route_id,
        main_frame_widget_route_id, frame_name, target_url, partition_id,
        session_storage_namespace);
  }

  return true;
}

void Browser::WebContentsCreated(WebContents* source_contents,
                                 int opener_render_process_id,
                                 int opener_render_frame_id,
                                 const std::string& frame_name,
                                 const GURL& target_url,
                                 WebContents* new_contents) {
  // Adopt the WebContents now, so all observers are in place, as the network
  // requests for its initial navigation will start immediately. The WebContents
  // will later be inserted into this browser using Browser::Navigate via
  // AddNewContents.
  TabHelpers::AttachTabHelpers(new_contents);

  // Make the tab show up in the task manager.
  task_manager::WebContentsTags::CreateForTabContents(new_contents);
}

void Browser::RendererUnresponsive(
    WebContents* source,
    content::RenderWidgetHost* render_widget_host) {
  // Ignore hangs if a tab is blocked.
  int index = tab_strip_model_->GetIndexOfWebContents(source);
  DCHECK_NE(TabStripModel::kNoTab, index);
  if (tab_strip_model_->IsTabBlocked(index))
    return;

  TabDialogs::FromWebContents(source)->ShowHungRendererDialog(
      render_widget_host);
}

void Browser::RendererResponsive(
    WebContents* source,
    content::RenderWidgetHost* render_widget_host) {
  TabDialogs::FromWebContents(source)->HideHungRendererDialog(
      render_widget_host);
}

void Browser::DidNavigateMainFramePostCommit(WebContents* web_contents) {
  if (web_contents == tab_strip_model_->GetActiveWebContents())
    UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_TAB_STATE);
}

content::JavaScriptDialogManager* Browser::GetJavaScriptDialogManager(
    WebContents* source) {
  return JavaScriptDialogTabHelper::FromWebContents(source);
}

content::ColorChooser* Browser::OpenColorChooser(
    WebContents* web_contents,
    SkColor initial_color,
    const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions) {
  return chrome::ShowColorChooser(web_contents, initial_color);
}

void Browser::RunFileChooser(content::RenderFrameHost* render_frame_host,
                             const content::FileChooserParams& params) {
  FileSelectHelper::RunFileChooser(render_frame_host, params);
}

void Browser::EnumerateDirectory(WebContents* web_contents,
                                 int request_id,
                                 const base::FilePath& path) {
  FileSelectHelper::EnumerateDirectory(web_contents, request_id, path);
}

bool Browser::EmbedsFullscreenWidget() const {
  return true;
}

void Browser::EnterFullscreenModeForTab(
    WebContents* web_contents,
    const GURL& origin,
    const blink::WebFullscreenOptions& options) {
}

void Browser::ExitFullscreenModeForTab(WebContents* web_contents) {
}

bool Browser::IsFullscreenForTabOrPending(
    const WebContents* web_contents) const {
  return false;
}

blink::WebDisplayMode Browser::GetDisplayMode(
    const WebContents* web_contents) const {
  if (window_->IsFullscreen())
    return blink::kWebDisplayModeFullscreen;

  if (is_type_popup())
    return blink::kWebDisplayModeStandalone;

  return blink::kWebDisplayModeBrowser;
}

void Browser::RegisterProtocolHandler(WebContents* web_contents,
                                      const std::string& protocol,
                                      const GURL& url,
                                      bool user_gesture) {
  content::BrowserContext* context = web_contents->GetBrowserContext();
  if (context->IsOffTheRecord())
    return;

  // Permission request UI cannot currently be rendered binocularly in VR mode,
  // so we suppress the UI. crbug.com/736568
  if (vr::VrTabHelper::IsInVr(web_contents))
    return;

  ProtocolHandler handler =
      ProtocolHandler::CreateProtocolHandler(protocol, url);

  ProtocolHandlerRegistry* registry =
      ProtocolHandlerRegistryFactory::GetForBrowserContext(context);
  if (registry->SilentlyHandleRegisterHandlerRequest(handler))
    return;

  TabSpecificContentSettings* tab_content_settings =
      TabSpecificContentSettings::FromWebContents(web_contents);
  if (!user_gesture && window_) {
    tab_content_settings->set_pending_protocol_handler(handler);
    tab_content_settings->set_previous_protocol_handler(
        registry->GetHandlerFor(handler.protocol()));
    return;
  }

  // Make sure content-setting icon is turned off in case the page does
  // ungestured and gestured RPH calls.
  if (window_) {
    tab_content_settings->ClearPendingProtocolHandler();
  }

  PermissionRequestManager* permission_request_manager =
      PermissionRequestManager::FromWebContents(web_contents);
  if (permission_request_manager) {
    permission_request_manager->AddRequest(
        new RegisterProtocolHandlerPermissionRequest(registry, handler,
                                                     url, user_gesture));
  }
}

void Browser::UnregisterProtocolHandler(WebContents* web_contents,
                                        const std::string& protocol,
                                        const GURL& url,
                                        bool user_gesture) {
  // user_gesture will be used in case we decide to have confirmation bubble
  // for user while un-registering the handler.
  content::BrowserContext* context = web_contents->GetBrowserContext();
  if (context->IsOffTheRecord())
    return;

  ProtocolHandler handler =
      ProtocolHandler::CreateProtocolHandler(protocol, url);

  ProtocolHandlerRegistry* registry =
      ProtocolHandlerRegistryFactory::GetForBrowserContext(context);
  registry->RemoveHandler(handler);
}

void Browser::FindReply(WebContents* web_contents,
                        int request_id,
                        int number_of_matches,
                        const gfx::Rect& selection_rect,
                        int active_match_ordinal,
                        bool final_update) {
  FindTabHelper* find_tab_helper = FindTabHelper::FromWebContents(web_contents);
  if (!find_tab_helper)
    return;

  find_tab_helper->HandleFindReply(request_id,
                                   number_of_matches,
                                   selection_rect,
                                   active_match_ordinal,
                                   final_update);
}

void Browser::RequestToLockMouse(WebContents* web_contents,
                                 bool user_gesture,
                                 bool last_unlocked_by_target) {
}

void Browser::LostMouseLock() {
}

void Browser::RequestKeyboardLock(WebContents* web_contents,
                                  bool esc_key_locked) {
}

void Browser::CancelKeyboardLockRequest(WebContents* web_contents) {
}

void Browser::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  const extensions::Extension* extension =
      GetExtensionForOrigin(profile_, request.security_origin);
  MediaCaptureDevicesDispatcher::GetInstance()->ProcessMediaAccessRequest(
      web_contents, request, callback, extension);
}

bool Browser::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    content::MediaStreamType type) {
  Profile* profile = Profile::FromBrowserContext(
      content::WebContents::FromRenderFrameHost(render_frame_host)
          ->GetBrowserContext());
  const extensions::Extension* extension =
      GetExtensionForOrigin(profile, security_origin);
  return MediaCaptureDevicesDispatcher::GetInstance()
      ->CheckMediaAccessPermission(render_frame_host, security_origin, type,
                                   extension);
}

std::string Browser::GetDefaultMediaDeviceID(content::WebContents* web_contents,
                                             content::MediaStreamType type) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  return MediaCaptureDevicesDispatcher::GetInstance()
      ->GetDefaultDeviceIDForProfile(profile, type);
}

bool Browser::RequestPpapiBrokerPermission(
    WebContents* web_contents,
    const GURL& url,
    const base::FilePath& plugin_path,
    const base::Callback<void(bool)>& callback) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  // TODO(wad): Add ephemeral device ID support for broker in guest mode.
  if (profile->IsGuestSession()) {
    callback.Run(false);
    return true;
  }

  TabSpecificContentSettings* tab_content_settings =
      TabSpecificContentSettings::FromWebContents(web_contents);

  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(profile);
  ContentSetting setting = content_settings->GetContentSetting(
      url, url, CONTENT_SETTINGS_TYPE_PPAPI_BROKER, std::string());

  if (setting == CONTENT_SETTING_ASK) {
    base::RecordAction(base::UserMetricsAction("PPAPI.BrokerInfobarDisplayed"));

    content::PluginService* plugin_service =
        content::PluginService::GetInstance();
    content::WebPluginInfo plugin;
    bool success = plugin_service->GetPluginInfoByPath(plugin_path, &plugin);
    DCHECK(success);
    std::unique_ptr<PluginMetadata> plugin_metadata(
        PluginFinder::GetInstance()->GetPluginMetadata(plugin));

    PepperBrokerInfoBarDelegate::Create(
        InfoBarService::FromWebContents(web_contents), url,
        plugin_metadata->name(), content_settings, tab_content_settings,
        callback);
    return true;
  }

  bool allowed = (setting == CONTENT_SETTING_ALLOW);
  base::RecordAction(allowed
                         ? base::UserMetricsAction("PPAPI.BrokerSettingAllow")
                         : base::UserMetricsAction("PPAPI.BrokerSettingDeny"));
  tab_content_settings->SetPepperBrokerAllowed(allowed);
  callback.Run(allowed);
  return true;
}

gfx::Size Browser::GetSizeForNewRenderView(WebContents* web_contents) const {
  // When navigating away from NTP with unpinned bookmark bar, the bookmark bar
  // would disappear on non-NTP pages, resulting in a bigger size for the new
  // render view.
  gfx::Size size = web_contents->GetContainerBounds().size();
  // Don't change render view size if bookmark bar is currently not detached,
  // or there's no pending entry, or navigating to a NTP page.
  if (size.IsEmpty() || bookmark_bar_state_ != BookmarkBar::DETACHED)
    return size;
  const NavigationEntry* pending_entry =
      web_contents->GetController().GetPendingEntry();
  if (pending_entry &&
      !search::IsNTPURL(pending_entry->GetVirtualURL(), profile_)) {
    size.Enlarge(
        0, window()->GetRenderViewHeightInsetWithDetachedBookmarkBar());
  }
  return size;
}

#if BUILDFLAG(ENABLE_PRINTING)
void Browser::PrintCrossProcessSubframe(
    content::WebContents* web_contents,
    const gfx::Rect& rect,
    int document_cookie,
    content::RenderFrameHost* subframe_host) const {
  auto* client = printing::PrintCompositeClient::FromWebContents(web_contents);
  if (client)
    client->PrintCrossProcessSubframe(rect, document_cookie, subframe_host);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Browser, CoreTabHelperDelegate implementation:

std::unique_ptr<content::WebContents> Browser::SwapTabContents(
    content::WebContents* old_contents,
    std::unique_ptr<content::WebContents> new_contents,
    bool did_start_load,
    bool did_finish_load) {
  // Copies the background color and contents of the old WebContents to a new
  // one that replaces it on the screen. This allows the new WebContents to
  // have something to show before having loaded any contents. As a result, we
  // avoid flashing white when navigating from a site whith a dark background to
  // another site with a dark background.
  if (old_contents && new_contents) {
    RenderWidgetHostView* old_view = old_contents->GetMainFrame()->GetView();
    RenderWidgetHostView* new_view = new_contents->GetMainFrame()->GetView();
    if (old_view && new_view)
      new_view->TakeFallbackContentFrom(old_view);
  }

  int index = tab_strip_model_->GetIndexOfWebContents(old_contents);
  DCHECK_NE(TabStripModel::kNoTab, index);
  return tab_strip_model_->ReplaceWebContentsAt(index, std::move(new_contents));
}

bool Browser::CanReloadContents(content::WebContents* web_contents) const {
  return chrome::CanReload(this);
}

bool Browser::CanSaveContents(content::WebContents* web_contents) const {
  return chrome::CanSavePage(this);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, web_modal::WebContentsModalDialogManagerDelegate implementation:

void Browser::SetWebContentsBlocked(content::WebContents* web_contents,
                                    bool blocked) {
  int index = tab_strip_model_->GetIndexOfWebContents(web_contents);
  if (index == TabStripModel::kNoTab) {
    // Removal of tabs from the TabStripModel can cause observer callbacks to
    // invoke this method. The WebContents may no longer exist in the
    // TabStripModel.
    return;
  }
  tab_strip_model_->SetTabBlocked(index, blocked);

  bool browser_active = BrowserList::GetInstance()->GetLastActive() == this;
  bool contents_is_active =
      tab_strip_model_->GetActiveWebContents() == web_contents;
  // If the WebContents is foremost (the active tab in the front-most browser)
  // and is being unblocked, focus it to make sure that input works again.
  if (!blocked && contents_is_active && browser_active)
    web_contents->Focus();
}

web_modal::WebContentsModalDialogHost*
Browser::GetWebContentsModalDialogHost() {
  return window_->GetWebContentsModalDialogHost();
}

///////////////////////////////////////////////////////////////////////////////
// Browser, BookmarkTabHelperDelegate implementation:

void Browser::URLStarredChanged(content::WebContents* web_contents,
                                bool starred) {
  if (web_contents == tab_strip_model_->GetActiveWebContents())
    window_->SetStarredState(starred);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, ZoomObserver implementation:

void Browser::OnZoomChanged(
    const zoom::ZoomController::ZoomChangedEventData& data) {
  if (data.web_contents == tab_strip_model_->GetActiveWebContents()) {
    window_->ZoomChangedForActiveTab(data.can_show_bubble);
    // Change the zoom commands state based on the zoom state
    command_controller_->ZoomStateChanged();
  }
}

///////////////////////////////////////////////////////////////////////////////
// Browser, ui::SelectFileDialog::Listener implementation:

void Browser::FileSelected(const base::FilePath& path, int index,
                           void* params) {
  FileSelectedWithExtraInfo(ui::SelectedFileInfo(path, path), index, params);
}

void Browser::FileSelectedWithExtraInfo(const ui::SelectedFileInfo& file_info,
                                        int index,
                                        void* params) {
  profile_->set_last_selected_directory(file_info.file_path.DirName());

  GURL url = net::FilePathToFileURL(file_info.local_path);

#if defined(OS_CHROMEOS)
  const GURL external_url =
      chromeos::CreateExternalFileURLFromPath(profile_, file_info.file_path);
  if (!external_url.is_empty())
    url = external_url;
#endif

  if (url.is_empty())
    return;

  OpenURL(OpenURLParams(url, Referrer(), WindowOpenDisposition::CURRENT_TAB,
                        ui::PAGE_TRANSITION_TYPED, false));
}

///////////////////////////////////////////////////////////////////////////////
// Browser, content::NotificationObserver implementation:

void Browser::Observe(int type,
                      const content::NotificationSource& source,
                      const content::NotificationDetails& details) {
  switch (type) {
#if !defined(OS_ANDROID)
    case chrome::NOTIFICATION_BROWSER_THEME_CHANGED:
      window()->UserChangedTheme();
      break;
#endif

    case chrome::NOTIFICATION_WEB_CONTENT_SETTINGS_CHANGED: {
      WebContents* web_contents = content::Source<WebContents>(source).ptr();
      if (web_contents == tab_strip_model_->GetActiveWebContents()) {
      }
      break;
    }

    default:
      NOTREACHED() << "Got a notification we didn't register for.";
  }
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
///////////////////////////////////////////////////////////////////////////////
// Browser, extensions::ExtensionRegistryObserver implementation:

void Browser::OnExtensionLoaded(content::BrowserContext* browser_context,
                                const extensions::Extension* extension) {
  command_controller_->ExtensionStateChanged();
}

void Browser::OnExtensionUnloaded(content::BrowserContext* browser_context,
                                  const extensions::Extension* extension,
                                  extensions::UnloadedExtensionReason reason) {
  command_controller_->ExtensionStateChanged();

  // Close any tabs from the unloaded extension, unless it's terminated,
  // in which case let the sad tabs remain.
  // Also, if tab is muted and the cause is the unloaded extension, unmute it.
  if (reason != extensions::UnloadedExtensionReason::TERMINATE) {
    // Iterate backwards as we may remove items while iterating.
    for (int i = tab_strip_model_->count() - 1; i >= 0; --i) {
      WebContents* web_contents = tab_strip_model_->GetWebContentsAt(i);
      // Two cases are handled here:

      // - The scheme check is for when an extension page is loaded in a
      // tab, e.g. chrome-extension://id/page.html.
      // - The extension_app check is for apps, which can have non-extension
      // schemes, e.g. https://mail.google.com if you have the Gmail app
      // installed.
      if ((web_contents->GetURL().SchemeIs(extensions::kExtensionScheme) &&
           web_contents->GetURL().host_piece() == extension->id()) ||
          (extensions::TabHelper::FromWebContents(web_contents)
               ->extension_app() == extension)) {
        tab_strip_model_->CloseWebContentsAt(i, TabStripModel::CLOSE_NONE);
      } else {
        chrome::UnmuteIfMutedByExtension(web_contents, extension->id());
      }
    }
  }
}
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

///////////////////////////////////////////////////////////////////////////////
// Browser, translate::ContentTranslateDriver::Observer implementation:

void Browser::OnIsPageTranslatedChanged(content::WebContents* source) {
  DCHECK(source);
  if (tab_strip_model_->GetActiveWebContents() == source) {
    window_->SetTranslateIconToggled(
        ChromeTranslateClient::FromWebContents(
            source)->GetLanguageState().IsPageTranslated());
  }
}

void Browser::OnTranslateEnabledChanged(content::WebContents* source) {
  DCHECK(source);
  if (tab_strip_model_->GetActiveWebContents() == source)
    UpdateToolbar(false);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, Command and state updating (private):

void Browser::OnDevToolsAvailabilityChanged() {
  using DTPH = policy::DeveloperToolsPolicyHandler;
  // TODO(pfeldman): If |new_value| maps to
  // |DTPH::Availability::kDisallowedForForceInstalledExtensions|, we may either
  // close all windows as a safety measure, or close those that are known to
  // correspond to such extensions (https://crbug.com/838146).
  if (DTPH::GetDevToolsAvailability(profile_->GetPrefs()) ==
      DTPH::Availability::kDisallowed) {
    content::DevToolsAgentHost::DetachAllClients();
  }
}

///////////////////////////////////////////////////////////////////////////////
// Browser, UI update coalescing and handling (private):

void Browser::UpdateToolbar(bool should_restore_state) {
  window_->UpdateToolbar(should_restore_state ?
      tab_strip_model_->GetActiveWebContents() : NULL);
}

void Browser::ScheduleUIUpdate(WebContents* source,
                               unsigned changed_flags) {
  DCHECK(source);
  int index = tab_strip_model_->GetIndexOfWebContents(source);
  DCHECK_NE(TabStripModel::kNoTab, index);

  // Do some synchronous updates.
  if (changed_flags & content::INVALIDATE_TYPE_URL) {
    if (source == tab_strip_model_->GetActiveWebContents()) {
      // Only update the URL for the current tab. Note that we do not update
      // the navigation commands since those would have already been updated
      // synchronously by NavigationStateChanged.
      UpdateToolbar(false);
    } else {
      // Clear the saved tab state for the tab that navigated, so that we don't
      // restore any user text after the old URL has been invalidated (e.g.,
      // after a new navigation commits in that tab while unfocused).
      window_->ResetToolbarTabState(source);
    }
    changed_flags &= ~content::INVALIDATE_TYPE_URL;
  }

  if (changed_flags & content::INVALIDATE_TYPE_LOAD) {
    // Update the loading state synchronously. This is so the throbber will
    // immediately start/stop, which gives a more snappy feel. We want to do
    // this for any tab so they start & stop quickly.
    tab_strip_model_->UpdateWebContentsStateAt(
        tab_strip_model_->GetIndexOfWebContents(source),
        TabChangeType::kLoadingOnly);
    // The status bubble needs to be updated during INVALIDATE_TYPE_LOAD too,
    // but we do that asynchronously by not stripping INVALIDATE_TYPE_LOAD from
    // changed_flags.
  }

  // If the only updates were synchronously handled above, we're done.
  if (changed_flags == 0)
    return;

  // Save the dirty bits.
  scheduled_updates_[source] |= changed_flags;

  if (!chrome_updater_factory_.HasWeakPtrs()) {
    // No task currently scheduled, start another.
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&Browser::ProcessPendingUIUpdates,
                       chrome_updater_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(kUIUpdateCoalescingTimeMS));
  }
}

void Browser::ProcessPendingUIUpdates() {
  LOG(INFO) << "[EXTENSIONS] Calling Browser::ProcessPendingUIUpdates ";
#ifndef NDEBUG
  // Validate that all tabs we have pending updates for exist. This is scary
  // because the pending list must be kept in sync with any detached or
  // deleted tabs.
  for (UpdateMap::const_iterator i = scheduled_updates_.begin();
       i != scheduled_updates_.end(); ++i) {
    bool found = false;
    for (int tab = 0; tab < tab_strip_model_->count(); tab++) {
      if (tab_strip_model_->GetWebContentsAt(tab) == i->first) {
        found = true;
        break;
      }
    }
    DCHECK(found);
  }
#endif

  chrome_updater_factory_.InvalidateWeakPtrs();

  for (UpdateMap::const_iterator i = scheduled_updates_.begin();
       i != scheduled_updates_.end(); ++i) {
    // Do not dereference |contents|, it may be out-of-date!
    const WebContents* contents = i->first;
    unsigned flags = i->second;

    if (contents == tab_strip_model_->GetActiveWebContents()) {
      // Updates that only matter when the tab is selected go here.

      // Updating the URL happens synchronously in ScheduleUIUpdate.
      if (flags & content::INVALIDATE_TYPE_LOAD && GetStatusBubble()) {
        GetStatusBubble()->SetStatus(CoreTabHelper::FromWebContents(
            tab_strip_model_->GetActiveWebContents())->GetStatusText());
      }

      if (flags & (content::INVALIDATE_TYPE_TAB |
                   content::INVALIDATE_TYPE_TITLE)) {
        window_->UpdateTitleBar();
      }
    }

    // Updates that don't depend upon the selected state go here.
    if (flags &
        (content::INVALIDATE_TYPE_TAB | content::INVALIDATE_TYPE_TITLE)) {
      tab_strip_model_->UpdateWebContentsStateAt(
          tab_strip_model_->GetIndexOfWebContents(contents),
          TabChangeType::kAll);
    }

    // Update the bookmark bar. It may happen that the tab is crashed, and if
    // so, the bookmark bar should be hidden.
    if (flags & content::INVALIDATE_TYPE_TAB)
      UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_TAB_STATE);

    // We don't need to process INVALIDATE_STATE, since that's not visible.
  }

  scheduled_updates_.clear();
}

void Browser::RemoveScheduledUpdatesFor(WebContents* contents) {
  if (!contents)
    return;

  UpdateMap::iterator i = scheduled_updates_.find(contents);
  if (i != scheduled_updates_.end())
    scheduled_updates_.erase(i);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, Getters for UI (private):

StatusBubble* Browser::GetStatusBubble() {
  // In kiosk and exclusive app mode, we want to always hide the status bubble.
  if (chrome::IsRunningInAppMode())
    return NULL;

  return window_ ? window_->GetStatusBubble() : NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Browser, Session restore functions (private):

void Browser::SyncHistoryWithTabs(int index) {
  SessionService* session_service =
      SessionServiceFactory::GetForProfileIfExisting(profile());
  if (session_service) {
    for (int i = index; i < tab_strip_model_->count(); ++i) {
      WebContents* web_contents = tab_strip_model_->GetWebContentsAt(i);
      if (web_contents) {
        SessionTabHelper* session_tab_helper =
            SessionTabHelper::FromWebContents(web_contents);
        session_service->SetTabIndexInWindow(
            session_id(), session_tab_helper->session_id(), i);
        session_service->SetPinnedState(
            session_id(),
            session_tab_helper->session_id(),
            tab_strip_model_->IsTabPinned(i));
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Browser, In-progress download termination handling (private):

bool Browser::CanCloseWithInProgressDownloads() {
  // If we've prompted, we need to hear from the user before we
  // can close.
  if (cancel_download_confirmation_state_ != NOT_PROMPTED)
    return cancel_download_confirmation_state_ != WAITING_FOR_RESPONSE;

  int num_downloads_blocking;
  Browser::DownloadClosePreventionType dialog_type =
      OkToCloseWithInProgressDownloads(&num_downloads_blocking);
  if (dialog_type == DOWNLOAD_CLOSE_OK)
    return true;

  // Closing this window will kill some downloads; prompt to make sure
  // that's ok.
  cancel_download_confirmation_state_ = WAITING_FOR_RESPONSE;
  window_->ConfirmBrowserCloseWithPendingDownloads(
      num_downloads_blocking,
      dialog_type,
      false,
      base::Bind(&Browser::InProgressDownloadResponse,
                 weak_factory_.GetWeakPtr()));

  // Return false so the browser does not close.  We'll close if the user
  // confirms in the dialog.
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// Browser, Assorted utility functions (private):

void Browser::SetAsDelegate(WebContents* web_contents, bool set_delegate) {
  LOG(INFO) << "[EXTENSIONS] Browser::SetAsDelegate - Step 1";
  Browser* delegate = set_delegate ? this : nullptr;

  LOG(INFO) << "[EXTENSIONS] Browser::SetAsDelegate - Step 2 - and delegate is " << delegate;
  // WebContents...
  web_contents->SetDelegate(delegate);

  LOG(INFO) << "[EXTENSIONS] Browser::SetAsDelegate - Step 3";
  // ...and all the helpers.
  BookmarkTabHelper::FromWebContents(web_contents)->set_delegate(delegate);
  LOG(INFO) << "[EXTENSIONS] Browser::SetAsDelegate - Step 4";
  LOG(INFO) << "[EXTENSIONS] Browser::SetAsDelegate - Step 4a: " << web_contents;
  LOG(INFO) << "[EXTENSIONS] Browser::SetAsDelegate - Step 4b: " << WebContentsModalDialogManager::FromWebContents(web_contents);
  WebContentsModalDialogManager::FromWebContents(web_contents)->
      SetDelegate(delegate);
  LOG(INFO) << "[EXTENSIONS] Browser::SetAsDelegate - Step 5";
  CoreTabHelper::FromWebContents(web_contents)->set_delegate(delegate);
  LOG(INFO) << "[EXTENSIONS] Browser::SetAsDelegate - Step 6";
  translate::ContentTranslateDriver& content_translate_driver =
      ChromeTranslateClient::FromWebContents(web_contents)->translate_driver();
  LOG(INFO) << "[EXTENSIONS] Browser::SetAsDelegate - Step 7";
  if (delegate) {
    zoom::ZoomController::FromWebContents(web_contents)->AddObserver(this);
    content_translate_driver.AddObserver(this);
  } else {
    zoom::ZoomController::FromWebContents(web_contents)->RemoveObserver(this);
    content_translate_driver.RemoveObserver(this);
  }
  LOG(INFO) << "[EXTENSIONS] Browser::SetAsDelegate - Step 8";
}

void Browser::CloseFrame() {
  window_->Close();
}

void Browser::TabDetachedAtImpl(content::WebContents* contents,
                                bool was_active,
                                DetachType type) {
  if (type == DETACH_TYPE_DETACH) {
    // Save the current location bar state, but only if the tab being detached
    // is the selected tab.  Because saving state can conditionally revert the
    // location bar, saving the current tab's location bar state to a
    // non-selected tab can corrupt both tabs.
    if (was_active) {
    }

    if (!tab_strip_model_->closing_all())
      SyncHistoryWithTabs(0);
  }

  SetAsDelegate(contents, false);
  RemoveScheduledUpdatesFor(contents);

  if (HasFindBarController() && was_active)
    find_bar_controller_->ChangeWebContents(NULL);

  for (size_t i = 0; i < interstitial_observers_.size(); i++) {
    if (interstitial_observers_[i]->web_contents() != contents)
      continue;

    delete interstitial_observers_[i];
    interstitial_observers_.erase(interstitial_observers_.begin() + i);
    return;
  }
}

void Browser::UpdateWindowForLoadingStateChanged(content::WebContents* source,
                                                 bool to_different_document) {
  window_->UpdateLoadingAnimations(tab_strip_model_->TabsAreLoading());
  window_->UpdateTitleBar();

  WebContents* selected_contents = tab_strip_model_->GetActiveWebContents();
  if (source == selected_contents) {
    bool is_loading = source->IsLoading() && to_different_document;
    command_controller_->LoadingStateChanged(is_loading, false);
    if (GetStatusBubble()) {
      GetStatusBubble()->SetStatus(CoreTabHelper::FromWebContents(
                                       tab_strip_model_->GetActiveWebContents())
                                       ->GetStatusText());
    }
  }
}

bool Browser::SupportsLocationBar() const {
  // Tabbed browser always show a location bar.
  if (is_type_tabbed())
    return true;

  // Non-app windows that aren't tabbed or system windows should always show a
  // location bar, unless they are from a trusted source.
  if (!is_app())
    return !is_trusted_source();

  // Hosted apps always support a location bar.
  if (hosted_app_controller_)
    return true;

  return false;
}

bool Browser::SupportsWindowFeatureImpl(WindowFeature feature,
                                        bool check_fullscreen) const {
  bool hide_ui_for_fullscreen = check_fullscreen && ShouldHideUIForFullscreen();

  unsigned int features = FEATURE_INFOBAR | FEATURE_DOWNLOADSHELF;

  if (is_type_tabbed())
    features |= FEATURE_BOOKMARKBAR;

  if (!hide_ui_for_fullscreen) {
    if (!is_type_tabbed())
      features |= FEATURE_TITLEBAR;

    if (is_type_tabbed())
      features |= FEATURE_TABSTRIP;

    if (is_type_tabbed())
      features |= FEATURE_TOOLBAR;

    if (SupportsLocationBar())
      features |= FEATURE_LOCATIONBAR;
  }
  return !!(features & feature);
}

void Browser::UpdateBookmarkBarState(BookmarkBarStateChangeReason reason) {
  BookmarkBar::State state;
  // The bookmark bar is always hidden for Guest Sessions and in fullscreen
  // mode, unless on the new tab page.
  if (profile_->IsGuestSession()) {
    state = BookmarkBar::HIDDEN;
  } else if (browser_defaults::bookmarks_enabled &&
      profile_->GetPrefs()->GetBoolean(bookmarks::prefs::kShowBookmarkBar) &&
      !ShouldHideUIForFullscreen()) {
    state = BookmarkBar::SHOW;
  } else {
    WebContents* web_contents = tab_strip_model_->GetActiveWebContents();
    BookmarkTabHelper* bookmark_tab_helper =
        web_contents ? BookmarkTabHelper::FromWebContents(web_contents) : NULL;
    if (bookmark_tab_helper && bookmark_tab_helper->ShouldShowBookmarkBar())
      state = BookmarkBar::DETACHED;
    else
      state = BookmarkBar::HIDDEN;
  }

  if (state == bookmark_bar_state_)
    return;

  bookmark_bar_state_ = state;

  if (!window_)
    return;  // This is called from the constructor when window_ is NULL.

  if (reason == BOOKMARK_BAR_STATE_CHANGE_TAB_SWITCH) {
    // Don't notify BrowserWindow on a tab switch as at the time this is invoked
    // BrowserWindow hasn't yet switched tabs. The BrowserWindow implementations
    // end up querying state once they process the tab switch.
    return;
  }

  bool should_animate = reason == BOOKMARK_BAR_STATE_CHANGE_PREF_CHANGE;
  window_->BookmarkBarStateChanged(should_animate ?
      BookmarkBar::ANIMATE_STATE_CHANGE :
      BookmarkBar::DONT_ANIMATE_STATE_CHANGE);
}

bool Browser::ShouldHideUIForFullscreen() const {
  // Windows and GTK remove the browser controls in fullscreen, but Mac and Ash
  // keep the controls in a slide-down panel.
  return window_ && window_->ShouldHideUIForFullscreen();
}

bool Browser::IsBrowserClosing() const {
  const BrowserList::BrowserSet& closing_browsers =
      BrowserList::GetInstance()->currently_closing_browsers();

  return base::ContainsKey(closing_browsers, this);
}

bool Browser::ShouldStartShutdown() const {
  if (IsBrowserClosing())
    return false;

  const size_t closing_browsers_count =
      BrowserList::GetInstance()->currently_closing_browsers().size();
  return BrowserList::GetInstance()->size() == closing_browsers_count + 1u;
}

bool Browser::MaybeCreateBackgroundContents(
    content::SiteInstance* source_site_instance,
    content::RenderFrameHost* opener,
    const GURL& opener_url,
    int32_t route_id,
    int32_t main_frame_route_id,
    int32_t main_frame_widget_route_id,
    const std::string& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  ExtensionService* extensions_service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();

  if (!opener_url.is_valid() || frame_name.empty() || !extensions_service ||
      !extensions_service->is_ready())
    return false;

  // Only hosted apps have web extents, so this ensures that only hosted apps
  // can create BackgroundContents. We don't have to check for background
  // permission as that is checked in RenderMessageFilter when the CreateWindow
  // message is processed.
  const Extension* extension = extensions::ExtensionRegistry::Get(profile_)
                                   ->enabled_extensions()
                                   .GetHostedAppByURL(opener_url);
  if (!extension)
    return false;

  // No BackgroundContents allowed if BackgroundContentsService doesn't exist.
  BackgroundContentsService* service =
      BackgroundContentsServiceFactory::GetForProfile(profile_);
  if (!service)
    return false;

  // Ensure that we're trying to open this from the extension's process.
  extensions::ProcessMap* process_map = extensions::ProcessMap::Get(profile_);
  if (!source_site_instance->GetProcess() ||
      !process_map->Contains(extension->id(),
                             source_site_instance->GetProcess()->GetID())) {
    return false;
  }

  // Only allow a single background contents per app.
  bool allow_js_access = extensions::BackgroundInfo::AllowJSAccess(extension);
  BackgroundContents* existing =
      service->GetAppBackgroundContents(extension->id());
  if (existing) {
    // For non-scriptable background contents, ignore the request altogether,
    // (returning true, so that a regular WebContents isn't created either).
    if (!allow_js_access)
      return true;
    // For scriptable background pages, if one already exists, close it (even
    // if it was specified in the manifest).
    delete existing;
  }

  // Passed all the checks, so this should be created as a BackgroundContents.
  BackgroundContents* contents = nullptr;
  if (allow_js_access) {
    contents = service->CreateBackgroundContents(
        source_site_instance, opener, route_id, main_frame_route_id,
        main_frame_widget_route_id, profile_, frame_name, extension->id(),
        partition_id, session_storage_namespace);
  } else {
    // If script access is not allowed, create the the background contents in a
    // new SiteInstance, so that a separate process is used. We must not use any
    // of the passed-in routing IDs, as they are objects in the opener's
    // process.
    contents = service->CreateBackgroundContents(
        content::SiteInstance::Create(
            source_site_instance->GetBrowserContext()),
        nullptr, MSG_ROUTING_NONE, MSG_ROUTING_NONE, MSG_ROUTING_NONE, profile_,
        frame_name, extension->id(), partition_id, session_storage_namespace);

    // When a separate process is used, the original renderer cannot access the
    // new window later, thus we need to navigate the window now.
    if (contents) {
      contents->web_contents()->GetController().LoadURL(
          target_url, content::Referrer(), ui::PAGE_TRANSITION_LINK,
          std::string());  // No extra headers.
    }
  }

  return contents != NULL;
}
