// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/browser_window_cocoa.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/download/download_shelf.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/metrics/browser_window_histogram_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/signin/chrome_signin_helper.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window_state.h"
#include "chrome/browser/ui/cocoa/autofill/save_card_bubble_view_views.h"
#import "chrome/browser/ui/cocoa/browser/exclusive_access_controller_views.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_utils.h"
#import "chrome/browser/ui/cocoa/chrome_event_processing_window.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"
#import "chrome/browser/ui/cocoa/download/download_shelf_controller.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"
#include "chrome/browser/ui/cocoa/find_bar/find_bar_bridge.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#include "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/nsmenuitem_additions.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_base_controller.h"
#include "chrome/browser/ui/cocoa/restart_browser.h"
#include "chrome/browser/ui/cocoa/status_bubble_mac.h"
#include "chrome/browser/ui/cocoa/task_manager_mac.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/profile_chooser_constants.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/translate/core/browser/language_state.h"
#include "content/public/browser/keyboard_event_processing_result.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/rect.h"

#if BUILDFLAG(ENABLE_ONE_CLICK_SIGNIN)
#import "chrome/browser/ui/cocoa/one_click_signin_dialog_controller.h"
#endif

using content::NativeWebKeyboardEvent;
using content::WebContents;

BrowserWindowCocoa::BrowserWindowCocoa(Browser* browser,
                                       BrowserWindowController* controller)
  : browser_(browser),
    controller_(controller),
    initial_show_state_(ui::SHOW_STATE_DEFAULT),
    attention_request_id_(0) {

  gfx::Rect bounds;
  chrome::GetSavedWindowBoundsAndShowState(browser_,
                                           &bounds,
                                           &initial_show_state_);
}

BrowserWindowCocoa::~BrowserWindowCocoa() {
}

void BrowserWindowCocoa::Show() {
  // The Browser associated with this browser window must become the active
  // browser at the time |Show()| is called. This is the natural behaviour under
  // Windows, but |-makeKeyAndOrderFront:| won't send |-windowDidBecomeMain:|
  // until we return to the runloop. Therefore any calls to
  // |chrome::FindLastActive| will return the previous browser instead if we
  // don't explicitly set it here.
  BrowserList::SetLastActive(browser_);

  bool is_session_restore = browser_->is_session_restore();
  NSWindowAnimationBehavior saved_animation_behavior =
      NSWindowAnimationBehaviorDefault;
  bool did_save_animation_behavior = false;
  // Turn off swishing when restoring windows or showing an app.
  if ((is_session_restore || browser_->is_app()) &&
      [window() respondsToSelector:@selector(animationBehavior)] &&
      [window() respondsToSelector:@selector(setAnimationBehavior:)]) {
    did_save_animation_behavior = true;
    saved_animation_behavior = [window() animationBehavior];
    [window() setAnimationBehavior:NSWindowAnimationBehaviorNone];
  }

  {
    TRACE_EVENT0("ui", "BrowserWindowCocoa::Show makeKeyAndOrderFront");
    // This call takes up a substantial part of startup time, and an even more
    // substantial part of startup time when any CALayers are part of the
    // window's NSView heirarchy.
    [window() makeKeyAndOrderFront:controller_];

    // At this point all the Browser's UI is painted on the screen. There's no
    // need to wait for the compositor, so pass the nullptr instead and don't
    // store the returned instance.
    BrowserWindowHistogramHelper::
        MaybeRecordValueAndCreateInstanceOnBrowserPaint(nullptr);
  }

  // When creating windows from nibs it is necessary to |makeKeyAndOrderFront:|
  // prior to |orderOut:| then |miniaturize:| when restoring windows in the
  // minimized state.
  if (initial_show_state_ == ui::SHOW_STATE_MINIMIZED) {
    [window() orderOut:controller_];
    [window() miniaturize:controller_];
  } else if (initial_show_state_ == ui::SHOW_STATE_FULLSCREEN) {
    chrome::ToggleFullscreenMode(browser_);
  }
  initial_show_state_ = ui::SHOW_STATE_DEFAULT;

  // Restore window animation behavior.
  if (did_save_animation_behavior)
    [window() setAnimationBehavior:saved_animation_behavior];

  browser_->OnWindowDidShow();
}

void BrowserWindowCocoa::ShowInactive() {
  [window() orderFront:controller_];
}

