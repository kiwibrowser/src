// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_tab_util.h"

#include <stddef.h>
#include <algorithm>

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/tab_contents/tab_contents_iterator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/common/extensions/api/tabs.h"
#include "chrome/common/url_constants.h"
#include "components/url_formatter/url_fixer.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "extensions/common/manifest_handlers/options_page_info.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/permissions_data.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "url/gurl.h"
#include "chrome/browser/android/devtools_manager_delegate_android.h"

#if defined(OS_CHROMEOS)
#include "ash/public/cpp/window_pin_type.h"
#endif

using content::NavigationEntry;
using content::WebContents;

namespace extensions {

namespace {

namespace keys = tabs_constants;

// |error_message| can optionally be passed in and will be set with an
// appropriate message if the window cannot be found by id.
Browser* GetBrowserInProfileWithId(Profile* profile,
                                   const int window_id,
                                   bool include_incognito,
                                   std::string* error_message) {
  Profile* incognito_profile =
      include_incognito && profile->HasOffTheRecordProfile()
          ? profile->GetOffTheRecordProfile()
          : nullptr;
  for (auto* browser : *BrowserList::GetInstance()) {
    if ((browser->profile() == profile ||
         browser->profile() == incognito_profile) &&
        ExtensionTabUtil::GetWindowId(browser) == window_id &&
        browser->window()) {
      return browser;
    }
  }

  if (error_message)
    *error_message = ErrorUtils::FormatErrorMessage(
        keys::kWindowNotFoundError, base::IntToString(window_id));

  return nullptr;
}

Browser* CreateBrowser(Profile* profile,
                       int window_id,
                       bool user_gesture,
                       std::string* error) {
  Browser::CreateParams params(Browser::TYPE_TABBED, profile, user_gesture);
  Browser* browser = new Browser(params);
  browser->window()->Show();
  return browser;
}

// Use this function for reporting a tab id to an extension. It will
// take care of setting the id to TAB_ID_NONE if necessary (for
// example with devtools).
int GetTabIdForExtensions(const WebContents* web_contents) {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  if (browser && !ExtensionTabUtil::BrowserSupportsTabs(browser))
    return -1;
  return SessionTabHelper::IdForTab(web_contents).id();
}

ExtensionTabUtil::Delegate* g_extension_tab_util_delegate = nullptr;

}  // namespace

ExtensionTabUtil::OpenTabParams::OpenTabParams()
    : create_browser_if_needed(false) {
}

ExtensionTabUtil::OpenTabParams::~OpenTabParams() {
}

// Opens a new tab for a given extension. Returns nullptr and sets |error| if an
// error occurs.
base::DictionaryValue* ExtensionTabUtil::OpenTab(
    UIThreadExtensionFunction* function,
    const OpenTabParams& params,
    bool user_gesture,
    std::string* error) {
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 1 - 0";
  if (true) {
    if (TabModelList::empty())
      return nullptr;

    TabModel* tab_model = TabModelList::get(0);
    if (!tab_model)
      return nullptr;

    GURL url = GURL("about:blank");
    if (params.url.get()) {
      std::string url_string = *params.url;
      url = ExtensionTabUtil::ResolvePossiblyRelativeURL(url_string,
                                                       function->extension());
      if (!url.is_valid()) {
        *error =
            ErrorUtils::FormatErrorMessage(keys::kInvalidUrlError, url_string);
        return nullptr;
      }
    }
    WebContents* web_contents = tab_model->CreateNewTabForDevTools(url, function->include_incognito());
    if (!web_contents)
      return nullptr;

    return ExtensionTabUtil::CreateTabObject(
               web_contents, kScrubTab,
               function->extension(), NULL, tab_model->GetActiveIndex())
        ->ToValue()
        .release();
  }
  ChromeExtensionFunctionDetails chrome_details(function);
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 2";
  Profile* profile = chrome_details.GetProfile();
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 3";
  // windowId defaults to "current" window.
  int window_id = extension_misc::kCurrentWindowId;
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 4";
  if (params.window_id.get())
    window_id = *params.window_id;
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 5";

  Browser* browser = GetBrowserFromWindowID(chrome_details, window_id, error);
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 6: " << browser;
  if (!browser) {
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 6a";
/*
    if (!params.create_browser_if_needed) {
      return nullptr;
    }
*/
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 6b";
    browser = CreateBrowser(profile, window_id, user_gesture, error);
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 6c: " << browser;
    if (!browser)
      return nullptr;
  }

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 7";

  // Ensure the selected browser is tabbed.
  if (!browser->is_type_tabbed() && browser->IsAttemptingToCloseBrowser())
    browser = chrome::FindTabbedBrowser(profile, function->include_incognito());
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 8";
  if (!browser || !browser->window()) {
    if (error)
      *error = keys::kNoCurrentWindowError;
    return nullptr;
  }

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 9";

  // TODO(jstritar): Add a constant, chrome.tabs.TAB_ID_ACTIVE, that
  // represents the active tab.
  WebContents* opener = nullptr;
  Browser* opener_browser = nullptr;
  if (params.opener_tab_id.get()) {
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 9a";
    int opener_id = *params.opener_tab_id;

    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 9b";
    if (!ExtensionTabUtil::GetTabById(
            opener_id, profile, function->include_incognito(), &opener_browser,
            nullptr, &opener, nullptr)) {
      if (error) {
        *error = ErrorUtils::FormatErrorMessage(keys::kTabNotFoundError,
                                                base::IntToString(opener_id));
      }
      return nullptr;
    }
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 9c";
  }

  // TODO(rafaelw): handle setting remaining tab properties:
  // -title
  // -favIconUrl
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 10";

  GURL url;
  if (params.url.get()) {
    std::string url_string = *params.url;
    url = ExtensionTabUtil::ResolvePossiblyRelativeURL(url_string,
                                                       function->extension());
    if (!url.is_valid()) {
      *error =
          ErrorUtils::FormatErrorMessage(keys::kInvalidUrlError, url_string);
      return nullptr;
    }
  } else {
    url = GURL(chrome::kChromeUINewTabURL);
  }

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 11";
  // Don't let extensions crash the browser or renderers.
  if (ExtensionTabUtil::IsKillURL(url)) {
    *error = keys::kNoCrashBrowserError;
    return nullptr;
  }
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 12";

  // Default to foreground for the new tab. The presence of 'active' property
  // will override this default.
  bool active = true;
  if (params.active.get())
    active = *params.active;

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 13";
  // Default to not pinning the tab. Setting the 'pinned' property to true
  // will override this default.
  bool pinned = false;
  if (params.pinned.get())
    pinned = *params.pinned;

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 14";

  // We can't load extension URLs into incognito windows unless the extension
  // uses split mode. Special case to fall back to a tabbed window.
  if (url.SchemeIs(kExtensionScheme) &&
      (!function->extension() ||
       !IncognitoInfo::IsSplitMode(function->extension())) &&
      browser->profile()->IsOffTheRecord()) {
    Profile* profile = browser->profile()->GetOriginalProfile();

    browser = chrome::FindTabbedBrowser(profile, false);
    if (!browser) {
      Browser::CreateParams params =
          Browser::CreateParams(Browser::TYPE_TABBED, profile, user_gesture);
      browser = new Browser(params);
      browser->window()->Show();
    }
  }

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 15";
  if (opener_browser && browser != opener_browser) {
    if (error) {
      *error = "Tab opener must be in the same window as the updated tab.";
    }
    return nullptr;
  }

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 16";

  // If index is specified, honor the value, but keep it bound to
  // -1 <= index <= tab_strip->count() where -1 invokes the default behavior.
  int index = -1;
  if (params.index.get())
    index = *params.index;

  TabStripModel* tab_strip = browser->tab_strip_model();


  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 17: " << tab_strip;
  index = std::min(std::max(index, -1), tab_strip->count());

  int add_types = active ? TabStripModel::ADD_ACTIVE : TabStripModel::ADD_NONE;
  add_types |= TabStripModel::ADD_FORCE_INDEX;
  if (pinned)
    add_types |= TabStripModel::ADD_PINNED;
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 18: " << tab_strip;
  NavigateParams navigate_params(browser, url, ui::PAGE_TRANSITION_LINK);
  navigate_params.disposition = active
                                    ? WindowOpenDisposition::NEW_FOREGROUND_TAB
                                    : WindowOpenDisposition::NEW_BACKGROUND_TAB;
  navigate_params.tabstrip_index = index;
  navigate_params.tabstrip_add_types = add_types;
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 19: " << tab_strip;
  Navigate(&navigate_params);

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 20: " << tab_strip;
  // The tab may have been created in a different window, so make sure we look
  // at the right tab strip.
  tab_strip = navigate_params.browser->tab_strip_model();
  int new_index = tab_strip->GetIndexOfWebContents(
      navigate_params.navigated_or_inserted_contents);
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 21: " << tab_strip;
  if (opener) {
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 21a: " << tab_strip;
    // Only set the opener if the opener tab is in the same tab strip as the
    // new tab.
    if (tab_strip->GetIndexOfWebContents(opener) != TabStripModel::kNoTab) {
      LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 21b: " << tab_strip;
      tab_strip->SetOpenerOfWebContentsAt(new_index, opener);
      LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 21c: " << tab_strip;
    }
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 21d: " << tab_strip;
  }

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 22";

  if (active) {
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 23";
    navigate_params.navigated_or_inserted_contents->SetInitialFocus();
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 24";
  }

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenTab - Step 25: "<< navigate_params.navigated_or_inserted_contents;

  // Return data about the newly created tab.
  return ExtensionTabUtil::CreateTabObject(
             navigate_params.navigated_or_inserted_contents, kScrubTab,
             function->extension(), tab_strip, new_index)
      ->ToValue()
      .release();
}

Browser* ExtensionTabUtil::GetBrowserFromWindowID(
    const ChromeExtensionFunctionDetails& details,
    int window_id,
    std::string* error) {
  if (window_id == extension_misc::kCurrentWindowId) {
    Browser* result = details.GetCurrentBrowser();
    if (!result || !result->window()) {
      if (error)
        *error = keys::kNoCurrentWindowError;
      return nullptr;
    }
    return result;
  } else {
    return GetBrowserInProfileWithId(details.GetProfile(),
                                     window_id,
                                     details.function()->include_incognito(),
                                     error);
  }
}

int ExtensionTabUtil::GetWindowId(const Browser* browser) {
  return browser->session_id().id();
}

int ExtensionTabUtil::GetWindowIdOfTabStripModel(
    const TabStripModel* tab_strip_model) {
  for (auto* browser : *BrowserList::GetInstance()) {
    if (browser->tab_strip_model() == tab_strip_model)
      return GetWindowId(browser);
  }
  return -1;
}

int ExtensionTabUtil::GetTabId(const WebContents* web_contents) {
  return SessionTabHelper::IdForTab(web_contents).id();
}

std::string ExtensionTabUtil::GetTabStatusText(bool is_loading) {
  return is_loading ? keys::kStatusValueLoading : keys::kStatusValueComplete;
}

int ExtensionTabUtil::GetWindowIdOfTab(const WebContents* web_contents) {
  return SessionTabHelper::IdForWindowContainingTab(web_contents).id();
}

// static
std::string ExtensionTabUtil::GetBrowserWindowTypeText(const Browser& browser) {
  if (browser.is_devtools())
    return keys::kWindowTypeValueDevTools;
  if (browser.is_type_popup())
    return keys::kWindowTypeValuePopup;
  // TODO(devlin): Browser::is_app() returns true whenever Browser::app_name_
  // is non-empty (and includes instances such as devtools). Platform apps
  // should no longer be returned here; are there any other cases (that aren't
  // captured by is_devtools() or is_type_popup() for an app-type browser?
  if (browser.is_app())
    return keys::kWindowTypeValueApp;
  return keys::kWindowTypeValueNormal;
}

// static
std::unique_ptr<api::tabs::Tab> ExtensionTabUtil::CreateTabObject(
    WebContents* contents,
    ScrubTabBehavior scrub_tab_behavior,
    const Extension* extension,
    TabStripModel* tab_strip,
    int tab_index) {
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 1";
//  if (!tab_strip)
//    ExtensionTabUtil::GetTabStripModel(contents, &tab_strip, &tab_index);
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 2: " << tab_strip;
  tab_strip = nullptr;
  bool is_loading = contents->IsLoading();
  auto tab_object = std::make_unique<api::tabs::Tab>();
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 2" << contents;
  tab_object->id = std::make_unique<int>(GetTabIdForExtensions(contents));
  tab_object->index = tab_index;
  if (extension && extension->id() == "mooikfkahbdckldjjndioackbalphokd") {
    tab_object->window_id = SessionTabHelper::IdForTab(contents).id();
  } else {
    tab_object->window_id = GetWindowIdOfTab(contents);
  }
  tab_object->status =
      std::make_unique<std::string>(GetTabStatusText(is_loading));
  tab_object->active = false;
  TabModel *tab_strip_android = nullptr;
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 3";
  if (!TabModelList::empty())
    tab_strip_android = *(TabModelList::begin());
  if (tab_strip_android) {
    int openingTab = (tab_strip_android->GetLastNonExtensionActiveIndex());
    if (extension && extension->id() == "mooikfkahbdckldjjndioackbalphokd")
      openingTab = (tab_strip_android->GetActiveIndex());
    if (openingTab == -1)
      openingTab = 0;
    if (tab_index == openingTab) {
      tab_object->active = true;
    }
//    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 4";
    for (int i = 0; i < tab_strip_android->GetTabCount(); ++i) {
      int openingTab = (tab_strip_android->GetLastNonExtensionActiveIndex());
      if (extension && extension->id() == "mooikfkahbdckldjjndioackbalphokd")
        openingTab = (tab_strip_android->GetActiveIndex());
      if (openingTab == -1)
        openingTab = 0;

      if (i != openingTab)
        continue;

      if (tab_strip_android->GetWebContentsAt(i) == contents) {
        tab_object->active = true;
      }
    }
  }
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 5";
  tab_object->selected = tab_object->active;
  tab_object->highlighted = tab_strip && tab_strip->IsTabSelected(tab_index);
  tab_object->pinned = tab_strip && tab_strip->IsTabPinned(tab_index);
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 6";
  tab_object->audible = std::make_unique<bool>(contents->WasRecentlyAudible());
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 7";
  tab_object->muted_info = CreateMutedInfo(contents);
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 8";
  tab_object->incognito = contents->GetBrowserContext()->IsOffTheRecord();
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 9";
  gfx::Size contents_size = contents->GetContainerBounds().size();
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 10";
  tab_object->width = std::make_unique<int>(contents_size.width());
  tab_object->height = std::make_unique<int>(contents_size.height());
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 11";

  tab_object->url = std::make_unique<std::string>(contents->GetURL().spec());
  size_t pos = (tab_object->url)->find("chrome-search://");
  if (pos != std::string::npos && pos == 0) {
    std::string src = "chrome-search://";
    tab_object->url = std::make_unique<std::string>((tab_object->url)->replace(pos, src.size(), "https://local.ntp/"));
  }
  tab_object->title =
      std::make_unique<std::string>(base::UTF16ToUTF8(contents->GetTitle()));
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 12";
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 3";
  NavigationEntry* entry = contents->GetController().GetVisibleEntry();
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 4: " << entry;
  if (entry && entry->GetFavicon().valid) {
    tab_object->fav_icon_url =
        std::make_unique<std::string>(entry->GetFavicon().url.spec());
  }

//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 13";
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 6";
  if (scrub_tab_behavior == kScrubTab) {
//    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 6a";
    ScrubTabForExtension(extension, contents, tab_object.get());
//    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 6b";
  }
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 14";
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::CreateTabObject - Step 7";
  return tab_object;
}

std::unique_ptr<base::ListValue> ExtensionTabUtil::CreateTabList(
    const Browser* browser,
    const Extension* extension) {
  std::unique_ptr<base::ListValue> tab_list(new base::ListValue());

  TabStripModel* tab_strip = browser->tab_strip_model();
  TabModel *my_tab_strip = nullptr;
  if (!TabModelList::empty())
    my_tab_strip = *(TabModelList::begin());
  if (my_tab_strip) {
    for (int i = 0; i < my_tab_strip->GetTabCount(); ++i) {
        WebContents* target_contents = my_tab_strip->GetWebContentsAt(i);
        if (target_contents) {
          tab_list->Append(CreateTabObject(target_contents, kScrubTab,
                                           extension, tab_strip, i)
                               ->ToValue());
        }
    }
  }
  return tab_list;
}

// static
std::unique_ptr<base::DictionaryValue>
ExtensionTabUtil::CreateWindowValueForExtension(
    const Browser& browser,
    const Extension* extension,
    PopulateTabBehavior populate_tab_behavior) {
  auto result = std::make_unique<base::DictionaryValue>();

//  LOG(INFO) << "[EXTENSIONS] Preparing data in CreateWindowValueForExtension - Step 1";
  result->SetString(keys::kWindowTypeKey, GetBrowserWindowTypeText(browser));
  ui::BaseWindow* window = browser.window();
  result->SetBoolean(keys::kFocusedKey, window->IsActive());
  const Profile* profile = browser.profile();
  result->SetBoolean(keys::kIncognitoKey, profile->IsOffTheRecord());
  result->SetBoolean(keys::kAlwaysOnTopKey, window->IsAlwaysOnTop());

  std::string window_state;
  if (window->IsMinimized()) {
    window_state = keys::kShowStateValueMinimized;
  } else if (window->IsFullscreen()) {
    window_state = keys::kShowStateValueFullscreen;
#if defined(OS_CHROMEOS)
    if (ash::IsWindowTrustedPinned(window))
      window_state = keys::kShowStateValueLockedFullscreen;
#endif
  } else if (window->IsMaximized()) {
    window_state = keys::kShowStateValueMaximized;
  } else {
    window_state = keys::kShowStateValueNormal;
  }
  result->SetString(keys::kShowStateKey, window_state);

  gfx::Rect bounds = gfx::Rect();
  result->SetInteger(keys::kLeftKey, bounds.x());
  result->SetInteger(keys::kTopKey, bounds.y());
  result->SetInteger(keys::kWidthKey, bounds.width());
  result->SetInteger(keys::kHeightKey, bounds.height());

  if (populate_tab_behavior == kPopulateTabs)
    result->Set(keys::kTabsKey, CreateTabList(&browser, extension));
//  LOG(INFO) << "[EXTENSIONS] Preparing data in CreateWindowValueForExtension - Step 2";

  if (extension && extension->id() == "mooikfkahbdckldjjndioackbalphokd") {
    int activeTab = browser.session_id().id();
    TabModel *tab_strip = nullptr;
    if (!TabModelList::empty())
      tab_strip = *(TabModelList::begin());
    if (tab_strip) {
      for (int i = 0; i < tab_strip->GetTabCount(); ++i) {
        WebContents* web_contents = tab_strip->GetWebContentsAt(i);
        int openingTab = (tab_strip->GetLastNonExtensionActiveIndex());
        if (extension && extension->id() == "mooikfkahbdckldjjndioackbalphokd")
          openingTab = (tab_strip->GetActiveIndex());
        if (openingTab == -1)
          openingTab = 0;
        if (i != openingTab)
          continue;
        activeTab = SessionTabHelper::IdForTab(web_contents).id();
      }
    }
    result->SetInteger(keys::kIdKey, activeTab);
  }
  else
    result->SetInteger(keys::kIdKey, browser.session_id().id());

  return result;
}

// static
std::unique_ptr<api::tabs::MutedInfo> ExtensionTabUtil::CreateMutedInfo(
    content::WebContents* contents) {
  DCHECK(contents);
  std::unique_ptr<api::tabs::MutedInfo> info(new api::tabs::MutedInfo);
  info->muted = contents->IsAudioMuted();
  switch (chrome::GetTabAudioMutedReason(contents)) {
    case TabMutedReason::NONE:
      break;
    case TabMutedReason::AUDIO_INDICATOR:
    case TabMutedReason::CONTENT_SETTING:
    case TabMutedReason::CONTENT_SETTING_CHROME:
    case TabMutedReason::CONTEXT_MENU:
      info->reason = api::tabs::MUTED_INFO_REASON_USER;
      break;
    case TabMutedReason::MEDIA_CAPTURE:
      info->reason = api::tabs::MUTED_INFO_REASON_CAPTURE;
      break;
    case TabMutedReason::EXTENSION:
      info->reason = api::tabs::MUTED_INFO_REASON_EXTENSION;
      info->extension_id.reset(
          new std::string(chrome::GetExtensionIdForMutedTab(contents)));
      break;
  }
  return info;
}

// static
void ExtensionTabUtil::SetPlatformDelegate(Delegate* delegate) {
  // Allow setting it only once (also allow reset to nullptr, but then take
  // special care to free it).
  CHECK(!g_extension_tab_util_delegate || !delegate);
  g_extension_tab_util_delegate = delegate;
}

// static
void ExtensionTabUtil::ScrubTabForExtension(const Extension* extension,
                                            content::WebContents* contents,
                                            api::tabs::Tab* tab) {
  bool has_permission = false;
  if (extension) {
    bool api_permission = false;
    std::string url;
    if (contents) {
      api_permission = extension->permissions_data()->HasAPIPermissionForTab(
          GetTabId(contents), APIPermission::kTab);
      url = contents->GetURL().spec();
    } else {
      api_permission =
          extension->permissions_data()->HasAPIPermission(APIPermission::kTab);
      url = *tab->url;
    }
    bool host_permission = extension->permissions_data()
                               ->active_permissions()
                               .HasExplicitAccessToOrigin(GURL(url));
    has_permission = api_permission || host_permission;
  }
  if (!has_permission) {
    tab->url.reset();
    tab->title.reset();
    tab->fav_icon_url.reset();
  }
  if (g_extension_tab_util_delegate)
    g_extension_tab_util_delegate->ScrubTabForExtension(extension, contents,
                                                        tab);
}

bool ExtensionTabUtil::GetTabStripModel(const WebContents* web_contents,
                                        TabStripModel** tab_strip_model,
                                        int* tab_index) {
  DCHECK(web_contents);
  DCHECK(tab_strip_model);
  DCHECK(tab_index);

  for (auto* browser : *BrowserList::GetInstance()) {
    TabStripModel* tab_strip = browser->tab_strip_model();
    int index = tab_strip->GetIndexOfWebContents(web_contents);
    if (index != -1) {
      *tab_strip_model = tab_strip;
      *tab_index = index;
      return true;
    }
  }

  return false;
}

bool ExtensionTabUtil::GetDefaultTab(Browser* browser,
                                     WebContents** contents,
                                     int* tab_id) {
  DCHECK(browser);
  DCHECK(contents);

  *contents = browser->tab_strip_model()->GetActiveWebContents();
  if (*contents) {
    if (tab_id)
      *tab_id = GetTabId(*contents);
    return true;
  }

  return false;
}

bool ExtensionTabUtil::GetTabById(int tab_id,
                                  content::BrowserContext* browser_context,
                                  bool include_incognito,
                                  Browser** browser,
                                  TabStripModel** tab_strip,
                                  WebContents** contents,
                                  int* tab_index) {
  if (tab_id == api::tabs::TAB_ID_NONE)
    return false;
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::GetTabById - Step 1";
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::GetTabById - Step 2 - IsOffTheRecordSessionActive: " << TabModelList::IsOffTheRecordSessionActive();
  TabModel *my_tab_strip = nullptr;
  if (!TabModelList::empty())
    my_tab_strip = *(TabModelList::begin());
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::GetTabById - Step 3 - TabModel: " << my_tab_strip;
  if (my_tab_strip) {
    for (int i = 0; i < my_tab_strip->GetTabCount(); ++i) {
        WebContents* target_contents = my_tab_strip->GetWebContentsAt(i);
        if (SessionTabHelper::IdForTab(target_contents).id() == tab_id) {
          if (contents)
            *contents = target_contents;
          if (tab_index)
            *tab_index = i;
//          LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::GetTabById - Step 4" << i;
          return true;
        }
    }
  }
//  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::GetTabById - Step 5";
  return false;
}

GURL ExtensionTabUtil::ResolvePossiblyRelativeURL(const std::string& url_string,
                                                  const Extension* extension) {
  GURL url = GURL(url_string);
  if (!url.is_valid())
    url = extension->GetResourceURL(url_string);

  return url;
}

bool ExtensionTabUtil::IsKillURL(const GURL& url) {
  static const char* const kill_hosts[] = {
      chrome::kChromeUICrashHost,         chrome::kChromeUIDelayedHangUIHost,
      chrome::kChromeUIHangUIHost,        chrome::kChromeUIKillHost,
      chrome::kChromeUIQuitHost,          chrome::kChromeUIRestartHost,
      content::kChromeUIBrowserCrashHost, content::kChromeUIMemoryExhaustHost,
  };

  // Check a fixed-up URL, to normalize the scheme and parse hosts correctly.
  GURL fixed_url =
      url_formatter::FixupURL(url.possibly_invalid_spec(), std::string());
  if (!fixed_url.SchemeIs(content::kChromeUIScheme))
    return false;

  base::StringPiece fixed_host = fixed_url.host_piece();
  for (size_t i = 0; i < arraysize(kill_hosts); ++i) {
    if (fixed_host == kill_hosts[i])
      return true;
  }

  return false;
}

void ExtensionTabUtil::CreateTab(std::unique_ptr<WebContents> web_contents,
                                 const std::string& extension_id,
                                 WindowOpenDisposition disposition,
                                 const gfx::Rect& initial_rect,
                                 bool user_gesture) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  Browser* browser = chrome::FindTabbedBrowser(profile, false);
  const bool browser_created = !browser;
  if (!browser) {
    Browser::CreateParams params = Browser::CreateParams(profile, user_gesture);
    browser = new Browser(params);
  }
  NavigateParams params(browser, std::move(web_contents));

