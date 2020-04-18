// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_BASE_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_BASE_CONTROLLER_H_

#import <AppKit/AppKit.h>

#include <memory>

#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/ui/avatar_button_error_controller.h"
#include "chrome/browser/ui/avatar_button_error_controller_delegate.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/has_weak_browser_pointer.h"

@class BaseBubbleController;
class Browser;
class ProfileUpdateObserver;

// This view controller manages the button that sits in the top of the
// window frame when using multi-profiles, and shows information about the
// current profile. Clicking the button will open the profile menu.
@interface AvatarBaseController : NSViewController<HasWeakBrowserPointer> {
 @protected
  Browser* browser_;

  // The avatar button. Child classes are responsible for implementing it.
  base::scoped_nsobject<NSButton> button_;

  // Observer that listens for updates to the ProfileAttributesStorage as well
  // as AvatarButtonErrorController.
  std::unique_ptr<ProfileUpdateObserver> profileObserver_;
}

// The avatar button view.
@property(readonly, nonatomic) NSButton* buttonView;

// The menu controller, if the menu is open.
@property(nonatomic, readonly, getter=isMenuOpened) BOOL menuOpened;

// Designated initializer.
- (id)initWithBrowser:(Browser*)browser;

// Returns YES if the avatar button should be a generic one. If there is a
// single local profile, then use the generic avatar button instead of the
// profile name. Never use the generic button if the active profile is Guest.
- (BOOL)shouldUseGenericButton;

// Shows the avatar bubble in the given mode.
- (void)showAvatarBubbleAnchoredAt:(NSView*)anchor
                          withMode:(BrowserWindow::AvatarBubbleMode)mode
                   withServiceType:(signin::GAIAServiceType)serviceType
                   fromAccessPoint:(signin_metrics::AccessPoint)accessPoint;

// Called when the avatar bubble is closing.
- (void)bubbleWillClose;

@end

class ProfileUpdateObserver : public ProfileAttributesStorage::Observer,
                              public AvatarButtonErrorControllerDelegate {
 public:
  ProfileUpdateObserver(Profile* profile,
                        AvatarBaseController* avatarController);
  ~ProfileUpdateObserver() override;

  // ProfileAttributesStorage::Observer:
  void OnProfileAdded(const base::FilePath& profile_path) override;
  void OnProfileWasRemoved(const base::FilePath& profile_path,
                           const base::string16& profile_name) override;
  void OnProfileNameChanged(const base::FilePath& profile_path,
                            const base::string16& old_profile_name) override;
  void OnProfileSupervisedUserIdChanged(
      const base::FilePath& profile_path) override;

  // AvatarButtonErrorControllerDelegate:
  void OnAvatarErrorChanged() override;

  bool HasAvatarError();

 private:
  AvatarButtonErrorController errorController_;
  Profile* profile_;
  AvatarBaseController* avatarController_;  // Weak; owns this.

  DISALLOW_COPY_AND_ASSIGN(ProfileUpdateObserver);
};

#endif  // CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_BASE_CONTROLLER_H_