void BrowserWindowCocoa::Hide() {
  [window() orderOut:controller_];
}

bool BrowserWindowCocoa::IsVisible() const {
  return [window() isVisible];
}

void BrowserWindowCocoa::SetBounds(const gfx::Rect& bounds) {
  gfx::Rect real_bounds = [controller_ enforceMinWindowSize:bounds];

  GetExclusiveAccessContext()->ExitFullscreen();
  NSRect cocoa_bounds = NSMakeRect(real_bounds.x(), 0,
                                   real_bounds.width(),
                                   real_bounds.height());
  // Flip coordinates based on the primary screen.
  NSScreen* screen = [[NSScreen screens] firstObject];
  cocoa_bounds.origin.y =
      NSHeight([screen frame]) - real_bounds.height() - real_bounds.y();

  [window() setFrame:cocoa_bounds display:YES];
}

// Callers assume that this doesn't immediately delete the Browser object.
// The controller implementing the window delegate methods called from
// |-performClose:| must take precautions to ensure that.
void BrowserWindowCocoa::Close() {
  // If there is an overlay window, we contain a tab being dragged between
  // windows. Don't hide the window as it makes the UI extra confused. We can
  // still close the window, as that will happen when the drag completes.
  if ([controller_ overlayWindow]) {
    [controller_ deferPerformClose];
  } else {
    // Using |-performClose:| can prevent the window from actually closing if
    // a JavaScript beforeunload handler opens an alert during shutdown, as
    // documented at <http://crbug.com/118424>. Re-implement
    // -[NSWindow performClose:] as closely as possible to how Apple documents
    // it.
    //
    // Before calling |-close|, hide the window immediately. |-performClose:|
    // would do something similar, and this ensures that the window is removed
    // from AppKit's display list. Not doing so can lead to crashes like
    // <http://crbug.com/156101>.
    id<NSWindowDelegate> delegate = [window() delegate];
    SEL window_should_close = @selector(windowShouldClose:);
    if ([delegate respondsToSelector:window_should_close]) {
      if ([delegate windowShouldClose:window()]) {
        [window() orderOut:nil];
        [window() close];
      }
    } else if ([window() respondsToSelector:window_should_close]) {
      if ([window() performSelector:window_should_close withObject:window()]) {
        [window() orderOut:nil];
        [window() close];
      }
    } else {
      [window() orderOut:nil];
      [window() close];
    }
  }
}

void BrowserWindowCocoa::Activate() {
  [controller_ activate];
}

void BrowserWindowCocoa::Deactivate() {
  // TODO(jcivelli): http://crbug.com/51364 Implement me.
  NOTIMPLEMENTED();
}

void BrowserWindowCocoa::FlashFrame(bool flash) {
  if (flash) {
    attention_request_id_ = [NSApp requestUserAttention:NSInformationalRequest];
  } else {
    [NSApp cancelUserAttentionRequest:attention_request_id_];
    attention_request_id_ = 0;
  }
}

bool BrowserWindowCocoa::IsAlwaysOnTop() const {
  return false;
}

void BrowserWindowCocoa::SetAlwaysOnTop(bool always_on_top) {
  // Not implemented for browser windows.
  NOTIMPLEMENTED();
}

bool BrowserWindowCocoa::IsActive() const {
  return [window() isKeyWindow];
}

gfx::NativeWindow BrowserWindowCocoa::GetNativeWindow() const {
  return window();
}

StatusBubble* BrowserWindowCocoa::GetStatusBubble() {
  return [controller_ statusBubble];
}

void BrowserWindowCocoa::UpdateTitleBar() {
  NSString* newTitle = WindowTitle();

  pending_window_title_.reset([BrowserWindowUtils
      scheduleReplaceOldTitle:pending_window_title_.get()
                 withNewTitle:newTitle
                    forWindow:window()]);
}

NSString* BrowserWindowCocoa::WindowTitle() {
  const bool include_app_name = true;
  if (alert_state_ == TabAlertState::AUDIO_PLAYING) {
    return l10n_util::GetNSStringF(IDS_WINDOW_AUDIO_PLAYING_MAC,
                                   browser_->GetWindowTitleForCurrentTab(
                                       include_app_name),
                                   base::SysNSStringToUTF16(@"🔊"));
  } else if (alert_state_ == TabAlertState::AUDIO_MUTING) {
    return l10n_util::GetNSStringF(IDS_WINDOW_AUDIO_MUTING_MAC,
                                   browser_->GetWindowTitleForCurrentTab(
                                       include_app_name),
                                   base::SysNSStringToUTF16(@"🔇"));
  }
  return base::SysUTF16ToNSString(
      browser_->GetWindowTitleForCurrentTab(include_app_name));
}

