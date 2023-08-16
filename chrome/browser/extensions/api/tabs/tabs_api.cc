// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/tabs/tabs_api.h"

#include <stddef.h>
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/pattern.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/api/tabs/windows_util.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/extensions/window_controller.h"
#include "chrome/browser/extensions/window_controller_list.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_external.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/ui/apps/chrome_app_delegate.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/browser/ui/window_sizer/window_sizer.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/api/tabs.h"
#include "chrome/common/extensions/api/windows.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/translate/core/browser/language_state.h"
#include "components/translate/core/common/language_detection_details.h"
#include "components/zoom/zoom_controller.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_zoom_request_client.h"
#include "extensions/browser/file_reader.h"
#include "extensions/browser/script_executor.h"
#include "extensions/common/api/extension_types.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/host_id.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/default_locale_handler.h"
#include "extensions/common/message_bundle.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/user_script.h"
#include "net/base/escape.h"
#include "skia/ext/image_operations.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/models/list_selection_model.h"
#include "ui/base/ui_base_types.h"

#if defined(OS_CHROMEOS)
#include "ash/public/cpp/config.h"
#include "ash/public/cpp/window_pin_type.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/public/interfaces/window_pin_type.mojom.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/ui/ash/chrome_screenshot_grabber.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "content/public/browser/devtools_agent_host.h"
#include "ui/aura/window.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_types.h"
#endif

#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "content/public/browser/web_contents.h"

using content::BrowserThread;
using content::NavigationController;
using content::NavigationEntry;
using content::OpenURLParams;
using content::Referrer;
using content::WebContents;
using zoom::ZoomController;

namespace extensions {

namespace windows = api::windows;
namespace keys = tabs_constants;
namespace tabs = api::tabs;

using api::extension_types::InjectDetails;

namespace {

template <typename T>
class ApiParameterExtractor {
 public:
  explicit ApiParameterExtractor(T* params) : params_(params) {}
  ~ApiParameterExtractor() {}

  bool populate_tabs() {
    if (params_->get_info.get() && params_->get_info->populate.get())
      return *params_->get_info->populate;
    return false;
  }

  WindowController::TypeFilter type_filters() {
    if (params_->get_info.get() && params_->get_info->window_types.get())
      return WindowController::GetFilterFromWindowTypes(
          *params_->get_info->window_types);
    return WindowController::kNoWindowFilter;
  }

