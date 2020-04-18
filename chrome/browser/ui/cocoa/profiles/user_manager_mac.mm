// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/profiles/user_manager_mac.h"

#include "base/callback.h"
#include "base/mac/foundation_util.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/app_controller_mac.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/ui/browser_dialogs.h"
#import "chrome/browser/ui/cocoa/browser_window_utils.h"
#include "chrome/browser/ui/cocoa/chrome_event_processing_window.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"
#include "chrome/browser/ui/user_manager.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "components/web_modal/web_contents_modal_dialog_manager_delegate.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/ui_features.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace {

// An open User Manager window. There can only be one open at a time. This
// is reset to NULL when the window is closed.
UserManagerMac* instance_ = nullptr;  // Weak.
std::vector<base::Closure>* user_manager_shown_callbacks_for_testing_ = nullptr;
BOOL instance_under_construction_ = NO;

void CloseInstanceDialog() {
  DCHECK(instance_);
  instance_->CloseDialog();
}

// The modal dialog host the User Manager uses to display the dialog.
class UserManagerModalHost : public web_modal::WebContentsModalDialogHost {
 public:
  UserManagerModalHost(gfx::NativeView host_view)
      : host_view_(host_view) {}

  gfx::Size GetMaximumDialogSize() override {
    return gfx::Size(UserManagerProfileDialog::kDialogWidth,
                     UserManagerProfileDialog::kDialogHeight);
  }

  ~UserManagerModalHost() override {}

  gfx::NativeView GetHostView() const override {
    return host_view_;
  }

  gfx::Point GetDialogPosition(const gfx::Size& size) override {
    return gfx::Point(0, 0);
  }

  void AddObserver(web_modal::ModalDialogHostObserver* observer) override {}
  void RemoveObserver(web_modal::ModalDialogHostObserver* observer) override {}

 private:
  gfx::NativeView host_view_;
};

// The modal manager delegate allowing the display of constrained windows for
// the dialog.
class UserManagerModalManagerDelegate :
    public web_modal::WebContentsModalDialogManagerDelegate {
 public:
  UserManagerModalManagerDelegate(gfx::NativeView host_view) {
    modal_host_.reset(new UserManagerModalHost(host_view));
  }

  web_modal::WebContentsModalDialogHost* GetWebContentsModalDialogHost()
      override {
    return modal_host_.get();
  }

  bool IsWebContentsVisible(content::WebContents* web_contents) override {
    return true;
  }

   ~UserManagerModalManagerDelegate() override {}
 protected:
  std::unique_ptr<UserManagerModalHost> modal_host_;
};

// Custom WebContentsDelegate that allows handling of hotkeys.
class UserManagerWebContentsDelegate : public content::WebContentsDelegate {
 public:
  UserManagerWebContentsDelegate() {}

  // WebContentsDelegate implementation. Forwards all unhandled keyboard events
  // to the current window.
  void HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override {
    if (![BrowserWindowUtils shouldHandleKeyboardEvent:event])
      return;

    // -getCommandId returns -1 if the event isn't a chrome accelerator.
    int chromeCommandId = [BrowserWindowUtils getCommandId:event];

    // Check for Cmd+A and Cmd+V events that could come from a password field.
    BOOL isTextEditingCommand = [BrowserWindowUtils isTextEditingEvent:event];

    // Only handle close window Chrome accelerators and text editing ones.
    if (chromeCommandId == IDC_CLOSE_WINDOW || chromeCommandId == IDC_EXIT ||
        isTextEditingCommand) {
      [[NSApp mainMenu] performKeyEquivalent:event.os_event];
    }
  }
};

class UserManagerProfileDialogDelegate
    : public UserManagerProfileDialog::BaseDialogDelegate,
      public ConstrainedWindowMacDelegate {
 public:
  UserManagerProfileDialogDelegate() {
    hotKeysWebContentsDelegate_.reset(new UserManagerWebContentsDelegate());
  }

  // UserManagerProfileDialog::BaseDialogDelegate:
  void CloseDialog() override { CloseInstanceDialog(); }

  // WebContentsDelegate::HandleKeyboardEvent:
  void HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override {
    hotKeysWebContentsDelegate_->HandleKeyboardEvent(source, event);
  }

  // ConstrainedWindowMacDelegate:
  void OnConstrainedWindowClosed(ConstrainedWindowMac* window) override {
    CloseDialog();
  }

 private:
  std::unique_ptr<UserManagerWebContentsDelegate> hotKeysWebContentsDelegate_;

  DISALLOW_COPY_AND_ASSIGN(UserManagerProfileDialogDelegate);
};

}  // namespace