bool BrowserWindowCocoa::IsToolbarShowing() const {
  if (!IsFullscreen())
    return true;

  return [cocoa_controller() isToolbarShowing] == YES;
}

void BrowserWindowCocoa::BookmarkBarStateChanged(
    BookmarkBar::AnimateChangeType change_type) {
  [[controller_ bookmarkBarController]
      updateState:browser_->bookmark_bar_state()
       changeType:change_type];
}

void BrowserWindowCocoa::UpdateDevTools() {
  [controller_ updateDevToolsForContents:
      browser_->tab_strip_model()->GetActiveWebContents()];
}

void BrowserWindowCocoa::UpdateLoadingAnimations(bool should_animate) {
  // Do nothing on Mac.
}

void BrowserWindowCocoa::SetStarredState(bool is_starred) {
  [controller_ setStarredState:is_starred];
}

void BrowserWindowCocoa::SetTranslateIconToggled(bool is_lit) {
  [controller_ setCurrentPageIsTranslated:is_lit];
}

void BrowserWindowCocoa::OnActiveTabChanged(content::WebContents* old_contents,
                                            content::WebContents* new_contents,
                                            int index,
                                            int reason) {
  [controller_ onActiveTabChanged:old_contents to:new_contents];
  // TODO(pkasting): Perhaps the code in
  // TabStripController::activateTabWithContents should move here?  Or this
  // should call that (instead of TabStripModelObserverBridge doing so)?  It's
  // not obvious to me why Mac doesn't handle tab changes in BrowserWindow the
  // way views and GTK do.
  // See http://crbug.com/340720 for discussion.
}

void BrowserWindowCocoa::ZoomChangedForActiveTab(bool can_show_bubble) {
  [controller_ zoomChangedForActiveTab:can_show_bubble ? YES : NO];
}

gfx::Rect BrowserWindowCocoa::GetRestoredBounds() const {
  // Flip coordinates based on the primary screen.
  NSScreen* screen = [[NSScreen screens] firstObject];
  NSRect frame = [controller_ regularWindowFrame];
  gfx::Rect bounds(frame.origin.x, 0, NSWidth(frame), NSHeight(frame));
  bounds.set_y(NSHeight([screen frame]) - NSMaxY(frame));
  return bounds;
}

std::string BrowserWindowCocoa::GetWorkspace() const {
  return std::string();
}

bool BrowserWindowCocoa::IsVisibleOnAllWorkspaces() const {
  return false;
}

ui::WindowShowState BrowserWindowCocoa::GetRestoredState() const {
  if (IsMaximized())
    return ui::SHOW_STATE_MAXIMIZED;
  if (IsMinimized())
    return ui::SHOW_STATE_MINIMIZED;
  return ui::SHOW_STATE_NORMAL;
}

gfx::Rect BrowserWindowCocoa::GetBounds() const {
  return GetRestoredBounds();
}

gfx::Size BrowserWindowCocoa::GetContentsSize() const {
  NSView* view = [[controller_ overlayableContentsController] view];
  const NSSize size = [view bounds].size;
  return gfx::Size(NSSizeToCGSize(size));
}

bool BrowserWindowCocoa::IsMaximized() const {
  // -isZoomed returns YES if the window's frame equals the rect returned by
  // -windowWillUseStandardFrame:defaultFrame:, even if the window is in the
  // dock, so have to explicitly check for miniaturization state first.
  return ![window() isMiniaturized] && [window() isZoomed];
}

bool BrowserWindowCocoa::IsMinimized() const {
  return [window() isMiniaturized];
}

void BrowserWindowCocoa::Maximize() {
  // Zoom toggles so only call if not already maximized.
  if (!IsMaximized())
    [window() zoom:controller_];
}

void BrowserWindowCocoa::Minimize() {
  [window() miniaturize:controller_];
}

