// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager_chromeos.h"

#include <set>
#include <vector>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/interfaces/window_actions.mojom.h"
#include "ash/shell.h"                                  // mash-ok
#include "ash/wm/tablet_mode/tablet_mode_controller.h"  // mash-ok
#include "base/auto_reset.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/ash/media_client.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/ash/multi_user/user_switch_animator_chromeos.h"
#include "chrome/browser/ui/ash/session_controller_client.h"
#include "chrome/browser/ui/ash/session_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ui_base_types.h"
#include "ui/events/event.h"
#include "ui/wm/core/transient_window_manager.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/core/window_util.h"

namespace {

// The animation time in milliseconds for a single window which is fading
// in / out.
const int kAnimationTimeMS = 100;

// The animation time in milliseconds for the fade in and / or out when
// switching users.
const int kUserFadeTimeMS = 110;

// The animation time in ms for a window which get teleported to another screen.
const int kTeleportAnimationTimeMS = 300;

// Used for UMA metrics. Do not reorder.
enum TeleportWindowType {
  TELEPORT_WINDOW_BROWSER = 0,
  TELEPORT_WINDOW_INCOGNITO_BROWSER,
  TELEPORT_WINDOW_V1_APP,
  TELEPORT_WINDOW_V2_APP,
  TELEPORT_WINDOW_PANEL,
  TELEPORT_WINDOW_POPUP,
  TELEPORT_WINDOW_UNKNOWN,
  NUM_TELEPORT_WINDOW_TYPES
};

// Records the type of window which was transferred to another desktop.
void RecordUMAForTransferredWindowType(aura::Window* window) {
  // We need to figure out what kind of window this is to record the transfer.
  Browser* browser = chrome::FindBrowserWithWindow(window);
  TeleportWindowType window_type = TELEPORT_WINDOW_UNKNOWN;
  if (browser) {
    if (browser->profile()->IsOffTheRecord()) {
      window_type = TELEPORT_WINDOW_INCOGNITO_BROWSER;
    } else if (browser->is_app()) {
      window_type = TELEPORT_WINDOW_V1_APP;
    } else if (browser->is_type_popup()) {
      window_type = TELEPORT_WINDOW_POPUP;
    } else {
      window_type = TELEPORT_WINDOW_BROWSER;
    }
  } else {
    // Unit tests might come here without a profile manager.
    if (!g_browser_process->profile_manager())
      return;
    // If it is not a browser, it is probably be a V2 application. In that case
    // one of the AppWindowRegistry instances should know about it.
    extensions::AppWindow* app_window = NULL;
    std::vector<Profile*> profiles =
        g_browser_process->profile_manager()->GetLoadedProfiles();
    for (std::vector<Profile*>::iterator it = profiles.begin();
         it != profiles.end() && app_window == NULL; it++) {
      app_window =
          extensions::AppWindowRegistry::Get(*it)->GetAppWindowForNativeWindow(
              window);
    }
    if (app_window) {
      if (app_window->window_type() ==
          extensions::AppWindow::WINDOW_TYPE_PANEL) {
        window_type = TELEPORT_WINDOW_PANEL;
      } else {
        window_type = TELEPORT_WINDOW_V2_APP;
      }
    }
  }
  UMA_HISTOGRAM_ENUMERATION("MultiProfile.TeleportWindowType", window_type,
                            NUM_TELEPORT_WINDOW_TYPES);
}

bool HasSystemModalTransientChildWindow(aura::Window* window) {
  if (window == nullptr)
    return false;

  aura::Window* system_modal_container = window->GetRootWindow()->GetChildById(
      ash::kShellWindowId_SystemModalContainer);
  if (window->parent() == system_modal_container)
    return true;

  aura::Window::Windows::const_iterator it =
      wm::GetTransientChildren(window).begin();
  for (; it != wm::GetTransientChildren(window).end(); ++it) {
    if (HasSystemModalTransientChildWindow(*it))
      return true;
  }
  return false;
}

}  // namespace

