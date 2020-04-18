// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_TRANSLATION_CONTROLLER_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_TRANSLATION_CONTROLLER_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

@class CWVTranslationLanguage;
@class CWVTranslationPolicy;
@protocol CWVTranslationControllerDelegate;

// The error domain for translation errors.
extern NSErrorDomain const CWVTranslationErrorDomain;

// Possible error codes during translation.
// No connectivity.
extern const NSInteger CWVTranslationErrorNetwork;
// The translation script failed to initialize.
extern const NSInteger CWVTranslationErrorInitializationError;
// The page's language could not be detected.
extern const NSInteger CWVTranslationErrorUnknownLanguage;
// The server detected a language that the browser does not know.
extern const NSInteger CWVTranslationErrorUnsupportedLanguage;
// The original and target languages are the same.
extern const NSInteger CWVTranslationErrorIdenticalLanguages;
// An error was reported by the translation script during translation.
extern const NSInteger CWVTranslationErrorTranslationError;
// The library doesn't finish the translation.
extern const NSInteger CWVTranslationErrorTranslationTimeout;
// The library raises an unexpected exception.
extern const NSInteger CWVTranslationErrorUnexpectedScriptError;
// The library is blocked because of bad origin.
extern const NSInteger CWVTranslationErrorBadOrigin;
// Loader fails to load a dependent JavaScript.
extern const NSInteger CWVTranslationErrorScriptLoadError;

// Allows page translation from one language to another.
CWV_EXPORT
@interface CWVTranslationController : NSObject

// Delegate to receive translation callbacks.
@property(nullable, nonatomic, weak) id<CWVTranslationControllerDelegate>
    delegate;

// The set of supported languages for translation.
@property(nonatomic, readonly)
    NSSet<CWVTranslationLanguage*>* supportedLanguages;

- (instancetype)init NS_UNAVAILABLE;

// Begins translation on the current page from |sourceLanguage| to
// |targetLanguage|. These language parameters must be chosen from
// |supportedLanguages|. Set |userInitiated| to YES if translation
// is a result of explicit user action. |userInitiated| will be
// passed along to the CWVTranslationControllerDelegate methods.
// Results in a No-op if there is no current page.
- (void)translatePageFromLanguage:(CWVTranslationLanguage*)sourceLanguage
                       toLanguage:(CWVTranslationLanguage*)targetLanguage
                    userInitiated:(BOOL)userInitiated;

// Reverts any translations done back to the original page language.
// Note that the original page language may be different from |sourceLanguage|
// passed to |translatePageFromLanguage:toLanguage:userInitiated:| above.
// Results in No-op if the page was never translated.
- (void)revertTranslation;

// Sets or retrieves translation policies associated with a specified language.
// |pageLanguage| should be the language code of the language.
- (void)setTranslationPolicy:(CWVTranslationPolicy*)policy
             forPageLanguage:(CWVTranslationLanguage*)pageLanguage;
- (CWVTranslationPolicy*)translationPolicyForPageLanguage:
    (CWVTranslationLanguage*)pageLanguage;

// Sets or retrieves translation policies associated with a specified page.
// |pageHost| should be the hostname of the website. Must not be empty.
- (void)setTranslationPolicy:(CWVTranslationPolicy*)policy
                 forPageHost:(NSString*)pageHost;
- (CWVTranslationPolicy*)translationPolicyForPageHost:(NSString*)pageHost;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_TRANSLATION_CONTROLLER_H_