void BrowserWindowCocoa::Restore() {
  if (IsMaximized())
    [window() zoom:controller_];  // Toggles zoom mode.
  else if (IsMinimized())
    [window() deminiaturize:controller_];
}

bool BrowserWindowCocoa::ShouldHideUIForFullscreen() const {
  // On Mac, fullscreen mode has most normal things (in a slide-down panel).
  return false;
}

bool BrowserWindowCocoa::IsFullscreen() const {
  return [controller_ isInAnyFullscreenMode];
}

bool BrowserWindowCocoa::IsFullscreenBubbleVisible() const {
  return false;  // Currently only called from toolkit-views page_info.
}

PageActionIconContainer* BrowserWindowCocoa::GetPageActionIconContainer() {
  return [controller_ locationBarBridge];
}

LocationBar* BrowserWindowCocoa::GetLocationBar() const {
  return [controller_ locationBarBridge];
}

void BrowserWindowCocoa::SetFocusToLocationBar(bool select_all) {
  [controller_ focusLocationBar:select_all ? YES : NO];
}

void BrowserWindowCocoa::UpdateReloadStopState(bool is_loading, bool force) {
  [controller_ setIsLoading:is_loading force:force];
}

void BrowserWindowCocoa::UpdateToolbar(content::WebContents* contents) {
  [controller_ updateToolbarWithContents:contents];
}

void BrowserWindowCocoa::ResetToolbarTabState(content::WebContents* contents) {
  [controller_ resetTabState:contents];
}

void BrowserWindowCocoa::FocusToolbar() {
  // Not needed on the Mac.
}

ToolbarActionsBar* BrowserWindowCocoa::GetToolbarActionsBar() {
  if ([controller_ hasToolbar])
    return [[[controller_ toolbarController] browserActionsController]
               toolbarActionsBar];
  return nullptr;
}

void BrowserWindowCocoa::ToolbarSizeChanged(bool is_animating) {
  // Not needed on the Mac.
}

void BrowserWindowCocoa::FocusAppMenu() {
  // Chrome uses the standard Mac OS X menu bar, so this isn't needed.
}

void BrowserWindowCocoa::RotatePaneFocus(bool forwards) {
  // Not needed on the Mac.
}

void BrowserWindowCocoa::FocusBookmarksToolbar() {
  // Not needed on the Mac.
}

void BrowserWindowCocoa::FocusInactivePopupForAccessibility() {
  // Not needed on the Mac.
}

bool BrowserWindowCocoa::IsBookmarkBarVisible() const {
  return browser_->profile()->GetPrefs()->GetBoolean(
      bookmarks::prefs::kShowBookmarkBar);
}

bool BrowserWindowCocoa::IsBookmarkBarAnimating() const {
  return [controller_ isBookmarkBarAnimating];
}

bool BrowserWindowCocoa::IsTabStripEditable() const {
  return ![controller_ isDragSessionActive];
}

bool BrowserWindowCocoa::IsToolbarVisible() const {
  return browser_->SupportsWindowFeature(Browser::FEATURE_TOOLBAR) ||
         browser_->SupportsWindowFeature(Browser::FEATURE_LOCATIONBAR);
}

void BrowserWindowCocoa::AddFindBar(
    FindBarCocoaController* find_bar_cocoa_controller) {
  [controller_ addFindBar:find_bar_cocoa_controller];
}

void BrowserWindowCocoa::UpdateAlertState(TabAlertState alert_state) {
  alert_state_ = alert_state;
  UpdateTitleBar();
}

void BrowserWindowCocoa::ShowUpdateChromeDialog() {
  if (chrome::ShowPilotDialogsWithViewsToolkit()) {
    chrome::ShowUpdateChromeDialogViews(GetNativeWindow());
  } else {
    restart_browser::RequestRestart(window());
  }
}

void BrowserWindowCocoa::ShowBookmarkBubble(const GURL& url,
                                            bool already_bookmarked) {
  [controller_ showBookmarkBubbleForURL:url
                      alreadyBookmarked:(already_bookmarked ? YES : NO)];
}

autofill::SaveCardBubbleView* BrowserWindowCocoa::ShowSaveCreditCardBubble(
    content::WebContents* web_contents,
    autofill::SaveCardBubbleController* controller,
    bool user_gesture) {
  return autofill::CreateSaveCardBubbleView(web_contents, controller,
                                            controller_, user_gesture);
}

