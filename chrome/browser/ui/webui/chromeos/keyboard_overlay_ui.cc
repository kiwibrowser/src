// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/keyboard_overlay_ui.h"

#include <stddef.h>

#include <memory>

#include "ash/public/cpp/ash_features.h"
#include "ash/shell.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/chromeos_switches.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/base/ime/chromeos/input_method_manager.h"
#include "ui/chromeos/events/keyboard_layout_util.h"
#include "ui/chromeos/events/modifier_key.h"
#include "ui/chromeos/events/pref_names.h"
#include "ui/display/manager/display_manager.h"

using content::WebUIMessageHandler;
using ui::chromeos::ModifierKey;
using ui::WebDialogUI;

namespace {

const char kLearnMoreURL[] =
#if defined(OFFICIAL_BUILD)
    "chrome-extension://honijodknafkokifofgiaalefdiedpko/"
    "main.html?answer=1047364";
#else
    "https://support.google.com/chromebook/answer/183101";
#endif

struct ModifierToLabel {
  const ModifierKey modifier;
  const char* label;
} kModifierToLabels[] = {
    {ModifierKey::kSearchKey, "search"},
    {ModifierKey::kControlKey, "ctrl"},
    {ModifierKey::kAltKey, "alt"},
    {ModifierKey::kVoidKey, "disabled"},
    {ModifierKey::kCapsLockKey, "caps lock"},
    {ModifierKey::kEscapeKey, "esc"},
    {ModifierKey::kBackspaceKey, "backspace"},
};

struct I18nContentToMessage {
  const char* i18n_content;
  int message;
} kI18nContentToMessage[] = {
    {"keyboardOverlayAssistantKeyLabel",
     IDS_KEYBOARD_OVERLAY_ASSISTANT_KEY_LABEL},
    {"keyboardOverlayPlayPauseKeyLabel",
     IDS_KEYBOARD_OVERLAY_PLAY_PAUSE_KEY_LABEL},
    {"keyboardOverlaySystemMenuKeyLabel",
     IDS_KEYBOARD_OVERLAY_SYSTEM_MENU_KEY_LABEL},
    {"keyboardOverlayLauncherKeyLabel",
     IDS_KEYBOARD_OVERLAY_LAUNCHER_KEY_LABEL},
    {"keyboardOverlayLearnMore", IDS_LEARN_MORE},
    {"keyboardOverlayTitle", IDS_KEYBOARD_OVERLAY_TITLE},
    {"keyboardOverlayEscKeyLabel", IDS_KEYBOARD_OVERLAY_ESC_KEY_LABEL},
    {"keyboardOverlayBackKeyLabel", IDS_KEYBOARD_OVERLAY_BACK_KEY_LABEL},
    {"keyboardOverlayForwardKeyLabel", IDS_KEYBOARD_OVERLAY_FORWARD_KEY_LABEL},
    {"keyboardOverlayReloadKeyLabel", IDS_KEYBOARD_OVERLAY_RELOAD_KEY_LABEL},
    {"keyboardOverlayFullScreenKeyLabel",
     IDS_KEYBOARD_OVERLAY_FULL_SCREEN_KEY_LABEL},
    {"keyboardOverlaySwitchWinKeyLabel",
     IDS_KEYBOARD_OVERLAY_SWITCH_WIN_KEY_LABEL},
    {"keyboardOverlayBrightDownKeyLabel",
     IDS_KEYBOARD_OVERLAY_BRIGHT_DOWN_KEY_LABEL},
    {"keyboardOverlayBrightUpKeyLabel",
     IDS_KEYBOARD_OVERLAY_BRIGHT_UP_KEY_LABEL},
    {"keyboardOverlayMuteKeyLabel", IDS_KEYBOARD_OVERLAY_MUTE_KEY_LABEL},
    {"keyboardOverlayVolDownKeyLabel", IDS_KEYBOARD_OVERLAY_VOL_DOWN_KEY_LABEL},
    {"keyboardOverlayVolUpKeyLabel", IDS_KEYBOARD_OVERLAY_VOL_UP_KEY_LABEL},
    {"keyboardOverlayPowerKeyLabel", IDS_KEYBOARD_OVERLAY_POWER_KEY_LABEL},
    {"keyboardOverlayBackspaceKeyLabel",
     IDS_KEYBOARD_OVERLAY_BACKSPACE_KEY_LABEL},
    {"keyboardOverlayTabKeyLabel", IDS_KEYBOARD_OVERLAY_TAB_KEY_LABEL},
    {"keyboardOverlaySearchKeyLabel", IDS_KEYBOARD_OVERLAY_SEARCH_KEY_LABEL},
    {"keyboardOverlayEnterKeyLabel", IDS_KEYBOARD_OVERLAY_ENTER_KEY_LABEL},
    {"keyboardOverlayShiftKeyLabel", IDS_KEYBOARD_OVERLAY_SHIFT_KEY_LABEL},
    {"keyboardOverlayCtrlKeyLabel", IDS_KEYBOARD_OVERLAY_CTRL_KEY_LABEL},
    {"keyboardOverlayAltKeyLabel", IDS_KEYBOARD_OVERLAY_ALT_KEY_LABEL},
    {"keyboardOverlayLeftKeyLabel", IDS_KEYBOARD_OVERLAY_LEFT_KEY_LABEL},
    {"keyboardOverlayRightKeyLabel", IDS_KEYBOARD_OVERLAY_RIGHT_KEY_LABEL},
    {"keyboardOverlayUpKeyLabel", IDS_KEYBOARD_OVERLAY_UP_KEY_LABEL},
    {"keyboardOverlayDownKeyLabel", IDS_KEYBOARD_OVERLAY_DOWN_KEY_LABEL},
    {"keyboardOverlayInstructionsHide", IDS_KEYBOARD_OVERLAY_INSTRUCTIONS_HIDE},
    {"keyboardOverlayActivateLastShelfItem",
     IDS_KEYBOARD_OVERLAY_ACTIVATE_LAST_SHELF_ITEM},
    {"keyboardOverlayActivateLastTab", IDS_KEYBOARD_OVERLAY_ACTIVATE_LAST_TAB},
    {"keyboardOverlayActivateShelfItem1",
     IDS_KEYBOARD_OVERLAY_ACTIVATE_SHELF_ITEM_1},
    {"keyboardOverlayActivateShelfItem2",
     IDS_KEYBOARD_OVERLAY_ACTIVATE_SHELF_ITEM_2},
    {"keyboardOverlayActivateShelfItem3",
     IDS_KEYBOARD_OVERLAY_ACTIVATE_SHELF_ITEM_3},
    {"keyboardOverlayActivateShelfItem4",
     IDS_KEYBOARD_OVERLAY_ACTIVATE_SHELF_ITEM_4},
    {"keyboardOverlayActivateShelfItem5",
     IDS_KEYBOARD_OVERLAY_ACTIVATE_SHELF_ITEM_5},
    {"keyboardOverlayActivateShelfItem6",
     IDS_KEYBOARD_OVERLAY_ACTIVATE_SHELF_ITEM_6},
    {"keyboardOverlayActivateShelfItem7",
     IDS_KEYBOARD_OVERLAY_ACTIVATE_SHELF_ITEM_7},
    {"keyboardOverlayActivateShelfItem8",
     IDS_KEYBOARD_OVERLAY_ACTIVATE_SHELF_ITEM_8},
    {"keyboardOverlayActivateNextTab", IDS_KEYBOARD_OVERLAY_ACTIVATE_NEXT_TAB},
    {"keyboardOverlayActivatePreviousTab",
     IDS_KEYBOARD_OVERLAY_ACTIVATE_PREVIOUS_TAB},
    {"keyboardOverlayActivateTab1", IDS_KEYBOARD_OVERLAY_ACTIVATE_TAB_1},
    {"keyboardOverlayActivateTab2", IDS_KEYBOARD_OVERLAY_ACTIVATE_TAB_2},
    {"keyboardOverlayActivateTab3", IDS_KEYBOARD_OVERLAY_ACTIVATE_TAB_3},
    {"keyboardOverlayActivateTab4", IDS_KEYBOARD_OVERLAY_ACTIVATE_TAB_4},
    {"keyboardOverlayActivateTab5", IDS_KEYBOARD_OVERLAY_ACTIVATE_TAB_5},
    {"keyboardOverlayActivateTab6", IDS_KEYBOARD_OVERLAY_ACTIVATE_TAB_6},
    {"keyboardOverlayActivateTab7", IDS_KEYBOARD_OVERLAY_ACTIVATE_TAB_7},
    {"keyboardOverlayActivateTab8", IDS_KEYBOARD_OVERLAY_ACTIVATE_TAB_8},
    {"keyboardOverlayAddWwwAndComAndOpenAddress",
     IDS_KEYBOARD_OVERLAY_ADD_WWW_AND_COM_AND_OPEN_ADDRESS},
    {"keyboardOverlayBookmarkAllTabs", IDS_KEYBOARD_OVERLAY_BOOKMARK_ALL_TABS},
    {"keyboardOverlayBookmarkCurrentPage",
     IDS_KEYBOARD_OVERLAY_BOOKMARK_CURRENT_PAGE},
    {"keyboardOverlayBookmarkManager", IDS_KEYBOARD_OVERLAY_BOOKMARK_MANAGER},
    {"keyboardOverlayCenterWindow", IDS_KEYBOARD_OVERLAY_CENTER_WINDOW},
    {"keyboardOverlayClearBrowsingDataDialog",
     IDS_KEYBOARD_OVERLAY_CLEAR_BROWSING_DATA_DIALOG},
    {"keyboardOverlayCloseTab", IDS_KEYBOARD_OVERLAY_CLOSE_TAB},
    {"keyboardOverlayCloseWindow", IDS_KEYBOARD_OVERLAY_CLOSE_WINDOW},
    {"keyboardOverlayContextMenu", IDS_KEYBOARD_OVERLAY_CONTEXT_MENU},
    {"keyboardOverlayCopy", IDS_KEYBOARD_OVERLAY_COPY},
    {"keyboardOverlayCut", IDS_KEYBOARD_OVERLAY_CUT},
    {"keyboardOverlayCycleThroughInputMethods",
     IDS_KEYBOARD_OVERLAY_CYCLE_THROUGH_INPUT_METHODS},
    {"keyboardOverlayDecreaseKeyBrightness",
     IDS_KEYBOARD_OVERLAY_DECREASE_KEY_BRIGHTNESS},
    {"keyboardOverlayDelete", IDS_KEYBOARD_OVERLAY_DELETE},
    {"keyboardOverlayDeleteWord", IDS_KEYBOARD_OVERLAY_DELETE_WORD},
    {"keyboardOverlayDeveloperTools", IDS_KEYBOARD_OVERLAY_DEVELOPER_TOOLS},
    {"keyboardOverlayDockWindowLeft", IDS_KEYBOARD_OVERLAY_DOCK_WINDOW_LEFT},
    {"keyboardOverlayDockWindowRight", IDS_KEYBOARD_OVERLAY_DOCK_WINDOW_RIGHT},
    {"keyboardOverlayDomInspector", IDS_KEYBOARD_OVERLAY_DOM_INSPECTOR},
    {"keyboardOverlayDownloads", IDS_KEYBOARD_OVERLAY_DOWNLOADS},
    {"keyboardOverlayEnd", IDS_KEYBOARD_OVERLAY_END},
    {"keyboardOverlayF1", IDS_KEYBOARD_OVERLAY_F1},
    {"keyboardOverlayF10", IDS_KEYBOARD_OVERLAY_F10},
    {"keyboardOverlayF11", IDS_KEYBOARD_OVERLAY_F11},
    {"keyboardOverlayF12", IDS_KEYBOARD_OVERLAY_F12},
    {"keyboardOverlayF2", IDS_KEYBOARD_OVERLAY_F2},
    {"keyboardOverlayF3", IDS_KEYBOARD_OVERLAY_F3},
    {"keyboardOverlayF4", IDS_KEYBOARD_OVERLAY_F4},
    {"keyboardOverlayF5", IDS_KEYBOARD_OVERLAY_F5},
    {"keyboardOverlayF6", IDS_KEYBOARD_OVERLAY_F6},
    {"keyboardOverlayF7", IDS_KEYBOARD_OVERLAY_F7},
    {"keyboardOverlayF8", IDS_KEYBOARD_OVERLAY_F8},
    {"keyboardOverlayF9", IDS_KEYBOARD_OVERLAY_F9},
    {"keyboardOverlayFindPreviousText",
     IDS_KEYBOARD_OVERLAY_FIND_PREVIOUS_TEXT},
    {"keyboardOverlayFindText", IDS_KEYBOARD_OVERLAY_FIND_TEXT},
    {"keyboardOverlayFindTextAgain", IDS_KEYBOARD_OVERLAY_FIND_TEXT_AGAIN},
    {"keyboardOverlayFocusAddressBar", IDS_KEYBOARD_OVERLAY_FOCUS_ADDRESS_BAR},
    {"keyboardOverlayFocusAddressBarInSearchMode",
     IDS_KEYBOARD_OVERLAY_FOCUS_ADDRESS_BAR_IN_SEARCH_MODE},
    {"keyboardOverlayFocusBookmarks", IDS_KEYBOARD_OVERLAY_FOCUS_BOOKMARKS},
    {"keyboardOverlayFocusShelf", IDS_KEYBOARD_OVERLAY_FOCUS_SHELF},
    {"keyboardOverlayFocusNextPane", IDS_KEYBOARD_OVERLAY_FOCUS_NEXT_PANE},
    {"keyboardOverlayFocusPreviousPane",
     IDS_KEYBOARD_OVERLAY_FOCUS_PREVIOUS_PANE},
    {"keyboardOverlayFocusToolbar", IDS_KEYBOARD_OVERLAY_FOCUS_TOOLBAR},
    {"keyboardOverlayGoBack", IDS_KEYBOARD_OVERLAY_GO_BACK},
    {"keyboardOverlayGoForward", IDS_KEYBOARD_OVERLAY_GO_FORWARD},
    {"keyboardOverlayHelp", IDS_KEYBOARD_OVERLAY_HELP},
    {"keyboardOverlayHistory", IDS_KEYBOARD_OVERLAY_HISTORY},
    {"keyboardOverlayHome", IDS_KEYBOARD_OVERLAY_HOME},
    {"keyboardOverlayIncreaseKeyBrightness",
     IDS_KEYBOARD_OVERLAY_INCREASE_KEY_BRIGHTNESS},
    {"keyboardOverlayInputUnicodeCharacters",
     IDS_KEYBOARD_OVERLAY_INPUT_UNICODE_CHARACTERS},
    {"keyboardOverlayInsert", IDS_KEYBOARD_OVERLAY_INSERT},
    {"keyboardOverlayJavascriptConsole",
     IDS_KEYBOARD_OVERLAY_JAVASCRIPT_CONSOLE},
    {"keyboardOverlayLockScreen", IDS_KEYBOARD_OVERLAY_LOCK_SCREEN},
    {"keyboardOverlayLockScreenOrPowerOff",
     IDS_KEYBOARD_OVERLAY_LOCK_SCREEN_OR_POWER_OFF},
    {"keyboardOverlayMagnifierDecreaseZoom",
     IDS_KEYBOARD_OVERLAY_MAGNIFIER_DECREASE_ZOOM},
    {"keyboardOverlayMagnifierIncreaseZoom",
     IDS_KEYBOARD_OVERLAY_MAGNIFIER_INCREASE_ZOOM},
    {"keyboardOverlayMaximizeWindow", IDS_KEYBOARD_OVERLAY_MAXIMIZE_WINDOW},
    {"keyboardOverlayMinimizeWindow", IDS_KEYBOARD_OVERLAY_MINIMIZE_WINDOW},
    {"keyboardOverlayMirrorMonitors", IDS_KEYBOARD_OVERLAY_MIRROR_MONITORS},
    // TODO(warx): keyboard overlay name for move window between displays
    // shortcuts need to be updated when new keyboard shortcuts helper is there.
    {"keyboardOverlayMoveActiveWindowBetweenDisplays",
     IDS_KEYBOARD_OVERLAY_MOVE_ACTIVE_WINDOW_BETWEEN_DISPLAYS},
    {"keyboardOverlayNewIncognitoWindow",
     IDS_KEYBOARD_OVERLAY_NEW_INCOGNITO_WINDOW},
    {"keyboardOverlayNewTab", IDS_KEYBOARD_OVERLAY_NEW_TAB},
    {"keyboardOverlayNewTerminal", IDS_KEYBOARD_OVERLAY_NEW_TERMINAL},
    {"keyboardOverlayNewWindow", IDS_KEYBOARD_OVERLAY_NEW_WINDOW},
    {"keyboardOverlayNextUser", IDS_KEYBOARD_OVERLAY_NEXT_USER},
    {"keyboardOverlayNextWindow", IDS_KEYBOARD_OVERLAY_NEXT_WINDOW},
    {"keyboardOverlayNextWord", IDS_KEYBOARD_OVERLAY_NEXT_WORD},
    {"keyboardOverlayOpen", IDS_KEYBOARD_OVERLAY_OPEN},
    {"keyboardOverlayOpenAddressInNewTab",
     IDS_KEYBOARD_OVERLAY_OPEN_ADDRESS_IN_NEW_TAB},
    {"keyboardOverlayOpenFileManager", IDS_KEYBOARD_OVERLAY_OPEN_FILE_MANAGER},
    {"keyboardOverlayPageDown", IDS_KEYBOARD_OVERLAY_PAGE_DOWN},
    {"keyboardOverlayPageUp", IDS_KEYBOARD_OVERLAY_PAGE_UP},
    {"keyboardOverlayPaste", IDS_KEYBOARD_OVERLAY_PASTE},
    {"keyboardOverlayPasteAsPlainText",
     IDS_KEYBOARD_OVERLAY_PASTE_AS_PLAIN_TEXT},
    {"keyboardOverlayPreviousUser", IDS_KEYBOARD_OVERLAY_PREVIOUS_USER},
    {"keyboardOverlayPreviousWindow", IDS_KEYBOARD_OVERLAY_PREVIOUS_WINDOW},
    {"keyboardOverlayPreviousWord", IDS_KEYBOARD_OVERLAY_PREVIOUS_WORD},
    {"keyboardOverlayPrint", IDS_KEYBOARD_OVERLAY_PRINT},
    {"keyboardOverlayReloadCurrentPage",
     IDS_KEYBOARD_OVERLAY_RELOAD_CURRENT_PAGE},
    {"keyboardOverlayReloadBypassingCache",
     IDS_KEYBOARD_OVERLAY_RELOAD_BYPASSING_CACHE},
    {"keyboardOverlayReopenLastClosedTab",
     IDS_KEYBOARD_OVERLAY_REOPEN_LAST_CLOSED_TAB},
    {"keyboardOverlayReportIssue", IDS_KEYBOARD_OVERLAY_REPORT_ISSUE},
    {"keyboardOverlayResetScreenZoom", IDS_KEYBOARD_OVERLAY_RESET_SCREEN_ZOOM},
    {"keyboardOverlayResetZoom", IDS_KEYBOARD_OVERLAY_RESET_ZOOM},
    {"keyboardOverlayRotateScreen", IDS_KEYBOARD_OVERLAY_ROTATE_SCREEN},
    {"keyboardOverlayRotateWindow", IDS_KEYBOARD_OVERLAY_ROTATE_WINDOW},
    {"keyboardOverlaySave", IDS_KEYBOARD_OVERLAY_SAVE},
    {"keyboardOverlayScreenshotRegion", IDS_KEYBOARD_OVERLAY_SCREENSHOT_REGION},
    {"keyboardOverlayScreenshotWindow", IDS_KEYBOARD_OVERLAY_SCREENSHOT_WINDOW},
    {"keyboardOverlayScrollUpOnePage", IDS_KEYBOARD_OVERLAY_SCROLL_UP_ONE_PAGE},
    {"keyboardOverlaySelectAll", IDS_KEYBOARD_OVERLAY_SELECT_ALL},
    {"keyboardOverlaySelectPreviousInputMethod",
     IDS_KEYBOARD_OVERLAY_SELECT_PREVIOUS_INPUT_METHOD},
    {"keyboardOverlaySelectWordAtATime",
     IDS_KEYBOARD_OVERLAY_SELECT_WORD_AT_A_TIME},
    {"keyboardOverlayShowImeBubble", IDS_KEYBOARD_OVERLAY_SHOW_IME_BUBBLE},
    {"keyboardOverlayShowMessageCenter",
     IDS_KEYBOARD_OVERLAY_SHOW_MESSAGE_CENTER},
    {"keyboardOverlayShowStatusMenu", IDS_KEYBOARD_OVERLAY_SHOW_STATUS_MENU},
    {"keyboardOverlayShowStylusTools", IDS_KEYBOARD_OVERLAY_SHOW_STYLUS_TOOLS},
    {"keyboardOverlayShowWrenchMenu", IDS_KEYBOARD_OVERLAY_SHOW_WRENCH_MENU},
    {"keyboardOverlaySignOut", IDS_KEYBOARD_OVERLAY_SIGN_OUT},
    {"keyboardOverlaySuspend", IDS_KEYBOARD_OVERLAY_SUSPEND},
    {"keyboardOverlaySwapPrimaryMonitor",
     IDS_KEYBOARD_OVERLAY_SWAP_PRIMARY_MONITOR},
    {"keyboardOverlayTakeScreenshot", IDS_KEYBOARD_OVERLAY_TAKE_SCREENSHOT},
    {"keyboardOverlayTaskManager", IDS_KEYBOARD_OVERLAY_TASK_MANAGER},
    {"keyboardOverlayToggleBookmarkBar",
     IDS_KEYBOARD_OVERLAY_TOGGLE_BOOKMARK_BAR},
    {"keyboardOverlayToggleCapsLock", IDS_KEYBOARD_OVERLAY_TOGGLE_CAPS_LOCK},
    {"keyboardOverlayDisableCapsLock", IDS_KEYBOARD_OVERLAY_DISABLE_CAPS_LOCK},
    {"keyboardOverlayToggleChromevoxSpokenFeedback",
     IDS_KEYBOARD_OVERLAY_TOGGLE_CHROMEVOX_SPOKEN_FEEDBACK},
    {"keyboardOverlayToggleDictation", IDS_KEYBOARD_OVERLAY_TOGGLE_DICTATION},
    {"keyboardOverlayToggleDockedMagnifier",
     IDS_KEYBOARD_OVERLAY_TOGGLE_DOCKED_MAGNIFIER},
    {"keyboardOverlayToggleFullscreenMagnifier",
     IDS_KEYBOARD_OVERLAY_TOGGLE_FULLSCREEN_MAGNIFIER},
    {"keyboardOverlayToggleHighContrastMode",
     IDS_KEYBOARD_OVERLAY_TOGGLE_HIGH_CONTRAST_MODE},
    {"keyboardOverlayToggleProjectionTouchHud",
     IDS_KEYBOARD_OVERLAY_TOGGLE_PROJECTION_TOUCH_HUD},
    {"keyboardOverlayTouchHudModeChange",
     IDS_KEYBOARD_OVERLAY_TOUCH_HUD_MODE_CHANGE},
    {"keyboardOverlayUndo", IDS_KEYBOARD_OVERLAY_UNDO},
    {"keyboardOverlayUnpin", IDS_KEYBOARD_OVERLAY_UNPIN},
    {"keyboardOverlayViewKeyboardOverlay",
     IDS_KEYBOARD_OVERLAY_VIEW_KEYBOARD_OVERLAY},
    {"keyboardOverlayViewSource", IDS_KEYBOARD_OVERLAY_VIEW_SOURCE},
    {"keyboardOverlayWordMove", IDS_KEYBOARD_OVERLAY_WORD_MOVE},
    {"keyboardOverlayZoomIn", IDS_KEYBOARD_OVERLAY_ZOOM_IN},
    {"keyboardOverlayZoomOut", IDS_KEYBOARD_OVERLAY_ZOOM_OUT},
    {"keyboardOverlayZoomScreenIn", IDS_KEYBOARD_OVERLAY_ZOOM_SCREEN_IN},
    {"keyboardOverlayZoomScreenOut", IDS_KEYBOARD_OVERLAY_ZOOM_SCREEN_OUT},
    {"keyboardOverlayVoiceInteraction",
     IDS_KEYBOARD_OVERLAY_VOICE_INTERACTION}};

bool TopRowKeysAreFunctionKeys(Profile* profile) {
  if (!profile)
    return false;

  const PrefService* prefs = profile->GetPrefs();
  return prefs ? prefs->GetBoolean(prefs::kLanguageSendFunctionKeys) : false;
}

std::string ModifierKeyToLabel(ModifierKey modifier) {
  for (size_t i = 0; i < arraysize(kModifierToLabels); ++i) {
    if (modifier == kModifierToLabels[i].modifier) {
      return kModifierToLabels[i].label;
    }
  }
  return "";
}

content::WebUIDataSource* CreateKeyboardOverlayUIHTMLSource(Profile* profile) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIKeyboardOverlayHost);

  for (size_t i = 0; i < arraysize(kI18nContentToMessage); ++i) {
    source->AddLocalizedString(kI18nContentToMessage[i].i18n_content,
                               kI18nContentToMessage[i].message);
  }

  // |kI18nContentToMessage| is a static array initialized before it's possible
  // to call ui::DeviceUsesKeyboardLayout2(), so we add the
  // |keyboardOverlayInstructions| string at runtime here.
  source->AddLocalizedString("keyboardOverlayInstructions",
                             ui::DeviceUsesKeyboardLayout2()
                                 ? IDS_KEYBOARD_OVERLAY_INSTRUCTIONS_LAYOUT2
                                 : IDS_KEYBOARD_OVERLAY_INSTRUCTIONS);

  source->AddString("keyboardOverlayLearnMoreURL",
                    base::UTF8ToUTF16(kLearnMoreURL));
  source->AddBoolean("keyboardOverlayHasChromeOSDiamondKey",
                     base::CommandLine::ForCurrentProcess()->HasSwitch(
                         chromeos::switches::kHasChromeOSDiamondKey));
  source->AddBoolean("keyboardOverlayTopRowKeysAreFunctionKeys",
                     TopRowKeysAreFunctionKeys(profile));
  source->AddBoolean("voiceInteractionEnabled",
                     chromeos::switches::IsVoiceInteractionEnabled());
  source->AddBoolean("displayMoveWindowAccelsEnabled",
                     ash::features::IsDisplayMoveWindowAccelsEnabled());
  source->AddBoolean("keyboardOverlayUsesLayout2",
                     ui::DeviceUsesKeyboardLayout2());
  ash::Shell* shell = ash::Shell::Get();
  display::DisplayManager* display_manager = shell->display_manager();
  source->AddBoolean("keyboardOverlayIsDisplayUIScalingEnabled",
                     display_manager->IsDisplayUIScalingEnabled());
  source->SetJsonPath("strings.js");
  source->AddResourcePath("keyboard_overlay.js", IDR_KEYBOARD_OVERLAY_JS);
  source->SetDefaultResource(IDR_KEYBOARD_OVERLAY_HTML);
  return source;
}

}  // namespace