// WindowController for the dialog.
@interface DialogWindowController : NSWindowController<NSWindowDelegate> {
 @private
  std::string emailAddress_;
  GURL url_;
  content::WebContents* webContents_;
  std::unique_ptr<UserManagerProfileDialogDelegate> webContentsDelegate_;
  std::unique_ptr<ConstrainedWindowMac> constrained_window_;
  std::unique_ptr<content::WebContents> dialogWebContents_;
}
- (id)initWithProfile:(Profile*)profile
                email:(std::string)email
                  url:(GURL)url
          webContents:(content::WebContents*)webContents;
- (void)showURL:(const GURL&)url;
- (void)close;
@end

@implementation DialogWindowController

- (id)initWithProfile:(Profile*)profile
                email:(std::string)email
                  url:(GURL)url
          webContents:(content::WebContents*)webContents {
  webContents_ = webContents;
  emailAddress_ = email;
  url_ = url;

  NSRect frame = NSMakeRect(0, 0, UserManagerProfileDialog::kDialogWidth,
                            UserManagerProfileDialog::kDialogHeight);
  base::scoped_nsobject<ConstrainedWindowCustomWindow> window(
      [[ConstrainedWindowCustomWindow alloc]
          initWithContentRect:frame
                    styleMask:NSTitledWindowMask | NSClosableWindowMask]);
  if ((self = [super initWithWindow:window])) {
    webContents_ = webContents;

    dialogWebContents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile));
    window.get().contentView = dialogWebContents_->GetNativeView();
    webContentsDelegate_.reset(new UserManagerProfileDialogDelegate());
    dialogWebContents_->SetDelegate(webContentsDelegate_.get());

    // Load the url for the WebContents before constrained window creation so
    // that the dialog can get focus properly.
    [self show];

    base::scoped_nsobject<CustomConstrainedWindowSheet> sheet(
       [[CustomConstrainedWindowSheet alloc]
           initWithCustomWindow:[self window]]);
    constrained_window_ =
        CreateAndShowWebModalDialogMac(
            webContentsDelegate_.get(), webContents_, sheet);

    // The close button needs to call CloseWebContentsModalDialog() on the
    // constrained window isntead of just [window close] so grab a reference to
    // it in the title bar and change its action.
    auto closeButton = [window standardWindowButton:NSWindowCloseButton];
    [closeButton setTarget:self];
    [closeButton setAction:@selector(closeButtonClicked:)];
  }

  return self;
}

- (void)showURL:(const GURL&)url {
  dialogWebContents_->GetController().LoadURL(url, content::Referrer(),
                                              ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                              std::string());
}

- (void)show {
  [self showURL:url_];
}

- (void)closeButtonClicked:(NSButton*)button {
  [self close];
}

- (void)close {
  constrained_window_->CloseWebContentsModalDialog();
}

- (void)dealloc {
  constrained_window_->CloseWebContentsModalDialog();

  [super dealloc];
}

@end

// Window controller for the User Manager view.
@interface UserManagerWindowController : NSWindowController <NSWindowDelegate> {
 @private
  std::unique_ptr<content::WebContents> webContents_;
  std::unique_ptr<UserManagerWebContentsDelegate> webContentsDelegate_;
  UserManagerMac* userManagerObserver_;  // Weak.
  std::unique_ptr<UserManagerModalManagerDelegate> modal_manager_delegate_;
  base::scoped_nsobject<DialogWindowController> dialog_window_controller_;
}
- (void)windowWillClose:(NSNotification*)notification;
- (void)dealloc;
- (id)initWithProfile:(Profile*)profile
         withObserver:(UserManagerMac*)userManagerObserver;
- (void)showURL:(const GURL&)url;
- (void)show;
- (void)close;
- (BOOL)isVisible;
- (void)showDialogWithProfile:(Profile*)profile
                        email:(std::string)email
                          url:(GURL)url;