// A class to temporarily change the animation properties for a window.
class AnimationSetter {
 public:
  AnimationSetter(aura::Window* window, int animation_time_in_ms)
      : window_(window),
        previous_animation_type_(wm::GetWindowVisibilityAnimationType(window_)),
        previous_animation_time_(
            wm::GetWindowVisibilityAnimationDuration(*window_)) {
    wm::SetWindowVisibilityAnimationType(
        window_, wm::WINDOW_VISIBILITY_ANIMATION_TYPE_FADE);
    wm::SetWindowVisibilityAnimationDuration(
        window_, base::TimeDelta::FromMilliseconds(animation_time_in_ms));
  }

  ~AnimationSetter() {
    wm::SetWindowVisibilityAnimationType(window_, previous_animation_type_);
    wm::SetWindowVisibilityAnimationDuration(window_, previous_animation_time_);
  }

 private:
  // The window which gets used.
  aura::Window* window_;

  // Previous animation type.
  const int previous_animation_type_;

  // Previous animation time.
  const base::TimeDelta previous_animation_time_;

  DISALLOW_COPY_AND_ASSIGN(AnimationSetter);
};

// This class keeps track of all applications which were started for a user.
// When an app gets created, the window will be tagged for that user. Note
// that the destruction does not need to be tracked here since the universal
// window observer will take care of that.
class AppObserver : public extensions::AppWindowRegistry::Observer {
 public:
  explicit AppObserver(const std::string& user_id) : user_id_(user_id) {}
  ~AppObserver() override {}

  // AppWindowRegistry::Observer overrides:
  void OnAppWindowAdded(extensions::AppWindow* app_window) override {
    aura::Window* window = app_window->GetNativeWindow();
    DCHECK(window);
    MultiUserWindowManagerChromeOS::GetInstance()->SetWindowOwner(
        window, AccountId::FromUserEmail(user_id_));
  }

 private:
  std::string user_id_;

  DISALLOW_COPY_AND_ASSIGN(AppObserver);
};

MultiUserWindowManagerChromeOS::MultiUserWindowManagerChromeOS(
    const AccountId& current_account_id)
    : current_account_id_(current_account_id),
      suppress_visibility_changes_(false),
      animation_speed_(ANIMATION_SPEED_NORMAL) {}

MultiUserWindowManagerChromeOS::~MultiUserWindowManagerChromeOS() {
  // When the MultiUserWindowManager gets destroyed, ash::Shell is mostly gone.
  // As such we should not try to finalize any outstanding user animations.
  // Note that the destruction of the object can be done later.
  if (animation_.get())
    animation_->CancelAnimation();

  // Remove all window observers.
  WindowToEntryMap::iterator window = window_to_entry_.begin();
  while (window != window_to_entry_.end()) {
    // Explicitly remove this from window observer list since OnWindowDestroyed
    // no longer does that.
    window->first->RemoveObserver(this);
    OnWindowDestroyed(window->first);
    window = window_to_entry_.begin();
  }

  if (user_manager::UserManager::IsInitialized())
    user_manager::UserManager::Get()->RemoveSessionStateObserver(this);

  // Remove all app observers.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  // might be nullptr in unit tests.
  if (!profile_manager)
    return;

  std::vector<Profile*> profiles = profile_manager->GetLoadedProfiles();
  for (auto it = profiles.begin(); it != profiles.end(); ++it) {
    const AccountId account_id = multi_user_util::GetAccountIdFromProfile(*it);
    AccountIdToAppWindowObserver::iterator app_observer_iterator =
        account_id_to_app_observer_.find(account_id);
    if (app_observer_iterator != account_id_to_app_observer_.end()) {
      extensions::AppWindowRegistry::Get(*it)->RemoveObserver(
          app_observer_iterator->second);
      delete app_observer_iterator->second;
      account_id_to_app_observer_.erase(app_observer_iterator);
    }
  }
}