ShowTranslateBubbleResult BrowserWindowCocoa::ShowTranslateBubble(
    content::WebContents* contents,
    translate::TranslateStep step,
    translate::TranslateErrors::Type error_type,
    bool is_user_gesture) {
  ChromeTranslateClient* chrome_translate_client =
      ChromeTranslateClient::FromWebContents(contents);
  translate::LanguageState& language_state =
      chrome_translate_client->GetLanguageState();
  language_state.SetTranslateEnabled(true);

  [controller_ showTranslateBubbleForWebContents:contents
                                            step:step
                                       errorType:error_type];

  return ShowTranslateBubbleResult::SUCCESS;
}

#if BUILDFLAG(ENABLE_ONE_CLICK_SIGNIN)
void BrowserWindowCocoa::ShowOneClickSigninConfirmation(
    const base::string16& email,
    const StartSyncCallback& start_sync_callback) {
  // Deletes itself when the dialog closes.
  new OneClickSigninDialogController(
      browser_->tab_strip_model()->GetActiveWebContents(), start_sync_callback,
      email);
}
#endif

bool BrowserWindowCocoa::IsDownloadShelfVisible() const {
  return [controller_ isDownloadShelfVisible] != NO;
}

DownloadShelf* BrowserWindowCocoa::GetDownloadShelf() {
  [controller_ createAndAddDownloadShelf];
  DownloadShelfController* shelfController = [controller_ downloadShelf];
  return [shelfController bridge];
}

// We allow closing the window here since the real quit decision on Mac is made
// in [AppController quit:].
void BrowserWindowCocoa::ConfirmBrowserCloseWithPendingDownloads(
      int download_count,
      Browser::DownloadClosePreventionType dialog_type,
      bool app_modal,
      const base::Callback<void(bool)>& callback) {
  callback.Run(true);
}

void BrowserWindowCocoa::UserChangedTheme() {
  [controller_ userChangedTheme];
  LocationBarViewMac* locationBar = [controller_ locationBarBridge];
  if (locationBar) {
    locationBar->OnThemeChanged();
  }
}

void BrowserWindowCocoa::ShowAppMenu() {
  // No-op. Mac doesn't support showing the menus via alt keys.
}

content::KeyboardEventProcessingResult
BrowserWindowCocoa::PreHandleKeyboardEvent(
    const NativeWebKeyboardEvent& event) {
  // Handle ESC to dismiss permission bubbles, but still forward it
  // to the window afterwards.
  if (event.windows_key_code == ui::VKEY_ESCAPE)
    [controller_ dismissPermissionBubble];

  if (![BrowserWindowUtils shouldHandleKeyboardEvent:event])
    return content::KeyboardEventProcessingResult::NOT_HANDLED;

  if (event.GetType() == blink::WebInputEvent::kRawKeyDown &&
      [controller_
          handledByExtensionCommand:event.os_event
                           priority:ui::AcceleratorManager::kHighPriority])
    return content::KeyboardEventProcessingResult::HANDLED;

  int id = [BrowserWindowUtils getCommandId:event];
  if (id == -1)
    return content::KeyboardEventProcessingResult::NOT_HANDLED;

  if (browser_->command_controller()->IsReservedCommandOrKey(id, event)) {
    using Result = content::KeyboardEventProcessingResult;
    return [BrowserWindowUtils handleKeyboardEvent:event.os_event
                                          inWindow:window()]
               ? Result::HANDLED
               : Result::NOT_HANDLED;
  }

  return content::KeyboardEventProcessingResult::NOT_HANDLED_IS_SHORTCUT;
}

void BrowserWindowCocoa::HandleKeyboardEvent(
    const NativeWebKeyboardEvent& event) {
  if ([BrowserWindowUtils shouldHandleKeyboardEvent:event]) {
    if (![BrowserWindowUtils handleKeyboardEvent:event.os_event
                                        inWindow:window()]) {

      // TODO(spqchan): This is a temporary fix for exit extension fullscreen.
      // A priority system for exiting extension fullscreen when there is a
      // conflict is being experimented. See Issue 536047.
      if (event.windows_key_code == ui::VKEY_ESCAPE)
        [controller_ exitExtensionFullscreenIfPossible];
    }
  }
}