- (void)displayErrorMessage;
- (void)closeDialog;
@end

@implementation UserManagerWindowController

- (id)initWithProfile:(Profile*)profile
         withObserver:(UserManagerMac*)userManagerObserver {
  // Center the window on the screen that currently has focus.
  NSScreen* mainScreen = [NSScreen mainScreen];
  CGFloat screenHeight = [mainScreen frame].size.height;
  CGFloat screenWidth = [mainScreen frame].size.width;

  NSRect contentRect =
      NSMakeRect((screenWidth - UserManager::kWindowWidth) / 2,
                 (screenHeight - UserManager::kWindowHeight) / 2,
                 UserManager::kWindowWidth, UserManager::kWindowHeight);
  ChromeEventProcessingWindow* window = [[ChromeEventProcessingWindow alloc]
      initWithContentRect:contentRect
                styleMask:NSTitledWindowMask |
                          NSClosableWindowMask |
                          NSResizableWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO
                   screen:mainScreen];
  [window setTitle:l10n_util::GetNSString(IDS_PRODUCT_NAME)];
  [window setMinSize:NSMakeSize(UserManager::kWindowWidth,
                                UserManager::kWindowHeight)];

  if ((self = [super initWithWindow:window])) {
    userManagerObserver_ = userManagerObserver;

    // Initialize the web view.
    webContents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile));
    window.contentView = webContents_->GetNativeView();

    // When a window has layer-backed subviews, its contentView must be
    // layer-backed or it won't mask the subviews to its rounded corners. See
    // https://crbug.com/620049
    // The static_cast is just needed for the 10.10 SDK. It can be removed when
    // we move to a newer one.
    static_cast<NSView*>(window.contentView).wantsLayer = YES;

    webContentsDelegate_.reset(new UserManagerWebContentsDelegate());
    webContents_->SetDelegate(webContentsDelegate_.get());

    web_modal::WebContentsModalDialogManager::CreateForWebContents(
        webContents_.get());
    modal_manager_delegate_.reset(
        new UserManagerModalManagerDelegate([[self window] contentView]));
    web_modal::WebContentsModalDialogManager::FromWebContents(
        webContents_.get())->SetDelegate(modal_manager_delegate_.get());

    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(windowWillClose:)
               name:NSWindowWillCloseNotification
             object:self.window];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  // Remove the ModalDailogManager that's about to be destroyed.
  auto* manager = web_modal::WebContentsModalDialogManager::FromWebContents(
      webContents_.get());
  if (manager)
    manager->SetDelegate(nullptr);

  [super dealloc];
}

- (void)showURL:(const GURL&)url {
  webContents_->GetController().LoadURL(url, content::Referrer(),
                                        ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                        std::string());
  content::RenderWidgetHostView* rwhv = webContents_->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetBackgroundColor(profiles::kUserManagerBackgroundColor);
  [self show];
}

- (void)show {
  // Because the User Manager isn't a BrowserWindowController, activating it
  // will not trigger a -windowChangedToProfile and update the menu bar.
  // This is only important if the active profile is Guest, which may have
  // happened after locking a profile.
  if (profiles::SetActiveProfileToGuestIfLocked())
    app_controller_mac::CreateGuestProfileIfNeeded();
  [[self window] makeKeyAndOrderFront:self];
}

- (void)close {
  [[self window] close];
}

-(BOOL)isVisible {
  return [[self window] isVisible];
}

- (void)windowWillClose:(NSNotification*)notification {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  DCHECK(userManagerObserver_);
  userManagerObserver_->WindowWasClosed();
}

- (void)showDialogWithProfile:(Profile*)profile
                        email:(std::string)email
                          url:(GURL)url {
  dialog_window_controller_.reset([[DialogWindowController alloc]
      initWithProfile:profile
                email:email
                  url:url
          webContents:webContents_.get()]);
}

- (void)displayErrorMessage {
  [dialog_window_controller_ showURL:GURL(chrome::kChromeUISigninErrorURL)];
}

- (void)closeDialog {
  [dialog_window_controller_ close];
}

@end