void MultiUserWindowManagerChromeOS::Init() {
  // Since we are setting the SessionStateObserver and adding the user, this
  // function should get called only once.
  DCHECK(account_id_to_app_observer_.find(current_account_id_) ==
         account_id_to_app_observer_.end());

  // Add a session state observer to be able to monitor session changes.
  if (user_manager::UserManager::IsInitialized())
    user_manager::UserManager::Get()->AddSessionStateObserver(this);

  // The BrowserListObserver would have been better to use then the old
  // notification system, but that observer fires before the window got created.
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_OPENED,
                 content::NotificationService::AllSources());

  // Add an app window observer & all already running apps.
  Profile* profile =
      multi_user_util::GetProfileFromAccountId(current_account_id_);
  if (profile)
    AddUser(profile);
}

void MultiUserWindowManagerChromeOS::SetWindowOwner(
    aura::Window* window,
    const AccountId& account_id) {
  // Make sure the window is valid and there was no owner yet.
  DCHECK(window);
  DCHECK(account_id.is_valid());
  if (GetWindowOwner(window) == account_id)
    return;
  DCHECK(GetWindowOwner(window).empty());
  window_to_entry_[window] = new WindowEntry(account_id);

  // Remember the initial visibility of the window.
  window_to_entry_[window]->set_show(window->IsVisible());

  // Add observers to track state changes.
  window->AddObserver(this);
  wm::TransientWindowManager::GetOrCreate(window)->AddObserver(this);

  // Check if this window was created due to a user interaction. If it was,
  // transfer it to the current user.
  if (window->GetProperty(aura::client::kCreatedByUserGesture))
    window_to_entry_[window]->set_show_for_user(current_account_id_);

  // Add all transient children to our set of windows. Note that the function
  // will add the children but not the owner to the transient children map.
  AddTransientOwnerRecursive(window, window);

  // Notify entry adding.
  for (Observer& observer : observers_)
    observer.OnOwnerEntryAdded(window);

  if (!IsWindowOnDesktopOfUser(window, current_account_id_))
    SetWindowVisibility(window, false, 0);
}

const AccountId& MultiUserWindowManagerChromeOS::GetWindowOwner(
    aura::Window* window) const {
  WindowToEntryMap::const_iterator it = window_to_entry_.find(window);
  return it != window_to_entry_.end() ? it->second->owner() : EmptyAccountId();
}

void MultiUserWindowManagerChromeOS::ShowWindowForUser(
    aura::Window* window,
    const AccountId& account_id) {
  const AccountId previous_owner(GetUserPresentingWindow(window));
  if (!ShowWindowForUserIntern(window, account_id))
    return;
  // The window switched to a new desktop and we have to switch to that desktop,
  // but only when it was on the visible desktop and the the target is not the
  // visible desktop.
  if (account_id == current_account_id_ ||
      previous_owner != current_account_id_)
    return;

  SessionControllerClient::DoSwitchActiveUser(account_id);
}

bool MultiUserWindowManagerChromeOS::AreWindowsSharedAmongUsers() const {
  WindowToEntryMap::const_iterator it = window_to_entry_.begin();
  for (; it != window_to_entry_.end(); ++it) {
    if (it->second->owner() != it->second->show_for_user())
      return true;
  }
  return false;
}

void MultiUserWindowManagerChromeOS::GetOwnersOfVisibleWindows(
    std::set<AccountId>* account_ids) const {
  for (WindowToEntryMap::const_iterator it = window_to_entry_.begin();
       it != window_to_entry_.end(); ++it) {
    if (it->first->IsVisible())
      account_ids->insert(it->second->owner());
  }
}

bool MultiUserWindowManagerChromeOS::IsWindowOnDesktopOfUser(
    aura::Window* window,
    const AccountId& account_id) const {
  const AccountId& presenting_user = GetUserPresentingWindow(window);
  return (!presenting_user.is_valid()) || presenting_user == account_id;
}