void BrowserWindowCocoa::CutCopyPaste(int command_id) {
  if (command_id == IDC_CUT)
    [NSApp sendAction:@selector(cut:) to:nil from:nil];
  else if (command_id == IDC_COPY)
    [NSApp sendAction:@selector(copy:) to:nil from:nil];
  else
    [NSApp sendAction:@selector(paste:) to:nil from:nil];
}

WindowOpenDisposition BrowserWindowCocoa::GetDispositionForPopupBounds(
    const gfx::Rect& bounds) {
  // When using Cocoa's System Fullscreen mode, convert popups into tabs.
  if ([controller_ isInAppKitFullscreen])
    return WindowOpenDisposition::NEW_FOREGROUND_TAB;
  return WindowOpenDisposition::NEW_POPUP;
}

FindBar* BrowserWindowCocoa::CreateFindBar() {
  // We could push the AddFindBar() call into the FindBarBridge
  // constructor or the FindBarCocoaController init, but that makes
  // unit testing difficult, since we would also require a
  // BrowserWindow object.
  FindBarBridge* bridge = new FindBarBridge(browser_);
  AddFindBar(bridge->find_bar_cocoa_controller());
  return bridge;
}

web_modal::WebContentsModalDialogHost*
    BrowserWindowCocoa::GetWebContentsModalDialogHost() {
  DCHECK([controller_ window]);
  ConstrainedWindowSheetController* sheet_controller =
      [ConstrainedWindowSheetController
          controllerForParentWindow:[controller_ window]];
  DCHECK(sheet_controller);
  return [sheet_controller dialogHost];
}

extensions::ActiveTabPermissionGranter*
    BrowserWindowCocoa::GetActiveTabPermissionGranter() {
  WebContents* web_contents =
      browser_->tab_strip_model()->GetActiveWebContents();
  if (!web_contents)
    return NULL;
  extensions::TabHelper* tab_helper =
      extensions::TabHelper::FromWebContents(web_contents);
  return tab_helper ? tab_helper->active_tab_permission_granter() : NULL;
}

void BrowserWindowCocoa::DestroyBrowser() {
  [controller_ destroyBrowser];

  // at this point the controller is dead (autoreleased), so
  // make sure we don't try to reference it any more.
}

NSWindow* BrowserWindowCocoa::window() const {
  return [controller_ window];
}

void BrowserWindowCocoa::ShowAvatarBubbleFromAvatarButton(
    AvatarBubbleMode mode,
    const signin::ManageAccountsParams& manage_accounts_params,
    signin_metrics::AccessPoint access_point,
    bool is_source_keyboard) {
  profiles::BubbleViewMode bubble_view_mode;
  profiles::BubbleViewModeFromAvatarBubbleMode(mode, &bubble_view_mode);

  if (SigninViewController::ShouldShowSigninForMode(bubble_view_mode)) {
    browser_->signin_view_controller()->ShowSignin(bubble_view_mode, browser_,
                                                   access_point);
  } else {
    AvatarBaseController* controller = [controller_ avatarButtonController];
    NSView* anchor = [controller buttonView];
    if ([anchor isHiddenOrHasHiddenAncestor])
      anchor = [[controller_ toolbarController] appMenuButton];
    [controller showAvatarBubbleAnchoredAt:anchor
                                  withMode:mode
                           withServiceType:manage_accounts_params.service_type
                           fromAccessPoint:access_point];
  }
}

int
BrowserWindowCocoa::GetRenderViewHeightInsetWithDetachedBookmarkBar() {
  if (browser_->bookmark_bar_state() != BookmarkBar::DETACHED)
    return 0;
  return GetCocoaLayoutConstant(BOOKMARK_BAR_NTP_HEIGHT);
}

void BrowserWindowCocoa::ExecuteExtensionCommand(
    const extensions::Extension* extension,
    const extensions::Command& command) {
  [cocoa_controller() executeExtensionCommand:extension->id() command:command];
}

ExclusiveAccessContext* BrowserWindowCocoa::GetExclusiveAccessContext() {
  return [controller_ exclusiveAccessController];
}

void BrowserWindowCocoa::ShowImeWarningBubble(
    const extensions::Extension* extension,
    const base::Callback<void(ImeWarningBubblePermissionStatus status)>&
        callback) {
  NOTREACHED() << "The IME warning bubble is unsupported on this platform.";
}
