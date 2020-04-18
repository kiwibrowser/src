// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/notifications/notification_builder_mac.h"

#import <AppKit/AppKit.h>

#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"

#include "chrome/browser/ui/cocoa/notifications/notification_constants_mac.h"

namespace {

// Internal builder constants representing the different notification fields
// They don't need to be exposed outside the builder.

NSString* const kNotificationTitle = @"title";
NSString* const kNotificationSubTitle = @"subtitle";
NSString* const kNotificationInformativeText = @"informativeText";
NSString* const kNotificationImage = @"icon";
NSString* const kNotificationButtonOne = @"buttonOne";
NSString* const kNotificationButtonTwo = @"buttonTwo";
NSString* const kNotificationTag = @"tag";
NSString* const kNotificationCloseButtonTag = @"closeButton";
NSString* const kNotificationOptionsButtonTag = @"optionsButton";
NSString* const kNotificationSettingsButtonTag = @"settingsButton";

}  // namespace

@implementation NotificationBuilder {
  base::scoped_nsobject<NSMutableDictionary> notificationData_;
}

- (instancetype)initWithCloseLabel:(NSString*)closeLabel
                      optionsLabel:(NSString*)optionsLabel
                     settingsLabel:(NSString*)settingsLabel {
  if ((self = [super init])) {
    notificationData_.reset([[NSMutableDictionary alloc] init]);
    [notificationData_ setObject:closeLabel forKey:kNotificationCloseButtonTag];
    [notificationData_ setObject:optionsLabel
                          forKey:kNotificationOptionsButtonTag];
    [notificationData_ setObject:settingsLabel
                          forKey:kNotificationSettingsButtonTag];
  }
  return self;
}

- (instancetype)initWithDictionary:(NSDictionary*)data {
  if ((self = [super init])) {
    notificationData_.reset([data copy]);
  }
  return self;
}

- (void)setTitle:(NSString*)title {
  if (title.length)
    [notificationData_ setObject:title forKey:kNotificationTitle];
}

- (void)setSubTitle:(NSString*)subTitle {
  if (subTitle.length)
    [notificationData_ setObject:subTitle forKey:kNotificationSubTitle];
}

- (void)setContextMessage:(NSString*)contextMessage {
  if (contextMessage.length)
    [notificationData_ setObject:contextMessage
                          forKey:kNotificationInformativeText];
}

- (void)setIcon:(NSImage*)icon {
  if (icon) {
    if ([icon conformsToProtocol:@protocol(NSSecureCoding)]) {
      [notificationData_ setObject:icon forKey:kNotificationImage];
    } else {  // NSImage only conforms to NSSecureCoding from 10.10 onwards.
      [notificationData_ setObject:[icon TIFFRepresentation]
                            forKey:kNotificationImage];
    }
  }
}

- (void)setButtons:(NSString*)primaryButton
    secondaryButton:(NSString*)secondaryButton {
  DCHECK(primaryButton.length);
  [notificationData_ setObject:primaryButton forKey:kNotificationButtonOne];
  if (secondaryButton.length) {
    [notificationData_ setObject:secondaryButton forKey:kNotificationButtonTwo];
  }
}

- (void)setTag:(NSString*)tag {
  if (tag.length)
    [notificationData_ setObject:tag forKey:kNotificationTag];
}

- (void)setOrigin:(NSString*)origin {
  if (origin.length)
    [notificationData_ setObject:origin
                          forKey:notification_constants::kNotificationOrigin];
}

- (void)setNotificationId:(NSString*)notificationId {
  DCHECK(notificationId.length);
  [notificationData_ setObject:notificationId
                        forKey:notification_constants::kNotificationId];
}

- (void)setProfileId:(NSString*)profileId {
  DCHECK(profileId.length);
  [notificationData_ setObject:profileId
                        forKey:notification_constants::kNotificationProfileId];
}

- (void)setIncognito:(BOOL)incognito {
  [notificationData_ setObject:[NSNumber numberWithBool:incognito]
                        forKey:notification_constants::kNotificationIncognito];
}

- (void)setNotificationType:(NSNumber*)notificationType {
  [notificationData_ setObject:notificationType
                        forKey:notification_constants::kNotificationType];
}

- (void)setShowSettingsButton:(BOOL)showSettingsButton {
  [notificationData_
      setObject:[NSNumber numberWithBool:showSettingsButton]
         forKey:notification_constants::kNotificationHasSettingsButton];
}

