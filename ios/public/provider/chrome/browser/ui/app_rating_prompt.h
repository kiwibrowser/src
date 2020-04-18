// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_APP_RATING_PROMPT_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_APP_RATING_PROMPT_H_

#import <Foundation/Foundation.h>

@class NSURL;
@class UIView;

// AppRatingPromptDelegate declares methods that are called when significant
// actions are performed on an AppRatingPrompt.
@protocol AppRatingPromptDelegate<NSObject>
@optional

// Called when a user taps the "Rate App" button in the prompt.
- (void)userTappedRateApp:(UIView*)view;

// Called when a user taps the "Send Feedback" button in the prompt.
- (void)userTappedSendFeedback:(UIView*)view;

// Called when a user taps the "Dismiss" button in the prompt.
- (void)userTappedDismiss:(UIView*)view;

@end

// An AppRatingPrompt displays a modal dialog prompting the user to rate the
// current app, with additional options to send feedback or dismiss the dialog.
@protocol AppRatingPrompt<NSObject>
@required

// The delegate for this prompt.
@property(nonatomic, readwrite, assign) id<AppRatingPromptDelegate> delegate;

// The URL to launch when the "Rate App" button is tapped.  This URL should lead
// to the ratings section of the app store for the current app.
@property(nonatomic, readwrite, retain) NSURL* appStoreURL;

// Presents the prompt on screen.  The dialog is dismissed automatically when
// the user taps a button, or it can be programatically dismissed using
// |dismiss|.
- (void)show;

// Programatically dismisses the dialog.
- (void)dismiss;

@end

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_APP_RATING_PROMPT_H_