// The handler for Javascript messages related to the "keyboardoverlay" view.
class KeyboardOverlayHandler
    : public WebUIMessageHandler,
      public base::SupportsWeakPtr<KeyboardOverlayHandler> {
 public:
  explicit KeyboardOverlayHandler(Profile* profile);
  ~KeyboardOverlayHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  // Called when the page requires the input method ID corresponding to the
  // current input method or keyboard layout during initialization.
  void GetInputMethodId(const base::ListValue* args);

  // Called when the page requres the information of modifier key remapping
  // during the initialization.
  void GetLabelMap(const base::ListValue* args);

  // Called when the learn more link is clicked.
  void OpenLearnMorePage(const base::ListValue* args);

  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardOverlayHandler);
};

////////////////////////////////////////////////////////////////////////////////
//
// KeyboardOverlayHandler
//
////////////////////////////////////////////////////////////////////////////////
KeyboardOverlayHandler::KeyboardOverlayHandler(Profile* profile)
    : profile_(profile) {
}

KeyboardOverlayHandler::~KeyboardOverlayHandler() {
}

void KeyboardOverlayHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getInputMethodId",
      base::BindRepeating(&KeyboardOverlayHandler::GetInputMethodId,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getLabelMap", base::BindRepeating(&KeyboardOverlayHandler::GetLabelMap,
                                         base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "openLearnMorePage",
      base::BindRepeating(&KeyboardOverlayHandler::OpenLearnMorePage,
                          base::Unretained(this)));
}