const AccountId& MultiUserWindowManagerChromeOS::GetUserPresentingWindow(
    aura::Window* window) const {
  WindowToEntryMap::const_iterator it = window_to_entry_.find(window);
  // If the window is not owned by anyone it is shown on all desktops and we
  // return the empty string.
  if (it == window_to_entry_.end())
    return EmptyAccountId();
  // Otherwise we ask the object for its desktop.
  return it->second->show_for_user();
}

void MultiUserWindowManagerChromeOS::AddUser(content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  const AccountId& account_id(
      multi_user_util::GetAccountIdFromProfile(profile));
  if (account_id_to_app_observer_.find(account_id) !=
      account_id_to_app_observer_.end())
    return;

  account_id_to_app_observer_[account_id] =
      new AppObserver(account_id.GetUserEmail());
  extensions::AppWindowRegistry::Get(profile)->AddObserver(
      account_id_to_app_observer_[account_id]);

  // Account all existing application windows of this user accordingly.
  const extensions::AppWindowRegistry::AppWindowList& app_windows =
      extensions::AppWindowRegistry::Get(profile)->app_windows();
  extensions::AppWindowRegistry::AppWindowList::const_iterator it =
      app_windows.begin();
  for (; it != app_windows.end(); ++it)
    account_id_to_app_observer_[account_id]->OnAppWindowAdded(*it);

  // Account all existing browser windows of this user accordingly.
  BrowserList* browser_list = BrowserList::GetInstance();
  BrowserList::const_iterator browser_it = browser_list->begin();
  for (; browser_it != browser_list->end(); ++browser_it) {
    if ((*browser_it)->profile()->GetOriginalProfile() == profile)
      AddBrowserWindow(*browser_it);
  }
}

void MultiUserWindowManagerChromeOS::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void MultiUserWindowManagerChromeOS::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void MultiUserWindowManagerChromeOS::ActiveUserChanged(
    const user_manager::User* active_user) {
  // This needs to be set before the animation starts.
  current_account_id_ = active_user->GetAccountId();

  // Here to avoid a very nasty race condition, we must destruct any previously
  // created animation before creating a new one. Otherwise, the newly
  // constructed will hide all windows of the old user in the first step of the
  // animation only to be reshown again by the destructor of the old animation.
  animation_.reset();
  animation_.reset(new UserSwitchAnimatorChromeOS(
      this, current_account_id_,
      GetAdjustedAnimationTimeInMS(kUserFadeTimeMS)));

  // Call RequestCaptureState here instead of having MediaClient observe
  // ActiveUserChanged because it must happen after
  // MultiUserWindowManagerChromeOS is notified. The MediaClient may be null in
  // tests.
  if (MediaClient::Get())
    MediaClient::Get()->RequestCaptureState();
}

void MultiUserWindowManagerChromeOS::OnWindowDestroyed(aura::Window* window) {
  if (GetWindowOwner(window).empty()) {
    // This must be a window in the transient chain - remove it and its
    // children from the owner.
    RemoveTransientOwnerRecursive(window);
    return;
  }
  wm::TransientWindowManager::GetOrCreate(window)->RemoveObserver(this);
  // Remove the window from the owners list.
  delete window_to_entry_[window];
  window_to_entry_.erase(window);

  // Notify entry change.
  for (Observer& observer : observers_)
    observer.OnOwnerEntryRemoved(window);
}

