// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_COCOA_H_

#include "base/mac/scoped_nsobject.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/extensions/extension_keybinding_registry.h"
#include "chrome/browser/signin/chrome_signin_helper.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/common/buildflags.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "ui/base/ui_base_types.h"

class Browser;
@class BrowserWindowController;
@class FindBarCocoaController;
@class NSEvent;
@class NSMenu;
@class NSWindow;

namespace extensions {
class ActiveTabPermissionGranter;
class Command;
class Extension;
}

// An implementation of BrowserWindow for Cocoa. Bridges between C++ and
// the Cocoa NSWindow. Cross-platform code will interact with this object when
// it needs to manipulate the window.

class BrowserWindowCocoa
    : public BrowserWindow,
      public extensions::ExtensionKeybindingRegistry::Delegate {
 public:
  BrowserWindowCocoa(Browser* browser,
                     BrowserWindowController* controller);
  ~BrowserWindowCocoa() override;

  // Overridden from BrowserWindow
  void Show() override;
  void ShowInactive() override;
  void Hide() override;
  bool IsVisible() const override;
  void SetBounds(const gfx::Rect& bounds) override;
  void Close() override;
  void Activate() override;
  void Deactivate() override;
  bool IsActive() const override;
  void FlashFrame(bool flash) override;
  bool IsAlwaysOnTop() const override;
  void SetAlwaysOnTop(bool always_on_top) override;
  gfx::NativeWindow GetNativeWindow() const override;
  StatusBubble* GetStatusBubble() override;
  void UpdateTitleBar() override;
  void BookmarkBarStateChanged(
      BookmarkBar::AnimateChangeType change_type) override;
  void UpdateDevTools() override;
  void UpdateLoadingAnimations(bool should_animate) override;
  void SetStarredState(bool is_starred) override;
  void SetTranslateIconToggled(bool is_lit) override;
  void OnActiveTabChanged(content::WebContents* old_contents,
                          content::WebContents* new_contents,
                          int index,
                          int reason) override;
  void ZoomChangedForActiveTab(bool can_show_bubble) override;
  gfx::Rect GetRestoredBounds() const override;
  std::string GetWorkspace() const override;
  bool IsVisibleOnAllWorkspaces() const override;
  ui::WindowShowState GetRestoredState() const override;
  gfx::Rect GetBounds() const override;
  gfx::Size GetContentsSize() const override;
  bool IsMaximized() const override;
  bool IsMinimized() const override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  bool ShouldHideUIForFullscreen() const override;
  bool IsFullscreen() const override;
  bool IsFullscreenBubbleVisible() const override;
  PageActionIconContainer* GetPageActionIconContainer() override;
  LocationBar* GetLocationBar() const override;
  void SetFocusToLocationBar(bool select_all) override;
  void UpdateReloadStopState(bool is_loading, bool force) override;
  void UpdateToolbar(content::WebContents* contents) override;
  void ResetToolbarTabState(content::WebContents* contents) override;
  void FocusToolbar() override;
  ToolbarActionsBar* GetToolbarActionsBar() override;
  void ToolbarSizeChanged(bool is_animating) override;
  void FocusAppMenu() override;
  void FocusBookmarksToolbar() override;
  void FocusInactivePopupForAccessibility() override;
  void RotatePaneFocus(bool forwards) override;
  bool IsBookmarkBarVisible() const override;
  bool IsBookmarkBarAnimating() const override;
  bool IsTabStripEditable() const override;
  bool IsToolbarVisible() const override;
  bool IsToolbarShowing() const override;
  void ShowUpdateChromeDialog() override;
  void ShowBookmarkBubble(const GURL& url, bool already_bookmarked) override;
  autofill::SaveCardBubbleView* ShowSaveCreditCardBubble(
      content::WebContents* contents,
      autofill::SaveCardBubbleController* controller,
      bool user_gesture) override;
  ShowTranslateBubbleResult ShowTranslateBubble(
      content::WebContents* contents,
      translate::TranslateStep step,
      translate::TranslateErrors::Type error_type,
      bool is_user_gesture) override;
#if BUILDFLAG(ENABLE_ONE_CLICK_SIGNIN)
  void ShowOneClickSigninConfirmation(
      const base::string16& email,
      const StartSyncCallback& start_sync_callback) override;
#endif
  bool IsDownloadShelfVisible() const override;
  DownloadShelf* GetDownloadShelf() override;
  void ConfirmBrowserCloseWithPendingDownloads(
      int download_count,
      Browser::DownloadClosePreventionType dialog_type,
      bool app_modal,
      const base::Callback<void(bool)>& callback) override;
  void UserChangedTheme() override;
  void ShowAppMenu() override;
  content::KeyboardEventProcessingResult PreHandleKeyboardEvent(
      const content::NativeWebKeyboardEvent& event) override;
  void HandleKeyboardEvent(
      const content::NativeWebKeyboardEvent& event) override;
  void CutCopyPaste(int command_id) override;
  WindowOpenDisposition GetDispositionForPopupBounds(
      const gfx::Rect& bounds) override;
  FindBar* CreateFindBar() override;
  web_modal::WebContentsModalDialogHost* GetWebContentsModalDialogHost()
      override;
  void ShowAvatarBubbleFromAvatarButton(
      AvatarBubbleMode mode,
      const signin::ManageAccountsParams& manage_accounts_params,
      signin_metrics::AccessPoint access_point,
      bool is_source_keyboard) override;
  int GetRenderViewHeightInsetWithDetachedBookmarkBar() override;
  void ExecuteExtensionCommand(const extensions::Extension* extension,
                               const extensions::Command& command) override;
  ExclusiveAccessContext* GetExclusiveAccessContext() override;
  void ShowImeWarningBubble(
      const extensions::Extension* extension,
      const base::Callback<void(ImeWarningBubblePermissionStatus status)>&
          callback) override;

  // Overridden from ExtensionKeybindingRegistry::Delegate:
  extensions::ActiveTabPermissionGranter* GetActiveTabPermissionGranter()
      override;

  // Adds the given FindBar cocoa controller to this browser window.
  void AddFindBar(FindBarCocoaController* find_bar_cocoa_controller);

  // Updates the window's alert state. If the new alert state is
  // TabAlertState::AUDIO_PLAYING or TabAlertState::AUDIO_MUTING then sets
  // the window's title to reflect that.
  void UpdateAlertState(TabAlertState alert_state);

  // Returns the cocoa-world BrowserWindowController
  BrowserWindowController* cocoa_controller() const { return controller_; }

  // Returns window title based on the active tab title and the window's alert
  // state.
  NSString* WindowTitle();

  // Returns the current alert state, determined by the alert state of tabs. Set
  // by UpdateAlertState.
  TabAlertState alert_state() { return alert_state_; }

 protected:
  void DestroyBrowser() override;

 private:
  NSWindow* window() const;  // Accessor for the (current) |NSWindow|.

  Browser* browser_;  // weak, owned by controller
  BrowserWindowController* controller_;  // weak, owns us
  base::scoped_nsobject<NSString> pending_window_title_;
  ui::WindowShowState initial_show_state_;
  NSInteger attention_request_id_;  // identifier from requestUserAttention

  // Preserves window alert state to show appropriate icon in the window title
  // which can be audio playing, muting or none (determined by alert state of
  // tabs.
  TabAlertState alert_state_;
};

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_COCOA_H_