void KeyboardOverlayHandler::GetInputMethodId(const base::ListValue* args) {
  chromeos::input_method::InputMethodManager* manager =
      chromeos::input_method::InputMethodManager::Get();
  const chromeos::input_method::InputMethodDescriptor& descriptor =
      manager->GetActiveIMEState()->GetCurrentInputMethod();
  base::Value param(descriptor.id());
  web_ui()->CallJavascriptFunctionUnsafe("initKeyboardOverlayId", param);
}

void KeyboardOverlayHandler::GetLabelMap(const base::ListValue* args) {
  DCHECK(profile_);
  PrefService* pref_service = profile_->GetPrefs();
  using ModifierMap = std::map<ModifierKey, ModifierKey>;
  ModifierMap modifier_map;
  modifier_map[ModifierKey::kSearchKey] = static_cast<ModifierKey>(
      pref_service->GetInteger(prefs::kLanguageRemapSearchKeyTo));
  modifier_map[ModifierKey::kControlKey] = static_cast<ModifierKey>(
      pref_service->GetInteger(prefs::kLanguageRemapControlKeyTo));
  modifier_map[ModifierKey::kAltKey] = static_cast<ModifierKey>(
      pref_service->GetInteger(prefs::kLanguageRemapAltKeyTo));
  // TODO(mazda): Support prefs::kLanguageRemapCapsLockKeyTo once Caps Lock is
  // added to the overlay UI.

  base::DictionaryValue dict;
  for (ModifierMap::const_iterator i = modifier_map.begin();
       i != modifier_map.end(); ++i) {
    dict.SetString(ModifierKeyToLabel(i->first), ModifierKeyToLabel(i->second));
  }

  web_ui()->CallJavascriptFunctionUnsafe("initIdentifierMap", dict);
}

void KeyboardOverlayHandler::OpenLearnMorePage(const base::ListValue* args) {
  web_ui()->GetWebContents()->GetDelegate()->OpenURLFromTab(
      web_ui()->GetWebContents(),
      content::OpenURLParams(GURL(kLearnMoreURL), content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK, false));
}

////////////////////////////////////////////////////////////////////////////////
//
// KeyboardOverlayUI
//
////////////////////////////////////////////////////////////////////////////////

KeyboardOverlayUI::KeyboardOverlayUI(content::WebUI* web_ui)
    : WebDialogUI(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  web_ui->AddMessageHandler(std::make_unique<KeyboardOverlayHandler>(profile));

  // Set up the chrome://keyboardoverlay/ source.
  content::WebUIDataSource::Add(profile,
                                CreateKeyboardOverlayUIHTMLSource(profile));
}