void MultiUserWindowManagerChromeOS::OnWindowVisibilityChanging(
    aura::Window* window,
    bool visible) {
  // This command gets called first and immediately when show or hide gets
  // called. We remember here the desired state for restoration IF we were
  // not ourselves issuing the call.
  // Note also that using the OnWindowVisibilityChanged callback cannot be
  // used for this.
  if (suppress_visibility_changes_)
    return;

  WindowToEntryMap::iterator it = window_to_entry_.find(window);
  // If the window is not owned by anyone it is shown on all desktops.
  if (it != window_to_entry_.end()) {
    // Remember what was asked for so that we can restore this when the user's
    // desktop gets restored.
    it->second->set_show(visible);
  } else {
    TransientWindowToVisibility::iterator it =
        transient_window_to_visibility_.find(window);
    if (it != transient_window_to_visibility_.end())
      it->second = visible;
  }
}

void MultiUserWindowManagerChromeOS::OnWindowVisibilityChanged(
    aura::Window* window,
    bool visible) {
  if (suppress_visibility_changes_)
    return;

  // Don't allow to make the window visible if it shouldn't be.
  if (visible && !IsWindowOnDesktopOfUser(window, current_account_id_)) {
    SetWindowVisibility(window, false, 0);
    return;
  }
  aura::Window* owned_parent = GetOwningWindowInTransientChain(window);
  if (owned_parent && owned_parent != window && visible &&
      !IsWindowOnDesktopOfUser(owned_parent, current_account_id_))
    SetWindowVisibility(window, false, 0);
}

void MultiUserWindowManagerChromeOS::OnTransientChildAdded(
    aura::Window* window,
    aura::Window* transient_window) {
  if (!GetWindowOwner(window).empty()) {
    AddTransientOwnerRecursive(transient_window, window);
    return;
  }
  aura::Window* owned_parent =
      GetOwningWindowInTransientChain(transient_window);
  if (!owned_parent)
    return;

  AddTransientOwnerRecursive(transient_window, owned_parent);
}

void MultiUserWindowManagerChromeOS::OnTransientChildRemoved(
    aura::Window* window,
    aura::Window* transient_window) {
  // Remove the transient child if the window itself is owned, or one of the
  // windows in its transient parents chain.
  if (!GetWindowOwner(window).empty() ||
      GetOwningWindowInTransientChain(window))
    RemoveTransientOwnerRecursive(transient_window);
}

void MultiUserWindowManagerChromeOS::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_BROWSER_OPENED, type);
  AddBrowserWindow(content::Source<Browser>(source).ptr());
}

void MultiUserWindowManagerChromeOS::SetAnimationSpeedForTest(
    MultiUserWindowManagerChromeOS::AnimationSpeed speed) {
  animation_speed_ = speed;
}

bool MultiUserWindowManagerChromeOS::IsAnimationRunningForTest() {
  return animation_ && !animation_->IsAnimationFinished();
}

const AccountId& MultiUserWindowManagerChromeOS::GetCurrentUserForTest() const {
  return current_account_id_;
}

bool MultiUserWindowManagerChromeOS::ShowWindowForUserIntern(
    aura::Window* window,
    const AccountId& account_id) {
  // If there is either no owner, or the owner is the current user, no action
  // is required.
  const AccountId& owner = GetWindowOwner(window);
  if ((!owner.is_valid()) ||
      (owner == account_id && IsWindowOnDesktopOfUser(window, account_id)))
    return false;

  bool minimized = wm::WindowStateIs(window, ui::SHOW_STATE_MINIMIZED);
  // Check that we are not trying to transfer ownership of a minimized window.
  if (account_id != owner && minimized)
    return false;

  if (!minimized) {
    // If the window was transferred without getting minimized, we should record
    // the window type. Otherwise it falls back to the original desktop.
    RecordUMAForTransferredWindowType(window);
  }

  WindowToEntryMap::iterator it = window_to_entry_.find(window);
  it->second->set_show_for_user(account_id);

  // Tests could either not have a UserManager or the UserManager does not
  // know the window owner.
  const user_manager::User* const window_owner =
      user_manager::UserManager::IsInitialized()
          ? user_manager::UserManager::Get()->FindUser(owner)
          : nullptr;

  const bool teleported = !IsWindowOnDesktopOfUser(window, owner);
  if (window_owner && teleported) {
    window->SetProperty(
        aura::client::kAvatarIconKey,
        new gfx::ImageSkia(GetAvatarImageForUser(window_owner)));
  } else {
    window->ClearProperty(aura::client::kAvatarIconKey);
  }

  // Show the window if the added user is the current one.
  if (account_id == current_account_id_) {
    // Only show the window if it should be shown according to its state.
    if (it->second->show())
      SetWindowVisibility(window, true, kTeleportAnimationTimeMS);
  } else {
    SetWindowVisibility(window, false, kTeleportAnimationTimeMS);
  }

  // Notify entry change.
  for (Observer& observer : observers_)
    observer.OnOwnerEntryChanged(window);
  return true;
}