- (NSUserNotification*)buildUserNotification {
  base::scoped_nsobject<NSUserNotification> toast(
      [[NSUserNotification alloc] init]);
  [toast setTitle:[notificationData_ objectForKey:kNotificationTitle]];
  [toast setSubtitle:[notificationData_ objectForKey:kNotificationSubTitle]];
  [toast setInformativeText:[notificationData_
                                objectForKey:kNotificationInformativeText]];

  // Icon
  if ([notificationData_ objectForKey:kNotificationImage]) {
    if ([[NSImage class] conformsToProtocol:@protocol(NSSecureCoding)]) {
      NSImage* image = [notificationData_ objectForKey:kNotificationImage];
      [toast setContentImage:image];
    } else {  // NSImage only conforms to NSSecureCoding from 10.10 onwards.
      base::scoped_nsobject<NSImage> image([[NSImage alloc]
          initWithData:[notificationData_ objectForKey:kNotificationImage]]);
      [toast setContentImage:image];
    }
  }

  // Type (needed to define the buttons)
  NSNumber* type = [notificationData_
      objectForKey:notification_constants::kNotificationType];

  // Extensions don't have a settings button.
  NSNumber* showSettingsButton = [notificationData_
      objectForKey:notification_constants::kNotificationHasSettingsButton];

  // Buttons
  if ([toast respondsToSelector:@selector(_showsButtons)]) {
    DCHECK([notificationData_ objectForKey:kNotificationCloseButtonTag]);
    DCHECK([notificationData_ objectForKey:kNotificationSettingsButtonTag]);
    DCHECK([notificationData_ objectForKey:kNotificationOptionsButtonTag]);
    DCHECK([notificationData_
        objectForKey:notification_constants::kNotificationHasSettingsButton]);

    BOOL settingsButton = [showSettingsButton boolValue];

    [toast setValue:@YES forKey:@"_showsButtons"];
    // A default close button label is provided by the platform but we
    // explicitly override it in case the user decides to not
    // use the OS language in Chrome.
    [toast setOtherButtonTitle:[notificationData_
                                   objectForKey:kNotificationCloseButtonTag]];

    // Display the Settings button as the action button if there are either no
    // developer-provided action buttons, or the alternate action menu is not
    // available on this Mac version. This avoids needlessly showing the menu.
    if (![notificationData_ objectForKey:kNotificationButtonOne] ||
        ![toast respondsToSelector:@selector(_alwaysShowAlternateActionMenu)]) {
      if (settingsButton) {
        [toast setActionButtonTitle:
                   [notificationData_
                       objectForKey:kNotificationSettingsButtonTag]];
      } else {
        [toast setHasActionButton:NO];
      }

    } else {
      // Otherwise show the alternate menu, then show the developer actions and
      // finally the settings one if needed.
      DCHECK(
          [toast respondsToSelector:@selector(_alwaysShowAlternateActionMenu)]);
      DCHECK(
          [toast respondsToSelector:@selector(_alternateActionButtonTitles)]);
      [toast
          setActionButtonTitle:[notificationData_
                                   objectForKey:kNotificationOptionsButtonTag]];
      [toast setValue:@YES forKey:@"_alwaysShowAlternateActionMenu"];

      NSMutableArray* buttons = [NSMutableArray arrayWithCapacity:3];
      [buttons
          addObject:[notificationData_ objectForKey:kNotificationButtonOne]];
      if ([notificationData_ objectForKey:kNotificationButtonTwo]) {
        [buttons
            addObject:[notificationData_ objectForKey:kNotificationButtonTwo]];
      }
      if (settingsButton) {
        [buttons addObject:[notificationData_
                               objectForKey:kNotificationSettingsButtonTag]];
      }

      [toast setValue:buttons forKey:@"_alternateActionButtonTitles"];
    }
  }

  // Tag
  if ([toast respondsToSelector:@selector(setIdentifier:)] &&
      [notificationData_ objectForKey:kNotificationTag]) {
    [toast setValue:[notificationData_ objectForKey:kNotificationTag]
             forKey:@"identifier"];
  }

  NSString* origin =
      [notificationData_
          objectForKey:notification_constants::kNotificationOrigin]
          ? [notificationData_
                objectForKey:notification_constants::kNotificationOrigin]
          : @"";
  DCHECK(
      [notificationData_ objectForKey:notification_constants::kNotificationId]);
  NSString* notificationId =
      [notificationData_ objectForKey:notification_constants::kNotificationId];

  DCHECK([notificationData_
      objectForKey:notification_constants::kNotificationProfileId]);
  NSString* profileId = [notificationData_
      objectForKey:notification_constants::kNotificationProfileId];

  DCHECK([notificationData_
      objectForKey:notification_constants::kNotificationIncognito]);
  NSNumber* incognito = [notificationData_
      objectForKey:notification_constants::kNotificationIncognito];

  toast.get().userInfo = @{
    notification_constants::kNotificationOrigin : origin,
    notification_constants::kNotificationId : notificationId,
    notification_constants::kNotificationProfileId : profileId,
    notification_constants::kNotificationIncognito : incognito,
    notification_constants::kNotificationType : type,
    notification_constants::kNotificationHasSettingsButton : showSettingsButton,
  };

  return toast.autorelease();
}

- (NSDictionary*)buildDictionary {
  return [[notificationData_ copy] autorelease];
}

@end
