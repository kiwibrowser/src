// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_CHROME_IDENTITY_INTERACTION_MANAGER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_CHROME_IDENTITY_INTERACTION_MANAGER_H_

#import <UIKit/UIKit.h>

#import "base/ios/block_types.h"

@class ChromeIdentity;
@protocol ChromeIdentityInteractionManagerDelegate;

// Error domain for Chrome identity errors.
extern NSString* kChromeIdentityErrorDomain;

typedef enum {
  CHROME_IDENTITY_OPERATION_ONGOING = -200,
} ChromeIdentityErrorCode;

// Callback for the signin operation methods.
// * |identity| is the identity that was added/reauthenticated.
// * |error| is nil unless there was an error during the operation.
typedef void (^SigninCompletionCallback)(ChromeIdentity* identity,
                                         NSError* error);

// ChromeIdentityInteractionManager abstracts the interaction to add identities
// on iOS.
@interface ChromeIdentityInteractionManager : NSObject

// Delegate used to present and dismiss the view controllers.
@property(nonatomic, weak) id<ChromeIdentityInteractionManagerDelegate>
    delegate;

// Whether the manager is currently being canceled. Delegates may inquire if the
// dismissal is due to sign-in being canceled.
@property(nonatomic, readonly) BOOL isCanceling;

// Starts the add account operation for a user. Presents user with the screen to
// enter credentials.
// Note: Calling this method will fail and the completion will be called with a
// CHROME_IDENTITY_OPERATION_ONGOING error if there is already another add
// account or reauthenticate operation ongoing.
// * |completion| will be called once the operation has finished.
- (void)addAccountWithCompletion:(SigninCompletionCallback)completion;

// Starts the reauthentication operation for a user. Presents user with the
// screen to enter credentials with the email pre-entered.
// Note: Calling this method will fail and the completion will be called with a
// CHROME_IDENTITY_OPERATION_ONGOING error if there is already another add
// account or reauthenticate operation ongoing.
// * |userID| is the unique identifier of the user.
// * |userEmail| will be pre-entered on the presented screen.
// * |completion| will be called once the operation has finished.
- (void)reauthenticateUserWithID:(NSString*)userID
                           email:(NSString*)userEmail
                      completion:(SigninCompletionCallback)completion;

// Cancels and dismisses any currently active operation. Completion will be
// called with a cancel error.
// * |animated| represents whether the UI should be dismissed with an animation.
- (void)cancelAndDismissAnimated:(BOOL)animated;

@end

// Protocol that allows custom handling of presentation/dismissal for the view
// controllers managed by a ChromeIdentityInteractionManager.
@protocol ChromeIdentityInteractionManagerDelegate<NSObject>

// Sent to the receiver when a new view controller should be modally presented
// to the user.
// * |interactionManager| is the manager calling this.
// * |viewController| is the view controller that should be presented.
// * |animated| is whether the view controller should be presented with an
//   animation.
// * |completion| is the completion block to call once the presenting operation
//   is finished.
- (void)interactionManager:(ChromeIdentityInteractionManager*)interactionManager
     presentViewController:(UIViewController*)viewController
                  animated:(BOOL)animated
                completion:(ProceduralBlock)completion;

// Sent to the receiver when the presented view controller should be dismissed.
// * |interactionManager| is the manager calling this.
// * |animated| is whether the view controller should be dismissed with an
//   animation.
// * |completion| is the completion block to call once the dismissal operation
//   is finished.
- (void)interactionManager:(ChromeIdentityInteractionManager*)interactionManager
    dismissViewControllerAnimated:(BOOL)animated
                       completion:(ProceduralBlock)completion;

@end

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_CHROME_IDENTITY_INTERACTION_MANAGER_H_