  // The extension_app_id parameter ends up as app_name in the Browser
  // which causes the Browser to return true for is_app().  This affects
  // among other things, whether the location bar gets displayed.
  // TODO(mpcomplete): This seems wrong. What if the extension content is hosted
  // in a tab?
  if (disposition == WindowOpenDisposition::NEW_POPUP)
    params.extension_app_id = extension_id;

  params.disposition = disposition;
  params.window_bounds = initial_rect;
  params.window_action = NavigateParams::SHOW_WINDOW;
  params.user_gesture = user_gesture;
  Navigate(&params);

  // Close the browser if Navigate created a new one.
  if (browser_created && (browser != params.browser))
    browser->window()->Close();
}

// static
void ExtensionTabUtil::ForEachTab(
    const base::Callback<void(WebContents*)>& callback) {
  for (auto* web_contents : AllTabContentses())
    callback.Run(web_contents);
}

// static
WindowController* ExtensionTabUtil::GetWindowControllerOfTab(
    const WebContents* web_contents) {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  if (browser != nullptr)
    return browser->extension_window_controller();

  return nullptr;
}

bool ExtensionTabUtil::OpenOptionsPageFromAPI(
    const Extension* extension,
    content::BrowserContext* browser_context) {
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPageFromAPI - Step 1";
  if (!OptionsPageInfo::HasOptionsPage(extension))
    return false;
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPageFromAPI - Step 2";
  Profile* profile = Profile::FromBrowserContext(browser_context);
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPageFromAPI - Step 3";
  // This version of OpenOptionsPage() is only called when the extension
  // initiated the command via chrome.runtime.openOptionsPage. For a spanning
  // mode extension, this API could only be called from a regular profile, since
  // that's the only place it's running.
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPageFromAPI - Step 4";
  DCHECK(!profile->IsOffTheRecord() || IncognitoInfo::IsSplitMode(extension));
  Browser* browser = chrome::FindBrowserWithProfile(profile);
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPageFromAPI - Step 5: " << browser;
  if (!browser)
    browser = new Browser(Browser::CreateParams(profile, true));
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPageFromAPI - Step 6: " << browser;
  return extensions::ExtensionTabUtil::OpenOptionsPage(extension, browser);
}