void MultiUserWindowManagerChromeOS::SetWindowVisibility(
    aura::Window* window,
    bool visible,
    int animation_time_in_ms) {
  // For a panel window, it's possible that this panel window is in the middle
  // of relayout animation because of hiding/reshowing shelf during profile
  // switch. Thus the window's visibility might not be its real visibility. See
  // crbug.com/564725 for more info.
  if (window->TargetVisibility() == visible)
    return;

  // Hiding a system modal dialog should not be allowed. Instead we switch to
  // the user which is showing the system modal window.
  // Note that in some cases (e.g. unit test) windows might not have a root
  // window.
  if (!visible && window->GetRootWindow()) {
    if (HasSystemModalTransientChildWindow(window)) {
      // The window is system modal and we need to find the parent which owns
      // it so that we can switch to the desktop accordingly.
      AccountId account_id = GetUserPresentingWindow(window);
      if (!account_id.is_valid()) {
        aura::Window* owning_window = GetOwningWindowInTransientChain(window);
        DCHECK(owning_window);
        account_id = GetUserPresentingWindow(owning_window);
        DCHECK(account_id.is_valid());
      }
      SessionControllerClient::DoSwitchActiveUser(account_id);
      return;
    }
  }

  // To avoid that these commands are recorded as any other commands, we are
  // suppressing any window entry changes while this is going on.
  base::AutoReset<bool> suppressor(&suppress_visibility_changes_, true);

  if (visible)
    ShowWithTransientChildrenRecursive(window, animation_time_in_ms);
  else
    SetWindowVisible(window, false, animation_time_in_ms);
}

void MultiUserWindowManagerChromeOS::NotifyAfterUserSwitchAnimationFinished() {
  for (Observer& observer : observers_)
    observer.OnUserSwitchAnimationFinished();
}

void MultiUserWindowManagerChromeOS::AddBrowserWindow(Browser* browser) {
  // A unit test (e.g. CrashRestoreComplexTest.RestoreSessionForThreeUsers) can
  // come here with no valid window.
  if (!browser->window() || !browser->window()->GetNativeWindow())
    return;
  SetWindowOwner(browser->window()->GetNativeWindow(),
                 multi_user_util::GetAccountIdFromProfile(browser->profile()));
}

void MultiUserWindowManagerChromeOS::ShowWithTransientChildrenRecursive(
    aura::Window* window,
    int animation_time_in_ms) {
  aura::Window::Windows::const_iterator it =
      wm::GetTransientChildren(window).begin();
  for (; it != wm::GetTransientChildren(window).end(); ++it)
    ShowWithTransientChildrenRecursive(*it, animation_time_in_ms);

  // We show all children which were not explicitly hidden.
  TransientWindowToVisibility::iterator it2 =
      transient_window_to_visibility_.find(window);
  if (it2 == transient_window_to_visibility_.end() || it2->second)
    SetWindowVisible(window, true, animation_time_in_ms);
}

aura::Window* MultiUserWindowManagerChromeOS::GetOwningWindowInTransientChain(
    aura::Window* window) const {
  if (!GetWindowOwner(window).empty())
    return NULL;
  aura::Window* parent = wm::GetTransientParent(window);
  while (parent) {
    if (!GetWindowOwner(parent).empty())
      return parent;
    parent = wm::GetTransientParent(parent);
  }
  return NULL;
}

