// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/profiles/avatar_base_controller.h"

#include "base/macros.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/profiles/profile_chooser_bridge_views.h"

@interface AvatarBaseController () {
  std::unique_ptr<ProfileChooserViewBridge> profileChooserViewObserverBridge_;
}

// Updates the profile name displayed by the avatar button. If |layoutParent| is
// yes, then the BrowserWindowController is notified to relayout the subviews,
// as the button needs to be repositioned.
- (void)updateAvatarButtonAndLayoutParent:(BOOL)layoutParent;

// Displays an error icon if any accounts associated with this profile have an
// auth error or sync error.
- (void)setErrorStatus:(BOOL)hasError;
@end

ProfileUpdateObserver::ProfileUpdateObserver(
    Profile* profile,
    AvatarBaseController* avatarController)
    : errorController_(this, profile),
      profile_(profile),
      avatarController_(avatarController) {
  g_browser_process->profile_manager()
      ->GetProfileAttributesStorage()
      .AddObserver(this);
}

ProfileUpdateObserver::~ProfileUpdateObserver() {
  g_browser_process->profile_manager()
      ->GetProfileAttributesStorage()
      .RemoveObserver(this);
}

void ProfileUpdateObserver::OnProfileAdded(const base::FilePath& profile_path) {
  [avatarController_ updateAvatarButtonAndLayoutParent:YES];
}

void ProfileUpdateObserver::OnProfileWasRemoved(
    const base::FilePath& profile_path,
    const base::string16& profile_name) {
  // If deleting the active profile, don't bother updating the avatar
  // button, as the browser window is being closed anyway.
  if (profile_->GetPath() != profile_path)
    [avatarController_ updateAvatarButtonAndLayoutParent:YES];
}

void ProfileUpdateObserver::OnProfileNameChanged(
    const base::FilePath& profile_path,
    const base::string16& old_profile_name) {
  if (profile_->GetPath() == profile_path)
    [avatarController_ updateAvatarButtonAndLayoutParent:YES];
}

void ProfileUpdateObserver::OnProfileSupervisedUserIdChanged(
    const base::FilePath& profile_path) {
  if (profile_->GetPath() == profile_path)
    [avatarController_ updateAvatarButtonAndLayoutParent:YES];
}

void ProfileUpdateObserver::OnAvatarErrorChanged() {
  [avatarController_ setErrorStatus:errorController_.HasAvatarError()];
}

bool ProfileUpdateObserver::HasAvatarError() {
  return errorController_.HasAvatarError();
}

@implementation AvatarBaseController

- (id)initWithBrowser:(Browser*)browser {
  if ((self = [super init])) {
    browser_ = browser;
    profileObserver_.reset(
        new ProfileUpdateObserver(browser_->profile(), self));
  }
  return self;
}

- (void)dealloc {
  [self browserWillBeDestroyed];
  [super dealloc];
}

- (void)browserWillBeDestroyed {
  profileChooserViewObserverBridge_.reset();
  browser_ = nullptr;
}

- (NSButton*)buttonView {
  CHECK(button_.get());  // Subclasses must set this.
  return button_.get();
}

- (BOOL)shouldUseGenericButton {
  ProfileAttributesStorage& storage =
      g_browser_process->profile_manager()->GetProfileAttributesStorage();
  return !browser_->profile()->IsGuestSession() &&
         storage.GetNumberOfProfiles() == 1 &&
         !storage.GetAllProfilesAttributes().front()->IsAuthenticated();
}

- (void)showAvatarBubbleAnchoredAt:(NSView*)anchor
                          withMode:(BrowserWindow::AvatarBubbleMode)mode
                   withServiceType:(signin::GAIAServiceType)serviceType
                   fromAccessPoint:(signin_metrics::AccessPoint)accessPoint {
  profiles::BubbleViewMode bubbleViewMode;
  profiles::BubbleViewModeFromAvatarBubbleMode(mode, &bubbleViewMode);
  profileChooserViewObserverBridge_ = ShowProfileChooserViews(
      self, anchor, accessPoint, browser_, bubbleViewMode);
}

// Shows the avatar bubble.
- (void)buttonClicked:(id)sender {
  [self showAvatarBubbleAnchoredAt:button_
                          withMode:BrowserWindow::AVATAR_BUBBLE_MODE_DEFAULT
                   withServiceType:signin::GAIA_SERVICE_TYPE_NONE
                   fromAccessPoint:signin_metrics::AccessPoint::
                                       ACCESS_POINT_AVATAR_BUBBLE_SIGN_IN];
}

- (void)bubbleWillClose {
  BrowserWindowController* bwc = [BrowserWindowController
      browserWindowControllerForWindow:browser_->window()->GetNativeWindow()];
  if (bwc)
    [bwc releaseToolbarVisibilityForOwner:self withAnimation:YES];
  profileChooserViewObserverBridge_.reset();
}

- (void)updateAvatarButtonAndLayoutParent:(BOOL)layoutParent {
  NOTREACHED();
}

- (void)setErrorStatus:(BOOL)hasError {
}

- (BOOL)isMenuOpened {
  return profileChooserViewObserverBridge_.get() != nullptr;
}

@end