// static
void UserManager::ShowCocoa(const base::FilePath& profile_path_to_focus,
                            profiles::UserManagerAction user_manager_action) {
  DCHECK(profile_path_to_focus != ProfileManager::GetGuestProfilePath());

  ProfileMetrics::LogProfileOpenMethod(ProfileMetrics::OPEN_USER_MANAGER);
  if (instance_) {
    // If there's a user manager window open already, just activate it.
    [instance_->window_controller() show];
    instance_->set_user_manager_started_showing(base::Time::Now());
    return;
  }

  // Under some startup conditions, we can try twice to create the User Manager.
  // Because creating the System profile is asynchronous, it's possible for
  // there to then be multiple pending operations and eventually multiple
  // User Managers.
  if (instance_under_construction_)
      return;
  instance_under_construction_ = YES;

  // Create the guest profile, if necessary, and open the User Manager
  // from the guest profile.
  profiles::CreateSystemProfileForUserManager(
      profile_path_to_focus,
      user_manager_action,
      base::Bind(&UserManagerMac::OnSystemProfileCreated, base::Time::Now()));
}

// static
void UserManager::HideCocoa() {
  if (instance_)
    [instance_->window_controller() close];
}

// static
bool UserManager::IsShowingCocoa() {
  return instance_ ? [instance_->window_controller() isVisible]: false;
}

// static
void UserManager::OnUserManagerShownCocoa() {
  if (instance_) {
    instance_->LogTimeToOpen();
    if (user_manager_shown_callbacks_for_testing_) {
      for (const auto& callback : *user_manager_shown_callbacks_for_testing_) {
        if (!callback.is_null())
          callback.Run();
      }
      // Delete the callback list after calling.
      delete user_manager_shown_callbacks_for_testing_;
      user_manager_shown_callbacks_for_testing_ = nullptr;
    }
  }
}

// static
void UserManager::AddOnUserManagerShownCallbackForTestingCocoa(
    const base::Closure& callback) {
  if (!user_manager_shown_callbacks_for_testing_)
    user_manager_shown_callbacks_for_testing_ = new std::vector<base::Closure>;
  user_manager_shown_callbacks_for_testing_->push_back(callback);
}

// static
base::FilePath UserManager::GetSigninProfilePathCocoa() {
  return instance_->GetSigninProfilePath();
}

// static
void UserManagerProfileDialog::ShowReauthDialogCocoa(
    content::BrowserContext* browser_context,
    const std::string& email,
    signin_metrics::Reason reason) {
  ShowReauthDialogWithProfilePath(browser_context, email, base::FilePath(),
                                  reason);
}

// static
void UserManagerProfileDialog::ShowReauthDialogWithProfilePathCocoa(
    content::BrowserContext* browser_context,
    const std::string& email,
    const base::FilePath& profile_path,
    signin_metrics::Reason reason) {
  // This method should only be called if the user manager is already showing.
  if (!UserManager::IsShowing())
    return;
  GURL url = signin::GetReauthURLWithEmailForDialog(
      signin_metrics::AccessPoint::ACCESS_POINT_USER_MANAGER, reason, email);
  instance_->SetSigninProfilePath(profile_path);
  instance_->ShowDialog(browser_context, email, url);
}

// static
void UserManagerProfileDialog::ShowSigninDialogCocoa(
    content::BrowserContext* browser_context,
    const base::FilePath& profile_path,
    signin_metrics::Reason reason) {
  if (!UserManager::IsShowing())
    return;
  DCHECK(reason ==
             signin_metrics::Reason::REASON_FORCED_SIGNIN_PRIMARY_ACCOUNT ||
         reason == signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT);
  instance_->SetSigninProfilePath(profile_path);
  GURL url = signin::GetPromoURLForDialog(
      signin_metrics::AccessPoint::ACCESS_POINT_USER_MANAGER, reason, true);
  instance_->ShowDialog(browser_context, std::string(), url);
}

// static
void UserManagerProfileDialog::ShowDialogAndDisplayErrorMessageCocoa(
    content::BrowserContext* browser_context) {
  if (!UserManager::IsShowing())
    return;

  // The error occurred before sign in happened, reset |signin_profile_path_|
  // so that the error page will show the error message that is assoicated with
  // the system profile.
  instance_->SetSigninProfilePath(base::FilePath());
  instance_->ShowDialog(browser_context, std::string(),
                        GURL(chrome::kChromeUISigninErrorURL));
}