bool ExtensionTabUtil::OpenOptionsPage(const Extension* extension,
                                       Browser* browser) {
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 1";
  if (!OptionsPageInfo::HasOptionsPage(extension))
    return false;

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 2";
  // Force the options page to open in non-OTR window if the extension is not
  // running in split mode, because it won't be able to save settings from OTR.
  // This version of OpenOptionsPage() can be called from an OTR window via e.g.
  // the action menu, since that's not initiated by the extension.
  std::unique_ptr<chrome::ScopedTabbedBrowserDisplayer> displayer;
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 3";
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 3a: " << browser;
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 3b: " << browser->profile();
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 3c: " << browser->profile()->IsOffTheRecord();
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 3d: " << IncognitoInfo::IsSplitMode(extension);
  if (browser->profile()->IsOffTheRecord() &&
      !IncognitoInfo::IsSplitMode(extension)) {
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 4";
    displayer.reset(new chrome::ScopedTabbedBrowserDisplayer(
        browser->profile()->GetOriginalProfile()));
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 5";
    browser = displayer->browser();
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 6";
  }

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 7";
  GURL url_to_navigate;
  bool open_in_tab = OptionsPageInfo::ShouldOpenInTab(extension);
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 8";
  if (open_in_tab) {
    // Options page tab is simply e.g. chrome-extension://.../options.html.
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 9";
    url_to_navigate = OptionsPageInfo::GetOptionsPage(extension);
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 10: " << url_to_navigate;
  } else {
    // Options page tab is Extension settings pointed at that Extension's ID,
    // e.g. chrome://extensions?options=...
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 11";
    url_to_navigate = GURL(chrome::kChromeUIExtensionsURL);
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 12: " << url_to_navigate;
    GURL::Replacements replacements;
    std::string query =
        base::StringPrintf("options=%s", extension->id().c_str());
    replacements.SetQueryStr(query);
    url_to_navigate = url_to_navigate.ReplaceComponents(replacements);
    LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 13: " << url_to_navigate;
  }

  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 14";
  NavigateParams params(
      GetSingletonTabNavigateParams(browser, url_to_navigate));
  // We need to respect path differences because we don't want opening the
  // options page to close a page that might be open to extension content.
  // However, if the options page opens inside the chrome://extensions page, we
  // can override an existing page.
  // Note: ref behavior is to ignore.
  params.path_behavior = open_in_tab ? NavigateParams::RESPECT
                                     : NavigateParams::IGNORE_AND_NAVIGATE;
  params.url = url_to_navigate;
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 15";
  ShowSingletonTabOverwritingNTP(browser, std::move(params));
  LOG(INFO) << "[EXTENSIONS] ExtensionTabUtil::OpenOptionsPage - Step 16";
  return true;
}

// static
bool ExtensionTabUtil::BrowserSupportsTabs(Browser* browser) {
  return browser && browser->tab_strip_model() && !browser->is_devtools();
}

}  // namespace extensions