void MultiUserWindowManagerChromeOS::AddTransientOwnerRecursive(
    aura::Window* window,
    aura::Window* owned_parent) {
  // First add all child windows.
  aura::Window::Windows::const_iterator it =
      wm::GetTransientChildren(window).begin();
  for (; it != wm::GetTransientChildren(window).end(); ++it)
    AddTransientOwnerRecursive(*it, owned_parent);

  // If this window is the owned window, we do not have to handle it again.
  if (window == owned_parent)
    return;

  // Remember the current visibility.
  DCHECK(transient_window_to_visibility_.find(window) ==
         transient_window_to_visibility_.end());
  transient_window_to_visibility_[window] = window->IsVisible();

  // Add observers to track state changes.
  window->AddObserver(this);
  wm::TransientWindowManager::GetOrCreate(window)->AddObserver(this);

  // Hide the window if it should not be shown. Note that this hide operation
  // will hide recursively this and all children - but we have already collected
  // their initial view state.
  if (!IsWindowOnDesktopOfUser(owned_parent, current_account_id_))
    SetWindowVisibility(window, false, kAnimationTimeMS);
}

void MultiUserWindowManagerChromeOS::RemoveTransientOwnerRecursive(
    aura::Window* window) {
  // First remove all child windows.
  aura::Window::Windows::const_iterator it =
      wm::GetTransientChildren(window).begin();
  for (; it != wm::GetTransientChildren(window).end(); ++it)
    RemoveTransientOwnerRecursive(*it);

  // Find from transient window storage the visibility for the given window,
  // set the visibility accordingly and delete the window from the map.
  TransientWindowToVisibility::iterator visibility_item =
      transient_window_to_visibility_.find(window);
  DCHECK(visibility_item != transient_window_to_visibility_.end());

  window->RemoveObserver(this);
  wm::TransientWindowManager::GetOrCreate(window)->RemoveObserver(this);

  bool unowned_view_state = visibility_item->second;
  transient_window_to_visibility_.erase(visibility_item);
  if (unowned_view_state && !window->IsVisible()) {
    // To prevent these commands from being recorded as any other commands, we
    // are suppressing any window entry changes while this is going on.
    // Instead of calling SetWindowVisible, only show gets called here since all
    // dependents have been shown previously already.
    base::AutoReset<bool> suppressor(&suppress_visibility_changes_, true);
    window->Show();
  }
}

void MultiUserWindowManagerChromeOS::SetWindowVisible(
    aura::Window* window,
    bool visible,
    int animation_time_in_ms) {
  // The TabletModeWindowManager will not handle invisible windows since they
  // are not user activatable. Since invisible windows are not being tracked,
  // we tell it to maximize / track this window now before it gets shown, to
  // reduce animation jank from multiple resizes.
  if (visible) {
    // TODO(erg): When we get rid of the classic ash, get rid of the direct
    // linkage on tablet_mode_controller() here.
    if (chromeos::GetAshConfig() == ash::Config::MASH) {
      aura::WindowTreeHostMus::ForWindow(window)->PerformWmAction(
          ash::mojom::kAddWindowToTabletMode);
    } else {
      ash::Shell::Get()->tablet_mode_controller()->AddWindow(window);
    }
  }

  AnimationSetter animation_setter(
      window, GetAdjustedAnimationTimeInMS(animation_time_in_ms));

  if (visible)
    window->Show();
  else
    window->Hide();
}

int MultiUserWindowManagerChromeOS::GetAdjustedAnimationTimeInMS(
    int default_time_in_ms) const {
  return animation_speed_ == ANIMATION_SPEED_NORMAL
             ? default_time_in_ms
             : (animation_speed_ == ANIMATION_SPEED_FAST ? 10 : 0);
}