// static
void UserManagerProfileDialog::DisplayErrorMessageCocoa() {
  DCHECK(instance_);
  instance_->DisplayErrorMessage();
}

// static
void UserManagerProfileDialog::HideDialogCocoa() {
  // This method should only be called if the user manager is already showing.
  if (!UserManager::IsShowing())
    return;

  instance_->CloseDialog();
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)

void UserManager::Show(const base::FilePath& profile_path_to_focus,
                       profiles::UserManagerAction user_manager_action) {
  ShowCocoa(profile_path_to_focus, user_manager_action);
}

void UserManager::Hide() {
  HideCocoa();
}

bool UserManager::IsShowing() {
  return IsShowingCocoa();
}

void UserManager::OnUserManagerShown() {
  OnUserManagerShownCocoa();
}

base::FilePath UserManager::GetSigninProfilePath() {
  return GetSigninProfilePathCocoa();
}

void UserManager::AddOnUserManagerShownCallbackForTesting(
    const base::Closure& callback) {
  UserManager::AddOnUserManagerShownCallbackForTestingCocoa(callback);
}

void UserManagerProfileDialog::ShowReauthDialog(
    content::BrowserContext* browser_context,
    const std::string& email,
    signin_metrics::Reason reason) {
  ShowReauthDialogWithProfilePath(browser_context, email, base::FilePath(),
                                  reason);
}

void UserManagerProfileDialog::ShowReauthDialogWithProfilePath(
    content::BrowserContext* browser_context,
    const std::string& email,
    const base::FilePath& profile_path,
    signin_metrics::Reason reason) {
  ShowReauthDialogWithProfilePathCocoa(browser_context, email, profile_path,
                                       reason);
}

void UserManagerProfileDialog::ShowSigninDialog(
    content::BrowserContext* browser_context,
    const base::FilePath& profile_path,
    signin_metrics::Reason reason) {
  ShowSigninDialogCocoa(browser_context, profile_path, reason);
}

void UserManagerProfileDialog::ShowDialogAndDisplayErrorMessage(
    content::BrowserContext* browser_context) {
  ShowDialogAndDisplayErrorMessageCocoa(browser_context);
}

void UserManagerProfileDialog::DisplayErrorMessage() {
  DisplayErrorMessageCocoa();
}

void UserManagerProfileDialog::HideDialog() {
  HideDialogCocoa();
}
#endif

void UserManagerMac::ShowDialog(content::BrowserContext* browser_context,
                                const std::string& email,
                                const GURL& url) {
  [window_controller_
      showDialogWithProfile:Profile::FromBrowserContext(browser_context)
                      email:email
                        url:url];
}

void UserManagerMac::CloseDialog() {
  [window_controller_ closeDialog];
}

UserManagerMac::UserManagerMac(Profile* profile) {
  window_controller_.reset([[UserManagerWindowController alloc]
      initWithProfile:profile withObserver:this]);
}

UserManagerMac::~UserManagerMac() {
}

// static
void UserManagerMac::OnSystemProfileCreated(const base::Time& start_time,
                                            Profile* system_profile,
                                            const std::string& url) {
  DCHECK(!instance_);
  instance_ = new UserManagerMac(system_profile);
  instance_->set_user_manager_started_showing(start_time);
  [instance_->window_controller() showURL:GURL(url)];
  instance_under_construction_ = NO;
}

void UserManagerMac::LogTimeToOpen() {
  if (user_manager_started_showing_ == base::Time())
    return;

  ProfileMetrics::LogTimeToOpenUserManager(
      base::Time::Now() - user_manager_started_showing_);
  user_manager_started_showing_ = base::Time();
}

void UserManagerMac::WindowWasClosed() {
  CloseDialog();
  instance_ = NULL;
  delete this;
}

void UserManagerMac::DisplayErrorMessage() {
  [window_controller_ displayErrorMessage];
}

void UserManagerMac::SetSigninProfilePath(const base::FilePath& profile_path) {
  signin_profile_path_ = profile_path;
}

base::FilePath UserManagerMac::GetSigninProfilePath() {
  return signin_profile_path_;
}