 private:
  T* params_;
};

bool GetBrowserFromWindowID(const ChromeExtensionFunctionDetails& details,
                            int window_id,
                            Browser** browser,
                            std::string* error) {
  Browser* result = nullptr;
  result = ExtensionTabUtil::GetBrowserFromWindowID(details, window_id, error);
  if (!result)
    return false;

  *browser = result;
  return true;
}

bool GetBrowserFromWindowID(UIThreadExtensionFunction* function,
                            int window_id,
                            Browser** browser,
                            std::string* error) {
  return GetBrowserFromWindowID(ChromeExtensionFunctionDetails(function),
                                window_id, browser, error);
}

// |error_message| can optionally be passed in and will be set with an
// appropriate message if the tab cannot be found by id.
bool GetTabById(int tab_id,
                content::BrowserContext* context,
                bool include_incognito,
                Browser** browser,
                TabStripModel** tab_strip,
                content::WebContents** contents,
                int* tab_index,
                std::string* error_message) {
  if (ExtensionTabUtil::GetTabById(tab_id, context, include_incognito, browser,
                                   tab_strip, contents, tab_index)) {
    return true;
  }

  if (error_message) {
    *error_message = ErrorUtils::FormatErrorMessage(
        keys::kTabNotFoundError, base::IntToString(tab_id));
  }

  return false;
}

// Returns true if either |boolean| is a null pointer, or if |*boolean| and
// |value| are equal. This function is used to check if a tab's parameters match
// those of the browser.
bool MatchesBool(bool* boolean, bool value) {
  return !boolean || *boolean == value;
}

template <typename T>
void AssignOptionalValue(const std::unique_ptr<T>& source,
                         std::unique_ptr<T>& destination) {
  if (source.get()) {
    destination.reset(new T(*source));
  }
}

void ReportRequestedWindowState(windows::WindowState state) {
  UMA_HISTOGRAM_ENUMERATION("TabsApi.RequestedWindowState", state,
                            windows::WINDOW_STATE_LAST + 1);
}

ui::WindowShowState ConvertToWindowShowState(windows::WindowState state) {
  switch (state) {
    case windows::WINDOW_STATE_NORMAL:
    case windows::WINDOW_STATE_DOCKED:
      return ui::SHOW_STATE_NORMAL;
    case windows::WINDOW_STATE_MINIMIZED:
      return ui::SHOW_STATE_MINIMIZED;
    case windows::WINDOW_STATE_MAXIMIZED:
      return ui::SHOW_STATE_MAXIMIZED;
    case windows::WINDOW_STATE_FULLSCREEN:
    case windows::WINDOW_STATE_LOCKED_FULLSCREEN:
      return ui::SHOW_STATE_FULLSCREEN;
    case windows::WINDOW_STATE_NONE:
      return ui::SHOW_STATE_DEFAULT;
  }
  NOTREACHED();
  return ui::SHOW_STATE_DEFAULT;
}

bool IsValidStateForWindowsCreateFunction(
    const windows::Create::Params::CreateData* create_data) {
  if (!create_data)
    return true;

  bool has_bound = create_data->left || create_data->top ||
                   create_data->width || create_data->height;
  bool is_panel = create_data->type == windows::CreateType::CREATE_TYPE_PANEL;

  switch (create_data->state) {
    case windows::WINDOW_STATE_MINIMIZED:
      // If minimised, default focused state should be unfocused.
      return !(create_data->focused && *create_data->focused) && !has_bound &&
             !is_panel;
    case windows::WINDOW_STATE_MAXIMIZED:
    case windows::WINDOW_STATE_FULLSCREEN:
    case windows::WINDOW_STATE_LOCKED_FULLSCREEN:
      // If maximised/fullscreen, default focused state should be focused.
      return !(create_data->focused && !*create_data->focused) && !has_bound &&
             !is_panel;
    case windows::WINDOW_STATE_NORMAL:
    case windows::WINDOW_STATE_DOCKED:
    case windows::WINDOW_STATE_NONE:
      return true;
  }
  NOTREACHED();
  return true;
}

bool ExtensionHasLockedFullscreenPermission(const Extension* extension) {
  return extension->permissions_data()->HasAPIPermission(
      APIPermission::kLockWindowFullscreenPrivate);
}

#if defined(OS_CHROMEOS)
void SetLockedFullscreenState(Browser* browser, bool locked) {
  aura::Window* window = browser->window()->GetNativeWindow();
  // TRUSTED_PINNED is used here because that one locks the window fullscreen
  // without allowing the user to exit (as opposed to regular PINNED).
  window->SetProperty(ash::kWindowPinTypeKey,
                      locked ? ash::mojom::WindowPinType::TRUSTED_PINNED
                             : ash::mojom::WindowPinType::NONE);

  // Update the set of available browser commands.
  browser->command_controller()->LockedFullscreenStateChanged();

  // Disallow screenshots in locked fullscreen mode.
  // TODO(isandrk, 816900): ChromeScreenshotGrabber isn't implemented in Mash
  // yet, remove this conditional when it becomes available.
  if (chromeos::GetAshConfig() != ash::Config::MASH)
    ChromeScreenshotGrabber::Get()->set_screenshots_allowed(!locked);

  // Reset the clipboard and kill dev tools when entering or exiting locked
  // fullscreen (security concerns).
  ui::Clipboard::GetForCurrentThread()->Clear(ui::CLIPBOARD_TYPE_COPY_PASTE);
  content::DevToolsAgentHost::DetachAllClients();
}

#endif  // defined(OS_CHROMEOS)

}  // namespace

void ZoomModeToZoomSettings(ZoomController::ZoomMode zoom_mode,
                            api::tabs::ZoomSettings* zoom_settings) {
  DCHECK(zoom_settings);
  switch (zoom_mode) {
    case ZoomController::ZOOM_MODE_DEFAULT:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_AUTOMATIC;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_ORIGIN;
      break;
    case ZoomController::ZOOM_MODE_ISOLATED:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_AUTOMATIC;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
      break;
    case ZoomController::ZOOM_MODE_MANUAL:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_MANUAL;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
      break;
    case ZoomController::ZOOM_MODE_DISABLED:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_DISABLED;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
      break;
  }
}

// Windows ---------------------------------------------------------------------

ExtensionFunction::ResponseAction WindowsGetFunction::Run() {
  std::unique_ptr<windows::Get::Params> params(
      windows::Get::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(INFO) << "[EXTENSIONS] WindowsGetFunction - Step 1";
  ApiParameterExtractor<windows::Get::Params> extractor(params.get());
  Browser* browser = nullptr;
  std::string error;
  if (!windows_util::GetBrowserFromWindowID(this, params->window_id,
                                            extractor.type_filters(), &browser,
                                            &error)) {
    LOG(INFO) << "[EXTENSIONS] WindowsGetFunction - Step 1a";
    if (!browser) {
      Profile* profile = Profile::FromBrowserContext(browser_context());
      browser = new Browser(Browser::CreateParams(profile, true));
    }
  }

  LOG(INFO) << "[EXTENSIONS] WindowsGetFunction - Step 2";

  ExtensionTabUtil::PopulateTabBehavior populate_tab_behavior =
      extractor.populate_tabs() ? ExtensionTabUtil::kPopulateTabs
                                : ExtensionTabUtil::kDontPopulateTabs;
  std::unique_ptr<base::DictionaryValue> windows =
      ExtensionTabUtil::CreateWindowValueForExtension(*browser, extension(),
                                                      populate_tab_behavior);
  LOG(INFO) << "[EXTENSIONS] WindowsGetFunction - Step 3";
  return RespondNow(OneArgument(std::move(windows)));
}

ExtensionFunction::ResponseAction WindowsGetCurrentFunction::Run() {
  std::unique_ptr<windows::GetCurrent::Params> params(
      windows::GetCurrent::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(INFO) << "[EXTENSIONS] WindowsGetCurrentFunction - Step 1";
  ApiParameterExtractor<windows::GetCurrent::Params> extractor(params.get());
  Browser* browser = nullptr;
  std::string error;
  if (!windows_util::GetBrowserFromWindowID(
          this, extension_misc::kCurrentWindowId, extractor.type_filters(),
          &browser, &error)) {
    LOG(INFO) << "[EXTENSIONS] WindowsGetCurrentFunction - Step 1a - Window not found";
    if (!browser) {
      Profile* profile = Profile::FromBrowserContext(browser_context());
      browser = new Browser(Browser::CreateParams(profile, true));
    }
  }

  LOG(INFO) << "[EXTENSIONS] WindowsGetCurrentFunction - Step 2";

  ExtensionTabUtil::PopulateTabBehavior populate_tab_behavior =
      extractor.populate_tabs() ? ExtensionTabUtil::kPopulateTabs
                                : ExtensionTabUtil::kDontPopulateTabs;
  LOG(INFO) << "[EXTENSIONS] WindowsGetCurrentFunction - Step 3";
  std::unique_ptr<base::DictionaryValue> windows =
      ExtensionTabUtil::CreateWindowValueForExtension(*browser, extension(),
                                                      populate_tab_behavior);
  return RespondNow(OneArgument(std::move(windows)));
}

ExtensionFunction::ResponseAction WindowsGetLastFocusedFunction::Run() {
  std::unique_ptr<windows::GetLastFocused::Params> params(
      windows::GetLastFocused::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  ApiParameterExtractor<windows::GetLastFocused::Params> extractor(
      params.get());
  // The WindowControllerList should contain a list of application,
  // browser and devtools windows.
  Browser* browser = nullptr;
  for (auto* controller : WindowControllerList::GetInstance()->windows()) {
    if (controller->GetBrowser() &&
        windows_util::CanOperateOnWindow(this, controller,
                                         extractor.type_filters())) {
      // TODO(devlin): Doesn't this mean that we'll use the last window in the
      // list if there is no active window? That seems wrong.
      // See https://crbug.com/809822.
      browser = controller->GetBrowser();
      if (controller->window()->IsActive())
        break;  // Use focused window.
    }
  }
  if (!browser) {
    Profile* profile = Profile::FromBrowserContext(browser_context());
    browser = new Browser(Browser::CreateParams(profile, true));
  }

  ExtensionTabUtil::PopulateTabBehavior populate_tab_behavior =
      extractor.populate_tabs() ? ExtensionTabUtil::kPopulateTabs
                                : ExtensionTabUtil::kDontPopulateTabs;
  std::unique_ptr<base::DictionaryValue> windows =
      ExtensionTabUtil::CreateWindowValueForExtension(*browser, extension(),
                                                      populate_tab_behavior);
  return RespondNow(OneArgument(std::move(windows)));
}

ExtensionFunction::ResponseAction WindowsGetAllFunction::Run() {
  std::unique_ptr<windows::GetAll::Params> params(
      windows::GetAll::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(INFO) << "[EXTENSIONS] Called chrome.windows.getAll() - Step 1";

  ApiParameterExtractor<windows::GetAll::Params> extractor(params.get());
  Browser* browser = nullptr;
  std::string error;
  if (!windows_util::GetBrowserFromWindowID(
          this, extension_misc::kCurrentWindowId, extractor.type_filters(),
          &browser, &error)) {
    LOG(INFO) << "[EXTENSIONS] WindowsGetCurrentFunction - Step 1a - Window not found";
    if (!browser) {
      Profile* profile = Profile::FromBrowserContext(browser_context());
      browser = new Browser(Browser::CreateParams(profile, true));
    }
  }

  LOG(INFO) << "[EXTENSIONS] Called chrome.windows.getAll() - Step 2";
  std::unique_ptr<base::ListValue> window_list(new base::ListValue());
  ExtensionTabUtil::PopulateTabBehavior populate_tab_behavior =
      extractor.populate_tabs() ? ExtensionTabUtil::kPopulateTabs
                                : ExtensionTabUtil::kDontPopulateTabs;

  LOG(INFO) << "[EXTENSIONS] Called chrome.windows.getAll() - Step 3";
  window_list->Append(ExtensionTabUtil::CreateWindowValueForExtension(
      *browser, extension(), populate_tab_behavior));
  LOG(INFO) << "[EXTENSIONS] Called chrome.windows.getAll() - Step 4";
  return RespondNow(OneArgument(std::move(window_list)));
}

bool WindowsCreateFunction::ShouldOpenIncognitoWindow(
    const windows::Create::Params::CreateData* create_data,
    std::vector<GURL>* urls,
    std::string* error) {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  const IncognitoModePrefs::Availability incognito_availability =
      IncognitoModePrefs::GetAvailability(profile->GetPrefs());
  bool incognito = false;
  if (create_data && create_data->incognito) {
    incognito = *create_data->incognito;
    if (incognito && incognito_availability == IncognitoModePrefs::DISABLED) {
      *error = keys::kIncognitoModeIsDisabled;
      return false;
    }
    if (!incognito && incognito_availability == IncognitoModePrefs::FORCED) {
      *error = keys::kIncognitoModeIsForced;
      return false;
    }
  } else if (incognito_availability == IncognitoModePrefs::FORCED) {
    // If incognito argument is not specified explicitly, we default to
    // incognito when forced so by policy.
    incognito = true;
  }

  // Remove all URLs that are not allowed in an incognito session. Note that a
  // ChromeOS guest session is not considered incognito in this case.
  if (incognito && !profile->IsGuestSession()) {
    std::string first_url_erased;
    for (size_t i = 0; i < urls->size();) {
      if (IsURLAllowedInIncognito((*urls)[i], profile)) {
        i++;
      } else {
        if (first_url_erased.empty())
          first_url_erased = (*urls)[i].spec();
        urls->erase(urls->begin() + i);
      }
    }
    if (urls->empty() && !first_url_erased.empty()) {
      *error = ErrorUtils::FormatErrorMessage(
          keys::kURLsNotAllowedInIncognitoError, first_url_erased);
      return false;
    }
  }
  return incognito;
}

ExtensionFunction::ResponseAction WindowsCreateFunction::Run() {
  std::unique_ptr<windows::Create::Params> params(
      windows::Create::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);
  std::vector<GURL> urls;
  TabStripModel* source_tab_strip = NULL;
  int tab_index = -1;

  windows::Create::Params::CreateData* create_data = params->create_data.get();

  // Look for optional url.
  if (create_data && create_data->url) {
    std::vector<std::string> url_strings;
    // First, get all the URLs the client wants to open.
    if (create_data->url->as_string)
      url_strings.push_back(*create_data->url->as_string);
    else if (create_data->url->as_strings)
      url_strings.swap(*create_data->url->as_strings);

    // Second, resolve, validate and convert them to GURLs.
    for (std::vector<std::string>::iterator i = url_strings.begin();
         i != url_strings.end(); ++i) {
      GURL url = ExtensionTabUtil::ResolvePossiblyRelativeURL(*i, extension());
      if (!url.is_valid())
        return RespondNow(Error(keys::kInvalidUrlError, *i));
      // Don't let the extension crash the browser or renderers.
      if (ExtensionTabUtil::IsKillURL(url))
        return RespondNow(Error(keys::kNoCrashBrowserError));
      urls.push_back(url);
    }
  }

  // Decide whether we are opening a normal window or an incognito window.
  std::string error;
  bool open_incognito_window =
      ShouldOpenIncognitoWindow(create_data, &urls, &error);
  if (!error.empty())
    return RespondNow(Error(error));

  Profile* calling_profile = Profile::FromBrowserContext(browser_context());
  Profile* window_profile = open_incognito_window
                                ? calling_profile->GetOffTheRecordProfile()
                                : calling_profile;

  // Look for optional tab id.
  if (create_data && create_data->tab_id) {
    // Find the tab. |source_tab_strip| and |tab_index| will later be used to
    // move the tab into the created window.
    Browser* source_browser = nullptr;
    if (!GetTabById(*create_data->tab_id, calling_profile, include_incognito(),
                    &source_browser, &source_tab_strip, nullptr, &tab_index,
                    &error)) {
      return RespondNow(Error(error));
    }
  }

  if (!IsValidStateForWindowsCreateFunction(create_data))
    return RespondNow(Error(keys::kInvalidWindowStateError));

  Browser::Type window_type = Browser::TYPE_TABBED;

  gfx::Rect window_bounds;
  bool focused = true;
  std::string extension_id;

  if (create_data) {
    // Report UMA stats to decide when to remove the deprecated "docked" windows
    // state (crbug.com/703733).
    ReportRequestedWindowState(create_data->state);

    // Figure out window type before figuring out bounds so that default
    // bounds can be set according to the window type.
    switch (create_data->type) {
      case windows::CREATE_TYPE_POPUP:
        window_type = Browser::TYPE_POPUP;
        extension_id = extension()->id();
        break;

      case windows::CREATE_TYPE_PANEL: {
        extension_id = extension()->id();
        // TODO(dimich): Eventually, remove the 'panel' values form valid
        // window.create parameters. However, this is a more breaking change, so
        // for now simply treat it as a POPUP.
        window_type = Browser::TYPE_POPUP;
        break;
      }

      case windows::CREATE_TYPE_NONE:
      case windows::CREATE_TYPE_NORMAL:
        break;
      default:
        return RespondNow(Error(keys::kInvalidWindowTypeError));
    }

    // Initialize default window bounds according to window type.
    if (window_type == Browser::TYPE_TABBED ||
        window_type == Browser::TYPE_POPUP) {
      ui::WindowShowState ignored_show_state = ui::SHOW_STATE_DEFAULT;
      WindowSizer::GetBrowserWindowBoundsAndShowState(
          std::string(), gfx::Rect(), nullptr, &window_bounds,
          &ignored_show_state);
    }

    // Any part of the bounds can optionally be set by the caller.
    if (create_data->left)
      window_bounds.set_x(*create_data->left);

    if (create_data->top)
      window_bounds.set_y(*create_data->top);

    if (create_data->width)
      window_bounds.set_width(*create_data->width);

    if (create_data->height)
      window_bounds.set_height(*create_data->height);

    if (create_data->focused)
      focused = *create_data->focused;
  }

  // Create a new BrowserWindow.
  Browser::CreateParams create_params(window_type, window_profile,
                                      user_gesture());
  if (extension_id.empty()) {
    create_params.initial_bounds = window_bounds;
  } else {
    create_params = Browser::CreateParams::CreateForApp(
        web_app::GenerateApplicationNameFromExtensionId(extension_id),
        false /* trusted_source */, window_bounds, window_profile,
        user_gesture());
  }
  create_params.initial_show_state = ui::SHOW_STATE_NORMAL;
  if (create_data && create_data->state) {
    if (create_data->state == windows::WINDOW_STATE_LOCKED_FULLSCREEN &&
        !ExtensionHasLockedFullscreenPermission(extension())) {
      return RespondNow(
          Error(keys::kMissingLockWindowFullscreenPrivatePermission));
    }
    create_params.initial_show_state =
        ConvertToWindowShowState(create_data->state);
  }

  Browser* new_window = new Browser(create_params);

  for (const GURL& url : urls) {
    NavigateParams navigate_params(new_window, url, ui::PAGE_TRANSITION_LINK);
    navigate_params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;

    // Depending on the |setSelfAsOpener| option, we need to put the new
    // contents in the same BrowsingInstance as their opener.  See also
    // https://crbug.com/713888.
    bool set_self_as_opener = create_data->set_self_as_opener &&  // present?
                              *create_data->set_self_as_opener;  // set to true?
    navigate_params.opener = set_self_as_opener ? render_frame_host() : nullptr;
    navigate_params.source_site_instance =
        render_frame_host()->GetSiteInstance();

    Navigate(&navigate_params);
  }

  WebContents* contents = NULL;
  // Move the tab into the created window only if it's an empty popup or it's
  // a tabbed window.
  if ((window_type == Browser::TYPE_POPUP && urls.empty()) ||
      window_type == Browser::TYPE_TABBED) {
    if (source_tab_strip) {
      std::unique_ptr<content::WebContents> detached_tab =
          source_tab_strip->DetachWebContentsAt(tab_index);
      contents = detached_tab.get();
      TabStripModel* target_tab_strip = new_window->tab_strip_model();
      target_tab_strip->InsertWebContentsAt(
          urls.size(), std::move(detached_tab), TabStripModel::ADD_NONE);
    }
  }
  // Create a new tab if the created window is still empty. Don't create a new
  // tab when it is intended to create an empty popup.
  if (!contents && urls.empty() && window_type != Browser::TYPE_POPUP) {
    chrome::NewTab(new_window);
  }
  chrome::SelectNumberedTab(new_window, 0);

#if defined(OS_CHROMEOS)
  // Lock the window fullscreen only after the new tab has been created
  // (otherwise the tabstrip is empty).
  if (create_data &&
      create_data->state == windows::WINDOW_STATE_LOCKED_FULLSCREEN) {
    SetLockedFullscreenState(new_window, true);
  }
#endif

  if (focused)
    new_window->window()->Show();
  else
    new_window->window()->ShowInactive();

  std::unique_ptr<base::Value> result;
  if (new_window->profile()->IsOffTheRecord() &&
      !browser_context()->IsOffTheRecord() && !include_incognito()) {
    // Don't expose incognito windows if extension itself works in non-incognito
    // profile and CanCrossIncognito isn't allowed.
    result = std::make_unique<base::Value>();
  } else {
    result = ExtensionTabUtil::CreateWindowValueForExtension(
        *new_window, extension(), ExtensionTabUtil::kPopulateTabs);
  }

  return RespondNow(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction WindowsUpdateFunction::Run() {
  std::unique_ptr<windows::Update::Params> params(
      windows::Update::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction WindowsRemoveFunction::Run() {
  std::unique_ptr<windows::Remove::Params> params(
      windows::Remove::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);
  return RespondNow(Error(kUnknownErrorDoNotUse));
}

// Tabs ------------------------------------------------------------------------

ExtensionFunction::ResponseAction TabsGetSelectedFunction::Run() {
  std::unique_ptr<tabs::GetSelected::Params> params(
      tabs::GetSelected::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());



  std::unique_ptr<base::ListValue> result(new base::ListValue());
  TabModel *tab_strip = nullptr;
  if (!TabModelList::empty())
    tab_strip = *(TabModelList::begin());
  if (tab_strip) {
    for (int i = 0; i < tab_strip->GetTabCount(); ++i) {
      WebContents* web_contents = tab_strip->GetWebContentsAt(i);

      int openingTab = (tab_strip->GetLastNonExtensionActiveIndex());
      if (extension() && extension()->id() == "mooikfkahbdckldjjndioackbalphokd")
        openingTab = (tab_strip->GetActiveIndex());
      if (openingTab == -1)
        openingTab = 0;

      if (i != openingTab)
        continue;

      if (!web_contents) {
        continue;
      }


      return RespondNow(ArgumentList(
          tabs::Get::Results::Create(*ExtensionTabUtil::CreateTabObject(
              web_contents, ExtensionTabUtil::kScrubTab, extension(), NULL,
              i))));

    }
  }
  return RespondNow(Error(keys::kNoSelectedTabError));
}

ExtensionFunction::ResponseAction TabsGetAllInWindowFunction::Run() {
  std::unique_ptr<tabs::GetAllInWindow::Params> params(
      tabs::GetAllInWindow::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - Step 1";

  std::unique_ptr<base::ListValue> result(new base::ListValue());
  LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - Step 2";
  Profile* profile = Profile::FromBrowserContext(browser_context());
  Browser* last_active_browser =
      chrome::FindAnyBrowser(profile, include_incognito());
  Browser* current_browser =
      ChromeExtensionFunctionDetails(this).GetCurrentBrowser();
  LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - Step 3: " << profile;
  LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - Step 3-1: " << last_active_browser;
  LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - Step 3-2: " << current_browser;
  LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - IsOffTheRecordSessionActive: " << TabModelList::IsOffTheRecordSessionActive();
  TabModel *tab_strip = nullptr;
  if (!TabModelList::empty())
    tab_strip = *(TabModelList::begin());
  LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - TabModel: " << tab_strip;
  if (tab_strip) {
    LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - TabModel - Open tabs: " << tab_strip->GetTabCount();
    LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - TabModel - Active index: " << tab_strip->GetActiveIndex();
    LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - TabModel - Last user-interacted active index: " << tab_strip->GetLastNonExtensionActiveIndex();
    for (int i = 0; i < tab_strip->GetTabCount(); ++i) {
      WebContents* web_contents = tab_strip->GetWebContentsAt(i);
      LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - Step 4d-1 (tab loop)";

      if (!web_contents) {
        LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - Step 4d-1b (there is no webcontents)";
        continue;
      }

      LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - Step 4d-2";

      result->Append(ExtensionTabUtil::CreateTabObject(
                         web_contents, ExtensionTabUtil::kScrubTab, extension(),
                         NULL, i)
                         ->ToValue());
    }
  }
  LOG(INFO) << "[EXTENSIONS] TabsGetAllInWindowFunction::Run - Step 6";

  return RespondNow(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction TabsQueryFunction::Run() {
  std::unique_ptr<tabs::Query::Params> params(
      tabs::Query::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  bool loading_status_set = params->query_info.status != tabs::TAB_STATUS_NONE;
  bool loading = params->query_info.status == tabs::TAB_STATUS_LOADING;

  URLPatternSet url_patterns;
  if (params->query_info.url.get()) {
    std::vector<std::string> url_pattern_strings;
    if (params->query_info.url->as_string)
      url_pattern_strings.push_back(*params->query_info.url->as_string);
    else if (params->query_info.url->as_strings)
      url_pattern_strings.swap(*params->query_info.url->as_strings);
    // It is o.k. to use URLPattern::SCHEME_ALL here because this function does
    // not grant access to the content of the tabs, only to seeing their URLs
    // and meta data.
    std::string error;
    if (!url_patterns.Populate(url_pattern_strings, URLPattern::SCHEME_ALL,
                               true, &error)) {
      return RespondNow(Error(error));
    }
  }

//  LOG(INFO) << "[EXTENSIONS] TabsQueryFunction::Run - Step 3";

  std::string title;
  if (params->query_info.title.get())
    title = *params->query_info.title;

  int window_id = extension_misc::kUnknownWindowId;
  if (params->query_info.window_id.get())
    window_id = *params->query_info.window_id;

  int index = -1;
  if (params->query_info.index.get())
    index = *params->query_info.index;

  std::string window_type;
  if (params->query_info.window_type != tabs::WINDOW_TYPE_NONE)
    window_type = tabs::ToString(params->query_info.window_type);

  std::unique_ptr<base::ListValue> result(new base::ListValue());
  TabModel *tab_strip = nullptr;
  if (!TabModelList::empty())
    tab_strip = *(TabModelList::begin());
  if (tab_strip) {
    for (int i = 0; i < tab_strip->GetTabCount(); ++i) {
      WebContents* web_contents = tab_strip->GetWebContentsAt(i);

      if (index > -1 && i != index) {
        continue;
      }

      int openingTab = (tab_strip->GetLastNonExtensionActiveIndex());
      // To Selenium IDE we show the current active window even if it's an extension window
      // when they try to access a specific window because it means
      // that it Selenium is looking for its own IDE
       if (extension() && extension()->id() == "mooikfkahbdckldjjndioackbalphokd")
         openingTab = (tab_strip->GetActiveIndex());
       if (openingTab == -1)
        openingTab = 0;

      // For Selenium IDE we do not check the active flag
      if (extension() && extension()->id() != "mooikfkahbdckldjjndioackbalphokd") {
        if (!MatchesBool(params->query_info.active.get(),
                         i == openingTab)) {
          continue;
        }
      }
      // except if there is status == complete
      if (extension() && extension()->id() == "mooikfkahbdckldjjndioackbalphokd" && params->query_info.status == tabs::TAB_STATUS_COMPLETE) {
        if (!MatchesBool(params->query_info.active.get(),
                         i == openingTab)) {
          continue;
        }
      }

      if (extension() && extension()->id() == "mooikfkahbdckldjjndioackbalphokd") {
        if (window_id >= 0 && window_id != SessionTabHelper::IdForTab(web_contents).id())
          continue;
      }

      if (!web_contents) {
//        LOG(INFO) << "[EXTENSIONS] TabsQueryFunction::Run - Step 5d-1b (there is no webcontents)";
        continue;
      }

//      LOG(INFO) << "[EXTENSIONS] TabsQueryFunction::Run - Step 5d-2";

      if (!title.empty() || !url_patterns.is_empty()) {
        // "title" and "url" properties are considered privileged data and can
        // only be checked if the extension has the "tabs" permission or it has
        // access to the WebContents's origin. Otherwise, this tab is considered
        // not matched.
        if (!extension_->permissions_data()->HasAPIPermissionForTab(
                ExtensionTabUtil::GetTabId(web_contents),
                APIPermission::kTab) &&
            !extension_->permissions_data()->HasHostPermission(
                web_contents->GetURL())) {
          continue;
        }

        if (!title.empty() &&
            !base::MatchPattern(web_contents->GetTitle(),
                                base::UTF8ToUTF16(title))) {
          continue;
        }

        if (!url_patterns.is_empty() &&
            !url_patterns.MatchesURL(web_contents->GetURL())) {
          continue;
        }
      }

//      LOG(INFO) << "[EXTENSIONS] TabsQueryFunction::Run - Step 5d-4";

      if (loading_status_set && loading != web_contents->IsLoading())
        continue;

//      LOG(INFO) << "[EXTENSIONS] TabsQueryFunction::Run - Step 5d-5";

      result->Append(ExtensionTabUtil::CreateTabObject(
                         web_contents, ExtensionTabUtil::kScrubTab, extension(),
                         NULL, i)
                         ->ToValue());
    }
  }
//  LOG(INFO) << "[EXTENSIONS] TabsQueryFunction::Run - Step 6";

  return RespondNow(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction TabsCreateFunction::Run() {
  std::unique_ptr<tabs::Create::Params> params(
      tabs::Create::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(INFO) << "[EXTENSIONS] Called chrome.tabs.create()";

  ExtensionTabUtil::OpenTabParams options;
  AssignOptionalValue(params->create_properties.window_id, options.window_id);
  AssignOptionalValue(params->create_properties.opener_tab_id,
                      options.opener_tab_id);
  AssignOptionalValue(params->create_properties.selected, options.active);
  // The 'active' property has replaced the 'selected' property.
  AssignOptionalValue(params->create_properties.active, options.active);
  AssignOptionalValue(params->create_properties.pinned, options.pinned);
  AssignOptionalValue(params->create_properties.index, options.index);
  AssignOptionalValue(params->create_properties.url, options.url);

  std::string error;
  std::unique_ptr<base::DictionaryValue> result(
      ExtensionTabUtil::OpenTab(this, options, user_gesture(), &error));
  if (!result)
    return RespondNow(Error(error));

  // Return data about the newly created tab.
  return RespondNow(has_callback() ? OneArgument(std::move(result))
                                   : NoArguments());
}

ExtensionFunction::ResponseAction TabsDuplicateFunction::Run() {
  std::unique_ptr<tabs::Duplicate::Params> params(
      tabs::Duplicate::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  int tab_id = params->tab_id;

  Browser* browser = NULL;
  TabStripModel* tab_strip = NULL;
  int tab_index = -1;
  std::string error;
  if (!GetTabById(tab_id, browser_context(), include_incognito(), &browser,
                  &tab_strip, NULL, &tab_index, &error)) {
    return RespondNow(Error(error));
  }

  WebContents* new_contents = chrome::DuplicateTabAt(browser, tab_index);
  if (!has_callback())
    return RespondNow(NoArguments());

  // Duplicated tab may not be in the same window as the original, so find
  // the window and the tab.
  TabStripModel* new_tab_strip = NULL;
  int new_tab_index = -1;
  ExtensionTabUtil::GetTabStripModel(new_contents,
                                     &new_tab_strip,
                                     &new_tab_index);
  if (!new_tab_strip || new_tab_index == -1) {
    return RespondNow(Error(kUnknownErrorDoNotUse));
  }

  return RespondNow(ArgumentList(
      tabs::Get::Results::Create(*ExtensionTabUtil::CreateTabObject(
          new_contents, ExtensionTabUtil::kScrubTab, extension(), new_tab_strip,
          new_tab_index))));
}

ExtensionFunction::ResponseAction TabsGetFunction::Run() {
  std::unique_ptr<tabs::Get::Params> params(tabs::Get::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  int tab_id = params->tab_id;

  TabStripModel* tab_strip = NULL;
  WebContents* contents = NULL;
  int tab_index = -1;
  std::string error;
  if (!GetTabById(tab_id, browser_context(), include_incognito(), NULL,
                  &tab_strip, &contents, &tab_index, &error)) {
    return RespondNow(Error(error));
  }

  return RespondNow(ArgumentList(tabs::Get::Results::Create(
      *ExtensionTabUtil::CreateTabObject(contents, ExtensionTabUtil::kScrubTab,
                                         extension(), tab_strip, tab_index))));
}

ExtensionFunction::ResponseAction TabsGetCurrentFunction::Run() {
  DCHECK(dispatcher());

  LOG(INFO) << "[EXTENSIONS] TabsGetCurrentFunction - Step 1";

  // Return the caller, if it's a tab. If not the result isn't an error but an
  // empty tab (hence returning true).
  WebContents* caller_contents = GetSenderWebContents();
  std::unique_ptr<base::ListValue> results;
  LOG(INFO) << "[EXTENSIONS] TabsGetCurrentFunction - Step 2";
  if (caller_contents && ExtensionTabUtil::GetTabId(caller_contents) >= 0) {
    LOG(INFO) << "[EXTENSIONS] TabsGetCurrentFunction - Step 3";
    results = tabs::Get::Results::Create(*ExtensionTabUtil::CreateTabObject(
        caller_contents, ExtensionTabUtil::kScrubTab, extension()));
  }
  if (results)
    LOG(INFO) << "[EXTENSIONS] TabsGetCurrentFunction - Step 4 has results";
  else
    LOG(INFO) << "[EXTENSIONS] TabsGetCurrentFunction - Step 4 has no results";
  return RespondNow(results ? ArgumentList(std::move(results)) : NoArguments());
}

ExtensionFunction::ResponseAction TabsHighlightFunction::Run() {
  std::unique_ptr<tabs::Highlight::Params> params(
      tabs::Highlight::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Get the window id from the params; default to current window if omitted.
  int window_id = extension_misc::kCurrentWindowId;
  if (params->highlight_info.window_id.get())
    window_id = *params->highlight_info.window_id;

  Browser* browser = NULL;
  std::string error;
  if (!GetBrowserFromWindowID(this, window_id, &browser, &error))
    return RespondNow(Error(error));

  TabStripModel* tabstrip = browser->tab_strip_model();
  ui::ListSelectionModel selection;
  int active_index = -1;

  if (params->highlight_info.tabs.as_integers) {
    std::vector<int>& tab_indices = *params->highlight_info.tabs.as_integers;
    // Create a new selection model as we read the list of tab indices.
    for (size_t i = 0; i < tab_indices.size(); ++i) {
      if (!HighlightTab(tabstrip, &selection, &active_index, tab_indices[i],
                        &error)) {
        return RespondNow(Error(error));
      }
    }
  } else {
    EXTENSION_FUNCTION_VALIDATE(params->highlight_info.tabs.as_integer);
    if (!HighlightTab(tabstrip, &selection, &active_index,
                      *params->highlight_info.tabs.as_integer, &error)) {
      return RespondNow(Error(error));
    }
  }

  // Make sure they actually specified tabs to select.
  if (selection.empty())
    return RespondNow(Error(keys::kNoHighlightedTabError));

  selection.set_active(active_index);
  browser->tab_strip_model()->SetSelectionFromModel(std::move(selection));
  return RespondNow(OneArgument(ExtensionTabUtil::CreateWindowValueForExtension(
      *browser, extension(), ExtensionTabUtil::kPopulateTabs)));
}

bool TabsHighlightFunction::HighlightTab(TabStripModel* tabstrip,
                                         ui::ListSelectionModel* selection,
                                         int* active_index,
                                         int index,
                                         std::string* error) {
  // Make sure the index is in range.
  if (!tabstrip->ContainsIndex(index)) {
    *error = ErrorUtils::FormatErrorMessage(keys::kTabIndexNotFoundError,
                                            base::IntToString(index));
    return false;
  }

  // By default, we make the first tab in the list active.
  if (*active_index == -1)
    *active_index = index;

  selection->AddIndexToSelection(index);
  return true;
}

TabsUpdateFunction::TabsUpdateFunction() : web_contents_(NULL) {
}

bool TabsUpdateFunction::RunAsync() {
  std::unique_ptr<tabs::Update::Params> params(
      tabs::Update::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  int tab_id = -1;
  WebContents* contents = NULL;
  TabModel *tab_strip = nullptr;
  if (!params->tab_id.get()) {
    if (!TabModelList::empty())
      tab_strip = *(TabModelList::begin());
    if (tab_strip) {
       for (int i = 0; i < tab_strip->GetTabCount(); ++i) {
          int openingTab = (tab_strip->GetLastNonExtensionActiveIndex());
          if (extension() && extension()->id() == "mooikfkahbdckldjjndioackbalphokd")
            openingTab = (tab_strip->GetActiveIndex());
          if (openingTab == -1)
            openingTab = 0;

          if (i != openingTab)
            continue;

          contents = tab_strip->GetWebContentsAt(i);
       }
    }
    if (!contents) {
      error_ = keys::kNoSelectedTabError;
      return false;
    }
    tab_id = SessionTabHelper::IdForTab(contents).id();
  } else {
    tab_id = *params->tab_id;
  }

  int tab_index = -1;
  Browser* browser = nullptr;
  if (!GetTabById(tab_id, browser_context(), include_incognito(), &browser,
                  nullptr, &contents, &tab_index, &error_)) {
    return false;
  }

  web_contents_ = contents;

  // TODO(rafaelw): handle setting remaining tab properties:
  // -title
  // -favIconUrl
  // Navigate the tab to a new location if the url is different.
  bool is_async = false;
  if (params->update_properties.url.get()) {
    std::string updated_url = *params->update_properties.url;
    if (!UpdateURL(updated_url, tab_id, &is_async))
      return false;
  }

  bool active = false;
  // TODO(rafaelw): Setting |active| from js doesn't make much sense.
  // Move tab selection management up to window.
  if (params->update_properties.selected.get())
    active = *params->update_properties.selected;

  // The 'active' property has replaced 'selected'.
  if (params->update_properties.active.get())
    active = *params->update_properties.active;

  if (params->update_properties.muted.get()) {
    TabMutedResult tab_muted_result = chrome::SetTabAudioMuted(
        contents, *params->update_properties.muted,
        TabMutedReason::EXTENSION, extension()->id());

    switch (tab_muted_result) {
      case TabMutedResult::SUCCESS:
        break;
      case TabMutedResult::FAIL_NOT_ENABLED:
        error_ = ErrorUtils::FormatErrorMessage(
            keys::kCannotUpdateMuteDisabled, base::IntToString(tab_id),
            switches::kEnableTabAudioMuting);
        return false;
      case TabMutedResult::FAIL_TABCAPTURE:
        error_ = ErrorUtils::FormatErrorMessage(keys::kCannotUpdateMuteCaptured,
                                                base::IntToString(tab_id));
        return false;
    }
  }

  if (!is_async) {
    PopulateResult();
    SendResponse(true);
  }
  return true;
}

bool TabsUpdateFunction::UpdateURL(const std::string &url_string,
                                   int tab_id,
                                   bool* is_async) {
  GURL url =
      ExtensionTabUtil::ResolvePossiblyRelativeURL(url_string, extension());

  if (!url.is_valid()) {
    error_ = ErrorUtils::FormatErrorMessage(
        keys::kInvalidUrlError, url_string);
    return false;
  }

  // Don't let the extension crash the browser or renderers.
  if (ExtensionTabUtil::IsKillURL(url)) {
    error_ = keys::kNoCrashBrowserError;
    return false;
  }

  const bool is_javascript_scheme = url.SchemeIs(url::kJavaScriptScheme);
  UMA_HISTOGRAM_BOOLEAN("Extensions.ApiTabUpdateJavascript",
                        is_javascript_scheme);
  // JavaScript URLs can do the same kinds of things as cross-origin XHR, so
  // we need to check host permissions before allowing them.
  if (is_javascript_scheme) {
    if (!extension()->permissions_data()->CanAccessPage(
            web_contents_->GetURL(),
            tab_id,
            &error_)) {
      return false;
    }

    TabHelper::FromWebContents(web_contents_)
        ->script_executor()
        ->ExecuteScript(
            HostID(HostID::EXTENSIONS, extension_id()),
            ScriptExecutor::JAVASCRIPT,
            net::UnescapeURLComponent(
                url.GetContent(),
                net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS |
                    net::UnescapeRule::PATH_SEPARATORS |
                    net::UnescapeRule::SPACES),
            ScriptExecutor::SINGLE_FRAME, ExtensionApiFrameIdMap::kTopFrameId,
            ScriptExecutor::DONT_MATCH_ABOUT_BLANK, UserScript::DOCUMENT_IDLE,
            ScriptExecutor::MAIN_WORLD, ScriptExecutor::DEFAULT_PROCESS, GURL(),
            GURL(), user_gesture(), base::nullopt, ScriptExecutor::NO_RESULT,
            base::Bind(&TabsUpdateFunction::OnExecuteCodeFinished, this));

    *is_async = true;
    return true;
  }

  bool use_renderer_initiated = false;
  // For the PDF extension, treat it as renderer-initiated so that it does not
  // show in the omnibox until it commits.  This avoids URL spoofs since urls
  // can be opened on behalf of untrusted content.
  // TODO(devlin|nasko): Make this the default for all extensions.
  if (extension() && extension()->id() == extension_misc::kPdfExtensionId)
    use_renderer_initiated = true;
  NavigationController::LoadURLParams load_params(url);
  load_params.is_renderer_initiated = use_renderer_initiated;
  web_contents_->GetController().LoadURLWithParams(load_params);

  // The URL of a tab contents never actually changes to a JavaScript URL, so
  // this check only makes sense in other cases.
  if (!url.SchemeIs(url::kJavaScriptScheme)) {
    // The URL should be present in the pending entry, though it may not be
    // visible in the omnibox until it commits.
    DCHECK_EQ(
        url, web_contents_->GetController().GetPendingEntry()->GetVirtualURL());
  }

  return true;
}

void TabsUpdateFunction::PopulateResult() {
  if (!has_callback())
    return;

  results_ = tabs::Get::Results::Create(*ExtensionTabUtil::CreateTabObject(
      web_contents_, ExtensionTabUtil::kScrubTab, extension()));
}

void TabsUpdateFunction::OnExecuteCodeFinished(
    const std::string& error,
    const GURL& url,
    const base::ListValue& script_result) {
  if (error.empty())
    PopulateResult();
  else
    error_ = error;
  SendResponse(error.empty());
}

ExtensionFunction::ResponseAction TabsMoveFunction::Run() {
  std::unique_ptr<tabs::Move::Params> params(
      tabs::Move::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  int new_index = params->move_properties.index;
  int* window_id = params->move_properties.window_id.get();
  std::unique_ptr<base::ListValue> tab_values(new base::ListValue());

  size_t num_tabs = 0;
  std::string error;
  if (params->tab_ids.as_integers) {
    std::vector<int>& tab_ids = *params->tab_ids.as_integers;
    num_tabs = tab_ids.size();
    for (size_t i = 0; i < tab_ids.size(); ++i) {
      if (!MoveTab(tab_ids[i], &new_index, i, tab_values.get(), window_id,
                   &error)) {
        return RespondNow(Error(error));
      }
    }
  } else {
    EXTENSION_FUNCTION_VALIDATE(params->tab_ids.as_integer);
    num_tabs = 1;
    if (!MoveTab(*params->tab_ids.as_integer, &new_index, 0, tab_values.get(),
                 window_id, &error)) {
      return RespondNow(Error(error));
    }
  }

  // TODO(devlin): It's weird that whether or not the method provides a callback
  // can determine its success (as we return errors below).
  if (!has_callback())
    return RespondNow(NoArguments());

  if (num_tabs == 0)
    return RespondNow(Error("No tabs given."));
  if (num_tabs == 1) {
    std::unique_ptr<base::Value> value;
    CHECK(tab_values->Remove(0, &value));
    return RespondNow(OneArgument(std::move(value)));
  }

  // Return the results as an array if there are multiple tabs.
  return RespondNow(OneArgument(std::move(tab_values)));
}

bool TabsMoveFunction::MoveTab(int tab_id,
                               int* new_index,
                               int iteration,
                               base::ListValue* tab_values,
                               int* window_id,
                               std::string* error) {
  Browser* source_browser = NULL;
  TabStripModel* source_tab_strip = NULL;
  WebContents* contents = NULL;
  int tab_index = -1;
  if (!GetTabById(tab_id, browser_context(), include_incognito(),
                  &source_browser, &source_tab_strip, &contents, &tab_index,
                  error)) {
    return false;
  }

  // Don't let the extension move the tab if the user is dragging tabs.
  if (true) {
    *error = keys::kTabStripNotEditableError;
    return false;
  }

  // Insert the tabs one after another.
  *new_index += iteration;

  if (window_id) {
    Browser* target_browser = NULL;

    if (!GetBrowserFromWindowID(this, *window_id, &target_browser, error))
      return false;

    if (!target_browser->window()->IsTabStripEditable()) {
      *error = keys::kTabStripNotEditableError;
      return false;
    }

    if (!target_browser->is_type_tabbed()) {
      *error = keys::kCanOnlyMoveTabsWithinNormalWindowsError;
      return false;
    }

    if (target_browser->profile() != source_browser->profile()) {
      *error = keys::kCanOnlyMoveTabsWithinSameProfileError;
      return false;
    }

    // If windowId is different from the current window, move between windows.
    if (ExtensionTabUtil::GetWindowId(target_browser) !=
        ExtensionTabUtil::GetWindowId(source_browser)) {
      TabStripModel* target_tab_strip = target_browser->tab_strip_model();
      std::unique_ptr<content::WebContents> web_contents =
          source_tab_strip->DetachWebContentsAt(tab_index);
      if (!web_contents) {
        *error = ErrorUtils::FormatErrorMessage(keys::kTabNotFoundError,
                                                base::IntToString(tab_id));
        return false;
      }

      // Clamp move location to the last position.
      // This is ">" because it can append to a new index position.
      // -1 means set the move location to the last position.
      if (*new_index > target_tab_strip->count() || *new_index < 0)
        *new_index = target_tab_strip->count();

      content::WebContents* web_contents_raw = web_contents.get();
      target_tab_strip->InsertWebContentsAt(*new_index, std::move(web_contents),
                                            TabStripModel::ADD_NONE);

      if (has_callback()) {
        tab_values->Append(ExtensionTabUtil::CreateTabObject(
                               web_contents_raw, ExtensionTabUtil::kScrubTab,
                               extension(), target_tab_strip, *new_index)
                               ->ToValue());
      }

      return true;
    }
  }

  // Perform a simple within-window move.
  // Clamp move location to the last position.
  // This is ">=" because the move must be to an existing location.
  // -1 means set the move location to the last position.
  if (*new_index >= source_tab_strip->count() || *new_index < 0)
    *new_index = source_tab_strip->count() - 1;

  if (*new_index != tab_index)
    source_tab_strip->MoveWebContentsAt(tab_index, *new_index, false);

  if (has_callback()) {
    tab_values->Append(ExtensionTabUtil::CreateTabObject(
                           contents, ExtensionTabUtil::kScrubTab, extension(),
                           source_tab_strip, *new_index)
                           ->ToValue());
  }

  return true;
}

ExtensionFunction::ResponseAction TabsReloadFunction::Run() {
  std::unique_ptr<tabs::Reload::Params> params(
      tabs::Reload::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  bool bypass_cache = false;
  if (params->reload_properties.get() &&
      params->reload_properties->bypass_cache.get()) {
    bypass_cache = *params->reload_properties->bypass_cache;
  }

  content::WebContents* web_contents = NULL;

  // If |tab_id| is specified, look for it. Otherwise default to selected tab
  // in the current window.
  Browser* current_browser =
      ChromeExtensionFunctionDetails(this).GetCurrentBrowser();
  if (!params->tab_id.get()) {
    if (!current_browser)
      return RespondNow(Error(keys::kNoCurrentWindowError));

    if (!ExtensionTabUtil::GetDefaultTab(current_browser, &web_contents, NULL))
      return RespondNow(Error(kUnknownErrorDoNotUse));
  } else {
    int tab_id = *params->tab_id;

    Browser* browser = NULL;
    std::string error;
    if (!GetTabById(tab_id, browser_context(), include_incognito(), &browser,
                    NULL, &web_contents, NULL, &error)) {
      return RespondNow(Error(error));
    }
  }

  if (web_contents->ShowingInterstitialPage()) {
    // This does as same as Browser::ReloadInternal.
    NavigationEntry* entry = web_contents->GetController().GetVisibleEntry();
    GURL reload_url = entry ? entry->GetURL() : GURL(url::kAboutBlankURL);
    OpenURLParams params(reload_url, Referrer(),
                         WindowOpenDisposition::CURRENT_TAB,
                         ui::PAGE_TRANSITION_RELOAD, false);
    current_browser->OpenURL(params);
  } else {
    web_contents->GetController().Reload(
        bypass_cache ? content::ReloadType::BYPASSING_CACHE
                     : content::ReloadType::NORMAL,
        true);
  }

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction TabsRemoveFunction::Run() {
  std::unique_ptr<tabs::Remove::Params> params(
      tabs::Remove::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(INFO) << "[EXTENSIONS] Calling chrome.tabs.remove()";

  std::string error;
  if (params->tab_ids.as_integers) {
    std::vector<int>& tab_ids = *params->tab_ids.as_integers;
    for (size_t i = 0; i < tab_ids.size(); ++i) {
      if (!RemoveTab(tab_ids[i], &error))
        return RespondNow(Error(error));
    }
  } else {
    EXTENSION_FUNCTION_VALIDATE(params->tab_ids.as_integer);
    if (!RemoveTab(*params->tab_ids.as_integer, &error))
      return RespondNow(Error(error));
  }
  return RespondNow(NoArguments());
}

bool TabsRemoveFunction::RemoveTab(int tab_id, std::string* error) {
  Browser* browser = NULL;
  WebContents* contents = NULL;
  if (!GetTabById(tab_id, browser_context(), include_incognito(), &browser,
                  nullptr, &contents, nullptr, error)) {
    return false;
  }

  // There's a chance that the tab is being dragged, or we're in some other
  // nested event loop. This code path ensures that the tab is safely closed
  // under such circumstances, whereas |TabStripModel::CloseWebContentsAt()|
  // does not.
  contents->Close();
  return true;
}

TabsCaptureVisibleTabFunction::TabsCaptureVisibleTabFunction()
    : chrome_details_(this) {
}

bool TabsCaptureVisibleTabFunction::HasPermission() {
  return true;
}

bool TabsCaptureVisibleTabFunction::IsScreenshotEnabled() const {
  PrefService* service = chrome_details_.GetProfile()->GetPrefs();
  if (service->GetBoolean(prefs::kDisableScreenshots)) {
    return false;
  }
  return true;
}

bool TabsCaptureVisibleTabFunction::ClientAllowsTransparency() {
  return false;
}

WebContents* TabsCaptureVisibleTabFunction::GetWebContentsForID(
    int window_id,
    std::string* error) {
  Browser* browser = NULL;
  if (!GetBrowserFromWindowID(chrome_details_, window_id, &browser, error))
    return nullptr;

  WebContents* contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!contents) {
    *error = "No active web contents to capture";
    return nullptr;
  }

  if (!extension()->permissions_data()->CanCaptureVisiblePage(
          contents->GetLastCommittedURL(),
          SessionTabHelper::IdForTab(contents).id(), error)) {
    return nullptr;
  }
  return contents;
}

ExtensionFunction::ResponseAction TabsCaptureVisibleTabFunction::Run() {
  using api::extension_types::ImageDetails;

  EXTENSION_FUNCTION_VALIDATE(args_);

  int context_id = extension_misc::kCurrentWindowId;
  args_->GetInteger(0, &context_id);

  std::unique_ptr<ImageDetails> image_details;
  if (args_->GetSize() > 1) {
    base::Value* spec = NULL;
    EXTENSION_FUNCTION_VALIDATE(args_->Get(1, &spec) && spec);
    image_details = ImageDetails::FromValue(*spec);
  }

  std::string error;
  WebContents* contents = GetWebContentsForID(context_id, &error);
  if (!contents)
    return RespondNow(Error(error));

  const CaptureResult capture_result = CaptureAsync(
      contents, image_details.get(),
      base::BindOnce(&TabsCaptureVisibleTabFunction::CopyFromSurfaceComplete,
                     this));
  if (capture_result == OK) {
    // CopyFromSurfaceComplete might have already responded.
    return did_respond() ? AlreadyResponded() : RespondLater();
  }

  return RespondNow(Error(CaptureResultToErrorMessage(capture_result)));
}

void TabsCaptureVisibleTabFunction::OnCaptureSuccess(const SkBitmap& bitmap) {
  std::string base64_result;
  if (!EncodeBitmap(bitmap, &base64_result)) {
    OnCaptureFailure(FAILURE_REASON_ENCODING_FAILED);
    return;
  }

  Respond(OneArgument(std::make_unique<base::Value>(base64_result)));
}

void TabsCaptureVisibleTabFunction::OnCaptureFailure(CaptureResult result) {
  Respond(Error(CaptureResultToErrorMessage(result)));
}

// static.
std::string TabsCaptureVisibleTabFunction::CaptureResultToErrorMessage(
    CaptureResult result) {
  const char* reason_description = "internal error";
  switch (result) {
    case FAILURE_REASON_READBACK_FAILED:
      reason_description = "image readback failed";
      break;
    case FAILURE_REASON_ENCODING_FAILED:
      reason_description = "encoding failed";
      break;
    case FAILURE_REASON_VIEW_INVISIBLE:
      reason_description = "view is invisible";
      break;
    case FAILURE_REASON_SCREEN_SHOTS_DISABLED:
      return keys::kScreenshotsDisabled;
    case OK:
      NOTREACHED() << "CaptureResultToErrorMessage should not be called"
                      " with a successful result";
      return kUnknownErrorDoNotUse;
  }
  return ErrorUtils::FormatErrorMessage("Failed to capture tab: *",
                                        reason_description);
}

void TabsCaptureVisibleTabFunction::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kDisableScreenshots, false);
}

bool TabsDetectLanguageFunction::RunAsync() {
  std::unique_ptr<tabs::DetectLanguage::Params> params(
      tabs::DetectLanguage::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  int tab_id = 0;
  Browser* browser = NULL;
  WebContents* contents = NULL;

  // If |tab_id| is specified, look for it. Otherwise default to selected tab
  // in the current window.
  if (params->tab_id.get()) {
    tab_id = *params->tab_id;
    if (!GetTabById(tab_id, browser_context(), include_incognito(), &browser,
                    nullptr, &contents, nullptr, &error_)) {
      return false;
    }
    if (!browser || !contents)
      return false;
  } else {
    browser = ChromeExtensionFunctionDetails(this).GetCurrentBrowser();
    if (!browser)
      return false;
    contents = browser->tab_strip_model()->GetActiveWebContents();
    if (!contents)
      return false;
  }

  if (contents->GetController().NeedsReload()) {
    // If the tab hasn't been loaded, don't wait for the tab to load.
    error_ = keys::kCannotDetermineLanguageOfUnloadedTab;
    return false;
  }

  AddRef();  // Balanced in GotLanguage().

  ChromeTranslateClient* chrome_translate_client =
      ChromeTranslateClient::FromWebContents(contents);
  if (!chrome_translate_client->GetLanguageState()
           .original_language()
           .empty()) {
    // Delay the callback invocation until after the current JS call has
    // returned.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &TabsDetectLanguageFunction::GotLanguage, this,
            chrome_translate_client->GetLanguageState().original_language()));
    return true;
  }
  // The tab contents does not know its language yet.  Let's wait until it
  // receives it, or until the tab is closed/navigates to some other page.
  registrar_.Add(this, chrome::NOTIFICATION_TAB_LANGUAGE_DETERMINED,
                 content::Source<WebContents>(contents));
  registrar_.Add(
      this, chrome::NOTIFICATION_TAB_CLOSING,
      content::Source<NavigationController>(&(contents->GetController())));
  registrar_.Add(
      this, content::NOTIFICATION_NAV_ENTRY_COMMITTED,
      content::Source<NavigationController>(&(contents->GetController())));
  return true;
}

void TabsDetectLanguageFunction::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  std::string language;
  if (type == chrome::NOTIFICATION_TAB_LANGUAGE_DETERMINED) {
    const translate::LanguageDetectionDetails* lang_det_details =
        content::Details<const translate::LanguageDetectionDetails>(details)
            .ptr();
    language = lang_det_details->adopted_language;
  }

  registrar_.RemoveAll();

  // Call GotLanguage in all cases as we want to guarantee the callback is
  // called for every API call the extension made.
  GotLanguage(language);
}

void TabsDetectLanguageFunction::GotLanguage(const std::string& language) {
  SetResult(std::make_unique<base::Value>(language));
  SendResponse(true);

  Release();  // Balanced in Run()
}

ExecuteCodeInTabFunction::ExecuteCodeInTabFunction()
    : chrome_details_(this), execute_tab_id_(-1) {
}

ExecuteCodeInTabFunction::~ExecuteCodeInTabFunction() {}

bool ExecuteCodeInTabFunction::HasPermission() {
  if (Init() == SUCCESS &&
      // TODO(devlin/lazyboy): Consider removing the following check as it isn't
      // doing anything. The fallback to ExtensionFunction::HasPermission()
      // below dictates what this function returns.
      extension_->permissions_data()->HasAPIPermissionForTab(
          execute_tab_id_, APIPermission::kTab)) {
    return true;
  }
  return ExtensionFunction::HasPermission();
}

ExecuteCodeFunction::InitResult ExecuteCodeInTabFunction::Init() {
  if (init_result_)
    return init_result_.value();

  // |tab_id| is optional so it's ok if it's not there.
  int tab_id = -1;
  if (args_->GetInteger(0, &tab_id) && tab_id < 0)
    return set_init_result(VALIDATION_FAILURE);

  // |details| are not optional.
  base::DictionaryValue* details_value = NULL;
  if (!args_->GetDictionary(1, &details_value))
    return set_init_result(VALIDATION_FAILURE);
  std::unique_ptr<InjectDetails> details(new InjectDetails());
  if (!InjectDetails::Populate(*details_value, details.get()))
    return set_init_result(VALIDATION_FAILURE);

  // If the tab ID wasn't given then it needs to be converted to the
  // currently active tab's ID.
  if (tab_id == -1) {
    TabModel *tab_strip = nullptr;
    if (!TabModelList::empty())
      tab_strip = *(TabModelList::begin());
    if (tab_strip) {
      for (int i = 0; i < tab_strip->GetTabCount(); ++i) {
        WebContents* web_contents = tab_strip->GetWebContentsAt(i);

        int openingTab = (tab_strip->GetLastNonExtensionActiveIndex());
        if (extension() && extension()->id() == "mooikfkahbdckldjjndioackbalphokd")
          openingTab = (tab_strip->GetActiveIndex());
        if (openingTab == -1)
          openingTab = 0;

        if (i != openingTab)
          continue;

        if (web_contents && ExtensionTabUtil::GetTabId(web_contents) >= 0) {
          tab_id = ExtensionTabUtil::GetTabId(web_contents);
          break;
        }
      }
    }
  }
  if (tab_id == -1) {
    return set_init_result_error(keys::kNoCurrentWindowError);
  }

  execute_tab_id_ = tab_id;
  details_ = std::move(details);
  set_host_id(HostID(HostID::EXTENSIONS, extension()->id()));
  LOG(INFO) << "[EXTENSIONS] ExecuteCodeInTabFunction::Init - Step 4";
  return set_init_result(SUCCESS);
}

bool ExecuteCodeInTabFunction::CanExecuteScriptOnPage(std::string* error) {
  content::WebContents* contents = nullptr;

  // If |tab_id| is specified, look for the tab. Otherwise default to selected
  // tab in the current window.
  CHECK_GE(execute_tab_id_, 0);
  if (!GetTabById(execute_tab_id_, browser_context(), include_incognito(),
                  nullptr, nullptr, &contents, nullptr, error)) {
    return false;
  }

  CHECK(contents);

  int frame_id = details_->frame_id ? *details_->frame_id
                                    : ExtensionApiFrameIdMap::kTopFrameId;
  content::RenderFrameHost* rfh =
      ExtensionApiFrameIdMap::GetRenderFrameHostById(contents, frame_id);
  if (!rfh) {
    *error = ErrorUtils::FormatErrorMessage(keys::kFrameNotFoundError,
                                            base::IntToString(frame_id),
                                            base::IntToString(execute_tab_id_));
    return false;
  }

  // Content scripts declared in manifest.json can access frames at about:-URLs
  // if the extension has permission to access the frame's origin, so also allow
  // programmatic content scripts at about:-URLs for allowed origins.
  GURL effective_document_url(rfh->GetLastCommittedURL());
  bool is_about_url = effective_document_url.SchemeIs(url::kAboutScheme);
  if (is_about_url && details_->match_about_blank &&
      *details_->match_about_blank) {
    effective_document_url = GURL(rfh->GetLastCommittedOrigin().Serialize());
  }

  if (!effective_document_url.is_valid()) {
    // Unknown URL, e.g. because no load was committed yet. Allow for now, the
    // renderer will check again and fail the injection if needed.
    return true;
  }

  // NOTE: This can give the wrong answer due to race conditions, but it is OK,
  // we check again in the renderer.
  if (!extension()->permissions_data()->CanAccessPage(effective_document_url,
                                                      execute_tab_id_, error)) {
    if (is_about_url &&
        extension()->permissions_data()->active_permissions().HasAPIPermission(
            APIPermission::kTab)) {
      *error = ErrorUtils::FormatErrorMessage(
          manifest_errors::kCannotAccessAboutUrl,
          rfh->GetLastCommittedURL().spec(),
          rfh->GetLastCommittedOrigin().Serialize());
    }
    return false;
  }

  return true;
}

ScriptExecutor* ExecuteCodeInTabFunction::GetScriptExecutor(
    std::string* error) {
  Browser* browser = nullptr;
  content::WebContents* contents = nullptr;

  LOG(INFO) << "[EXTENSIONS] ExecuteCodeInTabFunction::GetScriptExecutor - Step 1";

  bool success =
      GetTabById(execute_tab_id_, browser_context(), include_incognito(),
                 &browser, nullptr, &contents, nullptr, error) &&
      contents;

  LOG(INFO) << "[EXTENSIONS] ExecuteCodeInTabFunction::GetScriptExecutor - Step 2: " << contents << " - " << browser;
  if (!success)
    return nullptr;

  LOG(INFO) << "[EXTENSIONS] ExecuteCodeInTabFunction::GetScriptExecutor - Step 3";
  return TabHelper::FromWebContents(contents)->script_executor();
}

bool ExecuteCodeInTabFunction::IsWebView() const {
  return false;
}

const GURL& ExecuteCodeInTabFunction::GetWebViewSrc() const {
  return GURL::EmptyGURL();
}

bool TabsExecuteScriptFunction::ShouldInsertCSS() const {
  return false;
}

bool TabsInsertCSSFunction::ShouldInsertCSS() const {
  return true;
}

content::WebContents* ZoomAPIFunction::GetWebContents(int tab_id) {
  content::WebContents* web_contents = NULL;
  if (tab_id != -1) {
    // We assume this call leaves web_contents unchanged if it is unsuccessful.
    GetTabById(tab_id, browser_context(), include_incognito(),
               nullptr /* ignore Browser* output */,
               nullptr /* ignore TabStripModel* output */, &web_contents,
               nullptr /* ignore int tab_index output */, &error_);
  } else {
    Browser* browser = ChromeExtensionFunctionDetails(this).GetCurrentBrowser();
    if (!browser)
      error_ = keys::kNoCurrentWindowError;
    else if (!ExtensionTabUtil::GetDefaultTab(browser, &web_contents, NULL))
      error_ = keys::kNoSelectedTabError;
  }
  return web_contents;
}

bool TabsSetZoomFunction::RunAsync() {
  std::unique_ptr<tabs::SetZoom::Params> params(
      tabs::SetZoom::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  WebContents* web_contents = GetWebContents(tab_id);
  if (!web_contents)
    return false;

  GURL url(web_contents->GetVisibleURL());
  if (extension()->permissions_data()->IsRestrictedUrl(url, &error_))
    return false;

  ZoomController* zoom_controller =
      ZoomController::FromWebContents(web_contents);
  double zoom_level = params->zoom_factor > 0
                          ? content::ZoomFactorToZoomLevel(params->zoom_factor)
                          : zoom_controller->GetDefaultZoomLevel();

  scoped_refptr<ExtensionZoomRequestClient> client(
      new ExtensionZoomRequestClient(extension()));
  if (!zoom_controller->SetZoomLevelByClient(zoom_level, client)) {
    // Tried to zoom a tab in disabled mode.
    error_ = keys::kCannotZoomDisabledTabError;
    return false;
  }

  SendResponse(true);
  return true;
}

bool TabsGetZoomFunction::RunAsync() {
  std::unique_ptr<tabs::GetZoom::Params> params(
      tabs::GetZoom::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  WebContents* web_contents = GetWebContents(tab_id);
  if (!web_contents)
    return false;

  double zoom_level =
      ZoomController::FromWebContents(web_contents)->GetZoomLevel();
  double zoom_factor = content::ZoomLevelToZoomFactor(zoom_level);
  results_ = tabs::GetZoom::Results::Create(zoom_factor);
  SendResponse(true);
  return true;
}

bool TabsSetZoomSettingsFunction::RunAsync() {
  using api::tabs::ZoomSettings;

  std::unique_ptr<tabs::SetZoomSettings::Params> params(
      tabs::SetZoomSettings::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  WebContents* web_contents = GetWebContents(tab_id);
  if (!web_contents)
    return false;

  GURL url(web_contents->GetVisibleURL());
  if (extension()->permissions_data()->IsRestrictedUrl(url, &error_))
    return false;

  // "per-origin" scope is only available in "automatic" mode.
  if (params->zoom_settings.scope == tabs::ZOOM_SETTINGS_SCOPE_PER_ORIGIN &&
      params->zoom_settings.mode != tabs::ZOOM_SETTINGS_MODE_AUTOMATIC &&
      params->zoom_settings.mode != tabs::ZOOM_SETTINGS_MODE_NONE) {
    error_ = keys::kPerOriginOnlyInAutomaticError;
    return false;
  }

  // Determine the correct internal zoom mode to set |web_contents| to from the
  // user-specified |zoom_settings|.
  ZoomController::ZoomMode zoom_mode = ZoomController::ZOOM_MODE_DEFAULT;
  switch (params->zoom_settings.mode) {
    case tabs::ZOOM_SETTINGS_MODE_NONE:
    case tabs::ZOOM_SETTINGS_MODE_AUTOMATIC:
      switch (params->zoom_settings.scope) {
        case tabs::ZOOM_SETTINGS_SCOPE_NONE:
        case tabs::ZOOM_SETTINGS_SCOPE_PER_ORIGIN:
          zoom_mode = ZoomController::ZOOM_MODE_DEFAULT;
          break;
        case tabs::ZOOM_SETTINGS_SCOPE_PER_TAB:
          zoom_mode = ZoomController::ZOOM_MODE_ISOLATED;
      }
      break;
    case tabs::ZOOM_SETTINGS_MODE_MANUAL:
      zoom_mode = ZoomController::ZOOM_MODE_MANUAL;
      break;
    case tabs::ZOOM_SETTINGS_MODE_DISABLED:
      zoom_mode = ZoomController::ZOOM_MODE_DISABLED;
  }

  ZoomController::FromWebContents(web_contents)->SetZoomMode(zoom_mode);

  SendResponse(true);
  return true;
}

bool TabsGetZoomSettingsFunction::RunAsync() {
  std::unique_ptr<tabs::GetZoomSettings::Params> params(
      tabs::GetZoomSettings::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  WebContents* web_contents = GetWebContents(tab_id);
  if (!web_contents)
    return false;
  ZoomController* zoom_controller =
      ZoomController::FromWebContents(web_contents);

  ZoomController::ZoomMode zoom_mode = zoom_controller->zoom_mode();
  api::tabs::ZoomSettings zoom_settings;
  ZoomModeToZoomSettings(zoom_mode, &zoom_settings);
  zoom_settings.default_zoom_factor.reset(new double(
      content::ZoomLevelToZoomFactor(zoom_controller->GetDefaultZoomLevel())));

  results_ = api::tabs::GetZoomSettings::Results::Create(zoom_settings);
  SendResponse(true);
  return true;
}

ExtensionFunction::ResponseAction TabsDiscardFunction::Run() {
  std::unique_ptr<tabs::Discard::Params> params(
      tabs::Discard::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  WebContents* contents = nullptr;
  // If |tab_id| is given, find the web_contents respective to it.
  // Otherwise invoke discard function in TabManager with null web_contents
  // that will discard the least important tab.
  if (params->tab_id) {
    int tab_id = *params->tab_id;
    std::string error;
    if (!GetTabById(tab_id, browser_context(), include_incognito(), nullptr,
                    nullptr, &contents, nullptr, &error)) {
      return RespondNow(Error(error));
    }
  }
  // Discard the tab.
  contents =
      g_browser_process->GetTabManager()->DiscardTabByExtension(contents);

  // Create the Tab object and return it in case of success.
  if (contents) {
    return RespondNow(ArgumentList(
        tabs::Discard::Results::Create(*ExtensionTabUtil::CreateTabObject(
            contents, ExtensionTabUtil::kScrubTab, extension()))));
  }

  // Return appropriate error message otherwise.
  return RespondNow(Error(
      params->tab_id
          ? ErrorUtils::FormatErrorMessage(keys::kCannotDiscardTab,
                                           base::IntToString(*params->tab_id))
          : keys::kCannotFindTabToDiscard));
}

TabsDiscardFunction::TabsDiscardFunction() {}
TabsDiscardFunction::~TabsDiscardFunction() {}

}  // namespace extensions
